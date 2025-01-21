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

/// Number of FinalizedBlock objects to cache at any time
#define FINALIZEDBLOCK_CACHE_SIZE 100

/// Default size of the transaction cache
#define TX_CACHE_SIZE 10'000'000

// Size of block height to block hash cache
#define BLOCK_HASH_CACHE_SIZE 10'000

// Fixed to 2.5 GWei
static inline constexpr std::string_view FIXED_BASE_FEE_PER_GAS = "0x9502f900";

// ------------------------------------------------------------------
// FinalizedBlockCache
// ------------------------------------------------------------------

Blockchain::FinalizedBlockCache::FinalizedBlockCache(size_t capacity)
  : capacity_(capacity),
    ring_(capacity),
    nextInsertPos_(0)
{
}

void Blockchain::FinalizedBlockCache::insert(std::shared_ptr<const FinalizedBlock> x) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!x) {
    return;
  }
  auto& slot = ring_[nextInsertPos_];
  if (slot) {
    evictIndices(slot);
  }
  slot = x;
  byHeight_[x->getNHeight()] = x;
  byHash_[x->getHash()] = x;
  nextInsertPos_ = (nextInsertPos_ + 1) % capacity_;
}

std::shared_ptr<const FinalizedBlock> Blockchain::FinalizedBlockCache::getByHeight(uint64_t height) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = byHeight_.find(height);
  if (it != byHeight_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<const FinalizedBlock> Blockchain::FinalizedBlockCache::getByHash(const Hash& hash) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = byHash_.find(hash);
  if (it != byHash_.end()) {
    return it->second;
  }
  return nullptr;
}

void Blockchain::FinalizedBlockCache::evictIndices(const std::shared_ptr<const FinalizedBlock>& x) {
  auto itHeight = byHeight_.find(x->getNHeight());
  if (itHeight != byHeight_.end() && itHeight->second == x) {
    byHeight_.erase(itHeight);
  }
  auto itHash = byHash_.find(x->getHash());
  if (itHash != byHash_.end() && itHash->second == x) {
    byHash_.erase(itHash);
  }
}

// ------------------------------------------------------------------
// Blockchain
// ------------------------------------------------------------------

Blockchain::Blockchain(const std::string& blockchainPath, std::string instanceId)
  : instanceId_(instanceId),
    options_(Options::fromFile(blockchainPath)), // build the Options object from the blockchainPath/options.json file (or binarydefaults)
    comet_(this, instanceId, options_),
    state_(*this),
    storage_(*this),
    http_(options_.getHttpPort(), *this, instanceId),
    txCache_(TX_CACHE_SIZE),
    fbCache_(FINALIZEDBLOCK_CACHE_SIZE),
    blockHeightToHashCache_(BLOCK_HASH_CACHE_SIZE)
{
}

Blockchain::Blockchain(const Options& options, const std::string& blockchainPath, std::string instanceId)
  : instanceId_(instanceId),
    options_(options), // copy the given Options object
    comet_(this, instanceId, options_),
    state_(*this),
    storage_(*this),
    http_(options_.getHttpPort(), *this),
    txCache_(TX_CACHE_SIZE),
    fbCache_(FINALIZEDBLOCK_CACHE_SIZE),
    blockHeightToHashCache_(BLOCK_HASH_CACHE_SIZE)
{
}

bool Blockchain::getBlockRPC(const Hash& blockHash, json& ret) {
  Bytes hx = Hex::toBytes(blockHash.hex());
  std::string encodedHexBytes = base64::encode_into<std::string>(hx.begin(), hx.end());
  json params = { {"hash", encodedHexBytes} };
  return comet_.rpcSyncCall("block_by_hash", params, ret);
}

bool Blockchain::getBlockRPC(const uint64_t blockHeight, json& ret) {
  json params = { {"height", std::to_string(blockHeight)} };
  return comet_.rpcSyncCall("block", params, ret);
}

bool Blockchain::getTxRPC(const Hash& txHash, json& ret) {
  // (sha3->sha256 translation no longer needed as cometbft-bdk uses sha3)
  Bytes hx = Hex::toBytes(txHash.hex());
  std::string encodedHexBytes = base64::encode_into<std::string>(hx.begin(), hx.end());
  json params = { {"hash", encodedHexBytes} };
  return comet_.rpcSyncCall("tx", params, ret);
}

void Blockchain::putTx(const Hash& tx, const TxCacheValueType& val) {
  txCache_.put(tx, val);
}

void Blockchain::setValidators(const std::vector<CometValidatorUpdate>& newValidatorSet) {
  validators_ = newValidatorSet;
  validatorAddrs_.clear();
  for (int i = 0; i < validators_.size(); ++i) {
    const CometValidatorUpdate& v = validators_[i];
    Bytes cometAddrBytes = Comet::getCometAddressFromPubKey(v.publicKey);
    Address cometAddr(cometAddrBytes);
    validatorAddrs_[cometAddr] = i;
  }
}

Address Blockchain::validatorCometAddressToEthAddress(Address validatorCometAddress) {
  auto it = validatorAddrs_.find(validatorCometAddress);
  if (it == validatorAddrs_.end()) {
    return {};
  }
  const uint64_t& validatorIndex = it->second;
  if (validatorIndex >= validators_.size()) {
    throw DynamicException("Blockchain::validatorCometAddressToEthAddress() returned an index not in validators_.");
  }
  const CometValidatorUpdate& v = validators_[validatorIndex];
  PubKey pubKey(v.publicKey); // Compressed key (33 bytes)
  return Secp256k1::toAddress(pubKey); // Generate Eth address from validator pub key
}

void Blockchain::setGetTxCacheSize(const uint64_t cacheSize) {
  txCache_.resize(cacheSize);
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

  // FIXME/TODO
  // Right now we always start at state_.height_ == 0, since we don't have
  // the state load functionality restored yet, so we are replaying from
  // genesis all the time. In that case, we don't have to do anything here.
  // However, when we DO implement state load and we do Blockchain::start()
  // from a height that is greater than 0, we will have to set the latest_
  // block here (and set it on the fbCache_ also, since that's for free)
  // for that height > 0.

  // Start RPC
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
  std::shared_ptr<const FinalizedBlock> bp = fbCache_.getByHash(hash);
  if (bp) {
    return bp;
  }
  // Not cached in RAM; retrieve via cometbft RPC
  json ret;
  if (!getBlockRPC(hash, ret)) {
    return {};
  }
  // If the JSON response is invalid, fromRPC() will throw
  bp = std::make_shared<const FinalizedBlock>(FinalizedBlock::fromRPC(ret));
  // Feed the cache (since the cache object itself doesn't know about the data source)
  fbCache_.insert(bp);
  // Feed the blockHeightToHash cache as well
  blockHeightToHashCache_.put(bp->getNHeight(), bp->getHash());
  return bp;
}

std::shared_ptr<const FinalizedBlock> Blockchain::getBlock(uint64_t height) {
  std::shared_ptr<const FinalizedBlock> bp = fbCache_.getByHeight(height);
  if (bp) {
    return bp;
  }
  // Not cached in RAM; retrieve via cometbft RPC
  json ret;
  if (!getBlockRPC(height, ret)) {
    return {};
  }
  // If the JSON response is invalid, fromRPC() will throw
  bp = std::make_shared<const FinalizedBlock>(FinalizedBlock::fromRPC(ret));
  // Feed the cache (since the cache object itself doesn't know about the data source)
  fbCache_.insert(bp);
  // Feed the blockHeightToHash cache as well
  blockHeightToHashCache_.put(bp->getNHeight(), bp->getHash());
  return bp;
}

Hash Blockchain::getBlockHash(const uint64_t height) {
  std::shared_ptr<const FinalizedBlock> bp = fbCache_.getByHeight(height);
  if (bp) {
    return bp->getHash();
  }
  // We don't want to load the entire block on the fbCache_ just to figure out the
  // block hash for the block height.
  // First, check in the block height --> block hash cache.
  std::optional<Hash> hashOpt = blockHeightToHashCache_.get(height);
  if (hashOpt) {
    return *hashOpt;
  }

  // TODO: Use the cheaper 'blockchain' CometBFT RPC endpoint to figure out the
  // block hash and then use it to feed the block height --> block hash cache.
  //
  // getBlockHash() is currently used by the BDK RPC endpoint impls to return
  // the block hash in a transaction query. Since getTxByBlockNumberAndIndex()
  // internally will retrieve the full block and feed the fbCache() and then
  // feed the blockHeightToHashCache_, the following getBlockHah() call made
  // by the eth_getTransactionByBlockNumberAndIndex() will indeed find a hit
  // in the blockHeightToHashCache_ above. So we currently never hit this
  // condition here.
  // However, for completeness at least, we should query the CometBFT RPC
  // 'blockchain' endpoint and try to fetch the block hash from the block
  // height (should be in a 'block_metas' field in the RPC response).

  // Just return an empty hash if we can't figure it out.
  return Hash();
}

Blockchain::GetTxResultType Blockchain::getTx(const Hash& tx) {
  // First thing we would do is check a TxBlock object cached in RAM (or the
  //  TxBlock plus all the other elements in the tuple that we are fetching).
  //
  // We NEED this RAM cache because the transaction indexer at the cometbft
  //  end lags a bit -- the transaction simply isn't there for a while AFTER the
  //  block is delivered, so we need to cache this on our end in RAM anyway
  //  (we proactively cache the tx the moment we know it exists, via putTx()).
  // CometBFT RPC retrieval will work fine for older data that has been flushed
  //  from this RAM cache already.

  // Translate the transaction hash to (block height, block index).
  // If a mapping is found, we just transform this query into a
  //   GetTxByBlockNumberAndIndex() and we're done.
  std::optional<TxCacheValueType> valOpt = txCache_.get(tx);
  if (valOpt) {
    TxCacheValueType& val = *valOpt;
    LOGTRACE("Storage::getTx(" + tx.hex().get() + "): cache hit");
    return getTxByBlockNumberAndIndex(val.blockHeight, val.blockIndex);
  }
  LOGTRACE("Storage::getTx(" + tx.hex().get() + "): cache miss");

  // The txCache_ doesn't know about this transaction hash, meaning it's likely
  // (if the txCache_ is large enough) that we don't have a FinalizedBlock that
  // has the TxBlock we want. So we have to send an individual-transaction-by-hash
  // query to cometbft, use it to build a TxBlock and return the result tuple here.
  //
  // REVIEW: Think whether this standalone TxBlock that we have created here should
  //       be cached or not, in a different, separate cache from fbCache_.
  //       It doesn't belong to a FinalizedBlock object that is already in the fbCache_,
  //       but maybe we should still cache it instead of just returning it here and
  //       forgetting about it completely.
  //
  json ret;
  if (getTxRPC(tx, ret)) {
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
        std::shared_ptr<TxBlock> txBlock = std::make_shared<TxBlock>(txBytes, chainId, false); // verifySig=false

        // Block height and index of tx within block
        uint64_t blockHeight = std::stoull(result["height"].get<std::string>());
        uint64_t blockIndex = result["index"].get<int>();

        LOGTRACE(
          "getTx(" + tx.hex().get() + "): blockIndex=" +
          std::to_string(blockIndex) + " blockHeight=" +
          std::to_string(blockHeight)
        );

        // Assemble return value
        return { txBlock, blockIndex, blockHeight };
      } else {
        LOGTRACE("getTx(): bad tx call result: " + result.dump());
      }
    } else {
      LOGTRACE("getTx(): bad rpcSyncCall result: " + ret.dump());
    }
  } else {
    LOGTRACE("getTx(): rpcSyncCall('tx') failed");
  }
  LOGTRACE("getTx(" + tx.hex().get() + ") FAIL!");
  return {};
}

Blockchain::GetTxResultType Blockchain::getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex) {
  std::shared_ptr<const FinalizedBlock> bp = fbCache_.getByHash(blockHash);
  if (!bp) {
    // If FinalizedBlock cache miss, retrieve the block from RPC then feed the cache
    json ret;
    if (!getBlockRPC(blockHash, ret)) {
      return {};
    }
    // If the ret JSON is invalid, fromRPC() will throw
    bp = std::make_shared<const FinalizedBlock>(FinalizedBlock::fromRPC(ret));
    fbCache_.insert(bp);
    // Feed the blockHeightToHash cache as well
    blockHeightToHashCache_.put(bp->getNHeight(), bp->getHash());
  }
  return {
    bp->getTxs()[blockIndex],
    blockIndex,
    bp->getNHeight()
  };
}

Blockchain::GetTxResultType Blockchain::getTxByBlockNumberAndIndex(uint64_t blockHeight, uint64_t blockIndex) {
  std::shared_ptr<const FinalizedBlock> bp = fbCache_.getByHeight(blockHeight);
  if (!bp) {
    // If FinalizedBlock cache miss, retrieve the block from RPC then feed the cache
    json ret;
    if (!getBlockRPC(blockHeight, ret)) {
      return {};
    }
    // If the ret JSON is invalid, fromRPC() will throw
    bp = std::make_shared<const FinalizedBlock>(FinalizedBlock::fromRPC(ret));
    fbCache_.insert(bp);
    // Feed the blockHeightToHash cache as well
    blockHeightToHashCache_.put(bp->getNHeight(), bp->getHash());
  }
  return {
    bp->getTxs()[blockIndex],
    blockIndex,
    bp->getNHeight() // == blockHeight
  };
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
  setValidators(initialValidators);

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

void Blockchain::checkTx(const Bytes& tx, const bool recheck, int64_t& gasWanted, bool& accept) {
  // State::validateTransaction() has an internal model of the mempool, so it is capable
  // of knowing that transactions from the same account and a sequence of nonces (current nonce,
  // current nonce + 1, current nonce + 2, etc.) are all valid, as long as it looks like the
  // account can pay for all of them.
  // The mempool model is maintained automatically in a transparent fashion by State. It only
  // needs to know if State:: validateTransaction() is being called for CheckTx and is thus
  // determining transaction eviction from the mempool when CheckTx returns `false`.
  try {
    TxBlock parsedTx(tx, options_.getChainID(), !recheck); // Verify signature only if it's not a recheck.
    accept = state_.validateTransaction(parsedTx, true);
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
    // The factory method should construct a FinalizedBlock which is then
    // automatically moved into the shared_ptr, as it is a temporary.
    std::shared_ptr<const FinalizedBlock> fbPtr =
      std::make_shared<const FinalizedBlock>(
        FinalizedBlock::fromCometBlock(*block)
      );

    // Store the incoming FinalizedBlock in latest_ and fbCache_.
    latest_.store(fbPtr);
    fbCache_.insert(fbPtr);

    // Check if the block is valid
    if (!state_.validateNextBlock(*fbPtr)) {
      // Should never happen.
      // REVIEW: in fact, we shouldn't even need to verify the block at this point?
      LOGFATALP_THROW("Invalid block.");
    } else {
      // Advance machine state
      std::vector<bool> succeeded;
      std::vector<uint64_t> gasUsed;

      // NOTE: State::processBlock() knows that it has to remove the transactions that
      // are processed into the state from its internal mempool model.
      state_.processBlock(*fbPtr, succeeded, gasUsed);

      for (uint32_t i = 0; i < block->txs.size(); ++i) {
        const TxBlock& txBlock = *fbPtr->getTxs()[i];

        // Fill in the txResults that get sent to cometbft for storage
        CometExecTxResult txRes;
        txRes.code = succeeded[i] ? 0 : 1;
        txRes.gasUsed = gasUsed[i];
        txRes.gasWanted = static_cast<uint64_t>(txBlock.getGasLimit());
        //txRes.output = Bytes... FIXME: transaction execution result/return arbitrary bytes
        //in ContractHost::execute() there's an "output" var generated in the EVM code branch,
        //  but not in the CPP contract case branch
        txResults.emplace_back(txRes);

        // Add a txhash->(blockheight,blockindex) entry to the txCache_ so
        // queries by txhash can find the FinalizedBlock object and thus the
        // TxBlock object.
        putTx(txBlock.hash(), TxCacheValueType{block->height, i});
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
  noChange = false;

  // We need to build TxBlock objects from the raw transactions to parse from, nonce, etc.
  std::vector<std::optional<TxBlock>> parsedTxs;
  parsedTxs.reserve(block.txs.size());
  for (const auto& rawTx : block.txs) {
    try {
      parsedTxs.emplace_back(TxBlock(rawTx, options_.getChainID(), false)); // verifySig=false
    } catch (const std::exception& ex) {
      parsedTxs.emplace_back(std::nullopt);
    }
  }

  // Transaction map considering (from account, nonce, tx cost)
  std::map<
    Address, // from account address -->
    std::map<
      uint256_t, // nonce -->
      std::vector< // transactions (all with the same nonce)
        std::pair<
          size_t, // index into block.txs and parsedTxs
          uint256_t // transaction max fee (we want to pick the one that pays the most)
        >
      >
    >
  > accountToNonces;

  // Build the accountToNonces mapping for all parsedTxs
  for (size_t i = 0; i < parsedTxs.size(); ++i) {
    if (!parsedTxs[i].has_value()) {
      continue;
    }
    const TxBlock& txBlock = *parsedTxs[i];
    const Address& fromAddr = txBlock.getFrom();
    const uint256_t& nonce = txBlock.getNonce();
    // We will compare how much transactions with the same nonce pay, ignoring the value transferred.
    // (the value transferred is checked by validateTransactionInternal())
    const uint256_t maxFee = txBlock.getGasLimit() * txBlock.getMaxFeePerGas();
    auto& nonceVec = accountToNonces[fromAddr][nonce];
    nonceVec.emplace_back(i, maxFee);
  }

  // Add transactions to the block proposal outparam (txIds)
  MempoolModel mm; // Blank mempool model (see comment in Blockchain::validateBlockProposal() below)
  txIds.clear();
  txIds.reserve(block.txs.size());
  size_t totalBytes = 0;
  for (const auto& [account, nonceMap] : accountToNonces) {
    // Iterate over all nonces mentioned in txs with same from account, in ascending order.
    for (const auto& [nonce, txVec] : nonceMap) {
      // We break; here on any errors to simplify: we just stop adding transactions for the given account.
      // We could e.g. scan for the second-most expensive transaction with the same nonce and so on, but
      // that's probably overkill. We can just try again for this account on the next block.

      // Should never happen, but let's ensure.
      if (txVec.empty()) {
        break;
      }
      // For all txs with the same nonce, pick the most expensive (generates largest fee
      // and is probably the tx that the end user wants to override others w/ same nonce with).
      size_t bestTxIndex = txVec[0].first;
      uint256_t bestFee = txVec[0].second;
      for (size_t i = 1; i < txVec.size(); ++i) {
        if (txVec[i].second > bestFee) {
          bestTxIndex = txVec[i].first;
          bestFee = txVec[i].second;
        }
      }
      // Retrieve the corresponding parsed TxBlock object
      // (has_value() is true because it passed earlier filtering, but let's ensure)
      if (!parsedTxs[bestTxIndex].has_value()) {
        break;
      }
      const TxBlock& parsedTx = *parsedTxs[bestTxIndex];
      // This validity check will catch anything we haven't checked here.
      // We already know about the nonce sequence here, and we could do a nonce sequence check,
      //   and we could potentially use the information that we are validating txs from the same
      //   account in ascending nonce order to make the check of gas costs faster, as
      //   validateTransaction will scan every nonce in the sequence and add up the tx costs
      //   for each nonce we supply to it, while we could do this check with O(1) here by
      //   accumulating the gas cost in the loop. However, we'd probably just end up duplicating
      //   the entire validity check here, including eviction logic, so it's cleaner to just call
      //   validateTransaction() here.
      //   A potential mitigation would be to institute a limit in the number of txs this node
      //   implementation is willing to include, from the same sender account, in the same block.
      // Any failed transactions here will be flagged as "ejected" from State::mempoolModel_
      //   and NOT from the given temporary mm, as validateTransaction() always removes/ejects
      //   txs from the real mempool model, never from the custom temporary mm (as it would
      //   make zero sense to do maintenance in a throwaway, temporary mempool model anyways).
      if (!state_.validateTransaction(parsedTx, false, &mm)) {
        // Skip txs that fail the tx validity check.
        break;
      }
      // Check maxTxBytes limit
      const size_t thisTxSize = block.txs[bestTxIndex].size();
      if (totalBytes + thisTxSize > maxTxBytes) {
        return; // Done, don't add any more transactions.
      }
      // Include this transaction
      txIds.push_back(bestTxIndex);
      totalBytes += thisTxSize;
    }
  }
}

void Blockchain::validateBlockProposal(const CometBlock& block, bool& accept) {
  // By passing a custom (blank) MempoolModel to the verification step of all transactions
  //  in the proposal we are able to automatically verify if nonces for a same from account
  //  are out of order, exactly because the mempool model is empty -- so it only accepts
  //  txs from the same from account and increasing nonces if the nonces are added to the
  //  mempool model in order, meaning they are ordered in the `block.txs` proposal.
  // In addition, note that any transactions found to be invalid here will be ejected from
  //  the State::mempoolModel_ (the 'real' model) and not mm, since mm is a temporary
  //  model and doing maintenance on it would make no sense.
  MempoolModel mm;
  for (const auto& tx : block.txs) {
    try {
      TxBlock parsedTx(tx, options_.getChainID(), false); // verifySig=false
      if (!state_.validateTransaction(parsedTx, false, &mm)) {
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

json Blockchain::getBlockJson(const FinalizedBlock *block, bool includeTransactions) {
  json ret;
  if (block == nullptr) { ret = json::value_t::null; return ret; }
  ret["hash"] = block->getHash().hex(true);
  ret["parentHash"] = block->getPrevBlockHash().hex(true);
  ret["sha3Uncles"] = Hash().hex(true); // Uncles do not exist.

  //ret["miner"] = Secp256k1::toAddress(block->getValidatorPubKey()).hex(true);
  ret["miner"] = validatorCometAddressToEthAddress(block->getProposerAddr()).hex(true);

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
  // to get a block you have to serialize it entirely, this can be expensive.
  //ret["size"] = Hex::fromBytes(Utils::uintToBytes(block->serializeBlock().size()),true).forRPC();
  ret["size"] = Hex::fromBytes(Utils::uintToBytes(size_t(0)), true).forRPC();

  ret["transactions"] = json::array();
  uint64_t txIndex = 0;
  for (const auto& txPtr : block->getTxs()) {
    const auto& tx = *txPtr;
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

json Blockchain::eth_getBlockByHash(const json& request) {
  const auto [blockHash, optionalIncludeTxs] = parseAllParams<Hash, std::optional<bool>>(request);
  const bool includeTxs = optionalIncludeTxs.value_or(false);
  return getBlockJson(getBlock(blockHash).get(), includeTxs);
}

json Blockchain::eth_getBlockByNumber(const json& request) {
  const auto [blockNumberOrTag, optionalIncludeTxs] = parseAllParams<BlockTagOrNumber, std::optional<bool>>(request);
  const uint64_t blockNumber = blockNumberOrTag.number(getLatestHeight());
  const bool includeTxs = optionalIncludeTxs.value_or(false);
  return getBlockJson(getBlock(blockNumber).get(), includeTxs);
}

json Blockchain::eth_getBlockTransactionCountByHash(const json& request) {
  const auto [blockHash] = parseAllParams<Hash>(request);
  const auto block = getBlock(blockHash);
  if (block)
    return Hex::fromBytes(Utils::uintToBytes(block->getTxs().size()), true).forRPC();
  else
    return json::value_t::null;
}

json Blockchain::eth_getBlockTransactionCountByNumber(const json& request) {
  const auto [blockTagOrNumber] = parseAllParams<BlockTagOrNumber>(request);
  const uint64_t blockNumber = blockTagOrNumber.number(getLatestHeight());
  const auto block = getBlock(blockNumber);
  if (block)
    return Hex::fromBytes(Utils::uintToBytes(block->getTxs().size()), true).forRPC();
  else
    return json::value_t::null;
}

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
  //return options_.getCoinbase().hex(true);
  Address coinbaseAddress;
  Bytes validatorPubKeyBytes = comet_.getValidatorPubKey();
  // PubKey may be empty in the Comet driver if configuration step hasn't completed yet.
  if (!validatorPubKeyBytes.empty()) {
    PubKey pubKey(validatorPubKeyBytes); // Compressed key (33 bytes)
    coinbaseAddress = Secp256k1::toAddress(pubKey); // Generate Eth address from validator pub key
  }
  return coinbaseAddress.hex(true);
}

json Blockchain::eth_blockNumber(const json& request) {
  forbidParams(request);
  uint64_t blockNumber = getLatestHeight();
  return Hex::fromBytes(Utils::uintToBytes(blockNumber), true).forRPC();
}

json Blockchain::eth_call(const json& request) {
  auto [buffer, callInfo] = parseEvmcMessage(request, getLatestHeight(), true);
  callInfo.kind = EVMC_CALL;
  callInfo.flags = 0;
  callInfo.depth = 0;
  return Hex::fromBytes(state_.ethCall(callInfo), true);
}

json Blockchain::eth_estimateGas(const json& request) {
  auto [buffer, callInfo] = parseEvmcMessage(request, getLatestHeight(), false);
  callInfo.flags = 0;
  callInfo.depth = 0;

  // TODO: "kind" is uninitialized if recipient is not zeroes
  if (evmc::is_zero(callInfo.recipient))
    callInfo.kind = EVMC_CREATE;

  const auto usedGas = state_.estimateGas(callInfo);

  return Hex::fromBytes(Utils::uintToBytes(static_cast<uint64_t>(usedGas)), true).forRPC();
}

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

json Blockchain::eth_sendRawTransaction(const json& request) {
  const auto [txBytes] = parseAllParams<Bytes>(request);
  const TxBlock txBlock(txBytes, options_.getChainID());

  // The transaction is verified before it is sent to CometBFT.
  // CometBFT will call us back via ABCI to verify it again (CheckTx),
  //   and we can either have the txHash cached and save some time, or
  //   just check it again (depends on what we will do in our CheckTx).
  if (state_.validateTransaction(txBlock, false)) {
    uint64_t ticketId = comet_.sendTransaction(txBytes);
    if (ticketId == 0) {
      throw Error(-32000, "Error relaying transaction via CometBFT");
    }
  } else {
    throw Error(-32000, "Invalid transaction");
  }

  // Return value is the transaction sha3()
  return json { txBlock.hash().hex(true) };
}

json Blockchain::eth_getTransactionByHash(const json& request) {
  // CometBFT does NOT index transactions in the mempool.
  // You can't query CometBFT for an unconfirmed transaction by hash -- at all.
  // If we want to see and return unconfirmed txs here, we need to track our
  //   own guess about what txs are in the mempool. For example, when we receive
  //   CheckTx calls or when we eth_sendRawTransaction we would fill that structure
  //   with txs we know about and are likely in the mempool, and look them up there
  //   *after* we ask CometBFT (it would be a second cache, unrelated to the txCache
  //   that caches the CometBFT tx responses).
  // Alternatively, we can have this solved when we implement our own mempool
  //   (using the "nop" mempool config from CometBFT).

  requiresIndexing(storage_, "eth_getTransactionByHash");

  const auto [txHash] = parseAllParams<Hash>(request);

  GetTxResultType txResult = getTx(txHash);

  if (txResult.txBlockPtr != nullptr) {
    Hash blockHash = getBlockHash(txResult.blockHeight);
    TxBlock& tx = *txResult.txBlockPtr;
    json ret;
    ret["blockHash"] = blockHash.hex(true);
    ret["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(txResult.blockHeight), true).forRPC();
    ret["from"] = tx.getFrom().hex(true);
    ret["gas"] = Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()), true).forRPC();
    ret["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()), true).forRPC();
    ret["hash"] = tx.hash().hex(true);
    ret["input"] = Hex::fromBytes(tx.getData(), true);
    ret["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()), true).forRPC();
    ret["to"] = tx.getTo().hex(true);
    ret["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txResult.blockIndex), true).forRPC();
    ret["value"] = Hex::fromBytes(Utils::uintToBytes(tx.getValue()), true).forRPC();
    ret["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()), true).forRPC();
    ret["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()), true).forRPC();
    ret["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()), true).forRPC();
    return ret;
  }

  return json::value_t::null;
}

json Blockchain::eth_getTransactionByBlockHashAndIndex(const json& request) {
  const auto [blockHash, blockIndex] = parseAllParams<Hash, uint64_t>(request);

  GetTxResultType txResult = getTxByBlockHashAndIndex(blockHash, blockIndex);

  if (txResult.txBlockPtr != nullptr) {
    TxBlock& tx = *txResult.txBlockPtr;
    json ret;
    ret["blockHash"] = blockHash.hex(true);
    ret["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(txResult.blockHeight), true).forRPC();
    ret["from"] = tx.getFrom().hex(true);
    ret["gas"] = Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()), true).forRPC();
    ret["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()), true).forRPC();
    ret["hash"] = tx.hash().hex(true);
    ret["input"] = Hex::fromBytes(tx.getData(), true);
    ret["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()), true).forRPC();
    ret["to"] = tx.getTo().hex(true);
    ret["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txResult.blockIndex), true).forRPC();
    ret["value"] = Hex::fromBytes(Utils::uintToBytes(tx.getValue()), true).forRPC();
    ret["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()), true).forRPC();
    ret["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()), true).forRPC();
    ret["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()), true).forRPC();
    return ret;
  }

  return json::value_t::null;
}

json Blockchain::eth_getTransactionByBlockNumberAndIndex(const json& request) {
  const auto [blockNumber, blockIndex] = parseAllParams<uint64_t, uint64_t>(request);

  GetTxResultType txResult = getTxByBlockNumberAndIndex(blockNumber, blockIndex);

  if (txResult.txBlockPtr != nullptr) {
    TxBlock& tx = *txResult.txBlockPtr;
    json ret;
    ret["blockHash"] = getBlockHash(blockNumber).hex(true);
    ret["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(txResult.blockHeight), true).forRPC();
    ret["from"] = tx.getFrom().hex(true);
    ret["gas"] = Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()), true).forRPC();
    ret["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()), true).forRPC();
    ret["hash"] = tx.hash().hex(true);
    ret["input"] = Hex::fromBytes(tx.getData(), true);
    ret["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()), true).forRPC();
    ret["to"] = tx.getTo().hex(true);
    ret["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txResult.blockIndex), true).forRPC();
    ret["value"] = Hex::fromBytes(Utils::uintToBytes(tx.getValue()), true).forRPC();
    ret["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()), true).forRPC();
    ret["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()), true).forRPC();
    ret["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()), true).forRPC();
    return ret;
  }
  return json::value_t::null;
}

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

    auto callTrace = storage_.getCallTrace(tx->hash());

    if (!callTrace)
      continue;

    txTrace["txHash"] = tx->hash().hex(true);
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