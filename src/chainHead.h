#ifndef CHAINHEAD_H
#define CHAINHEAD_H

#include "utils.h"
#include "block.h"
#include "db.h"
#include <include/web3cpp/ethcore/TransactionBase.h>

// The ChainHead class holds two main pieces of information:
// A small history of blocks (up to 1000 blocks) and their transactions
// This is used to help the state process new blocks and transactions, and to help answering RPC queries.
// ChainHead periodically dumps to DB to keep the history of blocks and transactions lightweight.

class ChainHead {
  private:
    std::deque<Block> internalChainHead;
    std::map<std::string,std::shared_ptr<Block>> internalChainHeadLookupTableByHash; // Hash   -> block (pointer to std::deque)
    std::map<uint64_t,std::shared_ptr<Block>> internalChainHeadLookupTableByHeight; // Height -> block (pointer to std::deque)

    std::map<std::string,std::shared_ptr<dev::eth::TransactionBase>> internalLatestConfirmedTransactions; // Hash -> Tx (Pointer to tx in std::deque block)

    std::map<std::string, std::shared_ptr<Block> > internalTxToBlocksLookupTable; // Tx Hash -> Block (Pointer to std::deque)

    std::mutex internalChainHeadLock;

  public:

    ChainHead(std::shared_ptr<DBService> &dbServer);

    void push_back(Block& block);
    void push_front(Block& block);
    
    void pop_back();
    void pop_front();

    bool hasBlock(std::string &blockHash);
    bool hasBlock(uint64_t &blockHeight);

    Block getBlock(std::string &blockHash);
    Block getBlock(uint64_t &blockHeight);

    bool hasTransaction(std::string &txHash);

    dev::eth::TransactionBase getTransaction(std::string &txHash);
    Block getBlockFromTx(std::string &txHash);
    
    Block latest();

    // Load at max 1000 Blocks from DB. Always latest blocks.
    void loadFromDB(std::shared_ptr<DBService> &dbServer);

    // Dump the entire local state to DB.
    void dumpToDB(std::shared_ptr<DBService> &dbServer);

    // Function for periodically save the blocks into DB.
    void periodicSaveToDB(std::shared_ptr<DBService> &dbServer);

    uint64_t blockSize();
};


#endif