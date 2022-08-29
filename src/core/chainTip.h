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
    // TODO: Do we need to keep preferedBlockHash always updated?
    // In the case of avalancheGo not sending setPreference and calling State::createNewBlock
    // on a previously prefered block, older in the chain.
    std::string preferedBlockHash; // SetPreference from gRPC.
    std::unordered_map<std::string,std::shared_ptr<const Block>> internalChainTip; // Mempool, block hash -> block.
    std::unordered_map<std::string,BlockStatus> cachedBlockStatus;
    mutable std::mutex internalChainTipLock;

  public:
    ChainTip() = default;
    ~ChainTip() = default;
    void setBlockStatus(const std::string &blockHash, const BlockStatus &status);
    BlockStatus getBlockStatus(const std::string &blockHash) const;
    bool isProcessing(const std::string &blockHash) const;
    void accept(const std::string &blockHash, const std::shared_ptr<State> state, const std::shared_ptr<ChainHead> chainHead);
    void reject(const std::string &blockHash);
    void processBlock(std::shared_ptr<Block> block);
    const std::shared_ptr<const Block> getBlock(const std::string &blockHash) const;
    std::string getPreference() const;
    void setPreference(const std::string& blockHash);
};

#endif  // CHAINTIP_H
