#ifndef CHAINTIP_H
#define CHAINTIP_H

#include "block.h"
#include "chainHead.h"
#include "../utils/db.h"
#include "../utils/utils.h"
#include "../net/grpcserver.h"

class State; // Forward declaration.

class ChainTip {
  private:
    Hash preferedBlockHash; // SetPreference from gRPC.
    std::unordered_map<Hash,std::shared_ptr<const Block>, SafeHash> internalChainTip; // Mempool, block hash -> block.
    std::unordered_map<Hash,BlockStatus, SafeHash> cachedBlockStatus;
    mutable std::shared_mutex internalChainTipLock;

  public:
    ChainTip() = default;
    ~ChainTip() = default;
    void setBlockStatus(const Hash &blockHash, const BlockStatus &status);
    BlockStatus getBlockStatus(const Hash &blockHash) const;
    bool isProcessing(const Hash &blockHash) const;
    bool accept(const Hash &blockHash, const std::shared_ptr<State> state, const std::shared_ptr<ChainHead> chainHead, const std::shared_ptr<BlockManager> blockManager);
    void reject(const Hash &blockHash);
    void processBlock(std::shared_ptr<Block> block);
    bool exists(const Hash &blockHash) const;
    const std::shared_ptr<const Block> getBlock(const Hash &blockHash) const;
    Hash getPreference() const;
    void setPreference(const Hash& blockHash);
};

#endif  // CHAINTIP_H
