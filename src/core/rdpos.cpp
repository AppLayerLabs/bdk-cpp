/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "state.h"
#include "rdpos.h"

rdPoS::rdPoS(DB& db,
             DumpManager& dumpManager,
             const Storage& storage,
             P2P::ManagerNormal& p2p,
             const Options& options,
             State& state)
  : BaseContract("rdPoS", ProtocolContractAddresses.at("rdPoS"), Address(), options.getChainID(), db),
    db_(db),
    options_(options),
    storage_(storage),
    p2p_(p2p),
    state_(state),
    worker_(*this),
    validatorKey_(options.getValidatorPrivKey()),
    isValidator_((this->validatorKey_) ? true : false),
    randomGen_(Hash()), minValidators_(options.getMinValidators())
{
  // Initialize blockchain.
  std::unique_lock lock(this->mutex_);
  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Initializing rdPoS.");
  initializeBlockchain();

  /**
   * Load information from DB, stored as following:
   * DBPrefix::rdPoS -> rdPoS mapping (addresses)
   * DBPrefix::rdPoS -> misc: used for randomness currently.
   * Order doesn't matter, Validators are stored in a set (sorted by default).
   */
  auto validatorsDb = db_.getBatch(DBPrefix::rdPoS);
  if (validatorsDb.empty()) {
    // No rdPoS in DB, this should have been initialized by Storage.
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__, "No rdPoS in DB, cannot proceed.");
    throw DynamicException("No rdPoS in DB.");
  }
  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Found " + std::to_string(validatorsDb.size()) + " rdPoS in DB");
  // TODO: check if no index is missing from DB.
  for (const auto& validator : validatorsDb) {
    this->validators_.insert(Validator(Address(validator.value)));
  }

  // Load latest randomness from DB, populate and shuffle the random list.
  this->bestRandomSeed_ = storage_.latest()->getBlockRandomness();
  randomGen_.setSeed(this->bestRandomSeed_);
  this->randomList_ = std::vector<Validator>(this->validators_.begin(), this->validators_.end());
  randomGen_.shuffle(randomList_);
  // Register itself at dump management
  dumpManager.pushBack(this);
}

rdPoS::~rdPoS() {
  this->stoprdPoSWorker();
}

bool rdPoS::validateBlock(const FinalizedBlock& block) const {
  std::lock_guard lock(this->mutex_);
  auto latestBlock = this->storage_.latest();

  if (Secp256k1::toAddress(block.getValidatorPubKey()) != randomList_[0]) {
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                       "Block signature does not match randomList[0]. latest nHeight: "
                       + std::to_string(latestBlock->getNHeight())
                       + " Block nHeight: " + std::to_string(block.getNHeight())
      );
    return false;
  }

  if (block.getTxValidators().size() != this->minValidators_ * 2) {
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                       "Block contains invalid number of TxValidator transactions. latest nHeight: "
                       + std::to_string(latestBlock->getNHeight())
                       + " Block nHeight: " + std::to_string(block.getNHeight())
      );
    return false;
  }

  // Check if all transactions are of the same block height
  for (const auto& tx : block.getTxValidators()) {
    if (tx.getNHeight() != block.getNHeight()) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         "TxValidator transaction is not of the same block height. tx nHeight: "
                         + std::to_string(tx.getNHeight())
                         + " Block nHeight: " + std::to_string(block.getNHeight()));
      return false;
    }
  }

  /**
   * TxValidator transactions within the block must be *ordered* by their respective signer.
   * This is to ensure that the randomness is the same for all rdPoS.
   * Given a minValidator of 4, the block should contain 8 TxValidator transactions.
   * The first 4 (minValidators) transactions from should match randomList[1] to randomList[5] (minValidators + 1)
   * The first 4 (minValidators) transactions should be randomHash transactions. (0xcfffe746), which contains the Sha3(seed).
   * The remaining 4 (minValidators) transactions from should also match randomList[1] to randomList[5] (minValidators +1)
   * The remaining 4 (minValidators) transactions should be random transactions. (0x6fc5a2d6), which contains the seed itself.
   */
  std::unordered_map<TxValidator,TxValidator, SafeHash> txHashToSeedMap; // Tx randomHash -> Tx random
  for (uint64_t i = 0; i < this->minValidators_; i++) {
    if (Validator(block.getTxValidators()[i].getFrom()) != randomList_[i+1]) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         "TxValidator randomHash " + std::to_string(i) + " is not ordered correctly."
                         + "Expected: " + randomList_[i+1].hex().get()
                         + " Got: " + block.getTxValidators()[i].getFrom().hex().get()
        );
      return false;
    }
    if (Validator(block.getTxValidators()[i + this->minValidators_].getFrom()) != randomList_[i+1]) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         "TxValidator random " + std::to_string(i) + " is not ordered correctly."
                         + "Expected: " + randomList_[i+1].hex().get()
                         + " Got: " + block.getTxValidators()[i].getFrom().hex().get()
        );
      return false;
    }
    txHashToSeedMap.emplace(block.getTxValidators()[i],block.getTxValidators()[i + this->minValidators_]);
  }

  if (txHashToSeedMap.size() != this->minValidators_) {
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__, "txHashToSeedMap doesn't match minValidator size.");
    return false;
  }

  // Check the transactions within the block, we should have every transaction within the txHashToSeed map.
  for (auto const& [hashTx, seedTx] : txHashToSeedMap) {
    TxValidatorFunction hashTxFunction = rdPoS::getTxValidatorFunction(hashTx);
    TxValidatorFunction seedTxFunction = rdPoS::getTxValidatorFunction(seedTx);
    // Check if hash tx is invalid by itself.
    if (hashTxFunction == TxValidatorFunction::INVALID) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         std::string("TxValidator ") + hashTx.hash().hex().get()  + " is invalid."
        );
      return false;
    }
    if (seedTxFunction == TxValidatorFunction::INVALID) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         std::string("TxValidator ") + seedTx.hash().hex().get()  + " is invalid."
        );
      return false;
    }
    // Check if senders match.
    if (hashTx.getFrom() != seedTx.getFrom()) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         std::string("TxValidator sender ") + seedTx.hash().hex().get()
                         + " does not match TxValidator sender " + hashTx.hash().hex().get()
        );
      return false;
    }
    // Check if the left sided transaction is a randomHash transaction.
    if (hashTxFunction != TxValidatorFunction::RANDOMHASH) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         std::string("TxValidator ") + hashTx.hash().hex().get() + " is not a randomHash transaction."
        );
      return false;
    }
    // Check if the right sided transaction is a random transaction.
    if (seedTxFunction != TxValidatorFunction::RANDOMSEED) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         std::string("TxValidator ") + seedTx.hash().hex().get() + " is not a random transaction."
        );
      return false;
    }
    // Check if the randomHash transaction matches the random transaction.
    BytesArrView hashTxData = hashTx.getData();
    BytesArrView seedTxData = seedTx.getData();
    BytesArrView hash = hashTxData.subspan(4);
    BytesArrView random = seedTxData.subspan(4);

    // Size sanity check, should be 32 bytes.
    if (hash.size() != 32) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         std::string("TxValidator ") + hashTx.hash().hex().get() + " (hash) is not 32 bytes."
        );
      return false;
    }

    if (random.size() != 32) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         std::string("TxValidator ") + seedTx.hash().hex().get() + " (random) is not 32 bytes."
        );
      return false;
    }

    if (Utils::sha3(random) != hash) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                         std::string("TxValidator ") + seedTx.hash().hex().get()
                         + " does not match TxValidator " + hashTx.hash().hex().get() + " randomness"
        );
      return false;
    }
  }
  return true;
}

Hash rdPoS::processBlock(const FinalizedBlock& block) {
  std::unique_lock lock(this->mutex_);
  validatorMempool_.clear();
  this->randomList_ = std::vector<Validator>(this->validators_.begin(), this->validators_.end());
  this->bestRandomSeed_ = block.getBlockRandomness();
  this->randomGen_.setSeed(this->bestRandomSeed_);
  this->randomGen_.shuffle(this->randomList_);
  return this->bestRandomSeed_;
}

FinalizedBlock rdPoS::signBlock(MutableBlock &block) {
  uint64_t newTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
  FinalizedBlock finalized = block.finalize(this->validatorKey_, newTimestamp);
  this->worker_.blockCreated();
  return finalized;
}

bool rdPoS::addValidatorTx(const TxValidator& tx) {
  std::unique_lock lock(this->mutex_);
  if (this->validatorMempool_.contains(tx.hash())) {
    Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "TxValidator already exists in mempool.");
    return true;
  }

  if (tx.getNHeight() != this->storage_.latest()->getNHeight() + 1) {
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                       "TxValidator is not for the next block. Expected: "
                       + std::to_string(this->storage_.latest()->getNHeight() + 1)
                       + " Got: " + std::to_string(tx.getNHeight())
      );
    return false;
  }

  // Check if sender is a validator and can participate in this rdPoS round (check from existance in randomList)
  bool participates = false;
  for (uint64_t i = 1; i < this->minValidators_ + 1; i++) {
    if (Validator(tx.getFrom()) == this->randomList_[i]) {
      participates = true;
      break;
    }
  }
  if (!participates) {
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
                       "TxValidator sender is not a validator or is not participating in this rdPoS round."
      );
    return false;
  }

  // Do not allow duplicate transactions for the same function, we only have two functions (2 TxValidator per validator per block)
  std::vector<TxValidator> txs;
  for (auto const& [key, value] : this->validatorMempool_) {
    if (value.getFrom() == tx.getFrom()) txs.push_back(value);
  }
  if (txs.empty()) { // No transactions from this sender yet, add it.
    this->validatorMempool_.emplace(tx.hash(), tx);
    return true;
  } else if (txs.size() == 1) { // We already have one transaction from this sender, check if it is the same function.
    if (txs[0].getFunctor() == tx.getFunctor()) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__, "TxValidator sender already has a transaction for this function.");
      return false;
    }
    this->validatorMempool_.emplace(tx.hash(), tx);
  } else { // We already have two transactions from this sender, it is the max we can have per validator.
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__, "TxValidator sender already has two transactions.");
    return false;
  }

  return true;
}

void rdPoS::initializeBlockchain() const {
  auto validatorsDb = db_.getBatch(DBPrefix::rdPoS);
  if (validatorsDb.empty()) {
    Logger::logToDebug(LogType::INFO, Log::rdPoS,__func__, "No rdPoS in DB, initializing.");
    // Use the genesis validators from Options, OPTIONS JSON FILE VALIDATOR ARRAY ORDER **MATTERS**
    for (uint64_t i = 0; i < this->options_.getGenesisValidators().size(); ++i) {
      this->db_.put(Utils::uint64ToBytes(i), this->options_.getGenesisValidators()[i].get(), DBPrefix::rdPoS);
    }
  }
}

Hash rdPoS::parseTxSeedList(const std::vector<TxValidator>& txs) {
  if (txs.empty()) return Hash();
  Bytes seed;
  seed.reserve(txs.size() * 32);
  for (const TxValidator& tx : txs) {
    if (rdPoS::getTxValidatorFunction(tx) == TxValidatorFunction::RANDOMSEED) {
      seed.insert(seed.end(), tx.getData().begin() + 4, tx.getData().end());
    }
  }
  return Utils::sha3(seed);
}

const std::atomic<bool>& rdPoS::canCreateBlock() const {
  return this->worker_.getCanCreateBlock();
}

rdPoS::TxValidatorFunction rdPoS::getTxValidatorFunction(const TxValidator &tx) {
  constexpr Functor randomHashHash(3489654598);
  constexpr Functor randomSeedHash(1875223254);
  if (tx.getData().size() != 36) {
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__, "TxValidator data size is not 36 bytes.");
    // Both RandomHash and RandomSeed are 32 bytes, so if the data size is not 36 bytes, it is invalid.
    return TxValidatorFunction::INVALID;
  }
  Functor functionABI = tx.getFunctor();
  if (functionABI == randomHashHash) {
    return TxValidatorFunction::RANDOMHASH;
  } else if (functionABI == randomSeedHash) {
    return TxValidatorFunction::RANDOMSEED;
  } else {
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__, "TxValidator function ABI is not recognized: " + std::to_string(functionABI.value) + " tx Data: " + Hex::fromBytes(tx.getData()).get());
    return TxValidatorFunction::INVALID;
  }
}

void rdPoS::startrdPoSWorker() { this->worker_.start(); }

void rdPoS::stoprdPoSWorker() { this->worker_.stop(); }

bool rdPoSWorker::checkLatestBlock() {
  if (this->latestBlock_ == nullptr) {
    this->latestBlock_ = this->rdpos_.storage_.latest();
    return false;
  }
  if (this->latestBlock_ != this->rdpos_.storage_.latest()) return true;
  return false;
}

bool rdPoSWorker::workerLoop() {
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->rdpos_.validatorKey_)));
  this->latestBlock_ = this->rdpos_.storage_.latest();
  while (!this->stopWorker_) {
    // Check if we are the validator required for signing the block.
    bool isBlockCreator = false;
    // Scope for unique_lock.
    {
      std::unique_lock checkValidatorsList(this->rdpos_.mutex_);
      if (me == this->rdpos_.randomList_[0]) {
        isBlockCreator = true;
        checkValidatorsList.unlock();
        doBlockCreation();
      }

      // Check if we are one of the rdPoS that need to create random transactions.
      if (!isBlockCreator) {
        for (uint64_t i = 1; i <= this->rdpos_.getMinValidators(); i++) {
          if (me == this->rdpos_.randomList_[i]) {
            checkValidatorsList.unlock();
            doTxCreation(this->latestBlock_->getNHeight() + 1, me);
          }
        }
      }
    }

    // After processing everything. wait until the new block is appended to the chain.
    std::unique_ptr<std::tuple<uint64_t, uint64_t, uint64_t>> lastLog = nullptr;
    while (!this->checkLatestBlock() && !this->stopWorker_) {
      if (lastLog == nullptr ||
          std::get<0>(*lastLog) != this->latestBlock_->getNHeight() ||
          std::get<1>(*lastLog) != this->rdpos_.storage_.latest()->getNHeight() ||
          std::get<2>(*lastLog) != this->rdpos_.validatorMempool_.size()) {
        lastLog = std::make_unique<std::tuple<uint64_t, uint64_t, uint64_t>>(
          this->latestBlock_->getNHeight(),
          this->rdpos_.storage_.latest()->getNHeight(),
          this->rdpos_.validatorMempool_.size()
          );
        Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__,
                           "Waiting for new block to be appended to the chain. (Height: "
                           + std::to_string(this->latestBlock_->getNHeight()) + ")" + " latest height: "
                           + std::to_string(this->rdpos_.storage_.latest()->getNHeight())
          );
        Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__,
                           "Currently has " + std::to_string(this->rdpos_.validatorMempool_.size())
                           + " transactions in mempool."
          );
      }
      std::unique_lock mempoolSizeLock(this->rdpos_.mutex_);
      uint64_t mempoolSize = this->rdpos_.validatorMempool_.size();
      if (mempoolSize < this->rdpos_.getMinValidators()) { // Always try to fill the mempool to 8 transactions
        mempoolSizeLock.unlock();
        // Try to get more transactions from other nodes within the network
        auto connectedNodesList = this->rdpos_.p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
        for (auto const& nodeId : connectedNodesList) {
          if (this->checkLatestBlock() || this->stopWorker_) break;
          auto txList = this->rdpos_.p2p_.requestValidatorTxs(nodeId);
          if (this->checkLatestBlock() || this->stopWorker_) break;
          for (auto const& tx : txList) this->rdpos_.state_.addValidatorTx(tx);
        }
      } else {
        mempoolSizeLock.unlock();
      }
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    // Update latest block if necessary.
    if (isBlockCreator) this->canCreateBlock_ = false;
    this->latestBlock_ = this->rdpos_.storage_.latest();
  }
  return true;
}

void rdPoSWorker::doBlockCreation() {
  // TODO: add requesting transactions to other nodes when mempool is not filled up
  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Block creator: waiting for txs");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize != this->rdpos_.getMinValidators() * 2 && !this->stopWorker_)
  {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__,
                         "Block creator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool"
        );
    }
    // Scope for lock.
    {
      std::unique_lock mempoolSizeLock(this->rdpos_.mutex_);
      validatorMempoolSize = this->rdpos_.validatorMempool_.size();
    }
    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->rdpos_.p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      auto txList = this->rdpos_.p2p_.requestValidatorTxs(nodeId);
      if (this->stopWorker_) return;
      for (auto const& tx : txList) this->rdpos_.state_.addValidatorTx(tx);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Validator ready to create a block");
  // After processing everything, we can let everybody know that we are ready to create a block
  this->canCreateBlock_ = true;
}

void rdPoSWorker::doTxCreation(const uint64_t& nHeight, const Validator& me) {
  Hash randomness = Hash::random();
  Hash randomHash = Utils::sha3(randomness.get());
  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Creating random Hash transaction");
  Bytes randomHashBytes = Hex::toBytes("0xcfffe746");
  randomHashBytes.insert(randomHashBytes.end(), randomHash.get().begin(), randomHash.get().end());
  TxValidator randomHashTx(
    me.address(),
    randomHashBytes,
    this->rdpos_.options_.getChainID(),
    nHeight,
    this->rdpos_.validatorKey_
    );

  Bytes seedBytes = Hex::toBytes("0x6fc5a2d6");
  seedBytes.insert(seedBytes.end(), randomness.get().begin(), randomness.get().end());
  TxValidator seedTx(
    me.address(),
    seedBytes,
    this->rdpos_.options_.getChainID(),
    nHeight,
    this->rdpos_.validatorKey_
    );

  // Sanity check if tx is valid
  BytesArrView randomHashTxView(randomHashTx.getData());
  BytesArrView randomSeedTxView(seedTx.getData());
  if (Utils::sha3(randomSeedTxView.subspan(4)) != randomHashTxView.subspan(4)) {
    Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "RandomHash transaction is not valid!!!");
    return;
  }

  // Append to mempool and broadcast the transaction across all nodes.
  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Broadcasting randomHash transaction");
  this->rdpos_.state_.addValidatorTx(randomHashTx);
  this->rdpos_.p2p_.broadcastTxValidator(randomHashTx);

  // Wait until we received all randomHash transactions to broadcast the randomness transaction
  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Waiting for randomHash transactions to be broadcasted");
  uint64_t validatorMempoolSize = 0;
  std::unique_ptr<uint64_t> lastLog = nullptr;
  while (validatorMempoolSize < this->rdpos_.getMinValidators() && !this->stopWorker_) {
    if (lastLog == nullptr || *lastLog != validatorMempoolSize) {
      lastLog = std::make_unique<uint64_t>(validatorMempoolSize);
      Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__,
                         "Validator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool"
        );
    }
    // Scope for lock
    {
      std::unique_lock mempoolSizeLock(this->rdpos_.mutex_);
      validatorMempoolSize = this->rdpos_.validatorMempool_.size();
    }
    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->rdpos_.p2p_.getSessionsIDs(P2P::NodeType::NORMAL_NODE);
    for (auto const& nodeId : connectedNodesList) {
      if (this->stopWorker_) return;
      auto txList = this->rdpos_.p2p_.requestValidatorTxs(nodeId);
      for (auto const& tx : txList) this->rdpos_.state_.addValidatorTx(tx);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Broadcasting random transaction");
  // Append and broadcast the randomness transaction.
  this->rdpos_.state_.addValidatorTx(seedTx);
  this->rdpos_.p2p_.broadcastTxValidator(seedTx);
}

void rdPoSWorker::start() {
  if (this->rdpos_.isValidator_ && !this->workerFuture_.valid()) {
    this->workerFuture_ = std::async(std::launch::async, &rdPoSWorker::workerLoop, this);
  }
}

void rdPoSWorker::stop() {
  if (this->workerFuture_.valid()) {
    this->stopWorker_ = true;
    this->workerFuture_.wait();
    this->workerFuture_.get();
  }
}

DBBatch rdPoS::dump() const
{
  DBBatch dbBatch;
  // mutex lock
  std::unique_lock lock(this->mutex_);
  // logs
  Logger::logToDebug(LogType::INFO,
                     Log::rdPoS,
                     __func__,
                     "Create batch operations.");
  // index
  uint64_t i = 0;
  // add batch operations
  for (const auto &validator : this->validators_) {
    dbBatch.push_back(Utils::uint64ToBytes(i), validator.get(), DBPrefix::rdPoS);
    i++;
  }
  return dbBatch;
}
