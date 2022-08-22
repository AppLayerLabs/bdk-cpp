#include "chainTip.h"

void ChainTip::_push_back(Block& block) {
  this->internalChainTip.emplace_back(std::make_shared<Block>(block));
}

void ChainTip::_push_front(Block& block) {
  this->internalChainTip.emplace_front(std::make_shared<Block>(block));
}

void ChainTip::push_back(Block& block) {
  this->internalChainTipLock.lock();
  this->_push_back(block);
  this->internalChainTipLock.unlock();
}

void ChainTip::push_front(Block& block) {
  this->internalChainTipLock.lock();
  this->_push_front(block);
  this->internalChainTipLock.unlock();
}

void ChainTip::pop_back() {
  this->internalChainTipLock.lock();
  this->internalChainTip.pop_back();
  this->internalChainTipLock.unlock();
}

void ChainTip::pop_front() {
  this->internalChainTipLock.lock();
  this->internalChainTip.pop_front();
  this->internalChainTipLock.unlock();
}

void ChainTip::dumpToChainHead() {
  ; // TODO
}

