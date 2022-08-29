#include "chainTip.h"
#include "state.h"

void ChainTip::setBlockStatus(const std::string &blockHash, const BlockStatus &status) {
  internalChainTipLock.lock();
  this->cachedBlockStatus[blockHash] = status;
  internalChainTipLock.unlock();
}

BlockStatus ChainTip::getBlockStatus(const std::string &blockHash) const {
  internalChainTipLock.lock();
  if (this->cachedBlockStatus.count(blockHash) > 0) {
    auto ret = this->cachedBlockStatus.find(blockHash)->second;
    internalChainTipLock.unlock();
    return ret;
  }
  internalChainTipLock.unlock();
  return BlockStatus::Unknown;
}

bool ChainTip::isProcessing(const std::string &blockHash) const {
  internalChainTipLock.lock();
  if (this->cachedBlockStatus.count(blockHash) > 0) {
    bool ret = (this->cachedBlockStatus.find(blockHash)->second == BlockStatus::Processing) ? true : false;
    internalChainTipLock.unlock();
    return ret;
  }
  internalChainTipLock.unlock();
  return false;
};  

void ChainTip::accept(const std::string &blockHash, const std::shared_ptr<State> state, const std::shared_ptr<ChainHead> chainHead) {
  internalChainTipLock.lock();
  // TODO: Error handling: block not processing (not found).
  if (this->internalChainTip.find(blockHash)->second.unique()) {
  Utils::LogPrint(Log::chainTip, __func__, "Block is unique, moving to processNewBlock.");
  state->processNewBlock(std::move(this->internalChainTip.find(blockHash)->second), chainHead);
  this->internalChainTip.erase(blockHash);
  } else {
    // We have to create a copy of the block to process it.
    Utils::LogPrint(Log::chainTip, __func__, "Block not unique, creating copy to processNewBlock.");
    auto block = std::make_shared<Block>(*this->internalChainTip.find(blockHash)->second);
    state->processNewBlock(std::move(block), chainHead);
  }
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

void ChainTip::processBlock(std::shared_ptr<Block> block) {
  this->internalChainTipLock.lock();
  this->internalChainTip[block->getBlockHash()] = block;
  this->cachedBlockStatus[block->getBlockHash()] = BlockStatus::Processing;
  this->internalChainTipLock.unlock();
}

// TODO: handle block not found and similar errors
const std::shared_ptr<const Block> ChainTip::getBlock(const std::string &blockHash) const {
  internalChainTipLock.lock();
  const std::shared_ptr<const Block>& ret = internalChainTip.find(blockHash)->second;
  internalChainTipLock.unlock();
  return ret;
};

std::string ChainTip::getPreference() const {
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

