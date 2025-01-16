/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "blockchain.h"

#include "../utils/logger.h"
#include "../utils/evmcconv.h"

// FIXME: move parsing code out of net/...
#include "../net/http/jsonrpc/variadicparser.h"
#include "../net/http/jsonrpc/blocktag.h"
using namespace jsonrpc;

#include "../libs/base64.hpp"

//#define NODE_DATABASE_DIRECTORY_SUFFIX "/db/"

// ------------------------------------------------------------------
// Constants
// ------------------------------------------------------------------

// Fixed to 2.5 GWei
static inline constexpr std::string_view FIXED_BASE_FEE_PER_GAS = "0x9502f900";

// ------------------------------------------------------------------
// Blockchain
// ------------------------------------------------------------------

Blockchain::Blockchain(const std::string& blockchainPath, std::string instanceId)
  : instanceId_(instanceId),
    options_(Options::fromFile(blockchainPath)),
    comet_(this, instanceId, options_),
    state_(*this),
    storage_(*this),
    http_(options_.getHttpPort(), *this)//,
    //db_(options_.getRootPath() + NODE_DATABASE_DIRECTORY_SUFFIX)
{
}

Blockchain::Blockchain(const Options& options, const std::string& blockchainPath, std::string instanceId)
  : instanceId_(instanceId),
    options_(options),
    comet_(this, instanceId, options_),
    state_(*this),
    storage_(*this),
    http_(options_.getHttpPort(), *this)//,
    //db_(options_.getRootPath() + NODE_DATABASE_DIRECTORY_SUFFIX)
{
}

void Blockchain::setGetTxCacheSize(const uint64_t cacheSize) {
  txCacheSize_ = cacheSize;
  if (txCacheSize_ == 0) {
    std::scoped_lock lock(txCacheMutex_);
    txCache_[0].clear();
    txCache_[1].clear();
    txCacheBucket_ = 0;
  }
}

void Blockchain::start() {
  // Initialize necessary modules
  LOGINFOP("Starting BDK Node...");

  // FIXME/TODO: use cometbft seed-node/PEX to implement discoverynode
  // just setting the relevant config.toml options via Options::cometBFT::config.toml::xxx

  // FIXME/TODO: state saver
  // must checkpoint the entire machine State to disk synchronously (blocking)
  // every X blocks (you can't update the state while you are writing, you must
  // acquire an exclusive lock over the entire State during checkpointing to disk).
  // then, needs a synchronous loadCheckpoint("DB dir/name") function as well.
  // each checkpoint must have its own disk location (instead of writing multiple
  // checkpoints as entries inside the same database files/dir).
  // two ways to do this:
  // - fork the process to duplicate memory then write to disk in the fork
  // - run a dedicated checkpointing node together with the validator node
  // a regular node can just be a checkpointing node itself if it can afford
  // to get a little behind the chain or, if it wants to pay for the memory
  // cost, it can benefit from the process fork checkpointer.

  // Wait for Comet to be in RUNNING state, since that is required
  // for e.g. Comet::sendTransaction() to succeed.
  this->comet_.setPauseState(CometState::RUNNING);
  this->comet_.start();
  std::string cometErr = this->comet_.waitPauseState(10000);
  if (cometErr != "") {
    throw DynamicException("Error while waiting for CometBFT: " + cometErr);
  }
  this->comet_.setPauseState();

  this->http_.start();
}

void Blockchain::stop() {
  this->http_.stop();
  this->comet_.stop();
}

std::shared_ptr<const FinalizedBlock> Blockchain::latest() const {
  return latest_.load();
}

uint64_t Blockchain::getLatestHeight() const {
  auto latestPtr = latest_.load();
  if (!latestPtr) { return 0; }
  return latestPtr->getNHeight();
}

std::shared_ptr<const FinalizedBlock> Blockchain::getBlock(const Hash& hash) {
  // REVIEW: We may want to cache more than just the latest FinalizedBlock
  auto latestPtr = latest_.load();
  if (latestPtr && latestPtr->getHash() == hash)
    return latestPtr;
  // Not cached in RAM; retrieve via cometbft RPC
  json params;
  params["hash"] = hash.hex().get();
  json ret;
  if (!comet_.rpcSyncCall("block_by_hash", params, ret)) {
    return {};
  }
  return std::make_shared<const FinalizedBlock>(FinalizedBlock::fromRPC(ret));
}

std::shared_ptr<const FinalizedBlock> Blockchain::getBlock(uint64_t height) {
  // REVIEW: We may want to cache more than just the latest FinalizedBlock
  auto latestPtr = latest_.load();
  if (latestPtr && latestPtr->getNHeight() == height)
    return latestPtr;
  // Not cached in RAM; retrieve via cometbft RPC
  json params;
  params["height"] = std::to_string(height);
  json ret;
  if (!comet_.rpcSyncCall("block", params, ret)) {
    return {};
  }
  return std::make_shared<const FinalizedBlock>(FinalizedBlock::fromRPC(ret));
}

void Blockchain::putTx(const Hash& tx, const GetTxResultType& val) {
  std::scoped_lock lock(txCacheMutex_);

  auto& activeBucket = txCache_[txCacheBucket_];
  activeBucket[tx] = val;

  if (activeBucket.size() >= txCacheSize_) {
    txCacheBucket_ = 1 - txCacheBucket_;
    txCache_[txCacheBucket_].clear();
  }
}

GetTxResultType Blockchain::getTx(const Hash& tx) {
  // First thing we would do is check a TxBlock object cached in RAM (or the
  //  TxBlock plus all the other elements in the tuple that we are fetching).
  //
  // We NEED this RAM cache because the transaction indexer at the cometbft
  //  end lags a bit -- the transaction simply isn't there for a while AFTER the
  //  block is delivered, so we need to cache this on our end in RAM anyway
  //  (we proactively cache the tx the moment we know it exists, via putTx()).
  // CometBFT RPC retrieval will work fine for older data that has been flushed
  //  from this RAM cache already.

  std::unique_lock<std::mutex> lock(txCacheMutex_);
  for (int i = 0; i < 2; ++i) {
    auto& bucket = txCache_[(txCacheBucket_ + i) % 2];
    auto it = bucket.find(tx);
    if (it != bucket.end()) {
      LOGTRACE("Storage::getTx(" + tx.hex().get() + "): cache hit");
      return it->second;
    }
  }
  lock.unlock();
  LOGTRACE("Storage::getTx(" + tx.hex().get() + "): cache miss");

  // Translate `tx` (BDK sha3 hash) to a CometBFT sha256 hash
  Hash txSha256;
  if (storage_.getTxMap(tx, txSha256)) {

    // Get the data via a sync (blocking) RPC request to cometbft
    Bytes hx = Hex::toBytes(txSha256.hex());
    std::string encodedHexBytes = base64::encode_into<std::string>(hx.begin(), hx.end());
    json params = { {"hash", encodedHexBytes} };
    json ret;
    if (comet_.rpcSyncCall("tx", params, ret)) {
      if (ret.is_object() && ret.contains("result") && ret["result"].is_object()) {
        // Validate returned JSON
        const auto& result = ret["result"];
        if (
            result.contains("tx") && result["tx"].is_string() &&
            result.contains("height") && result["height"].is_string() &&
            result.contains("index") && result["index"].is_number_integer()
          )
        {
          // Base64-decode the tx string data into a Bytes
          Bytes txBytes = base64::decode_into<Bytes>(result["tx"].get<std::string>());

          // Decode Bytes into a TxBlock
          uint64_t chainId = options_.getChainID();
          std::shared_ptr<TxBlock> txBlock = std::make_shared<TxBlock>(txBytes, chainId);

          // Block height and index of tx within block
          uint64_t blockHeight = std::stoull(result["height"].get<std::string>());
          uint64_t blockIndex = result["index"].get<int>();

          // REVIEW: For some reason we need to fill in the hash of the
          //   block as well, but that would be another lookup for the hash
          //   of the block at a given block height.
          //   (This value seems unused and we should remove it anyways).
          Hash blockHash;

          LOGTRACE(
            "getTx(" + tx.hex().get() + "): blockIndex=" +
            std::to_string(blockIndex) + " blockHeight=" +
            std::to_string(blockHeight)
          );

          // Assemble return value
          return std::make_tuple(txBlock, blockHash, blockIndex, blockHeight);
        } else {
         LOGTRACE("getTx(): bad tx call result: " + result.dump());
        }
      } else {
        LOGTRACE("getTx(): bad rpcSyncCall result: " + ret.dump());
      }
    } else {
      LOGTRACE("getTx(): rpcSyncCall('tx') failed");
    }
  } else {
    LOGTRACE("getTx(): cannot find tx sha256");
  }
  LOGTRACE("getTx(" + tx.hex().get() + ") FAIL!");
  return std::make_tuple(nullptr, Hash(), 0u, 0u);
}


// ------------------------------------------------------------------
// CometListener
// ------------------------------------------------------------------

void Blockchain::initChain(
  const uint64_t genesisTime, const std::string& chainId, const Bytes& initialAppState, const uint64_t initialHeight,
  const std::vector<CometValidatorUpdate>& initialValidators, Bytes& appHash
) {
  // TODO: Ensure atoi(chainId) == this->options_.getChainID() (should be the case)

  // For now, the validator set is fixed on genesis and never changes.
  // TODO: When we get to validator set changes via governance, validators_ will have to be
  //   updated via incomingBlock(validatorUpdates).
  validators_ = initialValidators;

  // Initialize the machine state on InitChain.
  // State is not RAII. We are not creating the State instance here.
  // State is created in a pre-comet-consensus, default state given by the BDK, and is initialized here to actual
  //   comet genesis state.
  // TODO: replace this with a call to a private initState() function (Blockchain is friend of State).
  std::unique_lock<std::shared_mutex> lock(state_.stateMutex_);

  // Unfortunately, CometBFT set the state height counter to the height for which you
  // are waiting a block for. The default initial height is 1, not 0 (0 is invalid in
  // cometBFT). However, we do use height 0 to mean the state is at genesis and waiting
  // for the first actual block (with height 1), so we need to fix this on our side.
  state_.height_ = initialHeight - 1;

  LOGDEBUG("Blockchain::initChain(): Height = " + std::to_string(initialHeight));
  state_.timeMicros_ = genesisTime * 1'000'000; // genesisTime is in seconds, so convert to microseconds
  // TODO: If we have support for initialAppState, apply it here, or load it from a BDK side channel
  //   like a genesis State dump/snapshot.
}

void Blockchain::checkTx(const Bytes& tx, int64_t& gasWanted, bool& accept)
{
  // TODO/REVIEW: It is possible that we will keep our own view of the mempool, or our idea
  // of what is in the cometbft mempool, in such a way that we'd know that transactions for
  // some account and nonce, nonce+1, nonce+2 are already there for example (because they have
  // been accepted on our side), so when we see a nonce+3 here for the same account, we know,
  // by looking up on that mempool cache, that this is valid, even though the account in State
  // is still at "nonce". We cannot, unfortunately, e.g. make RPC calls to cometbft from here
  // to poke at the cometbft mempool because all ABCI methods should return "immediately" (if
  // RPC queries are being made to explore the mempool, that's done by some internal worker
  // thread instead, and here we'd just be reading what it has gathered so far).

  // Simply parse and validate the transaction in isolation (this is just checking for
  // an exact nonce match with the tx sender account).
  try {
    TxBlock parsedTx(tx, options_.getChainID());
    accept = state_.validateTransaction(parsedTx);
  } catch (const std::exception& ex) {
    LOGDEBUG("ERROR: Blockchain::checkTx(): " + std::string(ex.what()));
    accept = false;
  }
}

void Blockchain::incomingBlock(
  const uint64_t syncingToHeight, std::unique_ptr<CometBlock> block, Bytes& appHash,
  std::vector<CometExecTxResult>& txResults, std::vector<CometValidatorUpdate>& validatorUpdates
) {
  try {
    FinalizedBlock finBlock = FinalizedBlock::fromCometBlock(*block);
    latest_.store(std::make_shared<const FinalizedBlock>(finBlock));
    if (!state_.validateNextBlock(finBlock)) {
      // Should never happen.
      // REVIEW: in fact, we shouldn't even need to verify the block at this point?
      LOGFATALP_THROW("Invalid block.");
    } else {
      // Update the sha3 --> sha256 txhash interop map
      // FIXME/TODO: we will need a strategy to be able to clean up tx hash mappings
      //  for old blocks, such as adding a bucket ID prefix to each key, then looking
      //  up a hash multiple times in all buckets.
      for (uint32_t i = 0; i < block->txs.size(); ++i) {
        Hash txHashSha3 = Utils::sha3(block->txs[i]);
        Hash txHashSha256 = Utils::sha256(block->txs[i]);
        storage_.putTxMap(txHashSha3, txHashSha256);
      }
      // Advance machine state
      std::vector<bool> succeeded;
      std::vector<uint64_t> gasUsed;
      state_.processBlock(finBlock, succeeded, gasUsed);

      // Fill in the txResults that get sent to cometbft for storage
      for (uint32_t i = 0; i < block->txs.size(); ++i) {
        CometExecTxResult txRes;
        txRes.code = succeeded[i] ? 0 : 1;
        txRes.gasUsed = gasUsed[i];
        txRes.gasWanted = static_cast<uint64_t>(finBlock.getTxs()[i].getGasLimit());
        //txRes.output = Bytes... FIXME: transaction execution result/return arbitrary bytes
        //in ContractHost::execute() there's an "output" var generated in the EVM code branch,
        //  but not in the CPP contract case branch
        txResults.emplace_back(txRes);
      }
    }
  } catch (const std::exception& ex) {
    // We need to fail the blockchain node (fatal)
    LOGFATALP_THROW("FATAL: Blockchain::incomingBlock(): " + std::string(ex.what()));
  }
}

void Blockchain::buildBlockProposal(
  const uint64_t maxTxBytes, const CometBlock& block, bool& noChange, std::vector<size_t>& txIds
) {
  // TODO: exclude invalid transactions (because invalid nonce or no ability to pay)
  // TODO: reorder transactions to make the nonce valid (say n, n+1, n+2 txs on same account but out of order)
  noChange = true;
}

void Blockchain::validateBlockProposal(const CometBlock& block, bool& accept) {
  // FIXME/TODO: Validate all transactions in sequence (context-aware)
  // For now, just validate all of the transactions in isolation (this is insufficient!)
  for (const auto& tx : block.txs) {
    try {
      TxBlock parsedTx(tx, options_.getChainID());
      if (!state_.validateTransaction(parsedTx)) {
        accept = false;
        return;
      }
    } catch (const std::exception& ex) {
      LOGDEBUG("ERROR: Blockchain::validateBlockProposal(): " + std::string(ex.what()));
      accept = false;
      return;
    }
  }
  accept = true;
}

void Blockchain::getCurrentState(uint64_t& height, Bytes& appHash, std::string& appSemVer, uint64_t& appVersion) {
  // TODO: return our machine state
  //       state_.height_, the state root hash, and version info
  height = state_.getHeight();

  // FIXME/TODO: fetch the state root hash (account state hash? not the same?)
  appHash = Hash().asBytes();

  // TODO/REVIEW: Not sure if we should set the BDK version here, since behavior might not change
  // If this is for display and doesn't trigger some cometbft behavior, then this can be the BDK version
  appSemVer = "1.0.0";

  // TODO: This for sure just changes (is incremented) when we change the behavior in a new BDK release
  appVersion = 0;
}

void Blockchain::getBlockRetainHeight(uint64_t& height) {
  // TODO: automatic block history pruning
  height = 0;
}

void Blockchain::currentCometBFTHeight(const uint64_t height) {
  // TODO: here, we must ensure that our state_.height_ CANNOT be greater than height
  //       it must either be exactly height, or if < height, we'll need to ask for blocks to be replayed
}

void Blockchain::sendTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash, const Bytes& tx) {
}

void Blockchain::checkTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash) {
}

void Blockchain::rpcAsyncCallResult(const uint64_t tId, const bool success, const json& response, const std::string& method, const json& params) {
}

void Blockchain::cometStateTransition(const CometState newState, const CometState oldState) {
  // TODO: trace log
}

// ------------------------------------------------------------------
// NodeRPCInterface
// ------------------------------------------------------------------

static inline void forbidParams(const json& request) {
  if (request.contains("params") && !request["params"].empty())
    throw DynamicException("\"params\" are not required for method");
}

static std::pair<Bytes, evmc_message> parseEvmcMessage(const json& request, const uint64_t latestHeight, bool recipientRequired) {
  std::pair<Bytes, evmc_message> res{};

  Bytes& buffer = res.first;
  evmc_message& msg = res.second;

  const auto [txJson, optionalBlockNumber] = parseAllParams<json, std::optional<BlockTagOrNumber>>(request);

  if (optionalBlockNumber.has_value() && !optionalBlockNumber->isLatest(latestHeight))
    throw Error(-32601, "Only latest block is supported");

  msg.sender = parseIfExists<Address>(txJson, "from")
    .transform([] (const Address& addr) { return addr.toEvmcAddress(); })
    .value_or(evmc::address{});

  if (recipientRequired)
    msg.recipient = parse<Address>(txJson.at("to")).toEvmcAddress();
  else
    msg.recipient = parseIfExists<Address>(txJson, "to")
      .transform([] (const Address& addr) { return addr.toEvmcAddress(); })
      .value_or(evmc::address{});

  msg.gas = parseIfExists<uint64_t>(txJson, "gas").value_or(10000000);
  parseIfExists<uint64_t>(txJson, "gasPrice"); // gas price ignored as chain is fixed at 1 GWEI

  msg.value = parseIfExists<uint64_t>(txJson, "value")
    .transform([] (uint64_t val) { return EVMCConv::uint256ToEvmcUint256(uint256_t(val)); })
    .value_or(evmc::uint256be{});

  buffer = parseIfExists<Bytes>(txJson, "data").value_or(Bytes{});

  msg.input_size = buffer.size();
  msg.input_data = buffer.empty() ? nullptr : buffer.data();

  return res;
}

/*

  inline this functionality with a synchronous comet getblock
  in the lambda instead

static std::optional<uint64_t> Blockchain::getBlockNumber(const Hash& hash) {
  if (const auto block = storage.getBlock(hash); block != nullptr) return block->getNHeight();
  return std::nullopt;
}
*/

template<typename T, std::ranges::input_range R>
requires std::convertible_to<std::ranges::range_value_t<R>, T>
static std::vector<T> makeVector(R&& range) {
  std::vector<T> res(std::ranges::size(range));
  std::ranges::copy(std::forward<R>(range), res.begin());
  return res;
}

static json getBlockJson(const FinalizedBlock *block, bool includeTransactions) {
  json ret;
  if (block == nullptr) { ret = json::value_t::null; return ret; }
  ret["hash"] = block->getHash().hex(true);
  ret["parentHash"] = block->getPrevBlockHash().hex(true);
  ret["sha3Uncles"] = Hash().hex(true); // Uncles do not exist.
  ret["miner"] = Secp256k1::toAddress(block->getValidatorPubKey()).hex(true);
  ret["stateRoot"] = Hash().hex(true); // No State root.
  ret["transactionsRoot"] = block->getTxMerkleRoot().hex(true);
  ret["receiptsRoot"] = Hash().hex(true); // No receiptsRoot.
  ret["logsBloom"] = Hash().hex(true); // No logsBloom.
  ret["difficulty"] = "0x1";
  ret["number"] = Hex::fromBytes(Utils::uintToBytes(block->getNHeight()),true).forRPC();
  ret["gasLimit"] = Hex::fromBytes(Utils::uintToBytes(std::numeric_limits<uint64_t>::max()),true).forRPC();
  ret["gasUsed"] = Hex::fromBytes(Utils::uintToBytes(uint64_t(1000000000)),true).forRPC(); // Arbitrary number
  ret["timestamp"] = Hex::fromBytes(Utils::uintToBytes((block->getTimestamp()/1000000)),true).forRPC(); // Block tim
  ret["extraData"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
  ret["mixHash"] = Hash().hex(true); // No mixHash.
  ret["nonce"] = "0x0000000000000000";
  ret["totalDifficulty"] = "0x1";
  ret["baseFeePerGas"] = FIXED_BASE_FEE_PER_GAS;
  ret["withdrawRoot"] = Hash().hex(true); // No withdrawRoot.

  // FIXME/REVIEW: Do we *really* need to know the block size here?
  //               Who is consuming this / depending on this?
  //               It would be better to just add up the byte size of all transactions
  //               and record this in the FinalizedBlock object, maybe adding some
  //               constant guess for the header size, if we just want an estimate.
  //
  // TODO: to get a block you have to serialize it entirely, this can be expensive.
  //ret["size"] = Hex::fromBytes(Utils::uintToBytes(block->serializeBlock().size()),true).forRPC();
  ret["size"] = Hex::fromBytes(Utils::uintToBytes(size_t(0)), true).forRPC();

  ret["transactions"] = json::array();
  uint64_t txIndex = 0;
  for (const auto& tx : block->getTxs()) {
    if (!includeTransactions) { // Only include the transaction hashes.
      ret["transactions"].push_back(tx.hash().hex(true));
    } else { // Include the transactions as a whole.
      json txJson = json::object();
      txJson["blockHash"] = block->getHash().hex(true);
      txJson["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(block->getNHeight()),true).forRPC();
      txJson["from"] = tx.getFrom().hex(true);
      txJson["gas"] = Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()),true).forRPC();
      txJson["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC();
      txJson["hash"] = tx.hash().hex(true);
      txJson["input"] = Hex::fromBytes(tx.getData(), true);
      txJson["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()),true).forRPC();
      txJson["to"] = tx.getTo().hex(true);
      txJson["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txIndex++),true).forRPC();
      txJson["value"] = Hex::fromBytes(Utils::uintToBytes(tx.getValue()),true).forRPC();
      txJson["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()),true).forRPC();
      txJson["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()),true).forRPC();
      txJson["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()),true).forRPC();
      ret["transactions"].emplace_back(std::move(txJson));
    }
  }
  ret["withdrawls"] = json::array();
  ret["uncles"] = json::array();
  return ret;
}

static inline void requiresIndexing(const Storage& storage, std::string_view method) {
  if (storage.getIndexingMode() == IndexingMode::DISABLED) {
    throw Error::methodNotAvailable(method);
  }
}

static inline void requiresDebugIndexing(const Storage& storage, std::string_view method) {
  if (storage.getIndexingMode() != IndexingMode::RPC_TRACE) {
    throw Error::methodNotAvailable(method);
  }
}

// --------------------------------------------------------------------------

json Blockchain::web3_clientVersion(const json& request) {
  forbidParams(request);
  return options_.getWeb3ClientVersion();
}

json Blockchain::web3_sha3(const json& request) {
  const auto [data] = parseAllParams<Bytes>(request);
  return Utils::sha3(data).hex(true);
}

json Blockchain::net_version(const json& request) {
  forbidParams(request);
  return std::to_string(options_.getChainID());
}

json Blockchain::net_listening(const json& request) {
  forbidParams(request);
  return true;
}

json Blockchain::eth_protocolVersion(const json& request) {
  forbidParams(request);
  json ret;
  return options_.getSDKVersion();
}

json Blockchain::net_peerCount(const json& request) {
  forbidParams(request);
  json ret;
  uint64_t peerCount = 0;
  if (comet_.rpcSyncCall("net_info", json::object(), ret)) {
    if (ret.is_object() && ret.contains("result") && ret["result"].is_object()) {
      const auto& result = ret["result"];
      if (result.contains("n_peers") && result["n_peers"].is_string()) {
        try {
          peerCount = static_cast<uint64_t>(std::stoull(result["n_peers"].get<std::string>()));
        } catch (const std::exception& ex) {
          LOGDEBUG("ERROR: net_peerCount(): " + std::string(ex.what()));
        }
      }
    }
  }
  return Hex::fromBytes(Utils::uintToBytes(peerCount), true).forRPC();
}

json Blockchain::eth_getBlockByHash(const json& request){ return {}; }
json Blockchain::eth_getBlockByNumber(const json& request) { return {}; }
json Blockchain::eth_getBlockTransactionCountByHash(const json& request) { return {}; }
json Blockchain::eth_getBlockTransactionCountByNumber(const json& request) { return {}; }

json Blockchain::eth_chainId(const json& request) {
  forbidParams(request);
  return Hex::fromBytes(Utils::uintToBytes(options_.getChainID()), true).forRPC();
}

json Blockchain::eth_syncing(const json& request) {
  forbidParams(request);
  json ret;
  bool syncing = false;
  if (comet_.rpcSyncCall("status", json::object(), ret) &&
      ret.is_object() && ret.contains("result") && ret["result"].is_object())
  {
    const auto& result = ret["result"];
    if (result.contains("sync_info") && result["sync_info"].is_object()) {
      const auto& syncInfo = result["sync_info"];
      if (syncInfo.contains("catching_up") && syncInfo["catching_up"].is_boolean()) {
        syncing = syncInfo["catching_up"].get<bool>();
      }
    }
  }
  return syncing;
}

json Blockchain::eth_coinbase(const json& request) {
  forbidParams(request);
  return options_.getCoinbase().hex(true);
}

json Blockchain::eth_blockNumber(const json& request) {
  forbidParams(request);
  uint64_t blockNumber = getLatestHeight();
  return Hex::fromBytes(Utils::uintToBytes(blockNumber), true).forRPC();
}

json Blockchain::eth_call(const json& request) { return {}; }
json Blockchain::eth_estimateGas(const json& request) { return {}; }

json Blockchain::eth_gasPrice(const json& request) {
  forbidParams(request);
  return FIXED_BASE_FEE_PER_GAS;
}

json Blockchain::eth_feeHistory(const json& request) {
  // FIXME/TODO
  // We should probably just have this computed and saved in RAM as we
  // process incoming blocks.
  return {};
}

json Blockchain::eth_getLogs(const json& request) {
  const auto [logsObj] = parseAllParams<json>(request);
  const auto getBlockByHash = [this] (const Hash& hash) {
    try {
      auto finBlockPtr = getBlock(hash);
      return std::optional<uint64_t>{finBlockPtr->getNHeight()};
    } catch (std::exception& ex) {
      LOGDEBUG("ERROR eth_getLogs(): " + std::string(ex.what()));
      return std::optional<uint64_t>{};
    }
  };

  uint64_t latestHeight = getLatestHeight();

  const std::optional<Hash> blockHash = parseIfExists<Hash>(logsObj, "blockHash");

  const uint64_t fromBlock = parseIfExists<BlockTagOrNumber>(logsObj, "fromBlock")
    .transform([latestHeight](const BlockTagOrNumber& b) { return b.number(latestHeight); })
    .or_else([&blockHash, &getBlockByHash]() { return blockHash.and_then(getBlockByHash); })
    .value_or(ContractGlobals::getBlockHeight());

  const uint64_t toBlock = parseIfExists<BlockTagOrNumber>(logsObj, "toBlock")
    .transform([latestHeight](const BlockTagOrNumber& b) { return b.number(latestHeight); })
    .or_else([&blockHash, &getBlockByHash]() { return blockHash.and_then(getBlockByHash); })
    .value_or(ContractGlobals::getBlockHeight());

  const std::optional<Address> address = parseIfExists<Address>(logsObj, "address");

  const std::vector<Hash> topics = parseArrayIfExists<Hash>(logsObj, "topics")
    .transform([](auto&& arr) { return makeVector<Hash>(std::forward<decltype(arr)>(arr)); })
    .value_or(std::vector<Hash>{});

  json result = json::array();

  for (const auto& event : storage_.getEvents(fromBlock, toBlock, address.value_or(Address{}), topics))
    result.push_back(event.serializeForRPC());

  return result;
}

json Blockchain::eth_getBalance(const json& request) {
  const auto [address, block] = parseAllParams<Address, BlockTagOrNumber>(request);

  if (!block.isLatest(getLatestHeight()))
    throw DynamicException("Only the latest block is supported");

  return Hex::fromBytes(Utils::uintToBytes(state_.getNativeBalance(address)), true).forRPC();
}

json Blockchain::eth_getTransactionCount(const json& request) {
  const auto [address, block] = parseAllParams<Address, BlockTagOrNumber>(request);

  if (!block.isLatest(getLatestHeight()))
    throw DynamicException("Only the latest block is supported");

  return Hex::fromBytes(Utils::uintToBytes(state_.getNativeNonce(address)), true).forRPC();
}

json Blockchain::eth_getCode(const json& request) {
  const auto [address, block] = parseAllParams<Address, BlockTagOrNumber>(request);

  if (!block.isLatest(getLatestHeight()))
    throw DynamicException("Only the latest block is supported");

  return Hex::fromBytes(state_.getContractCode(address), true).forRPC();
}

json Blockchain::eth_sendRawTransaction(const json& request) { return {}; }
json Blockchain::eth_getTransactionByHash(const json& request) { return {}; }
json Blockchain::eth_getTransactionByBlockHashAndIndex(const json& request){ return {}; }
json Blockchain::eth_getTransactionByBlockNumberAndIndex(const json& request) { return {}; }
json Blockchain::eth_getTransactionReceipt(const json& request) { return {}; }

json Blockchain::eth_getUncleByBlockHashAndIndex(const json& request) {
  return json::value_t::null;
}

json Blockchain::txpool_content(const json& request) {
  forbidParams(request);
  json result;
  result["queued"] = json::array();
  json& pending = result["pending"];

  pending = json::array();
/*
  FIXME/TODO
  - We can build something here that would be useful during development
  or testing, but if an application wants to detect the actual entire mempool
  in production (with potentially millions of transactions) then they should
  probably *be* a node instead.
  - CometBFT RPC: num_unconfirmed_txs, unconfirmed_txs
  - we could also implement our own custom mempool later, meaning we would
  already have the data to answer this in this same process.
  - ADR 102 (RPC Companion) also potentially relevant for this (& other ETH RPC
  method implementation decisions)

  for (const auto& [hash, tx] : state.getPendingTxs()) {
    json accountJson;
    json& txJson = accountJson[tx.getFrom().hex(true)][tx.getNonce().str()];
    txJson["blockHash"] = json::value_t::null;
    txJson["blockNumber"] = json::value_t::null;
    txJson["from"] = tx.getFrom().hex(true);
    txJson["to"] = tx.getTo().hex(true);
    txJson["gasUsed"] = json::value_t::null;
    txJson["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC();
    txJson["getMaxFeePerGas"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC();
    txJson["chainId"] = Hex::fromBytes(Utils::uintToBytes(tx.getChainId()),true).forRPC();
    txJson["input"] = Hex::fromBytes(tx.getData(), true).forRPC();
    txJson["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()), true).forRPC();
    txJson["transactionIndex"] = json::value_t::null;
    txJson["type"] = "0x2"; // Legacy Transactions ONLY. TODO: change this to 0x2 when we support EIP-1559
    txJson["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()), true).forRPC();
    txJson["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()), true).forRPC();
    txJson["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()), true).forRPC();
    pending.push_back(std::move(accountJson));
  }
*/
  return result;
}

json Blockchain::debug_traceBlockByNumber(const json& request) {
  requiresDebugIndexing(storage_, "debug_traceBlockByNumber");

  json res = json::array();
  auto [blockNumber, traceJson] = parseAllParams<uint64_t, json>(request);

  if (!traceJson.contains("tracer"))
    throw Error(-32000, "trace type missing");

  if (traceJson["tracer"] != "callTracer")
    throw Error(-32000, std::string("trace mode \"") + traceJson["tracer"].get<std::string>() + "\" not supported");

  const auto block = getBlock(blockNumber);

  if (!block)
    throw Error(-32000, std::string("block ") + std::to_string(blockNumber) + " not found");

  for (const auto& tx : block->getTxs()) {
    json txTrace;

    auto callTrace = storage_.getCallTrace(tx.hash());

    if (!callTrace)
      continue;

    txTrace["txHash"] = tx.hash().hex(true);
    txTrace["result"] = callTrace->toJson();

    res.push_back(std::move(txTrace));
  }

  return res;
}

json Blockchain::debug_traceTransaction(const json& request) {
  requiresDebugIndexing(storage_, "debug_traceTransaction");

  json res;
  auto [txHash, traceJson] = parseAllParams<Hash, json>(request);

  if (!traceJson.contains("tracer"))
    throw Error(-32000, "trace mode missing");

  if (traceJson["tracer"] != "callTracer")
    throw Error(-32000, std::string("trace mode \"") + traceJson["tracer"].get<std::string>() + "\" not supported");

  std::optional<trace::Call> callTrace = storage_.getCallTrace(txHash);

  if (!callTrace)
    return json::value_t::null;

  res = callTrace->toJson();

  return res;
}