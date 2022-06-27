#include "chainHead.h"

ChainHead::ChainHead(std::shared_ptr<DBService> &dbServer) {
  this->loadFromDB(dbServer);
};

void ChainHead::push_back(Block& block) {
  this->internalChainHeadLock.lock();
  this->internalChainHead.push_back(block);
  
  auto latestBlock = std::make_shared<Block>(internalChainHead.back());
  this->internalChainHeadLookupTableByHash[latestBlock->getBlockHash()] = latestBlock;
  this->internalChainHeadLookupTableByHeight[latestBlock->nHeight()] = latestBlock;

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
  this->internalChainHeadLookupTableByHeight[latestBlock->nHeight()] = latestBlock;

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
  this->internalChainHeadLookupTableByHeight.erase(blockHeight);

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
  this->internalChainHeadLookupTableByHeight.erase(blockHeight);

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
  bool result = this->internalChainHeadLookupTableByHeight.count(blockHeight) > 0;
  this->internalChainHeadLock.unlock();
  return result;
}

Block ChainHead::getBlock(std::string &blockHash) {
  this->internalChainHeadLock.lock();
  if (this->hasBlock(blockHash)) {
    Block result = *this->internalChainHeadLookupTableByHash[blockHash];
    this->internalChainHeadLock.unlock();
    return result;
  } else {
    this->internalChainHeadLock.unlock();
    throw std::runtime_error("Block not found");
  }
}

Block ChainHead::getBlock(uint64_t &blockHeight) {
  this->internalChainHeadLock.lock();
  if (this->hasBlock(blockHeight)) {
    Block result = *this->internalChainHeadLookupTableByHeight[blockHeight];
    this->internalChainHeadLock.unlock();
    return result;
  } else {
    this->internalChainHeadLock.unlock();
    throw std::runtime_error("Block not found");
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
  // TODO. load from DB.
  this->push_back(latestBlock);

}