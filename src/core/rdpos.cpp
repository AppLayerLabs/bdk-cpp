/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "rdpos.h"
#include "storage.h"
#include "state.h"
#include "../contract/contractmanager.h"

rdPoS::rdPoS(const DB& db,
             DumpManager& dumpManager,
             const Storage& storage,
             P2P::ManagerNormal& p2p,
             const Options& options,
             State& state)
  : BaseContract("rdPoS", ProtocolContractAddresses.at("rdPoS"), Address(), options.getChainID()),
    options_(options),
    storage_(storage),
    p2p_(p2p),
    state_(state),
    validatorKey_(options.getValidatorPrivKey()),
    isValidator_((this->validatorKey_) ? true : false),
    randomGen_(Hash()), minValidators_(options.getMinValidators())
{
  // Initialize blockchain.
  Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Initializing rdPoS.");
  /**
   * Load information from DB, stored as following:
   * DBPrefix::rdPoS -> rdPoS mapping (addresses)
   * DBPrefix::rdPoS -> misc: used for randomness currently.
   * Order doesn't matter, Validators are stored in a set (sorted by default).
   */
  auto validatorsDb = db.getBatch(DBPrefix::rdPoS);
  if (validatorsDb.empty()) {
    // No rdPoS in DB, this should have been initialized by Storage.
    Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "No rdPoS in DB, initializing chain with Options.");
    for (const auto& address : this->options_.getGenesisValidators()) {
      this->validators_.insert(Validator(address));
    }
  } else {
    Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "Found " + std::to_string(validatorsDb.size()) + " rdPoS in DB");
    // TODO: check if no index is missing from DB.
    for (const auto& validator : validatorsDb) {
      this->validators_.insert(Validator(Address(validator.value)));
    }
  }

  // Load latest randomness from DB, populate and shuffle the random list.
  this->bestRandomSeed_ = storage_.latest()->getBlockRandomness();
  randomGen_.setSeed(this->bestRandomSeed_);
  this->randomList_ = std::vector<Validator>(this->validators_.begin(), this->validators_.end());
  randomGen_.shuffle(randomList_);
  // Register itself at dump management
  dumpManager.pushBack(this);
}

rdPoS::~rdPoS() {}

bool rdPoS::validateBlock(const FinalizedBlock& block) const {
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
  this->validatorMempool_.clear();
  this->randomList_ = std::vector<Validator>(this->validators_.begin(), this->validators_.end());
  this->bestRandomSeed_ = block.getBlockRandomness();
  this->randomGen_.setSeed(this->bestRandomSeed_);
  this->randomGen_.shuffle(this->randomList_);
  return this->bestRandomSeed_;
}

TxStatus rdPoS::addValidatorTx(const TxValidator& tx) {
  if (this->validatorMempool_.contains(tx.hash())) {
    Logger::logToDebug(LogType::INFO, Log::rdPoS, __func__, "TxValidator already exists in mempool.");
    return TxStatus::ValidExisting;
  }

  if (tx.getNHeight() != this->storage_.latest()->getNHeight() + 1) {
    Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__,
      "TxValidator is not for the next block. Expected: "
      + std::to_string(this->storage_.latest()->getNHeight() + 1)
      + " Got: " + std::to_string(tx.getNHeight())
    );
    return TxStatus::InvalidUnexpected;
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
    return TxStatus::InvalidUnexpected;
  }

  // Do not allow duplicate transactions for the same function, we only have two functions (2 TxValidator per validator per block)
  std::vector<TxValidator> txs;
  for (auto const& [key, value] : this->validatorMempool_) {
    if (value.getFrom() == tx.getFrom()) txs.push_back(value);
  }

  if (txs.empty()) { // No transactions from this sender yet, add it.
    this->validatorMempool_.emplace(tx.hash(), tx);
    return TxStatus::ValidNew;
  }

  if (txs.size() == 1) { // We already have one transaction from this sender, check if it is the same function.
    if (txs[0].getFunctor() == tx.getFunctor()) {
      Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__, "TxValidator sender already has a transaction for this function.");
      return TxStatus::InvalidRedundant;
    }
    this->validatorMempool_.emplace(tx.hash(), tx);
    return TxStatus::ValidNew;
  }

  // We already have two transactions from this sender, it is the max we can have per validator.
  Logger::logToDebug(LogType::ERROR, Log::rdPoS, __func__, "TxValidator sender already has two transactions.");
  return TxStatus::InvalidRedundant;
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

DBBatch rdPoS::dump() const
{
  DBBatch dbBatch;
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