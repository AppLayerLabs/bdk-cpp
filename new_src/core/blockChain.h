#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <mutex>

#include "block.h"
#include "../utils/db.h"
#include "../utils/utils.h"
#include "../utils/random.h"

class BlockChain {
  private:
    std::shared_ptr<DB> db;
    std::deque<std::shared_ptr<const Block>> chain;
    std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> blockByHash;
    std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> blockByTxHash;
    std::unordered_map<Hash, std::shared_ptr<const Tx>, SafeHash> txByHash;
    std::unordered_map<Hash,uint64_t, SafeHash> blockHeightByHash;
    std::unordered_map<uint64_t,Hash, SafeHash> blockHashByHeight;
    mutable std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> cachedBlocks;
    mutable std::unordered_map<Hash, std::shared_ptr<const Tx>, SafeHash> cachedTxs;
    mutable std::mutex chainLock;
    std::thread periodicSaveThread;
    uint64_t periodicSaveCooldown = 15;
    bool stopPeriodicSave = false;
    void pushBackInternal(const std::shared_ptr<const Block>&& block);
    void pushFrontInternal(const std::shared_ptr<const Block>&& block);
    void saveToDB();
    void loadFromDB();
  public:
    BlockChain(const std::shared_ptr<DB>& db) : db(db) {
      this->loadFromDB();
      this->periodicSaveThread = std::thread([&]{ this->periodicSaveToDB(); });
      this->periodicSaveThread.detach();
    }
    void pushBack(const std::shared_ptr<const Block>&& block);
    void pushFront(const std::shared_ptr<const Block>&& block);
    void popBack();
    void popFront();
    bool hasBlock(const Hash& hash);
    bool hasBlock(const uint64_t& height);
    bool exists(const Hash& hash);
    bool exists(const uint64_t& height);
    const std::shared_ptr<const Block> getBlock(const Hash& hash);
    const std::shared_ptr<const Block> getBlock(const uint64_t& height);
    bool hasTx(const Hash& tx);
    const std::shared_ptr<const Tx> getTx(const Hash& tx);
    const std::shared_ptr<const Block> getBlockFromTx(const Hash& tx);
    const std::shared_ptr<const Block> latest();
    uint64_t blockSize();
    void periodicSaveToDB();
    void stopPeriodicSaveToDB() { this->stopPeriodicSave = true; }
};

#endif  // BLOCKCHAIN_H
