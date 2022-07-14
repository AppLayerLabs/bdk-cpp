#ifndef CHAINHEAD_H
#define CHAINHEAD_H

#include "utils.h"
#include "block.h"
#include "db.h"
#include <include/web3cpp/ethcore/TransactionBase.h>

/**
 * The ChainHead class holds two main pieces of information:
 * a small history of blocks (up to 1000 blocks) and their transactions.
 * This is used to help the state process new blocks and transactions,
 * and to help answering RPC queries.
 * ChainHead periodically dumps to DB to keep the history of blocks and
 * transactions lightweight in memory.
 */
class ChainHead {
  // TODO: getBlock/getTransaction is only accessing memory, should use DB in case of not found.
  private:
    std::shared_ptr<DBService> &dbServer; // Pointer to DB Service, which ChainHead accesses when needed.
    std::deque<Block> internalChainHead;
    std::unordered_map<std::string,std::shared_ptr<Block>> internalChainHeadLookupTableByHash; // Hash -> block (pointer to std::deque)

    /**
     * We keep a table of already known blocks in memory. The reasoning for
     * that is to mitigate any possible problem by using the (slow) gRPC
     * DB service that AvalancheGo already knows.
     * Unfortunately, storing a map for all the transactions would be too
     * expensive. Because of that we only store a small history of
     * transactions in memory, and the rest in the DB.
     */

    // Indexes all blocks known by the node, looking by height -> hash (used to speed up block lookup).
    std::unordered_map<uint64_t,std::string> diskChainHeadLookupTableByHeight;

    // Indexes all blocks known by the node, looking by hash -> height (used to speed up block lookup).
    std::unordered_map<std::string,uint64_t> diskChainHeadLookupTableByHash;

    std::unordered_map<std::string,std::shared_ptr<dev::eth::TransactionBase>> internalLatestConfirmedTransactions; // Hash -> Tx (Pointer to tx in std::deque block)
    std::unordered_map<std::string,std::shared_ptr<Block>> internalTxToBlocksLookupTable;                         // Tx Hash -> Block (Pointer to std::deque)

    std::mutex internalChainHeadLock;

    // Checks if block is found in memory or disk, search by hash.
    bool hasBlock(std::string &blockHash);

    // Checks if block is found in memory or disk, search by height.
    bool hasBlock(uint64_t &blockHeight);

  public:
    ChainHead(std::shared_ptr<DBService> &_dbService);

    void push_back(Block& block);
    void push_front(Block& block);

    void pop_back();
    void pop_front();

    bool exists(std::string &blockHash);
    bool exists(uint64_t &blockHeight);

    Block getBlock(std::string &blockHash);
    Block getBlock(uint64_t &blockHeight);

    bool hasTransaction(std::string &txHash);

    dev::eth::TransactionBase getTransaction(std::string &txHash);
    Block getBlockFromTx(std::string &txHash);

    Block latest();

    void loadFromDB();        // Load up to 1000 latest blocks from DB.
    void dumpToDB();          // Dump the entire local state to DB.
    void periodicSaveToDB();  // Periodically save blocks to DB. // TODO: implement this

    uint64_t blockSize();
};


#endif  // CHAINHEAD_H
