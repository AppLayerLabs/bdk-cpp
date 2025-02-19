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

#include "../contract/templates/systemcontract.h"

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
    options_(Options::fromFile(blockchainPath)), // Build the Options object from blockchainPath/options.json
    comet_(this, instanceId, options_),
    state_(*this),
    storage_(*this),
    http_(options_.getHttpPort(), *this, instanceId),
    txCache_(TX_CACHE_SIZE),
    fbCache_(FINALIZEDBLOCK_CACHE_SIZE),
    blockHeightToHashCache_(BLOCK_HASH_CACHE_SIZE)
{
}

Blockchain::Blockchain(const Options& options, std::string instanceId)
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
  std::unique_lock<std::mutex> lock(validatorMutex_);
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
  std::unique_lock<std::mutex> lock(validatorMutex_);
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

void Blockchain::saveSnapshot() {
  // NOTE/IMPORTANT: Validator nodes should never be used to save snapshots.
  // Must always use a dedicated snapshotter node.
  // Setting stateDumpTrigger > 0 in Options for a validator node is probably an user error or a documentation fail.
  uint64_t currentHeight = state_.getHeight();
  if (currentHeight == 0) {
    return; // genesis state, nothing to save
  }
  try {
    // Ensure we can find or create the snapshots root; skip if we can't
    std::filesystem::path snapshotsRoot = options_.getRootPath() + std::string("/snapshots/");
    if (!std::filesystem::exists(snapshotsRoot) || !std::filesystem::is_directory(snapshotsRoot)) {
      if (!std::filesystem::create_directory(snapshotsRoot)) {
        throw DynamicException("Failed to create snapshots directory " + snapshotsRoot.string());
      }
    }
    // Snapshot (DB) dir will be <bdk-root-dir>/snapshots/<heightnum>/
    std::filesystem::path snapshotDir = snapshotsRoot / std::to_string(currentHeight);
    // Skip snapshot generation if the snapshot dir for the current height already exists
    if (std::filesystem::exists(snapshotDir)) {
      throw DynamicException("Skipping saveSnapshot (directory already exists): " + snapshotDir.string());
    }
    // Write the snapshot
    state_.saveSnapshot(snapshotDir);
  } catch (std::exception& ex) {
    throw DynamicException(
      "Cannot save snapshot for height " + std::to_string(currentHeight) +
      ": " + std::string(ex.what())
    );
  }
}

void Blockchain::start() {
  // Simple state transition checker
  if (started_) {
    LOGWARNINGP("Blockchain::start(): BDK node is already started.");
    return;
  }
  started_ = true;
  LOGINFOP("Starting BDK node...");

  // Make sure all Blockchain state variables are reset
  syncing_ = false;
  persistStateSkipCount_ = 0;

  // If this is a restart of Blockchain, we must set State to its default,
  // "undefined" state. This should trigger either another initChain() or a
  // loadSnapshot() down the line, which brings us to an actual state that is
  // valid, considering the actual genesis state (which is not known here).
  state_.resetState();

  // Wait for Comet to be in RUNNING state, since that is required
  // for e.g. Comet::sendTransaction() to succeed.
  this->comet_.setPauseState(CometState::RUNNING);
  this->comet_.start();
  // NOTE: We must wait forever (0) since we don't know how long it takes to load
  //       snapshots, for example.
  //       That also means the driver must throw an error if CometBFT fails or
  //       any other failure happens before the RUNNING state is reached, since
  //       an error condition raised by the driver will interrupt waitPauseState(0).
  std::string cometErr = this->comet_.waitPauseState(0);
  if (cometErr != "") {
    throw DynamicException("Error while waiting for CometBFT: " + cometErr);
  }
  this->comet_.setPauseState();

  // Start RPC
  this->http_.start();
}

void Blockchain::stop() {
  // Simple state transition checker
  if (!started_) {
    LOGWARNINGP("Blockchain::stop(): BDK node is already stopped.");
    return;
  }
  started_ = false;
  LOGINFOP("Stopping BDK node...");

  // Stop
  this->http_.stop();
  this->comet_.stop();
}

Blockchain::~Blockchain() {
  if (started_) {
    stop();
  }
}

std::shared_ptr<const FinalizedBlock> Blockchain::latest() const {
  return latest_.load();
}

uint64_t Blockchain::getLatestHeight() const {
  auto latestPtr = latest_.load();
  if (!latestPtr) { return 0; }
  return latestPtr->getNHeight();
}

void Blockchain::getValidatorSet(std::vector<CometValidatorUpdate>& validatorSet, uint64_t& height) {
  std::unique_lock<std::mutex> lock(validatorMutex_);
  validatorSet = validators_;
  height = state_.getHeight();
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

std::shared_ptr<TxBlock> Blockchain::getUnconfirmedTx(const Hash& txHash) {
  std::shared_lock<std::shared_mutex> lock(mempoolMutex_);
  auto it = mempool_.find(txHash);
  if (it != mempool_.end()) {
    return it->second;
  }
  return std::shared_ptr<TxBlock>{}; // nullptr
}

// ------------------------------------------------------------------
// CometListener
// ------------------------------------------------------------------

void Blockchain::initChain(
  const uint64_t genesisTime, const std::string& chainId, const Bytes& initialAppState, const uint64_t initialHeight,
  const std::vector<CometValidatorUpdate>& initialValidators, Bytes& appHash
) {
  try {
    // Ensure Init chain ID matches Options chain ID
    uint64_t providedChainId = std::stoull(chainId);
    if (providedChainId != this->options_.getChainID()) {
      throw DynamicException(
        "initChain(): Chain ID " + std::to_string(providedChainId) +
        " does not match Options chain ID " + std::to_string(this->options_.getChainID())
      );
    }
  } catch (const std::exception& e) {
    throw DynamicException("initChain(): Invalid chain ID: " + std::string(e.what()));
  }

  // Set the initial validator set.
  // NOTE: The SystemContract has its own idea of what the validator set is (it stores it
  // in a pair of SafeVector, which is not as useful as the this->validators_ map). However,
  // both Blockchain and SystemContract should see the same validator set, as the SystemContract
  // will seed itself from Options::getCometBFT() (which must match &initialValidators).
  setValidators(initialValidators);

  // Initialize the machine state on InitChain.
  // NOTE: State height counting is skewed +1 in ABCI Init for some reason.
  //   The genesis.json "initial height" value of 0 is invalid; 1 is the minimum.
  //   This seemingly only applies to ABCI Init; height behaves as expected
  //   elsewhere (0 == chain/state after 0 blocks, 1 == chain/state after 1 block, etc.)
  // NOTE: BDK doesn't use CometBFT's initial_app_state from genesis.json.
  //   Instead, it checks for /snapshots/0; if it exists, pass the snapshot location to
  //   state_.initChain().
  std::string genesisSnapshot = "";
  std::filesystem::path snapshot0 = options_.getRootPath() + std::string("/snapshots/0");
  if (std::filesystem::exists(snapshot0) && std::filesystem::is_directory(snapshot0)) {
    genesisSnapshot = snapshot0.string();
    LOGINFO("Found genesis snapshot: " + genesisSnapshot);
  }
  state_.initChain(initialHeight - 1, genesisTime, genesisSnapshot);
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
    // Get a TxBlock
    std::shared_ptr<TxBlock> parsedTx;
    bool makeNew = false;
    if (recheck) {
      // If it's a recheck, we must already have a TxBlock on our side
      Hash txHash = Utils::sha3(tx);
      parsedTx = getUnconfirmedTx(txHash);
      if (!parsedTx) {
        // should never happen, since we should always find the TxBlock in the mempool during a recheck
        LOGWARNING("Transaction " + txHash.hex().get() + " not found in mirror mempool during a recheck.");
        makeNew = true;
      }
    } else {
      // Not a recheck, so must parse tx Bytes into a TxBlock
      makeNew = true;
    }
    if (makeNew) {
      // Verify signature only if it's not a recheck.
      parsedTx = std::make_shared<TxBlock>(tx, options_.getChainID(), !recheck);
    }

    // Validate the TxBlock
    accept = state_.validateTransaction(*parsedTx, true);

    // Do mirror mempool maintenance
    if (accept) {
       if (!recheck) {
        // Add to our mirror mempool if the tx is accepted (is valid) and it is not a recheck.
        // NOTE: Txs that have already been included in a finalized block necessarily need to
        // fail validateTransaction(), since they would e.g. contain a dead account nonce, and
        // thus wouldn't be re-included in either the CometBFT mempool or our mempool_ model.
        std::unique_lock<std::shared_mutex> lock(mempoolMutex_);
        mempool_[parsedTx->hash()] = parsedTx;
       }
    } else {
      if (recheck) {
        // Remove from our mirror mempool if the tx is rejected (is invalid) and it is a recheck.
        // (If it is not a recheck, the transaction never entered the mempool, so no need to erase).4
        std::unique_lock<std::shared_mutex> lock(mempoolMutex_);
        mempool_.erase(parsedTx->hash());
      }
    }
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
    // Update syncing status (don't persist state to disk while syncing (?))
    syncing_ = syncingToHeight > block->height;

    // The factory method should construct a FinalizedBlock which is then automatically moved
    //  into the shared_ptr, as it is a temporary.
    // NOTE: We pass in a pointer to our mempool_, which allows FinalizedBlock::fromCometBlock()
    //  to (i) build the FinalizedBlock instance by reusing the TxBlock objects in the mempool_
    //  instead of parsing bytes again and (ii) doing maintenace to mempool_ for us by removing
    //  the txs from mempool_ that are included in the block (since they are no longer unconfirmed,
    //  they *must* be removed from mempool_).
    // -------------------------------------------------------------------------------------
    // FIXME/REVIEW: It might be the case that we can get away with CometBlock NOT having the
    // tx bytes in it, and instead just having the txHashes, making the incomingBlock()
    // call lighter. That could also mean we could move the mirror mempool to the Comet
    // driver as well...
    // -------------------------------------------------------------------------------------
    std::shared_ptr<const FinalizedBlock> fbPtr;
    {
      std::unique_lock<std::shared_mutex> lock(mempoolMutex_);
      fbPtr = std::make_shared<const FinalizedBlock>(
        FinalizedBlock::fromCometBlock(*block, &mempool_)
      );
    }

    // Store the incoming FinalizedBlock in latest_ and fbCache_.
    latest_.store(fbPtr);
    fbCache_.insert(fbPtr);

    // Advance machine state
    std::vector<bool> succeeded;
    std::vector<uint64_t> gasUsed;

    // NOTE: State::processBlock() knows that it has to remove the transactions that
    // are processed into the state from its internal mempool model.
    state_.processBlock(*fbPtr, succeeded, gasUsed);

    // The last raw tx in the block is not an actual transaction, so it doesn't get an
    // actual tx result generated by processBlock() (which operates on TxBlock objs only).
    // fbTxs.size() must already be exactly the raw tx count - 1, otherwise this block
    // would have not been validated (and thus finalized).
    const auto& fbTxs = fbPtr->getTxs();
    size_t txBlockCount = fbTxs.size();
    for (uint32_t i = 0; i < txBlockCount; ++i) {
      const TxBlock& txBlock = *fbTxs[i];

      // Fill in the txResults that get sent to cometbft for storage
      CometExecTxResult txRes;
      txRes.code = succeeded[i] ? 0 : 1;
      txRes.gasUsed = gasUsed[i];
      txRes.gasWanted = static_cast<uint64_t>(txBlock.getGasLimit());
      txResults.emplace_back(txRes);

      // Add a txhash->(blockheight,blockindex) entry to the txCache_ so
      // queries by txhash can find the FinalizedBlock object and thus the
      // TxBlock object.
      putTx(txBlock.hash(), TxCacheValueType{block->height, i});
    }

    // All raw transactions must generate a txResult entry, so append a fake result for
    //  the obligatory last raw tx, which is the randomness hash and not an actual Eth tx.
    txResults.emplace_back(CometExecTxResult{}); // code==0, gasWanted==0, gasUsed==0

    // Collect validator changes accumulated in the singleton system contract
    // and return them to CometBFT via the `validatorUpdates` outparam.
    Bytes validatorDbLog;
    std::vector<std::pair<PubKey, uint64_t>> validatorDeltas;
    SystemContract* systemContractPtr = state_.getSystemContract();
    systemContractPtr->finishBlock(validatorDeltas);
    for (const auto& validatorDelta : validatorDeltas) {
      CometValidatorUpdate validatorUpdate;
      validatorUpdate.publicKey = validatorDelta.first.asBytes();
      validatorUpdate.power = static_cast<int64_t>(validatorDelta.second);
      validatorUpdates.push_back(validatorUpdate);
      Utils::appendBytes(validatorDbLog, validatorUpdate.publicKey);
      Utils::appendBytes(validatorDbLog, IntConv::int64ToBytes(validatorUpdate.power));
    }
    storage_.putValidatorUpdates(block->height, validatorDbLog);

    // Update Blockchain's validator set view to match exactly whatever the SystemContract has
    SafeVector<PubKey>& scValidators = systemContractPtr->getValidators();
    SafeVector<uint64_t>& scValidatorVotes = systemContractPtr->getValidatorVotes();
    uint64_t scNumSlots = systemContractPtr->getNumSlots();
    if (scValidators.size() != scValidatorVotes.size() || scValidators.size() < scNumSlots) {
      throw DynamicException("SystemContract has inconsistent validator set data");
    }
    std::vector<CometValidatorUpdate> newValidatorSet;
    for (int i = 0; i < scNumSlots; ++i) {
      newValidatorSet.emplace_back(
        CometValidatorUpdate{
          scValidators[i].asBytes(),
          static_cast<int64_t>(scValidatorVotes[i])
        }
      );
    }
    setValidators(newValidatorSet);

  } catch (const std::exception& ex) {
    // We need to fail the blockchain node (fatal)
    LOGFATALP_THROW("FATAL: Blockchain::incomingBlock(): " + std::string(ex.what()));
  }
}

void Blockchain::buildBlockProposal(
  const uint64_t maxTxBytes, const CometBlock& block, bool& noChange, std::vector<size_t>& txIds,
  std::vector<Bytes>& injectTxs
) {
  noChange = false;

  // Validator generates a random number that user transactions can directly access from
  // the block that they are included in; this will always be at the last raw transaction index.
  // This basic randomness source is secure for an use case if (i) the proposer can be trusted,
  // and (ii) there are enough validators available to honestly vote for the block (or at least
  // not behave in a faulty manner on purpose to reject an otherwise good proposed block with
  // its proposed simple randomness source and included txs).
  if (maxTxBytes < 32) {
    throw DynamicException("Cannot buildBlockProposal: maxTxBytes must be at least 32");
  }
  uint64_t effectiveMaxTxBytes = maxTxBytes - 32; // randomHash uses block tx space
  injectTxs.emplace_back(Utils::randBytes(32));

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
  size_t totalBytes = 0;
  for (const auto& [account, nonceMap] : accountToNonces) {
    // Iterate over all nonces mentioned in txs with same from account, in ascending order.
    for (const auto& [nonce, txVec] : nonceMap) {
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
        // LOGXTRACE("tx " + parsedTx.hash().hex().get() + " for account is invalid, continuing to next nonce");
        // Skip txs that are invalid for whatever reason.
        // We need to continue; here instead of break; mostly because testing becomes a pain otherwise.
        // We could also break; and skip whatever the entire account is attempting on this block.
        continue;
      }
      // Check cumulative tx size limit
      const size_t thisTxSize = block.txs[bestTxIndex].size();
      if (totalBytes + thisTxSize > effectiveMaxTxBytes) {
        // Whenever we consider a single transaction that exceeds the block limit, we just stop.
        return;
      }
      // Include this transaction
      txIds.push_back(bestTxIndex);
      totalBytes += thisTxSize;
    }
  }
  // Do not add code here (see return; above).
}

void Blockchain::validateBlockProposal(const CometBlock& block, bool& accept) {
  // NOTE: the last block.txs is not a real transaction, it is just a random number.
  // All we have to do is (i) ensure it is there, and (ii) ensure it is exactly 32 bytes long.
  if (block.txs.empty()) {
    LOGDEBUG("ERROR: Blockchain::validateBlockProposal(): txs is empty (missing randomHash).");
    accept = false;
    return;
  }
  if (block.txs[block.txs.size()-1].size() != 32) {
    LOGDEBUG("ERROR: Blockchain::validateBlockProposal(): txs[txs.size()-1].size() != 32 (broken randomHash).");
    accept = false;
    return;
  }

  // By passing a custom (blank) MempoolModel to the verification step of all transactions
  //  in the proposal we are able to automatically verify if nonces for a same from account
  //  are out of order, exactly because the mempool model is empty -- so it only accepts
  //  txs from the same from account and increasing nonces if the nonces are added to the
  //  mempool model in order, meaning they are ordered in the `block.txs` proposal.
  // In addition, note that any transactions found to be invalid here will be ejected from
  //  the State::mempoolModel_ (the 'real' model) and not mm, since mm is a temporary
  //  model and doing maintenance on it would make no sense.
  MempoolModel mm;
  for (int i = 0; i < block.txs.size()-1; ++i) { // last block.txs is not a real tx, so skip it (size()-1)
    const Bytes& tx = block.txs[i];
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
  // NOTE: genesis height is 0 here
  //       all heights for the Comet driver user are based at 0==genesis
  height = state_.getHeight();

  // NOTE: This value should not change if the state doesn't change, because a new appHash generates
  //   extra activity in CometBFT. However, an active blockchain would change the apphash in every
  //   block height in any case.
  //
  //appHash = Hash().asBytes(); // 32-byte zeroed out apphash
  // IMPORTANT:
  //   One way to flag "we don't have appHashes" is leaving it at "" (like it is in the default
  //   genesis.json: app_hash: ""). The other is having a 32-byte "0000000000..." hash.
  //   Should not matter which one we pick (we can pick any value to be the app hash that
  //   never changes since we aren't using it) as long as it is the same, otherwise we'll get
  //   errors like "0000...0000" != "" when doing restarts and replays.
  //
  appHash.clear(); // using "" empty 0-byte apphash

  // TODO/REVIEW: Not sure if we should set the BDK version here, since behavior might not change
  // If this is for display and doesn't trigger some cometbft behavior, then this can be the BDK version
  appSemVer = "1.0.0";

  // TODO: This for sure just changes (is incremented) when we change the behavior in a new BDK release
  //       This should be taken from some constant, not hardcoded to 0 here.
  appVersion = 0;
}

void Blockchain::persistState(uint64_t& height) {
  // Set the retain-block-height outparam
  // TODO: block history pruning based on a BDK option
  height = 0;

  // Trigger snapshotting every X blocks.
  // Do not save snapshots when syncing the blockchain or if snapshotting is disabled via stateDumpTrigger == 0.
  // TODO/REVIEW: We could catch a signal (like SIGUSR1) that forces snapshot generation on the next commit,
  // so nodes could have stateDumpTrigger set to 0 and still be able to generate snapshots.

  // If we are syncing or state dump is disabled, there's nothing else to do.
  if (syncing_ || options_.getStateDumpTrigger() == 0) {
    return;
  }

  // If the dump trigger is satisifed and we don't have a snapshot save already active (future is invalid) then start one.
  if (++persistStateSkipCount_ >= options_.getStateDumpTrigger()) {
    LOGXTRACE("Saving snapshot for height: " + std::to_string(state_.getHeight()));

    // This can take an arbitrary amount of time, and cause CometBFT to lose connections, etc.
    // That is OK, since this should be happening in a dedicated snapshotter node.
    saveSnapshot();

    LOGXTRACE("Saved snapshot for height: " + std::to_string(state_.getHeight()));

    // Reset the dump trigger skip counter.
    persistStateSkipCount_ = 0;

    // Maybe we return in time, if the snapshot is small.
    // Probably we don't return in time, in which case CometBFT should somehow recover
    //   from timeouts, disconnections, etc. (by attempting to reconnect) etc. or crash.
    //   If it crashes (it shouldn't) then the Comet driver should transparently restart it.
    // Re: disconnections, a snapshotter should have a single unconditional or persistent
    //   peer connection to a sentry node, so it should be free to e.g. time out at Commit.
    // REIVEW: We may need to crash the cometbft process on purpose here, or after Commit.
    //   But let's not do this unless it's actually necessary (it shouldn't).
  }
}

void Blockchain::currentCometBFTHeight(const uint64_t height) {
  LOGINFO("Blockchain::currentCometBFTHeight(): " + std::to_string(height));

  // NOTE: We will use this one-time Comet driver callback, which runs *before* we run CometBFT
  // as a node (it takes the `height` param from cometbft inspect) to prepare the State at the
  // optimal height from a snapshot, so when CometBFT starts and connects to the app via ABCI,
  // the application is already at the best available height.
  //
  // Here, we must ensure that our state_.height_ CANNOT be greater than the given height param,
  //   which is the block height of the last block in cometbft's block store.
  // state_.height_ must either be exactly height, or if < height, cometbft will replay blocks.
  try {
    std::filesystem::path snapshotsRoot = options_.getRootPath() + std::string("/snapshots/");
    if (std::filesystem::exists(snapshotsRoot) && std::filesystem::is_directory(snapshotsRoot)) {
      // List all available snapshot directories
      std::vector<uint64_t> availableHeights;
      for (const auto& entry : std::filesystem::directory_iterator(snapshotsRoot)) {
        if (entry.is_directory()) {
          std::string dirname = entry.path().filename().string();
          try {
            uint64_t snapshotHeight = std::stoull(dirname);
            // app state cannot be ahead cometbft's block store, otherwise cometbft panics.
            // ALSO, it cannot be == to height (so <, not <= below), since CometBFT may attempt
            // to replay the block that produces "height" due to how it works internally, in which
            // case loading app state exactly at "height" would cause it to panic since it would
            // be in the "future" then (considering the -1 height rollback CometBFT may do).
            if (snapshotHeight < height) {
              availableHeights.push_back(snapshotHeight);
            }
          } catch (const std::exception& ex) {
          }
        }
      }
      // Try to load a snapshot from the list, most recent first
      if (!availableHeights.empty()) {
        // Sort the heights in descending order to prioritize the most recent snapshots
        // that are eligible (by being "snapshotHeight < height"; see above)
        std::sort(availableHeights.begin(), availableHeights.end(), std::greater<uint64_t>());
        for (const auto& snapshotHeight : availableHeights) {
          std::filesystem::path snapshotDir = snapshotsRoot / std::to_string(snapshotHeight);
          try {
            LOGINFO("Loading snapshot for height " + std::to_string(snapshotHeight));
            state_.loadSnapshot(snapshotDir);
            LOGINFO("Successfully loaded snapshot for height " + std::to_string(snapshotHeight));
            break;
          } catch (const std::exception& ex) {
            LOGERROR(
              "Failed to load snapshot for height " + std::to_string(snapshotHeight) +
              ": " + std::string(ex.what())
            );
          }
        }
      } else {
        LOGINFO("No snapshots available in the snapshots directory with height <= " + std::to_string(height) + ".");
      }
    } else {
      LOGINFO("Snapshots root directory does not exist. No snapshots to load.");
    }
  } catch (const std::exception& ex) {
    LOGERROR("Error searching & loading an initial snapshot: " + std::string(ex.what()));
  }
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

std::tuple<Address, Address, Gas, uint256_t, Bytes> Blockchain::parseMessage(const json& request, bool recipientRequired) {
  std::tuple<Address, Address, Gas, uint256_t, Bytes> result;

  auto& [from, to, gas, value, data] = result;

  const auto [txJson, optionalBlockNumber] = parseAllParams<json, std::optional<BlockTagOrNumber>>(request);

  // Metamask can't keep up with a fast enough moving blockchain
  // Causing it to constantly fail with "Only the latest block is supported"
  // as it requests information on the block it knows it was the latest (not the "latest" flag)
  //if (optionalBlockNumber.has_value() && !optionalBlockNumber->isLatest(storage))
  //  throw Error(-32601, "Only the latest block is supported");

  from = parseIfExists<Address>(txJson, "from").value_or(Address{});

  to = recipientRequired
    ? parse<Address>(txJson.at("to"))
    : parseIfExists<Address>(txJson, "to").value_or(Address{});

  gas = Gas(parseIfExists<uint64_t>(txJson, "gas").value_or(10'000'000));

  value = parseIfExists<uint256_t>(txJson, "value").value_or(0);

  data = parseIfExists<Bytes>(txJson, "data").value_or(Bytes{});

  return result;
}

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
  auto [from, to, gas, value, data] = parseMessage(request, true);
  EncodedStaticCallMessage msg(from, to, gas, data);
  return Hex::fromBytes(state().ethCall(msg));
}

json Blockchain::eth_estimateGas(const json& request) {
  auto [from, to, gas, value, data] = parseMessage(request, false);

  uint64_t gasUsed;

  if (to == Address()) {
    EncodedCreateMessage msg(from, gas, value, data);
    gasUsed = state().estimateGas(msg);
  } else {
    EncodedCallMessage msg(from, to, gas, value, data);
    gasUsed = state().estimateGas(msg);
  }

  return Hex::fromBytes(Utils::uintToBytes(static_cast<uint64_t>(gasUsed)), true).forRPC();
}

json Blockchain::eth_gasPrice(const json& request) {
  forbidParams(request);
  return FIXED_BASE_FEE_PER_GAS;
}

json Blockchain::eth_feeHistory(const json& request) {
  // FIXME/TODO: We should probably just have this computed and saved in RAM as we process incoming blocks.
  // The previous implementation wasn't doing anything (it was just returning a default value, which can be
  // returned here directly as well); it was just requesting 1,024 blocks from (potentially, or at least now
  // with the new Comet-backed Blockchain::getBlock()) the backing storage, which is expensive and an attack
  // vector on the node, really, and we should just not do that.
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

  // Same reasoning as on parseEvmcMessage (Metamask not keeping up)
  // if (!block.isLatest(getLatestHeight()))
  //   throw DynamicException("Only the latest block is supported");

  return Hex::fromBytes(Utils::uintToBytes(state_.getNativeBalance(address)), true).forRPC();
}

json Blockchain::eth_getTransactionCount(const json& request) {
  const auto [address, block] = parseAllParams<Address, BlockTagOrNumber>(request);

  // Same reasoning as on parseEvmcMessage (Metamask not keeping up)
  // if (!block.isLatest(getLatestHeight()))
  //   throw DynamicException("Only the latest block is supported");

  return Hex::fromBytes(Utils::uintToBytes(state_.getNativeNonce(address)), true).forRPC();
}

json Blockchain::eth_getCode(const json& request) {
  const auto [address, block] = parseAllParams<Address, BlockTagOrNumber>(request);

  // Same reasoning as on parseEvmcMessage (Metamask not keeping up)
  // if (!block.isLatest(getLatestHeight()))
  //   throw DynamicException("Only the latest block is supported");

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
  return txBlock.hash().hex(true);
}

json Blockchain::eth_getTransactionByHash(const json& request) {
  requiresIndexing(storage_, "eth_getTransactionByHash");

  const auto [txHash] = parseAllParams<Hash>(request);
  json ret;

  // First, check if tx is in the mempool (so it exists but is necessarily unconfirmed)
  std::shared_ptr<TxBlock> txOnMempool = getUnconfirmedTx(txHash);
  if (txOnMempool) {
    ret["blockHash"] = json::value_t::null;
    ret["blockIndex"] = json::value_t::null;
    ret["from"] = txOnMempool->getFrom().hex(true);
    ret["gas"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getGasLimit()), true).forRPC();
    ret["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getMaxFeePerGas()), true).forRPC();
    ret["hash"] = txOnMempool->hash().hex(true);
    ret["input"] = Hex::fromBytes(txOnMempool->getData(), true);
    ret["nonce"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getNonce()), true).forRPC();
    ret["to"] = txOnMempool->getTo().hex(true);
    ret["transactionIndex"] = json::value_t::null;
    ret["value"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getValue()), true).forRPC();
    ret["v"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getV()), true).forRPC();
    ret["r"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getR()), true).forRPC();
    ret["s"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getS()), true).forRPC();
    return ret;
  }

  // Second, check if tx is in a block
  GetTxResultType txResult = getTx(txHash);
  if (txResult.txBlockPtr != nullptr) {
    Hash blockHash = getBlockHash(txResult.blockHeight);
    TxBlock& tx = *txResult.txBlockPtr;
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

  // Tx is not known
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

json Blockchain::eth_getTransactionReceipt(const json& request) {
  requiresIndexing(storage_, "eth_getTransactionReceipt");

  // NOTE: this is a workaround for MetaMask on Firefox, for some reason it
  // sends the "params" field enclosed in an extra array, which fails parsing,
  // thus never returning the receipt and in turn never updating the tx status
  // (which probably does go through in the background)
  json req = request;
  if (req["params"][0].is_array()) {
    req["params"][0] = req["params"][0][0].get<std::string>();
  }

  const auto [txHash] = parseAllParams<Hash>(req);
  GetTxResultType txInfo = this->getTx(txHash);
  const std::shared_ptr<TxBlock>& tx = txInfo.txBlockPtr;
  const Hash& blockHash = getBlockHash(txInfo.blockHeight);
  const uint64_t& txIndex = txInfo.blockIndex;
  const uint64_t& blockHeight = txInfo.blockHeight;

  if (tx != nullptr) {
    json ret;

    const TxAdditionalData txAddData = storage_.getTxAdditionalData(tx->hash())
      .or_else([]() -> std::optional<TxAdditionalData> {
        throw DynamicException("Unable to fetch existing transaction data");
      }).value();

    ret["transactionHash"] = tx->hash().hex(true);
    ret["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txIndex), true).forRPC();
    ret["blockHash"] = blockHash.hex(true);
    ret["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(blockHeight), true).forRPC();
    ret["from"] = tx->getFrom().hex(true);
    ret["to"] = tx->getTo().hex(true);
    ret["cumulativeGasUsed"] = Hex::fromBytes(Utils::uintToBytes(txAddData.gasUsed), true).forRPC(); // TODO: Fix this, cumulativeGasUsed is not the same as gasUsed
    ret["effectiveGasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()),true).forRPC();
    ret["gasUsed"] =  Hex::fromBytes(Utils::uintToBytes(txAddData.gasUsed), true).forRPC();
    ret["contractAddress"] = bool(txAddData.contractAddress) ? json(txAddData.contractAddress.hex(true)) : json(json::value_t::null);
    ret["logs"] = json::array();
    ret["logsBloom"] = Hash().hex(true);
    ret["type"] = "0x2";
    ret["status"] = txAddData.succeeded ? "0x1" : "0x0";
    for (const Event& e : storage_.getEvents(blockHeight, txIndex)) {
      ret["logs"].push_back(e.serializeForRPC());
    }

    return ret;
  }
  return json::value_t::null;
}

json Blockchain::eth_getUncleByBlockHashAndIndex(const json& request) {
  return json::value_t::null;
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

json Blockchain::txpool_content(const json& request) {
  forbidParams(request);
  json result;

  json& queued = result["queued"];
  queued = json::array();

  json& pending = result["pending"];
  pending = json::array();

  std::shared_lock<std::shared_mutex> lock(mempoolMutex_);
  for (const auto& [hash, txPtr] : mempool_) {
    TxBlock& tx = *txPtr;
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
    txJson["type"] = "0x2";
    txJson["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()), true).forRPC();
    txJson["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()), true).forRPC();
    txJson["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()), true).forRPC();

    // If the tx nonce is greater than the tx from account's nonce, it is a queued tx
    // and should be returned in "queued". Otherwise, the tx nonce should be equal
    // to the tx from account's nonce (can't be less than, otherwise wouldn't be in
    // the mempool) and thus it goes in the "pending" result array.
    if (static_cast<uint64_t>(tx.getNonce()) > state_.getNativeNonce(tx.getFrom())) {
      queued.push_back(std::move(accountJson));
    } else {
      pending.push_back(std::move(accountJson));
    }
  }
  return result;
}
