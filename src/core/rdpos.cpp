#include "rdpos.h"
#include "storage.h"
#include "state.h"
#include "../utils/block.h"

rdPoS::rdPoS(const std::unique_ptr<DB>& db,
  const std::unique_ptr<Storage>& storage,
  const std::unique_ptr<P2P::ManagerNormal>& p2p,
  const std::unique_ptr<Options>& options,
  const std::unique_ptr<State>& state) :
  Contract("rdPoS", ProtocolContractAddresses.at("rdPoS"), Address(), options->getChainID(), db),
  storage(storage),
  p2p(p2p),
  options(options),
  state(state),
  worker(std::make_unique<rdPoSWorker>(*this)),
  validatorKey(options->getValidatorPrivKey()),
  isValidator((validatorKey) ? true : false),
  randomGen(Hash()) 
{
  std::unique_lock lock(this->mutex);
  Utils::logToDebug(Log::rdPoS, __func__, "Initializing rdPoS.");
  // Initialize blockchain.
  initializeBlockchain();
  // Load information from DB.
  // DB is stored as following
  // DBPrefix::validators -> validators mapping (addresses)
  // DBPrefix::rdPoS -> misc: used for randomness currently.
  // DB Order doesn't matter, Validators are stored in a set (sorted by default)
  auto validatorsDb = db->getBatch(DBPrefix::validators);
  if (validatorsDb.size() == 0) {
    // No validators in DB, this should have been initialized by Storage.
    Utils::logToDebug(Log::rdPoS, __func__, "No validators in DB, cannot proceed.");
    throw std::runtime_error("No validators in DB.");
  }
  Utils::logToDebug(Log::rdPoS, __func__, "Found " + std::to_string(validatorsDb.size()) + " validators in DB");
  // TODO: check if no index is missing from DB.
  for (const auto& validator : validatorsDb) {
    this->validators.insert(Validator(Address(validator.value, true)));
  }

  // Load latest randomness from DB.
  this->bestRandomSeed = storage->latest()->getBlockRandomness();

  // Populate the random list
  randomGen.setSeed(bestRandomSeed);
  this->randomList = std::vector<Validator>(this->validators.begin(), this->validators.end());

  randomGen.shuffle(randomList);
}

rdPoS::~rdPoS() {
  this->stoprdPoSWorker();
  std::unique_lock lock(this->mutex);
  DBBatch validatorsBatch;
  Utils::logToDebug(Log::rdPoS, __func__, "Descontructing rdPoS, saving to DB.");
  // Save validators to DB.
  uint64_t index = 0;
  for (const auto &validator : validators) {
    validatorsBatch.puts.emplace_back(Utils::uint64ToBytes(index), validator.get());
    ++index;
  }
}

bool rdPoS::validateBlock(const Block& block) const {
  std::lock_guard lock(this->mutex);
  auto latestBlock = this->storage->latest();
  // Check if block signature matches randomList[0]
  if (!block.isFinalized()) {
    Utils::logToDebug(Log::rdPoS, __func__, "Block is not finalized, cannot be validated. latest nHeight: "
                      + std::to_string(latestBlock->getNHeight()) + " Block nHeight: " + std::to_string(block.getNHeight()));
    return false;
  }

  if (Secp256k1::toAddress(block.getValidatorPubKey()) != randomList[0]) {
    Utils::logToDebug(Log::rdPoS, __func__, "Block signature does not match randomList[0]. latest nHeight: "
                      + std::to_string(latestBlock->getNHeight()) + " Block nHeight: " + std::to_string(block.getNHeight()));
    return false;
  }

  if (block.getTxValidators().size() != this->minValidators * 2) {
    Utils::logToDebug(Log::rdPoS, __func__, "Block contains invalid number of TxValidator transactions. latest nHeight: "
                      + std::to_string(latestBlock->getNHeight()) + " Block nHeight: " + std::to_string(block.getNHeight()));
    return false;
  }

  // Check if all transactions are of the block height
  for (const auto& tx : block.getTxValidators()) {
    if (tx.getNHeight() != block.getNHeight()) {
      Utils::logToDebug(Log::rdPoS, __func__, "TxValidator transaction is not of the same block height. tx nHeight: "
                        + std::to_string(tx.getNHeight()) + " Block nHeight: " + std::to_string(block.getNHeight()));
      return false;
    }
  }

  // TxValidator transactions within the block must be *ordered* by their respective signer.
  // This is to ensure that the randomness is the same for all validators.
  // Given a minValidator of 4, the block should contain 8 TxValidator transactions.
  // The first 4 (minValidators) transactions from should match randomList[1] to randomList[5] (minValidators + 1)
  // The first 4 (minValidators) transactions should be randomHash transactions. (0xcfffe746), which contains the Sha3(seed).
  // The remaining 4 (minValidators) transactions from should also match randomList[1] to randomList[5] (minValidators +1)
  // The remaining 4 (minValidators) transactions should be random transactions. (0x6fc5a2d6), which contains the seed itself.

  std::unordered_map<TxValidator,TxValidator, SafeHash> txHashToSeedMap; /// Tx randomHash -> Tx random
  for (uint64_t i = 0; i < this->minValidators; ++i) {
    if (Validator(block.getTxValidators()[i].getFrom()) != randomList[i+1]) {
      Utils::logToDebug(Log::rdPoS, __func__, "TxValidator randomHash " + std::to_string(i) + " is not ordered correctly." +
                        "Expected: " + randomList[i+1].hex().get() + " Got: " + block.getTxValidators()[i].getFrom().hex().get());
      return false;
    }
    if (Validator(block.getTxValidators()[i + this->minValidators].getFrom()) != randomList[i+1]) {
      Utils::logToDebug(Log::rdPoS, __func__, "TxValidator random " + std::to_string(i) + " is not ordered correctly." +
                        "Expected: " + randomList[i+1].hex().get() + " Got: " + block.getTxValidators()[i].getFrom().hex().get());
      return false;
    }

    txHashToSeedMap.emplace(block.getTxValidators()[i],block.getTxValidators()[i + this->minValidators]);
  }

  if (txHashToSeedMap.size() != this->minValidators) {
    Utils::logToDebug(Log::rdPoS, __func__, "txHashToSeedMap doesn't match minValidator size.");
    return false;
  }

  // Check the transactions within the block, we should have every transaction within the txHashToSeed map.
  for (auto const& [hashTx, seedTx] : txHashToSeedMap) {
    TxValidatorFunction hashTxFunction = this->getTxValidatorFunction(hashTx);
    TxValidatorFunction seedTxFunction = this->getTxValidatorFunction(seedTx);
    // Check if hash tx is invalid by itself.
    if (hashTxFunction == TxValidatorFunction::INVALID) {
      Utils::logToDebug(Log::rdPoS, __func__, std::string("TxValidator ") + hashTx.hash().hex().get()  + " is invalid.");
      return false;
    }
    if (seedTxFunction == TxValidatorFunction::INVALID) {
      Utils::logToDebug(Log::rdPoS, __func__, std::string("TxValidator ") + seedTx.hash().hex().get()  + " is invalid.");
      return false;
    }
    // Check if senders match.
    if (hashTx.getFrom() != seedTx.getFrom()) {
      Utils::logToDebug(Log::rdPoS, __func__, std::string("TxValidator sender ") + seedTx.hash().hex().get()
                                              + " does not match TxValidator sender " 
                                              + hashTx.hash().hex().get());
      return false;
    }
    // Check if the left sided transaction is a randomHash transaction.
    if (hashTxFunction != TxValidatorFunction::RANDOMHASH) {
      Utils::logToDebug(Log::rdPoS, __func__, std::string("TxValidator ") + hashTx.hash().hex().get()  + " is not a randomHash transaction.");
      return false;
    }
    // Check if the right sided transaction is a random transaction.
    if (seedTxFunction != TxValidatorFunction::RANDOMSEED) {
      Utils::logToDebug(Log::rdPoS, __func__, std::string("TxValidator ") + seedTx.hash().hex().get()  + " is not a random transaction.");
      return false;
    }
    // Check if the randomHash transaction matches the random transaction.
    std::string_view hash = std::string_view(hashTx.getData()).substr(4);
    std::string_view random = std::string_view(seedTx.getData()).substr(4);
    if (Utils::sha3(random) != hash) {
      Utils::logToDebug(Log::rdPoS, __func__, std::string("TxValidator ") + seedTx.hash().hex().get()
                                              + " does not match TxValidator " + hashTx.hash().hex().get()
                                              + " randomness");
      return false;
    }
  }

  return true;
}

Hash rdPoS::processBlock(const Block& block) {
  std::unique_lock lock(this->mutex);
  if (!block.isFinalized()) {
    Utils::logToDebug(Log::rdPoS, __func__, "Block is not finalized.");
    throw std::runtime_error("Block is not finalized.");
  }
  validatorMempool.clear();
  this->randomList = std::vector<Validator>(this->validators.begin(), this->validators.end());
  this->bestRandomSeed = block.getBlockRandomness();
  randomGen.setSeed(bestRandomSeed);
  randomGen.shuffle(randomList);
  return this->bestRandomSeed;
}

void rdPoS::signBlock(Block &block) {
  uint64_t newTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  block.finalize(this->validatorKey, newTimestamp);
  this->worker->blockCreated();
}

bool rdPoS::addValidatorTx(const TxValidator& tx) {
  std::unique_lock lock(this->mutex);
  if (this->validatorMempool.contains(tx.hash())) {
    Utils::logToDebug(Log::rdPoS, __func__, "TxValidator already exists in mempool.");
    return true;
  }

  if (tx.getNHeight() != this->storage->latest()->getNHeight() + 1) {
    Utils::logToDebug(Log::rdPoS, __func__, "TxValidator is not for the next block. expected: "
                        + std::to_string(this->storage->latest()->getNHeight() + 1) + " got: " + std::to_string(tx.getNHeight()));
    return false;
  }

  // Check if sender is a validator and can participate in this rdPoS round (check from existance in randomList)
  bool participates = false;
  for (uint64_t i = 1; i < this->minValidators + 1; ++i) {
    if (Validator(tx.getFrom()) == randomList[i]) {
      participates = true;
      break;
    }
  }
  if (!participates) {
    Utils::logToDebug(Log::rdPoS, __func__, "TxValidator sender is not a validator or is not participating in this rdPoS round.");
    return false;
  }

  // Do not allow duplicate transactions for the same function, we only have two functions (2 TxValidator per validator per block)
  std::vector<TxValidator> txs;
  for (auto const& [key, value] : validatorMempool) {
    if (value.getFrom() == tx.getFrom()) {
      txs.push_back(value);
    }
  }

  if (txs.size() == 0) { // No transactions from this sender yet, add it.
    validatorMempool.emplace(tx.hash(), tx);
    return true;
  } else if (txs.size() == 1) { // We already have one transaction from this sender, check if it is the same function.
    if (txs[0].getData().substr(0,4) == tx.getData().substr(0,4)) {
      Utils::logToDebug(Log::rdPoS, __func__, "TxValidator sender already has a transaction for this function.");
      return false;
    }
    validatorMempool.emplace(tx.hash(), tx);
  } else { // We already have two transactions from this sender, it is the max we can have per validator.
    Utils::logToDebug(Log::rdPoS, __func__, "TxValidator sender already has two transactions.");
    return false;
  }

  return true;
}

void rdPoS::initializeBlockchain() {
  auto validatorsDb = db->getBatch(DBPrefix::validators);
  if (validatorsDb.size() == 0) {
    Utils::logToDebug(Log::rdPoS,__func__, "No validators in DB, initializing.");
    // TODO: CHANGE THIS ON PUBLIC!!! THOSE PRIVATE KEYS SHOULD ONLY BE USED FOR LOCAL TESTING
    // 0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759
    this->db->put(Utils::uint64ToBytes(0), Address(Hex::toBytes("0x7588b0f553d1910266089c58822e1120db47e572"), true).get(), DBPrefix::validators);
    // 0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4
    this->db->put(Utils::uint64ToBytes(1), Address(Hex::toBytes("0xcabf34a268847a610287709d841e5cd590cc5c00"), true).get(), DBPrefix::validators);
    // 0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751
    this->db->put(Utils::uint64ToBytes(2), Address(Hex::toBytes("0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3"), true).get(), DBPrefix::validators);
    // 0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7
    this->db->put(Utils::uint64ToBytes(3), Address(Hex::toBytes("0x795083c42583842774febc21abb6df09e784fce5"), true).get(), DBPrefix::validators);
    // 0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6
    this->db->put(Utils::uint64ToBytes(4), Address(Hex::toBytes("0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"), true).get(), DBPrefix::validators);
  }
}

Hash rdPoS::parseTxSeedList(const std::vector<TxValidator>& txs) {
  std::string seed;
  if (txs.size() == 0) return Hash();
  for (const TxValidator& tx : txs) {
    if (rdPoS::getTxValidatorFunction(tx) == TxValidatorFunction::RANDOMSEED) {
      seed += tx.getData().substr(4,32);
    }
  }
  return Utils::sha3(seed);
}

const std::atomic<bool>& rdPoS::canCreateBlock() const {
  return this->worker->getCanCreateBlock();
}

rdPoS::TxValidatorFunction rdPoS::getTxValidatorFunction(const TxValidator &tx) {
  constexpr std::string_view randomHashHash("\xcf\xff\xe7\x46",4);
  constexpr std::string_view randomSeedHash("\x6f\xc5\xa2\xd6",4);
  if (tx.getData().size() != 36) {
    Utils::logToDebug(Log::rdPoS, __func__, "TxValidator data size is not 36 bytes.");
    // Both RandomHash and RandomSeed are 32 bytes, so if the data size is not 36 bytes, it is invalid.
    return TxValidatorFunction::INVALID;
  }
  std::string_view functionABI = std::string_view(tx.getData()).substr(0, 4);
  if (functionABI == randomHashHash) {
    return TxValidatorFunction::RANDOMHASH;
  } else if (functionABI == randomSeedHash) {
    return TxValidatorFunction::RANDOMSEED;
  } else {
    Utils::logToDebug(Log::rdPoS, __func__, "TxValidator function ABI is not recognized.");
    return TxValidatorFunction::INVALID;
  }
}

void rdPoS::startrdPoSWorker() {
  this->worker->start();
}

void rdPoS::stoprdPoSWorker() {
  this->worker->stop();
}

bool rdPoSWorker::checkLatestBlock() {
  if (this->latestBlock == nullptr) {
    this->latestBlock = this->rdpos.storage->latest();
    return false;
  }
  if (this->latestBlock != this->rdpos.storage->latest()) {
    return true;
  }
  return false;
}

bool rdPoSWorker::workerLoop() {
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->rdpos.validatorKey)));
  this->latestBlock = this->rdpos.storage->latest();
  while (!this->stopWorker) {
    // Check if we are the validator required for signing the block.
    bool isBlockCreator = false;
    // Scope for unique_lock.
    {
      std::unique_lock checkValidatorsList(this->rdpos.mutex);
      if (me == this->rdpos.randomList[0]) {
        isBlockCreator = true;
        checkValidatorsList.unlock();
        doBlockCreation();
      }

      // Check if we are one of the validators that need to create random transactions.
      if (!isBlockCreator) {
        for (uint64_t i = 1; i <= this->rdpos.minValidators; ++i) {
          if (me == this->rdpos.randomList[i]) {
            checkValidatorsList.unlock();
            doTxCreation(latestBlock->getNHeight() + 1, me);
          }
        }
      }
    }

    // After processing everything. wait until the new block is appended to the chain.
    while (!this->checkLatestBlock() && !this->stopWorker) {
      Utils::logToDebug(Log::rdPoS, __func__, "Waiting for new block to be appended to the chain. (Height: " + std::to_string(latestBlock->getNHeight()) + ")" + " latest height: " + std::to_string(this->rdpos.storage->latest()->getNHeight()));
      Utils::logToDebug(Log::rdPoS, __func__, "Currently has " + std::to_string(this->rdpos.validatorMempool.size()) + " transactions in mempool.");
      std::unique_lock mempoolSizeLock(this->rdpos.mutex);
      uint64_t mempoolSize = this->rdpos.validatorMempool.size();
      if (mempoolSize < this->rdpos.minValidators) { /// Always try to fill the mempool to 8 transaction
        mempoolSizeLock.unlock();
        // Try to get more transactions from other nodes within the network
        auto connectedNodesList = this->rdpos.p2p->getSessionsIDs();
        for (auto const& nodeId : connectedNodesList) {
          if (this->checkLatestBlock() || this->stopWorker) break;
          auto txList = this->rdpos.p2p->requestValidatorTxs(nodeId);
          if (this->checkLatestBlock() || this->stopWorker) break;
          for (auto const& tx : txList) {
            this->rdpos.state->addValidatorTx(tx);
          }
        }
      } else {
        mempoolSizeLock.unlock();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    if (isBlockCreator) this->canCreateBlock = false;
    // Update latest block.
    this->latestBlock = this->rdpos.storage->latest();
  }
  return true;
}

void rdPoSWorker::doBlockCreation() {
  // TODO: add requesting transactions to other nodes when mempool is not filled up
  Utils::logToDebug(Log::rdPoS, __func__, "Block creator: waiting for txs");
  uint64_t validatorMempoolSize = 0;
  while (validatorMempoolSize != this->rdpos.minValidators * 2 && !this->stopWorker)
  {
    Utils::logToDebug(Log::rdPoS, __func__, "Block creator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool");
    // Scope for lock.
    {
      std::unique_lock mempoolSizeLock(this->rdpos.mutex);
      validatorMempoolSize = this->rdpos.validatorMempool.size();
    }
    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->rdpos.p2p->getSessionsIDs();
    for (auto const& nodeId : connectedNodesList) {
      auto txList = this->rdpos.p2p->requestValidatorTxs(nodeId);
      if (this->stopWorker) return;
      for (auto const& tx : txList) {
        this->rdpos.state->addValidatorTx(tx);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  Utils::logToDebug(Log::rdPoS, __func__, "Validator ready to create a block");
  // After processing everything, we can let everybody know that we are ready to create a block
  this->canCreateBlock = true;
}

void rdPoSWorker::doTxCreation(const uint64_t& nHeight, const Validator& me) {
  Hash randomness = Hash::random();
  Hash randomHash = Utils::sha3(randomness.get());
  Utils::logToDebug(Log::rdPoS, __func__, "Creating random Hash transaction");
  TxValidator randomHashTx(
      me.address(),
      Hex::toBytes("0xcfffe746") + randomHash.get(),
      this->rdpos.options->getChainID(),
      nHeight,
      this->rdpos.validatorKey
  );

  TxValidator seedTx(
      me.address(),
      Hex::toBytes("0x6fc5a2d6") + randomness.get(),
      this->rdpos.options->getChainID(),
      nHeight,
      this->rdpos.validatorKey
    );

  // Sanity check if tx is valid
  if (Utils::sha3(std::string_view(seedTx.getData()).substr(4)) != std::string_view(randomHashTx.getData().substr(4))) {
    Utils::logToDebug(Log::rdPoS, __func__, "RandomHash transaction is not valid!!!");
    return;
  }

  // Append to mempool and broadcast the transaction across all nodes.
  Utils::logToDebug(Log::rdPoS, __func__, "Broadcasting randomHash transaction");
  this->rdpos.state->addValidatorTx(randomHashTx);
  this->rdpos.p2p->broadcastTxValidator(randomHashTx);

  // Wait until we received all randomHash transactions to broadcast the randomness transaction
  Utils::logToDebug(Log::rdPoS, __func__, "Waiting for randomHash transactions to be broadcasted");
  uint64_t validatorMempoolSize = 0;
  while (validatorMempoolSize < this->rdpos.minValidators && !this->stopWorker) {
    Utils::logToDebug(Log::rdPoS, __func__, "Validator has: " + std::to_string(validatorMempoolSize) + " transactions in mempool");
    // Scope for lock
    {
      std::unique_lock mempoolSizeLock(this->rdpos.mutex);
      validatorMempoolSize = this->rdpos.validatorMempool.size();
    }
    // Try to get more transactions from other nodes within the network
    auto connectedNodesList = this->rdpos.p2p->getSessionsIDs();
    for (auto const& nodeId : connectedNodesList) {
      if (this->stopWorker) return;
      auto txList = this->rdpos.p2p->requestValidatorTxs(nodeId);
      for (auto const& tx : txList) {
        this->rdpos.state->addValidatorTx(tx);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }

  Utils::logToDebug(Log::rdPoS, __func__, "Broadcasting random transaction");
  // Append and broadcast the randomness transaction.
  this->rdpos.state->addValidatorTx(seedTx);
  this->rdpos.p2p->broadcastTxValidator(seedTx);
}
void rdPoSWorker::start() {
  if (this->rdpos.isValidator && !this->workerFuture.valid()) {
    this->workerFuture = std::async(std::launch::async, &rdPoSWorker::workerLoop, this);
  }
}

void rdPoSWorker::stop() {
  if (this->workerFuture.valid()) {
    this->stopWorker = true;
    this->workerFuture.wait();
    this->workerFuture.get();
  }
}