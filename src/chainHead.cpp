#include "chainHead.h"

ChainHead::ChainHead(std::shared_ptr<DBService> &dbServer) {
  this->loadFromDB(dbServer);
};

void ChainHead::push_back(Block& block) {
  this->internalChainHeadLock.lock();
  this->internalChainHead.push_back(block);
  
  auto latestBlock = std::make_shared<Block>(internalChainHead.back());
  this->internalChainHeadLookupTableByHash[latestBlock->getBlockHash()] = latestBlock;
  this->diskChainHeadLookupTableByHeight[latestBlock->nHeight()] = latestBlock->getBlockHash();
  this->diskChainHeadLookupTableByHash[latestBlock->getBlockHash()] = latestBlock->nHeight();

  for (auto &tx : latestBlock->transactions()) {
    this->internalLatestConfirmedTransactions[tx.hash()] = std::make_shared<dev::eth::TransactionBase>(tx);
    this->internalTxToBlocksLookupTable[tx.hash()] = latestBlock;
  }

  this->internalChainHeadLock.unlock();
  return;
}

void ChainHead::push_front(Block& block) {
  this->internalChainHeadLock.lock();
  this->internalChainHead.push_front(block);
  
  auto latestBlock = std::make_shared<Block>(internalChainHead.front());
  this->internalChainHeadLookupTableByHash[latestBlock->getBlockHash()] = latestBlock;
  this->diskChainHeadLookupTableByHeight[latestBlock->nHeight()] = latestBlock->getBlockHash();
  this->diskChainHeadLookupTableByHash[latestBlock->getBlockHash()] = latestBlock->nHeight();

  for (auto &tx : latestBlock->transactions()) {
    this->internalLatestConfirmedTransactions[tx.hash()] = std::make_shared<dev::eth::TransactionBase>(tx);
    this->internalTxToBlocksLookupTable[tx.hash()] = latestBlock;
  }

  this->internalChainHeadLock.unlock();
  return;
}

void ChainHead::pop_back() {
  this->internalChainHeadLock.lock();
  Block& blockToDelete = this->internalChainHead.back();
  // Gather information about the block to delete the references to it on the internal mappings.
  // We have to collect: blockHash, blockHeight, all Tx Hashes.
  std::string blockHash = blockToDelete.getBlockHash(); 
  uint64_t blockHeight = blockToDelete.nHeight();

  // Delete all tx references from mappings.
  for (auto &tx : blockToDelete.transactions()) {
    this->internalLatestConfirmedTransactions.erase(tx.hash());
    this->internalTxToBlocksLookupTable.erase(tx.hash());
  }

  // Delete the block from the internal mappings.
  this->internalChainHeadLookupTableByHash.erase(blockHash);

  // Delete the block from the internal deque.
  this->internalChainHead.pop_back();

  this->internalChainHeadLock.unlock();
  return;
}

void ChainHead::pop_front() {
  this->internalChainHeadLock.lock();
  Block& blockToDelete = this->internalChainHead.front();
  // Gather information about the block to delete the references to it on the internal mappings.
  // We have to collect: blockHash, blockHeight, all Tx Hashes.
  std::string blockHash = blockToDelete.getBlockHash(); 
  uint64_t blockHeight = blockToDelete.nHeight();

  // Delete all tx references from mappings.
  for (auto &tx : blockToDelete.transactions()) {
    this->internalLatestConfirmedTransactions.erase(tx.hash());
    this->internalTxToBlocksLookupTable.erase(tx.hash());
  }

  // Delete the block from the internal mappings.
  this->internalChainHeadLookupTableByHash.erase(blockHash);

  // Delete the block from the internal deque.
  this->internalChainHead.pop_front();

  this->internalChainHeadLock.unlock();
  return;
}


bool ChainHead::hasBlock(std::string &blockHash) {
  this->internalChainHeadLock.lock();
  bool result = this->internalChainHeadLookupTableByHash.count(blockHash) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

bool ChainHead::hasBlock(uint64_t &blockHeight) {
  this->internalChainHeadLock.lock();
  bool result = this->internalChainHeadLookupTableByHash.count(this->diskChainHeadLookupTableByHeight[blockHeight]) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

bool ChainHead::exists(std::string &blockHash) {
  this->internalChainHeadLock.lock();
  bool result = this->diskChainHeadLookupTableByHash.count(blockHash) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

bool ChainHead::exists(uint64_t &blockHeight) {
  this->internalChainHeadLock.lock();
  bool result = this->diskChainHeadLookupTableByHeight.count(blockHeight) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

Block ChainHead::getBlock(std::string &blockHash) {
  this->internalChainHeadLock.lock();
  if (this->exists(blockHash)) {
    if (this->hasBlock(blockHash)) {
      Block result = *this->internalChainHeadLookupTableByHash[blockHash];
      this->internalChainHeadLock.unlock();
      return result;
    } else {
      // TODO: Load block from DB
      throw "";
    }
  } else {
    this->internalChainHeadLock.unlock();
    throw std::runtime_error("Block doesn't exists");
  }
}

Block ChainHead::getBlock(uint64_t &blockHeight) {
  this->internalChainHeadLock.lock();
  if (this->exists(blockHeight)) {
    if (this->hasBlock(blockHeight)) {
      Block result = *this->internalChainHeadLookupTableByHash[this->diskChainHeadLookupTableByHeight[blockHeight]];
      this->internalChainHeadLock.unlock();
      return result;
    } else {
      // TODO: Load block from DB.
      throw "";
    }
  } else {
    this->internalChainHeadLock.unlock();
    throw std::runtime_error("Block doesn't exists");
  }
}

bool ChainHead::hasTransaction(std::string &txHash) {
  this->internalChainHeadLock.lock();
  bool result = this->internalLatestConfirmedTransactions.count(txHash) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

dev::eth::TransactionBase ChainHead::getTransaction(std::string &txHash) {
  this->internalChainHeadLock.lock();
  if (this->hasTransaction(txHash)) {
    dev::eth::TransactionBase result = *this->internalLatestConfirmedTransactions[txHash];
    this->internalChainHeadLock.unlock();
    return result;
  } else {
    this->internalChainHeadLock.unlock();
    throw std::runtime_error("Transaction not found");
  }
}

Block ChainHead::getBlockFromTx(std::string &txHash) {
  this->internalChainHeadLock.lock();
  if (this->hasTransaction(txHash)) {
    Block result = *this->internalTxToBlocksLookupTable[txHash];
    this->internalChainHeadLock.unlock();
    return result;
  } else {
    this->internalChainHeadLock.unlock();
    throw std::runtime_error("Transaction not found");
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

void ChainHead::loadFromDB(std::shared_ptr<DBService> &dbServer) {
  if (!dbServer->has("latest", DBPrefix::blocks)) {
    Block genesis(
        0,
        1656356645000000,
        0);
    dbServer->put("latest", genesis.serializeToBytes(), DBPrefix::blocks);
    dbServer->put(Utils::uint64ToBytes(0), genesis.serializeToBytes(), DBPrefix::blocks);
  }
  Block latestBlock = Block(dbServer->get("latest", DBPrefix::blocks));

  uint64_t depth = latestBlock.nHeight();

  if (depth < 1001) {
    // Chain is too short to load from DB.
    return;
  }
  depth = depth - 1000;
  std::vector<DBKey> blocksToRead;
  for (uint64_t i = 0; i <= 1000; ++i) {
    blocksToRead.emplace_back(DBKey(Utils::uint64ToBytes(depth + i)));
  }
  std::vector<DBEntry> blocks = dbServer->readBatch(blocksToRead, DBPrefix::blocks);
  this->internalChainHeadLock.lock();
  for (auto &block : blocks) {
    Block newBlock(block.value);
    this->push_back(newBlock);
  }

  std::vector<DBEntry> blockMaps = dbServer->readBatch(DBPrefix::blockHeightMaps);
  for (auto &blockMap : blockMaps) {
    this->diskChainHeadLookupTableByHeight[Utils::bytesToUint64(blockMap.key)] = blockMap.value;
    this->diskChainHeadLookupTableByHash[blockMap.value] = Utils::bytesToUint64(blockMap.key);
  }

  this->internalChainHeadLock.unlock();
  return;
}

void ChainHead::dumpToDB(std::shared_ptr<DBService> &dbServer) {
  // Emplace all blocks into a DB vector.
  WriteBatchRequest blockBatch;
  WriteBatchRequest heightBatch;
  WriteBatchRequest txBatch;
  WriteBatchRequest txToBlockBatch;
  this->internalChainHeadLock.lock();
  while (!this->internalChainHead.empty()) {
    Block& blockToDelete = this->internalChainHead.back();
    blockBatch.puts.emplace_back(DBEntry(blockToDelete.getBlockHash(), blockToDelete.serializeToBytes()));
    heightBatch.puts.emplace_back(DBEntry(Utils::uint64ToBytes(blockToDelete.nHeight()), blockToDelete.getBlockHash()));
    // We cannot call this->pop_back() because of the std::mutex.
    // Gather information about the block to delete the references to it on the internal mappings.
    // We have to collect: blockHash, blockHeight, all Tx Hashes.
    std::string blockHash = blockToDelete.getBlockHash(); 
    uint64_t blockHeight = blockToDelete.nHeight();

    // Delete all tx references from mappings.
    // Also append them to DB.
    for (auto &tx : blockToDelete.transactions()) {
      txBatch.puts.emplace_back(DBEntry(tx.hash(), dev::toHex(tx.rlp())));
      txToBlockBatch.puts.emplace_back(DBEntry(tx.hash(), blockToDelete.getBlockHash()));

      this->internalLatestConfirmedTransactions.erase(tx.hash());
      this->internalTxToBlocksLookupTable.erase(tx.hash());
    }

    // Delete the block from the internal mappings.
    this->internalChainHeadLookupTableByHash.erase(blockHash);

    // Delete the block from the internal deque.
    this->internalChainHead.pop_back();
  }

  this->internalChainHeadLock.unlock();
  return;
}