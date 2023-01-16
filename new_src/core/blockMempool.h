#ifndef BLOCKMEMPOOL_H
#define BLOCKMEMPOOL_H

#include <mutex>

#include "block.h"
#include "blockChain.h"
#include "state.h"
#include "../net/grpcserver.h"
#include "../utils/db.h"
#include "../utils/utils.h"

class BlockMempool {
  private:
    Hash preferredBlockHash;
    std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> mempool;
    std::unordered_map<Hash, BlockStatus, SafeHash> cachedBlockStatus;
    mutable std::mutex mempoolLock;
    std::shared_ptr<State> state;
    std::shared_ptr<BlockChain> chain;
    std::shared_ptr<BlockManager> mgr;
  public:
    BlockManager(
      const std::shared_ptr<State>& state, const std::shared_ptr<BlockChain>& chain,
      const std::shared_ptr<BlockManager>& mgr
    ) : state(state), chain(chain), mgr(mgr);
    const Hash& getPreferredBlockHash() { return this->preferredBlockHash; }
    void setPreferredBlockHash(const Hash& hash) { this->preferredBlockHash = hash; }
    const BlockStatus getBlockStatus(const Hash& hash);
    void setBlockStatus(const Hash& hash, const BlockStatus& status);
    bool isProcessing(const Hash& block);
    bool accept(const Hash& block);
    bool reject(const Hash& block);
    void processBlock(std::shared_ptr<Block> block);
    bool exists(const Hash& block);
    const std::shared_ptr<const Block> getBlock(const Hash& hash);
};

#endif  // BLOCKMEMPOOL_H
