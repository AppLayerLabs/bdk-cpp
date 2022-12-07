#ifndef CHAINHEAD_H
#define CHAINHEAD_H

#include <shared_mutex>
#include "block.h"
#include "../utils/db.h"
#include "../utils/utils.h"
#include "../utils/random.h"

class ChainHead {
  private:
    std::shared_ptr<DBService> &dbServer;
    std::deque<std::shared_ptr<const Block>> internalChainHead;
    std::unordered_map<Hash,std::shared_ptr<const Block>, SafeHash> lookupBlockByHash;
    std::unordered_map<Hash,std::shared_ptr<const Block>, SafeHash> lookupBlockByTxHash;
    std::unordered_map<Hash,std::shared_ptr<const Tx::Base>, SafeHash> lookupTxByHash;
    std::unordered_map<Hash,uint64_t, SafeHash> lookupBlockHeightByHash;
    std::unordered_map<uint64_t,Hash, SafeHash> lookupBlockHashByHeight;
    // Used for cacheing blocks and transactions when they are used to load from a DB.
    // See that functions getBlock returns a reference to a shared_ptr.
    // We need to make sure that the reference exists after the scope of the function.
    // TODO: figure out a way to clean up shared_ptr after scope of parent function or after X unused time.
    // shared_ptr::use_count can be used for this.
    mutable std::unordered_map<Hash,std::shared_ptr<const Block>, SafeHash> cachedBlocks;
    mutable std::unordered_map<Hash,std::shared_ptr<const Tx::Base>, SafeHash> cachedTxs;
    // Mutable provides better const correctness for getBlock and other functions. Its use is acceptable for mutexes and cache.
    mutable std::shared_mutex internalChainHeadLock;
    bool hasBlock(Hash const &blockHash) const;
    bool hasBlock(uint64_t const &blockHeight) const;
    // Only access these functions if you are absolute sure that internalChainHeadLock is locked.
    void _push_back(const std::shared_ptr<const Block> &&block);
    void _push_front(const std::shared_ptr<const Block> &&block);
    void loadFromDB();
    std::thread periodicSaveThread;
    uint64_t periodicSaveCooldown = 15; // In seconds
    bool stopPeriodicSave = false;

  public:
    ChainHead(std::shared_ptr<DBService> &_dbService) : dbServer(_dbService) {
      this->loadFromDB();
      this->periodicSaveThread = std::thread([&]{ this->periodicSaveToDB(); });
      this->periodicSaveThread.detach();
    }
    // Mutex locked.
    // When a block is accepted, it is directly moved to chainHead.
    // Any function calling push_back/front should keep in mind that block will be moved.
    void push_back(const std::shared_ptr<const Block> &&block);
    void push_front(const std::shared_ptr<const Block> &&block);
    void pop_back();
    void pop_front();
    const bool exists(Hash const &blockHash) const;
    const bool exists(uint64_t const &blockHeight) const;
    // chainHead does not return a reference to the pointer
    // Because we need to make sure that the reference exists after the scope of the function.
    const std::shared_ptr<const Block> getBlock(Hash const &blockHash) const;
    const std::shared_ptr<const Block> getBlock(uint64_t const &blockHeight) const;
    bool hasTransaction(const Hash &txHash) const;
    const std::shared_ptr<const Tx::Base> getTransaction(const Hash &txHash) const;
    const std::shared_ptr<const Block> getBlockFromTx(const Hash &txHash) const;
    const std::shared_ptr<const Block> latest() const;
    uint64_t blockSize();
    void dumpToDB();
    void periodicSaveToDB();
    void stopPeriodicSaveToDB() { this->stopPeriodicSave = true; }
};

#endif  // CHAINHEAD_H
