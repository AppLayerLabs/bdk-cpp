#include "blockmanager.h"
#include "subnet.h"
#include "state.h"
#include "block.h"

BlockManager::BlockManager(std::shared_ptr<DBService> &db, 
                           const std::shared_ptr<const ChainHead> chainHead, 
                           std::shared_ptr<P2PManager> p2pmanager,
                           std::shared_ptr<VMCommClient> grpcClient,
                           const Address &address, const Address &owner) : 
                            _isValidator(false), 
                            Contract(address, owner), 
                            gen(Hash()), 
                            chainHead(chainHead),
                            p2pmanager(p2pmanager),
                            grpcClient(grpcClient) {
  loadFromDB(db);
  Utils::logToFile("BlockManager Loaded " + std::to_string(validatorsList.size()) + " validators");
  for (auto &validator : validatorsList) {
    Utils::logToFile("Validator: " + validator.hex());
  }
}

BlockManager::BlockManager(std::shared_ptr<DBService> &db, 
                           const std::shared_ptr<const ChainHead> chainHead, 
                           std::shared_ptr<P2PManager> p2pmanager,
                           std::shared_ptr<VMCommClient> grpcClient,
                           const PrivKey& privKey, 
                           const Address &address, 
                           const Address &owner) : 
                            _validatorPrivKey(privKey), 
                            _isValidator(true), 
                            Contract(address, owner), 
                            gen(Hash()), 
                            chainHead(chainHead),
                            p2pmanager(p2pmanager) {
  loadFromDB(db);
  Utils::logToFile("BlockManager Loaded " + std::to_string(validatorsList.size()) + " validators");
  for (auto &validator : validatorsList) {
    Utils::logToFile("Validator: " + validator.hex());
  }
}


// Validators are stored in DB as list, 8 bytes (uint64_t) index -> 32 bytes (pubkey).
void BlockManager::loadFromDB(std::shared_ptr<DBService> &db) {
  auto validators = db->readBatch(DBPrefix::validators);
  validatorsList.reserve(validators.size());
  for (auto const &validator : validators) {
    auto it = validatorsList.begin();
    if (validator.key.size() != 8) {
      Utils::LogPrint(Log::blockManager,__func__,"Validator key size is not 8 bytes");
      throw std::runtime_error("Validator key size is not 8 bytes");
    }
    Utils::LogPrint(Log::blockManager,__func__,std::to_string(validator.value.size()));
    if (validator.value.size() != 20) {
      Utils::LogPrint(Log::blockManager,__func__,"Validator value is not 20 bytes (address)");
      throw std::runtime_error("Validator value is not 20 bytes (address)");
    }
    // Emplace_back, make sure that the list is *ordered* before saving to DB.
    validatorsList.emplace_back(Validator(Address(validator.value, false)));
    Utils::logToFile("Loop End");
  }
  this->randomList = std::vector<std::reference_wrapper<Validator>>(validatorsList.begin(), validatorsList.end());
  this->gen.setSeed(chainHead->latest()->randomness());
  this->gen.shuffleVector(this->randomList);

  for (uint64_t i = 0; i < randomList.size(); ++i) {
    Utils::logToFile(std::string("Validator ") + std::to_string(i) + " " + randomList[i].get().hex());
  }
}

bool BlockManager::validateBlock(const std::shared_ptr<const Block> &block) const {
  // Check validator signature agaisn't block height prevBlockHash. (not sure if enought for safety)
  managerLock.lock();
  Hash hash = block->getBlockHash();

  // TODO: do we need to include chainId In the block signature?
  auto pubkey = Secp256k1::recover(block->signature(), hash);
  if (Secp256k1::toAddress(pubkey) != this->validatorsList[0].get()) {
    Utils::LogPrint(Log::blockManager,__func__,"Block validator signature does not match validator[0]");
    managerLock.unlock();
    return false;
  }

  managerLock.unlock();
  return true;
}

Hash BlockManager::parseTxListSeed(const std::unordered_map<uint64_t, Tx::Validator, SafeHash> &transactions) {
  std::string seed;
  if (transactions.size() == 0) {
    return Hash();
  }
  for (uint64_t i = 0; i < transactions.size(); i++) {
    seed += transactions.at(i).data().substr(4,32);
  }
  return Utils::sha3(seed);
}

BlockManager::TransactionTypes BlockManager::getTransactionType(const Tx::Validator &tx) {
  std::string_view functor = tx.data().substr(0,4);

  if (functor == Utils::hexToBytes("0x4d238c8e")) {
    return TransactionTypes::addValidator;
  }

  if (functor == Utils::hexToBytes("0x40a141ff")) {
    return TransactionTypes::removeValidator;
  }

  if (functor == Utils::hexToBytes("0xcfffe746")) {
    return TransactionTypes::randomHash;
  }

  if (functor == Utils::hexToBytes("0x6fc5a2d6")) {
    return TransactionTypes::randomSeed;
  }

  Utils::LogPrint(Log::blockManager, __func__, std::string("Error: functor not found"));
  throw std::runtime_error("Functor not found in contract");
}


// TODO: Edge cases, malicious validators, etc.
// There is no handling to malicious validators answering invalid hashes, it will simply throw and segfault!
void BlockManager::validatorLoop() {
  Validator myself(Secp256k1::toAddress(Secp256k1::toPub(this->_validatorPrivKey)));

  while (true) {
    auto latestBlock = this->chainHead->latest();
    // Check if we are next in line, and which type (block creator or randomizer)
    if (myself == this->randomList[0].get()) {
      // We are the validator which should create the next block

      // Wait until we have all the transactions we need.
      Utils::LogPrint(Log::blockManager, __func__, "Waiting for all transactions for block creation...");
      this->managerLock.lock();
      uint32_t tries = 0;
      while (validatorMempool.size() != this->minValidators * 2) {
        Utils::logToFile("validatorLoop: mempool size: " + std::to_string(validatorMempool.size()));
        managerLock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (tries == 10) {
          Utils::LogPrint(Log::blockManager, __func__, std::string("Requesting validator transactions..."));
          this->p2pmanager->requestValidatorTransactionsToAll();
        }
        ++tries;
        this->managerLock.lock();
      }
      this->managerLock.unlock();

      // Tell back AvalancheGo that we are ready to create the block.
      this->grpcClient->requestBlock();
    } 

    for (uint64_t i = 1; i <= this->minValidators; ++i) {
      // If we are any of the validators selected for the randomizer, create the hash transaction and relay it, 
      // then wait for the next set of transactions (randomness transactions)
      Utils::logToFile(std::string("Looping: ") + std::to_string(i) + " min: " + std::to_string(this->minValidators));
      this->managerLock.lock();
      if (myself == this->randomList[i].get()) {
        this->managerLock.unlock();
        // Create the random hash transaction
        Hash myRandom = Hash::random();
        Utils::LogPrint(Log::blockManager, __func__, std::string("Creating random hash transaction for block: ") + latestBlock->getBlockHash().hex() + " height: " + std::to_string(latestBlock->nHeight()));
        Tx::Validator randomHashTx(myself.get(), Utils::hexToBytes("0xcfffe746") + Utils::sha3(myRandom.get_view()).get(), 8848, latestBlock->nHeight());
        randomHashTx.sign(this->_validatorPrivKey);
        p2pmanager->broadcastValidatorTx(randomHashTx);

        // Sleep until all the hash transactions of the other nodes were broadcasted and rebroadcast your own tx.
        this->managerLock.lock();
        this->validatorMempool[randomHashTx.hash()] = randomHashTx;
        uint32_t tries = 0;
        while (this->validatorMempool.size() != this->minValidators) {
          this->managerLock.unlock();
          Utils::LogPrint(Log::blockManager, __func__, std::string("Sleeping until all hash transactions are broadcasted ") + std::to_string(this->validatorMempool.size()));
          if (tries == 10) { // Wait for 10 seconds before asking...
            tries = 0;
            Utils::LogPrint(Log::blockManager, __func__, std::string("Requesting validator transactions..."));
            this->p2pmanager->requestValidatorTransactionsToAll();
          }
          std::this_thread::sleep_for(std::chrono::seconds(1));
          this->managerLock.lock();
          ++tries;
        }
        this->managerLock.unlock();
        
        // Send the transaction with the original random.
        
        
        Tx::Validator randomTx(myself.get(), Utils::hexToBytes("0x6fc5a2d6") + myRandom.get(), 8848, latestBlock->nHeight());
        randomTx.sign(this->_validatorPrivKey);
        p2pmanager->broadcastValidatorTx(randomHashTx);
        this->managerLock.lock();
        this->validatorMempool[randomTx.hash()] = randomHashTx;
      }
      this->managerLock.unlock();
      
      
    }

    // Sleep until next block.
    while (this->chainHead->latest()->nHeight() == latestBlock->nHeight()) {
      Utils::LogPrint(Log::blockManager, __func__, std::string("Sleeping until new block. mempool size: ") + std::to_string(validatorMempool.size()));
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}


 /*
 Validator(const Address &from,
        const std::string &data, 
        const uint64_t &chainId, 
        const uint256_t &nHeight)
 
 */
void BlockManager::startValidatorThread() {
  this->managerLock.lock();
  if (this->_isValidator) {
    if (!this->_isValidatorThreadRunning) {
      this->_isValidatorThreadRunning = true;
      Utils::LogPrint(Log::blockManager, __func__, "Starting validator thread...");
      std::thread t(&BlockManager::validatorLoop, this);
      t.detach();
    }
  }
  this->managerLock.unlock();
}

void BlockManager::addValidatorTx(const Tx::Validator &tx) {
  this->managerLock.lock();
  bool broadcast = false;
  if (this->validatorMempool.count(tx.hash()) == 0) { broadcast = true; }
  if (tx.nHeight() == this->chainHead->latest()->nHeight()) {
    for (uint64_t i = 1; i <= this->minValidators; ++i) {
      if (this->randomList[i].get() == tx.from()) {
        this->validatorMempool[tx.hash()] = tx;
        this->managerLock.unlock();
        if (broadcast) {
          this->p2pmanager->broadcastValidatorTx(tx);
        }
        return;
      }
    }
  }
  this->managerLock.unlock();
}

std::unordered_map<Hash, Tx::Validator, SafeHash> BlockManager::getMempoolCopy() {
  this->managerLock.lock();
  auto ret = this->validatorMempool;
  this->managerLock.unlock();
  return ret;
}

std::vector<std::reference_wrapper<Validator>> BlockManager::getRandomListCopy() {
  this->managerLock.lock();
  auto ret = this->randomList;
  this->managerLock.unlock();
  return ret;
}