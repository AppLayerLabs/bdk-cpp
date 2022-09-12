#ifndef CHAINTIP_H
#define CHAINTIP_H

#include "block.h"
#include "chainHead.h"
#include "db.h"
#include "utils.h"
#include "../net/grpcserver.h"

class State; // Forward declaration.

class ChainTip {
  private:
    std::string preferedBlockHash; // SetPreference from gRPC.
    std::unordered_map<std::string,std::shared_ptr<const Block>, SafeHash> internalChainTip; // Mempool, block hash -> block.
    std::unordered_map<std::string,BlockStatus, SafeHash> cachedBlockStatus;
    mutable std::shared_mutex internalChainTipLock;

  public:
    ChainTip() = default;
    ~ChainTip() = default;
    void setBlockStatus(const std::string &blockHash, const BlockStatus &status);
    BlockStatus getBlockStatus(const std::string &blockHash) const;
    bool isProcessing(const std::string &blockHash) const;
    bool accept(const std::string &blockHash, const std::shared_ptr<State> state, const std::shared_ptr<ChainHead> chainHead);
    void reject(const std::string &blockHash);
    void processBlock(std::shared_ptr<Block> block);
    bool exists(const std::string &blockHash) const;
    const std::shared_ptr<const Block> getBlock(const std::string &blockHash) const;
    std::string getPreference() const;
    void setPreference(const std::string& blockHash);
};

#endif  // CHAINTIP_H
