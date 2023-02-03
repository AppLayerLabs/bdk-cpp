#include "rdpos.h"

void rdPoS::loadFromDB() {
  std::vector<DBEntry> validators = this->db->getBatch(DBPrefix::validators);
  validatorList.reserve(validators.size());
  for (const DBEntry& v : validators) {
    auto it = validatorList.begin();
    if (v.key.size() != 8) {
      Utils::logToDebug(Log::rdpos, __func__, "Validator key size is not 8 bytes");
      throw std::runtime_error("Validator key size is not 8 bytes");
    }
    if (v.value.size() != 20) {
      Utils::LogPrint(Log::rdpos, __func__, "Validator value size is not 20 bytes (address)");
      throw std::runtime_error("Validator value size is not 20 bytes (address)");
    }
    // Make sure that list is *ordered* before savind to DB
    validatorList.emplace_back(Validator(Address(v.value, false)));
  }
  this->randomList = std::vector<std::reference_wrapper<Validator>>(
    validatorList.begin(), validatorList.end()
  );
  this->gen.setSeed(this->storage->latest()->getRandomness());
  this->gen.shuffle(this->randomList);

  for (uint64_t i = 0; i < randomList.size(); i++) {
    Utils::logToDebug(Log::rdpos, __func__,
      std::string("Validator ") + std::to_string(i)
      + std::string(" - ") + randomList[i].get().hex());
  }
}

bool rdPoS::shuffle() {
  ; // TODO: not implemented in original
}

// TODO: edge cases, handle malicious validators, etc.
// There is no handling to malicious validators answering invalid hashes, it will simply throw and segfault!
void rdPoS::validatorLoop() {
  Validator me(Secp256k1::toAddress(Secp256k1::toPub(this->validatorPrivKey)));

  while (true) {
    // Check if we're next in line, and which type (block creator or randomizer)
    std::shared_ptr<const Block> latest = this->storage->latest();
    if (me == this->randomList[0].get()) {
      // We're the block creator so we wait until we have all the txs we need
      Utils::logToDebug(Log::rdpos, __func__, "Block creator: waiting for txs...");
      this->lock.lock();
      uint32_t tries = 0;
      while (this->validatorMempool.size() != this->minValidators * 2) {
        Utils::logToDebug(Log::rdpos, __func__,
          std::string("Block creator: mempool size = ")
          + std::to_string(validatorMempool.size())
        );
        this->lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (tries == 10) {
          Utils::logToDebug(Log::rdpos, __func__,
            std::string("Block creator: requesting validator txs...")
          );
          this->p2p->requestValidatorTxsToAll();
          tries = 0;
        }
        tries++;
        this->lock.lock();
      }
      this->lock.unlock();

      // Tell AvalancheGo that we're ready to create the block
      //this->grpcClient->requestBlock(); // TODO: we're not using gRPC here
    }

    for (uint64_t i = 1; i <= this->minValidators; i++) {
      // If we're any of the randomizer validators, we create the random
      // hash tx and relay it, then wait for the next set of random hash txs
      this->lock.lock();
      if (me == this->randomList[i].get()) {
        this->lock.unlock();
        // Create the random hash tx
        Hash randomHash = Hash::random();
        Utils::logToDebug(Log::rdpos, __func__,
          std::string("Randomizer: creating random hash tx for block ")
          + latest->getBlockHash().hex() + std::string(" at height ")
          + std::to_string(latest->getNHeight())
        );
        TxValidator randomHashTx(me.get(),
          Utils::hexToBytes("0xcfffe746") + Utils::sha3(randomHash.view()).get(),
          8848, latest->getNHeight()
        );
        randomHashTx.sign(this->validatorPrivKey);
        this->p2p->broadcastValidatorTx(randomHashTx);

        // Wait until all other random hash txs were broadcast,
        // then rebroadcast our own random hash tx
        this->lock.lock();
        this->validatorMempool[randomHashTx.hash()] = randomHashTx;
        uint32_t tries = 0;
        while (this->validatorMempool.size() < this->minValidators) {
          this->lock.unlock();
          Utils::logToDebug(Log::rdpos, __func__,
            std::string("Randomizer: waiting until all hash txs are broadcast... validator mempool size = ")
            + std::to_string(this->validatorMempool.size())
          );
          if (tries == 10) { // Wait for 10 seconds before asking...
            tries = 0;
            Utils::logToDebug(Log::rdpos, __func__,
              std::string("Randomizer: requesting validator txs...")
            );
            this->p2p->requestValidatorTxsToAll();
          }
          std::this_thread::sleep_for(std::chrono::seconds(1));
          this->lock.lock();
          tries++;
        }
        this->lock.unlock();

        // Send the tx and the original random.
        TxValidator randomTx(me.get(),
          Utils::hexToBytes("0x6fc5a2d6") + randomHash.get(),
          8848, latest->getNHeight()
        );
        randomTx.sign(this->validatorPrivKey);
        this->p2p->broadcastValidatorTx(randomTx);
        this->lock.lock();
        this->validatorMempool[randomTx.hash()] = randomTx;
      }
      this->lock.unlock();
    }

    // Wait until next block
    while (this->storage->latest()->getNHeight() == latest->getNHeight()) {
      Utils::logToDebug(Log:rdpos, __func__,
        std::string("Waiting until new block... validator mempool size = ")
        + std::to_string(validatorMempool.size())
      );
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

bool rdPoS::validatorIsKnown(const Validator& val) {
  ; // TODO: not implemented in original
}

bool rdPoS:: saveToDB() {
  ; // TODO: not implemented in original
}

bool rdPoS::validateBlock(const std::shared_ptr<const Block>& block) {
  // Check validator signature against block height of prevBlockHash (not sure if enough for safety)
  this->lock.lock();
  Hash hash = block->getBlockHash();

  // TODO: do we need to include chainId In the block signature?
  UPubkey key = Secp256k1::recover(block->getValidatorSig(), hash);
  if (Secp256k1::toAddress(key) != this->randomList[0].get().get()) {
    Utils::logToDebug(Log::rdpos,__func__,
      std::string("Block validator signature does not match validator[0] signature: ")
      + block->getValidatorSig().hex()
    );
    this->lock.unlock();
    return false;
  }

  this->lock.unlock();
  return true;
}

Hash rdPoS::processBlock(const std::shared_ptr<const Block>& block) {
  this->lock.lock();
  this->randomList = std::vector<std::reference_wrapper<Validator>>(
    validatorList.begin(), validatorList.end()
  );
  this->gen.shuffle(this->randomList);
  this->gen.setSeed(block->getRandomness());
  this->validatorMempool.clear();
  this->lock.unlock();
  return block->getRandomness();
}

void rdPoS::addValidatorTx(const TxValidator& tx) {
  this->lock.lock();
  bool broadcast = (this->validatorMempool.count(tx.hash()) == 0);
  if (tx.getNHeight() == this->storage->latest()->getNHeight()) {
    for (uint64_t i = 1; i <= this->minValidators; i++) {
      if (this->randomList[i].get() == tx.getFrom()) {
        this->validatorMempool[tx.hash()] = tx;
        this->lock.unlock();
        if (broadcast) this->p2p->broadcastValidatorTx(tx);
        return;
      }
    }
  }
  this->lock.unlock();
}

static Hash rdPoS::parseTxSeedList(const std::unordered_map<uint64_t, TxValidator, SafeHash> txs) {
  std::string seed;
  if (txs.size() == 0) return Hash();
  for (uint64_t i = 0; i < txs.size(); i++) seed += txs.at(i).data().substr(4,32);
  return Utils::sha3(seed);
}

void rdPoS::startValidatorThread() {
  this->lock.lock();
  if (this->isValidator && !this->isValidatorThreadRunning) {
    this->isValidatorThreadRunning = true;
    Utils::logToDebug(Log::rdpos, __func__, "Starting validator thread...");
    std::thread t(&rdPoS::validatorLoop, this);
    t.detach();
  }
  this->lock.unlock();
}

