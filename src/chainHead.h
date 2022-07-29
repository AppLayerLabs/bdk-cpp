#ifndef CHAINHEAD_H
#define CHAINHEAD_H

#include "utils.h"
#include "block.h"
#include "db.h"

class ChainHead {
  // TODO: find a way to merge lookupBlockHeightByHash and lookupBlockHashByHeight into one and cut those tables' RAM usage in half
  // TODO: We are storing txs in the block, but we are also storing the tx itself on the database. We should place the tx only one location, preferable on blocks.
  //       It is possible to use DBPrefix::txToBlocks to catch the block that the tx is included.
  private:
    std::shared_ptr<DBService> &dbServer;
    std::deque<std::shared_ptr<Block>> internalChainHead;
    std::unordered_map<std::string,std::shared_ptr<Block>> lookupBlockByHash;
    std::unordered_map<std::string,std::shared_ptr<Block>> lookupBlockByTxHash;
    std::unordered_map<std::string,std::shared_ptr<Tx::Base>> lookupTxByHash;
    std::unordered_map<std::string,uint64_t> lookupBlockHeightByHash;
    std::unordered_map<uint64_t,std::string> lookupBlockHashByHeight;
    std::mutex internalChainHeadLock;
    bool hasBlock(std::string &blockHash);
    bool hasBlock(uint64_t &blockHeight);
    // Only access these functions if you are absolute sure that internalChainHeadLock is locked.
    void _push_back(Block& block);
    void _push_front(Block& block);


  public:
    ChainHead(std::shared_ptr<DBService> &_dbService);
    // Mutex locked.
    void push_back(Block& block);
    void push_front(Block& block);
    void pop_back();
    void pop_front();
    bool exists(std::string &blockHash);
    bool exists(uint64_t &blockHeight);
    Block getBlock(std::string &blockHash);
    Block getBlock(uint64_t &blockHeight);
    bool hasTransaction(std::string &txHash);
    Tx::Base getTransaction(std::string &txHash);
    Block getBlockFromTx(std::string &txHash);
    Block latest();
    uint64_t blockSize();
    void loadFromDB();
    void dumpToDB();
    void periodicSaveToDB();  // TODO: implement this
};

#endif  // CHAINHEAD_H
