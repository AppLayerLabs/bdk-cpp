#include "chainHead.h"

void ChainHead::_push_back(const std::shared_ptr<const Block>&& block) {
  this->internalChainHead.emplace_back(std::move(block));

  auto latestBlock = internalChainHead.back();
  auto hash = latestBlock->getBlockHash();
  this->lookupBlockByHash[latestBlock->getBlockHash()] = latestBlock;
  this->lookupBlockHashByHeight[latestBlock->nHeight()] = latestBlock->getBlockHash();
  this->lookupBlockHeightByHash[latestBlock->getBlockHash()] = latestBlock->nHeight();

  for (const auto &tx : latestBlock->transactions()) {
    this->lookupTxByHash[tx.second.hash()] = std::make_shared<Tx::Base>(tx.second);
    this->lookupBlockByTxHash[tx.second.hash()] = latestBlock;
  }
}

void ChainHead::_push_front(const std::shared_ptr<const Block>&& block) {
  this->internalChainHead.emplace_front(std::move(block));

  auto latestBlock = this->internalChainHead.front();
  this->lookupBlockByHash[latestBlock->getBlockHash()] = latestBlock;
  this->lookupBlockHashByHeight[latestBlock->nHeight()] = latestBlock->getBlockHash();
  this->lookupBlockHeightByHash[latestBlock->getBlockHash()] = latestBlock->nHeight();

  for (const auto &tx : latestBlock->transactions()) {
    this->lookupTxByHash[tx.second.hash()] = std::make_shared<Tx::Base>(tx.second);
    this->lookupBlockByTxHash[tx.second.hash()] = latestBlock;
  }
}

void ChainHead::push_back(const std::shared_ptr<const Block> &&block) {
  this->internalChainHeadLock.lock();
  this->_push_back(std::move(block));
  this->internalChainHeadLock.unlock();
}

void ChainHead::push_front(const std::shared_ptr<const Block> &&block) {
  this->internalChainHeadLock.lock();
  this->_push_front(std::move(block));
  this->internalChainHeadLock.unlock();
}

void ChainHead::pop_back() {
  this->internalChainHeadLock.lock();
  auto blockToDelete = this->internalChainHead.back();

  /**
   * Gather information about the block to delete references to it on internal mappings.
   * We have to collect: blockHash, blockHeight, all tx hashes.
   */
  Hash blockHash = blockToDelete->getBlockHash();
  uint64_t blockHeight = blockToDelete->nHeight();

  // Delete all tx references from mappings.
  for (const auto &tx : blockToDelete->transactions()) {
    this->lookupTxByHash.erase(tx.second.hash());
    this->lookupBlockByTxHash.erase(tx.second.hash());
  }

  // Delete the block from the internal mappings.
  this->lookupBlockByHash.erase(blockHash);

  // Delete the block from the internal deque.
  this->internalChainHead.pop_back();

  this->internalChainHeadLock.unlock();
}

void ChainHead::pop_front() {
  this->internalChainHeadLock.lock();
  auto blockToDelete = this->internalChainHead.front();
  /**
   * Gather information about the block to delete references to it on internal mappings.
   * We have to collect: blockHash, blockHeight, all tx hashes.
   */
  Hash blockHash = blockToDelete->getBlockHash();
  uint64_t blockHeight = blockToDelete->nHeight();

  // Delete all tx references from mappings.
  for (const auto &tx : blockToDelete->transactions()) {
    this->lookupTxByHash.erase(tx.second.hash());
    this->lookupBlockByTxHash.erase(tx.second.hash());
  }

  // Delete the block from the internal mappings.
  this->lookupBlockByHash.erase(blockHash);

  // Delete the block from the internal deque.
  this->internalChainHead.pop_front();

  this->internalChainHeadLock.unlock();
}

bool ChainHead::hasBlock(Hash const &blockHash) const {
  this->internalChainHeadLock.lock_shared();
  bool result = this->lookupBlockByHash.count(blockHash) > 0;
  this->internalChainHeadLock.unlock_shared();
  return result;
}

bool ChainHead::hasBlock(uint64_t const &blockHeight) const {
  this->internalChainHeadLock.lock_shared();
  bool result = this->lookupBlockHashByHeight.count(blockHeight) > 0;
  this->internalChainHeadLock.unlock_shared();
  return result;
}

const bool ChainHead::exists(Hash const &blockHash) const{
  if (this->hasBlock(blockHash)) return true;
  return this->dbServer->has(blockHash.get(), DBPrefix::blocks);  // Check DB.
}

const bool ChainHead::exists(uint64_t const &blockHeight) const {
  if (this->hasBlock(blockHeight)) return true;
  return this->dbServer->has(Utils::uint64ToBytes(blockHeight), DBPrefix::blockHeightMaps); // Check DB.
}

const std::shared_ptr<const Block> ChainHead::getBlock(Hash const &blockHash) const {
  if (this->exists(blockHash)) {
    if (this->hasBlock(blockHash)) {
      this->internalChainHeadLock.lock_shared();
      const std::shared_ptr<const Block> result = this->lookupBlockByHash.find(blockHash)->second;
      this->internalChainHeadLock.unlock_shared();
      return result;
    }
    Utils::LogPrint(Log::chainHead, __func__, "blockHash: " + blockHash.get());
    this->internalChainHeadLock.lock_shared();
    if (this->cachedBlocks.count(blockHash) > 0) {
      const std::shared_ptr<const Block> result = this->cachedBlocks[blockHash];
      this->internalChainHeadLock.unlock_shared();
      return result;
    }

    this->cachedBlocks[blockHash] = std::make_shared<Block>(dbServer->get(blockHash.get(), DBPrefix::blocks), true);
    auto result = this->cachedBlocks[blockHash];
    this->internalChainHeadLock.unlock_shared();
    return this->cachedBlocks[blockHash];
  } else {
    return nullptr;
  }
}

const std::shared_ptr<const Block> ChainHead::getBlock(uint64_t const &blockHeight) const {
  if (this->exists(blockHeight)) {
    if (this->hasBlock(blockHeight)) {
      this->internalChainHeadLock.lock_shared();
      const std::shared_ptr<const Block> result = this->lookupBlockByHash.find(this->lookupBlockHashByHeight.find(blockHeight)->second)->second;
      this->internalChainHeadLock.unlock_shared();
      return result;
    }
    Hash blockHash(dbServer->get(Utils::uint64ToBytes(blockHeight), DBPrefix::blockHeightMaps));
    Utils::LogPrint(Log::chainHead, __func__, "blockHeight: " + blockHeight);
    this->internalChainHeadLock.lock_shared();
    if (this->cachedBlocks.count(blockHash) > 0) {
      const std::shared_ptr<const Block> result = this->cachedBlocks[blockHash];
      this->internalChainHeadLock.unlock_shared();
      return result;
    }

    this->cachedBlocks[blockHash] = std::make_shared<Block>(dbServer->get(blockHash.get(), DBPrefix::blocks), true);
    const std::shared_ptr<const Block> result = this->cachedBlocks[blockHash];
    this->internalChainHeadLock.unlock_shared();
    return result;
  } else {
    return nullptr;
  }
}

bool ChainHead::hasTransaction(const Hash &txHash) const {
  this->internalChainHeadLock.lock_shared();
  bool result = this->lookupTxByHash.count(txHash) > 0;
  this->internalChainHeadLock.unlock_shared();
  return result;
}

const std::shared_ptr<const Tx::Base> ChainHead::getTransaction(const Hash &txHash) const {
  if (this->hasTransaction(txHash)) {
    this->internalChainHeadLock.lock_shared();
    const std::shared_ptr<const Tx::Base> result = this->lookupTxByHash.find(txHash)->second;
    this->internalChainHeadLock.unlock_shared();
    return result;
  }


  // Check if cache has it first.
  this->internalChainHeadLock.lock_shared();
  if (this->cachedTxs.count(txHash) > 0) {
    const std::shared_ptr<const Tx::Base> result = this->cachedTxs[txHash];
    this->internalChainHeadLock.unlock_shared();
    return result;
  }
  this->internalChainHeadLock.unlock_shared();

  // Check DB.
  if (this->dbServer->has(txHash.get(), DBPrefix::TxToBlocks)) {
    Hash blockHash(dbServer->get(txHash.get(), DBPrefix::TxToBlocks));
    std::string txBytes = dbServer->get(blockHash.get(), DBPrefix::blocks);
    this->internalChainHeadLock.lock_shared();
    this->cachedTxs[txHash] = std::make_shared<Tx::Base>(txBytes, true);
    const std::shared_ptr<const Tx::Base> result = this->cachedTxs[txHash]; // No need to check a tx again.
    this->internalChainHeadLock.lock_shared();
    return result;
  }

  throw std::runtime_error(std::string(__func__) + ": " +
    std::string("Transaction does not exist")
  );
}

const std::shared_ptr<const Block> ChainHead::getBlockFromTx(const Hash &txHash) const {
  if (this->hasTransaction(txHash)) {
    this->internalChainHeadLock.lock_shared();
    const std::shared_ptr<const Block> result = this->lookupBlockByTxHash.find(txHash)->second;
    this->internalChainHeadLock.unlock_shared();
    return result;
  }
  throw std::runtime_error(std::string(__func__) + ": " +
    std::string("Block does not exist")
  );
}

const std::shared_ptr<const Block> ChainHead::latest() const {
  this->internalChainHeadLock.lock_shared();
  const std::shared_ptr<const Block> result = this->internalChainHead.back();
  this->internalChainHeadLock.unlock_shared();
  return result;
}

uint64_t ChainHead::blockSize() {
  this->internalChainHeadLock.lock_shared();
  uint64_t result = this->internalChainHead.size();
  this->internalChainHeadLock.unlock_shared();
  return result;
}

void ChainHead::loadFromDB() {
  if (!dbServer->has("latest", DBPrefix::blocks)) {
    Block genesis(Hash(Utils::uint256ToBytes(0)), 1656356645000000, 0);
    genesis.finalizeBlock();
    dbServer->put("latest", genesis.serializeToBytes(false), DBPrefix::blocks);
    dbServer->put(Utils::uint64ToBytes(genesis.nHeight()), genesis.getBlockHash().get(), DBPrefix::blockHeightMaps);
    dbServer->put(genesis.getBlockHash().get(), genesis.serializeToBytes(false), DBPrefix::blocks);
    // TODO: CHANGE THIS ON PUBLIC!!!
    dbServer->put(Utils::uint64ToBytes(0),Address("7588b0f553d1910266089c58822e1120db47e572", true).get(), DBPrefix::validators); // 0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759
    dbServer->put(Utils::uint64ToBytes(1),Address("cabf34a268847a610287709d841e5cd590cc5c00", true).get(), DBPrefix::validators); // 0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4
    dbServer->put(Utils::uint64ToBytes(2),Address("5fb516dc2cfc1288e689ed377a9eebe2216cf1e3", true).get(), DBPrefix::validators); // 0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751
    dbServer->put(Utils::uint64ToBytes(3),Address("795083c42583842774febc21abb6df09e784fce5", true).get(), DBPrefix::validators); // 0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7
    dbServer->put(Utils::uint64ToBytes(4),Address("bec7b74f70c151707a0bfb20fe3767c6e65499e0", true).get(), DBPrefix::validators); // 0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6
    // WARNING: THE PREVIOUS PRIVATE KEYS SHOULD ONLY BE USED FOR LOCAL TESTING PURPOSES.
    Utils::LogPrint(Log::chainHead, __func__, "Created genesis block");
    Utils::LogPrint(Log::chainHead, __func__, std::string("Created genesis block: ") + Utils::bytesToHex(genesis.getBlockHash().get()));
  }

  Utils::LogPrint(Log::chainHead, __func__, "Loading chain head from DB: getting latest block");
  Block latestBlock = Block(dbServer->get("latest", DBPrefix::blocks), true);
  Utils::LogPrint(Log::chainHead, __func__, std::string("Loading chain head from DB: ") + dev::toHex(latestBlock.getBlockHash()) + " " + std::to_string(latestBlock.nHeight()));
  uint64_t depth = latestBlock.nHeight();

  this->internalChainHeadLock.lock();

  Utils::LogPrint(Log::chainHead, __func__, "Loading chain head from DB: parsing block mappings");

  // Load block mappings (hash -> height and height -> hash) from DB.
  std::vector<DBEntry> blockMaps = dbServer->readBatch(DBPrefix::blockHeightMaps);
  for (DBEntry &blockMap : blockMaps) {
    this->lookupBlockHashByHeight[Utils::bytesToUint64(blockMap.key)] = Hash(blockMap.value);
    this->lookupBlockHeightByHash[Hash(blockMap.value)] = Utils::bytesToUint64(blockMap.key);
  }

  // Append up to 1000 blocks from history.
  for (uint64_t i = 0; i <= 1000 && i <= depth; ++i) {
    auto block = std::make_shared<Block>(dbServer->get(this->lookupBlockHashByHeight[depth-i].get(), DBPrefix::blocks), true);
    this->_push_front(block);
  }

  Utils::LogPrint(Log::chainHead, __func__, "Loading chain head from DB: done");
  this->internalChainHeadLock.unlock();
}

void ChainHead::dumpToDB() {
  // Emplace all blocks into a DB vector.
  WriteBatchRequest blockBatch;
  WriteBatchRequest heightBatch;
  WriteBatchRequest txToBlockBatch;
  this->internalChainHeadLock.lock();
  auto latest = this->internalChainHead.back();

  while (!this->internalChainHead.empty()) {
    // We can't call this->pop_back() because of std::mutex.
    auto blockToDelete = this->internalChainHead.front();
    if (blockToDelete == latest) break; // Skip latest block
    blockBatch.puts.emplace_back(DBEntry(blockToDelete->getBlockHash().get(), blockToDelete->serializeToBytes(true)));
    heightBatch.puts.emplace_back(DBEntry(Utils::uint64ToBytes(blockToDelete->nHeight()), blockToDelete->getBlockHash().get()));

    // Gather info about block to delete references to it on internal mappings.
    // We have to collect: blockHash, blockHeight, all tx hashes.
    Hash blockHash = blockToDelete->getBlockHash();
    uint64_t blockHeight = blockToDelete->nHeight();

    // Delete all tx references from mappings and append them to the DB.
    for (const auto &tx : blockToDelete->transactions()) {
      txToBlockBatch.puts.emplace_back(DBEntry(tx.second.hash().get(), blockToDelete->getBlockHash().get()));
      this->lookupTxByHash.erase(tx.second.hash());
      this->lookupBlockByTxHash.erase(tx.second.hash());
    }

    // Delete block from internal mappings and deque.
    this->lookupBlockByHash.erase(blockHash);
    this->internalChainHead.pop_front();
  }

  dbServer->writeBatch(blockBatch, DBPrefix::blocks);
  dbServer->writeBatch(heightBatch, DBPrefix::blockHeightMaps);
  dbServer->writeBatch(txToBlockBatch, DBPrefix::TxToBlocks);

  dbServer->put("latest", latest->serializeToBytes(true), DBPrefix::blocks);

  this->internalChainHeadLock.unlock();
}

void ChainHead::periodicSaveToDB() {
  while (!this->stopPeriodicSave) {
    std::this_thread::sleep_for(std::chrono::seconds(this->periodicSaveCooldown));
    if (
      !this->stopPeriodicSave &&
      (this->cachedBlocks.size() > 1000 || this->cachedTxs.size() > 1000000)
    ) {
      this->dumpToDB();
    }
  }
}

