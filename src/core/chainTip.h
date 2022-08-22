#ifndef CHAINTIP_H
#define CHAINTIP_H

#include "block.h"
#include "chainHead.h"
#include "db.h"
#include "utils.h"

class ChainTip {
  private:
    std::shared_ptr<ChainHead> &chainHead; // Pointer to blockchain
    std::deque<std::shared_ptr<Block>> internalChainTip; // Mempool
    std::mutex internalChainTipLock;
    // Only access these functions if you are absolute sure that internalChainTipLock is locked.
    void _push_back(Block& block);
    void _push_front(Block& block);
  public:
    ChainTip(std::shared_ptr<Chainhead> &_chainHead) : chainHead(_chainHead) {}
    // Mutex locked.
    void push_back(Block& block);
    void push_front(Block& block);
    void pop_back();
    void pop_front();
    void dumpToChainHead();
};

#endif  // CHAINTIP_H
