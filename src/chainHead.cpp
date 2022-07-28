#include "chainHead.h"

ChainHead::ChainHead(std::shared_ptr<DBService> &dbService) : dbServer(dbService) {
  this->loadFromDB();
};

void ChainHead::_push_back(Block& block) {
  this->internalChainHead.push_back(block);

  auto latestBlock = std::make_shared<Block>(internalChainHead.back());
  this->lookupBlockByHash[latestBlock->getBlockHash()] = latestBlock;
  this->lookupBlockHashByHeight[latestBlock->nHeight()] = latestBlock->getBlockHash();
  this->lookupBlockHeightByHash[latestBlock->getBlockHash()] = latestBlock->nHeight();

  for (auto &tx : latestBlock->transactions()) {
    this->lookupTxByHash[tx.hash()] = std::make_shared<dev::eth::TransactionBase>(tx);
    this->lookupBlockByTxHash[tx.hash()] = latestBlock;
  }

  return;
}

void ChainHead::_push_front(Block& block) {
  this->internalChainHead.push_front(block);

  auto latestBlock = std::make_shared<Block>(internalChainHead.front());
  this->lookupBlockByHash[latestBlock->getBlockHash()] = latestBlock;
  this->lookupBlockHashByHeight[latestBlock->nHeight()] = latestBlock->getBlockHash();
  this->lookupBlockHeightByHash[latestBlock->getBlockHash()] = latestBlock->nHeight();

  for (auto &tx : latestBlock->transactions()) {
    this->lookupTxByHash[tx.hash()] = std::make_shared<dev::eth::TransactionBase>(tx);
    this->lookupBlockByTxHash[tx.hash()] = latestBlock;
  }

  return;
}

void ChainHead::push_front(Block& block) {
  this->internalChainHeadLock.lock();
  this->_push_front(block);
  this->internalChainHeadLock.unlock();
}

void ChainHead::push_back(Block& block) {
  this->internalChainHeadLock.lock();
  this->_push_back(block);
  this->internalChainHeadLock.unlock();
}

void ChainHead::pop_back() {
  this->internalChainHeadLock.lock();
  Block& blockToDelete = this->internalChainHead.back();

  /**
   * Gather information about the block to delete references to it on internal mappings.
   * We have to collect: blockHash, blockHeight, all tx hashes.
   */
  std::string blockHash = blockToDelete.getBlockHash();
  uint64_t blockHeight = blockToDelete.nHeight();

  // Delete all tx references from mappings.
  for (auto &tx : blockToDelete.transactions()) {
    this->lookupTxByHash.erase(tx.hash());
    this->lookupBlockByTxHash.erase(tx.hash());
  }

  // Delete the block from the internal mappings.
  this->lookupBlockByHash.erase(blockHash);

  // Delete the block from the internal deque.
  this->internalChainHead.pop_back();

  this->internalChainHeadLock.unlock();
  return;
}

void ChainHead::pop_front() {
  this->internalChainHeadLock.lock();
  Block& blockToDelete = this->internalChainHead.front();
  /**
   * Gather information about the block to delete references to it on internal mappings.
   * We have to collect: blockHash, blockHeight, all tx hashes.
   */
  std::string blockHash = blockToDelete.getBlockHash();
  uint64_t blockHeight = blockToDelete.nHeight();

  // Delete all tx references from mappings.
  for (auto &tx : blockToDelete.transactions()) {
    this->lookupTxByHash.erase(tx.hash());
    this->lookupBlockByTxHash.erase(tx.hash());
  }

  // Delete the block from the internal mappings.
  this->lookupBlockByHash.erase(blockHash);

  // Delete the block from the internal deque.
  this->internalChainHead.pop_front();

  this->internalChainHeadLock.unlock();
  return;
}


bool ChainHead::hasBlock(std::string &blockHash) {
  this->internalChainHeadLock.lock();
  bool result = this->lookupBlockByHash.count(blockHash) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

bool ChainHead::hasBlock(uint64_t &blockHeight) {
  this->internalChainHeadLock.lock();
  bool result = this->lookupBlockByHash.count(
    this->lookupBlockHashByHeight[blockHeight]
  ) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

bool ChainHead::exists(std::string &blockHash) {
  if (this->hasBlock(blockHash)) return true;
  return this->dbServer->has(blockHash, DBPrefix::blocks);  // Check DB.
}

bool ChainHead::exists(uint64_t &blockHeight) {
  if (this->hasBlock(blockHeight)) return true;
  return this->dbServer->has(Utils::uint64ToBytes(blockHeight), DBPrefix::blockHeightMaps); // Check DB.
}

Block ChainHead::getBlock(std::string &blockHash) {
  if (this->exists(blockHash)) {
    if (this->hasBlock(blockHash)) {
      this->internalChainHeadLock.lock();
      Block result = *this->lookupBlockByHash[blockHash];
      this->internalChainHeadLock.unlock();
      return result;
    } else {
      Block result(dbServer->get(blockHash, DBPrefix::blocks));
      return result;
    }
  } else {
    throw std::runtime_error("Block doesn't exist");
  }
}

Block ChainHead::getBlock(uint64_t &blockHeight) {
  if (this->exists(blockHeight)) {
    if (this->hasBlock(blockHeight)) {
      this->internalChainHeadLock.lock();
      Block result = *this->lookupBlockByHash[this->lookupBlockHashByHeight[blockHeight]];
      this->internalChainHeadLock.unlock();
      return result;
    } else {
      std::string blockHash = dbServer->get(Utils::uint64ToBytes(blockHeight), DBPrefix::blockHeightMaps);
      Utils::LogPrint(Log::chainHead, __func__, "blockHash: " + blockHash);
      Block result(dbServer->get(blockHash, DBPrefix::blocks));
      return result;
    }
  } else {
    this->internalChainHeadLock.unlock();
    throw std::runtime_error("Block doesn't exist");
  }
}

bool ChainHead::hasTransaction(std::string &txHash) {
  this->internalChainHeadLock.lock();
  bool result = this->lookupTxByHash.count(txHash) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

dev::eth::TransactionBase ChainHead::getTransaction(std::string &txHash) {
  if (this->hasTransaction(txHash)) {
    this->internalChainHeadLock.lock();
    dev::eth::TransactionBase result = *this->lookupTxByHash[txHash];
    this->internalChainHeadLock.unlock();
    return result;
  }

  // Check DB.
  if (this->dbServer->has(txHash, DBPrefix::transactions)) {
    dev::eth::TransactionBase result(dev::fromHex(dbServer->get(txHash, DBPrefix::transactions)), dev::eth::CheckTransaction::None); // No need to check a tx again.
    return result;
  } else {
    throw std::runtime_error("Transaction doesn't exist");
  }
}

Block ChainHead::getBlockFromTx(std::string &txHash) {
  if (this->hasTransaction(txHash)) {
    this->internalChainHeadLock.lock();
    Block result = *this->lookupBlockByTxHash[txHash];
    this->internalChainHeadLock.unlock();
    return result;
  } else {
    throw std::runtime_error("Block not found");
  }
}

Block ChainHead::latest() {
  this->internalChainHeadLock.lock();
  Block result = this->internalChainHead.back();
  this->internalChainHeadLock.unlock();
  return result;
}

uint64_t ChainHead::blockSize() {
  this->internalChainHeadLock.lock();
  uint64_t result = this->internalChainHead.size();
  this->internalChainHeadLock.unlock();
  return result;
}

void ChainHead::loadFromDB() {
  if (!dbServer->has("latest", DBPrefix::blocks)) {
    Block genesis(0, 1656356645000000, 0);
    dbServer->put("latest", genesis.serializeToBytes(), DBPrefix::blocks);
    dbServer->put(Utils::uint64ToBytes(genesis.nHeight()), genesis.getBlockHash(), DBPrefix::blockHeightMaps);
    dbServer->put(genesis.getBlockHash(), genesis.serializeToBytes(), DBPrefix::blocks);
  }

  Utils::LogPrint(Log::chainHead, __func__, "Loading chain head from DB: getting latest block");
  Block latestBlock = Block(dbServer->get("latest", DBPrefix::blocks));
  Utils::LogPrint(Log::chainHead, __func__, std::string("Loading chain head from DB: ") + dev::toHex(latestBlock.getBlockHash()) + " " + std::to_string(latestBlock.nHeight()));
  uint64_t depth = latestBlock.nHeight();

  this->internalChainHeadLock.lock();

  Utils::LogPrint(Log::chainHead, __func__, "Loading chain head from DB: parsing block mappings");

  // Load block mappings (hash -> height and height -> hash) from DB.
  std::vector<DBEntry> blockMaps = dbServer->readBatch(DBPrefix::blockHeightMaps);
  for (auto &blockMap : blockMaps) {
    this->lookupBlockHashByHeight[Utils::bytesToUint64(blockMap.key)] = blockMap.value;
    this->lookupBlockHeightByHash[blockMap.value] = Utils::bytesToUint64(blockMap.key);
  }

  // Append up to 1000 blocks from history.
  for (uint64_t i = 0; i <= 1000 && i <= depth; ++i) {
    Block block(dbServer->get(this->lookupBlockHashByHeight[depth-i], DBPrefix::blocks));
    this->_push_front(block);
  }

  Utils::LogPrint(Log::chainHead, __func__, "Loading chain head from DB: done");
  this->internalChainHeadLock.unlock();
  return;
}

void ChainHead::dumpToDB() {
  // Emplace all blocks into a DB vector.
  WriteBatchRequest blockBatch;
  WriteBatchRequest heightBatch;
  WriteBatchRequest txBatch;
  WriteBatchRequest txToBlockBatch;
  this->internalChainHeadLock.lock();
  Block latest = this->internalChainHead.back();
  while (!this->internalChainHead.empty()) {
    // We can't call this->pop_back() because of std::mutex.
    Block& blockToDelete = this->internalChainHead.front();
    blockBatch.puts.emplace_back(DBEntry(blockToDelete.getBlockHash(), blockToDelete.serializeToBytes()));
    heightBatch.puts.emplace_back(DBEntry(Utils::uint64ToBytes(blockToDelete.nHeight()), blockToDelete.getBlockHash()));

    /**
     * Gather information about the block to delete references to it on internal mappings.
     * We have to collect: blockHash, blockHeight, all tx hashes.
     */
    std::string blockHash = blockToDelete.getBlockHash();
    uint64_t blockHeight = blockToDelete.nHeight();

    // Delete all tx references from mappingsand append them to the DB.
    for (auto &tx : blockToDelete.transactions()) {
      txBatch.puts.emplace_back(DBEntry(tx.hash(), dev::toHex(tx.rlp())));
      txToBlockBatch.puts.emplace_back(DBEntry(tx.hash(), blockToDelete.getBlockHash()));
      this->lookupTxByHash.erase(tx.hash());
      this->lookupBlockByTxHash.erase(tx.hash());
    }

    // Delete the block from the internal mappings.
    this->lookupBlockByHash.erase(blockHash);

    // Delete the block from the internal deque.
    this->internalChainHead.pop_front();
  }

  dbServer->writeBatch(blockBatch, DBPrefix::blocks);
  dbServer->writeBatch(heightBatch, DBPrefix::blockHeightMaps);
  dbServer->writeBatch(txBatch, DBPrefix::transactions);
  dbServer->writeBatch(txToBlockBatch, DBPrefix::TxToBlocks);

  dbServer->put("latest", latest.serializeToBytes(), DBPrefix::blocks);

  this->internalChainHeadLock.unlock();
  return;
}

