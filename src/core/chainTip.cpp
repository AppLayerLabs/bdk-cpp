#include "chainTip.h"


std::string ChainTip::getPreference() { 
  internalChainTipLock.lock(); 
  std::string ret = preferedBlockHash; 
  internalChainTipLock.unlock(); 
  return ret; 
}

void ChainTip::setPreference(const std::string &blockHash) {
  internalChainTipLock.lock();
  preferedBlockHash = blockHash;
  internalChainTipLock.unlock();
}

bool ChainTip::isProcessing (const std::string &blockHash) {
  internalChainTipLock.lock();
  bool ret = (this->cachedBlockStatus[blockHash] == BlockStatus::Processing) ? true : false;
  internalChainTipLock.unlock();
  return ret;
};

bool ChainTip::processBlock(std::shared_ptr<Block> block) {
  this->internalChainTipLock.lock();
  this->internalChainTip[block->getBlockHash()] = block;
  this->cachedBlockStatus[block->getBlockHash()] = BlockStatus::Processing;
  this->internalChainTipLock.unlock();
  return true;
}

// TODO: handle block not being found and similar errors.
const std::shared_ptr<const Block> ChainTip::getBlock(const std::string &blockHash) {
  internalChainTipLock.lock();
  auto ret = internalChainTip[blockHash];
  internalChainTipLock.unlock();
  return ret;
};

void ChainTip::accept(const std::string &blockHash) {
  internalChainTipLock.lock();
  this->internalChainTip.erase(blockHash);
  this->cachedBlockStatus[blockHash] = BlockStatus::Accepted;
  internalChainTipLock.unlock();
  return;
}

void ChainTip::reject(const std::string &blockHash) {
  internalChainTipLock.lock();
  this->internalChainTip.erase(blockHash);
  this->cachedBlockStatus[blockHash] = BlockStatus::Rejected;
  internalChainTipLock.unlock();
  return;
}