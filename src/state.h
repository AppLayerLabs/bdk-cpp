#ifndef STATE_H
#define STATE_H

#include <vector>
#include <unordered_map>
#include <include/web3cpp/ethcore/TransactionBase.h>
#include <atomic>
#include <deque>
#include <chrono>

#include "utils.h"
#include "block.h"
#include "db.h"
#include "chainHead.h"

struct Account {
  uint256_t balance = 0;
  uint32_t nonce = 0;
};

// Forward declaration
class VMCommClient;
/**
 * The State class is used to store the state of the system, such as
 * native balances, contract statuses, mempool transactions, token balances
 * and the shared inner variables of the blockchain.
 * State can only be updated with blocks, either by creating one itself
 * or receiving one from the network.
 */
class State {
  private:
    std::unordered_map<std::string, Account> nativeAccount;             // Address -> Account
    std::unordered_map<std::string, dev::eth::TransactionBase> mempool; // Tx Hash -> Tx
    std::mutex stateLock;

    // used to notify avalancheGo when to create new blocks.
    std::shared_ptr<VMCommClient> &grpcClient;

    // Save accounts from memory to DB. Does a batch operation.
    bool saveState(std::shared_ptr<DBService> &dbServer);

    // Load accounts from DB to memory.
    bool loadState(std::shared_ptr<DBService> &dbServer);

    // Process a new transaction from a given block (only used by processNewBlock).
    bool processNewTransaction(const dev::eth::TransactionBase &tx);

  public:
    State(std::shared_ptr<DBService> &dbServer, std::shared_ptr<VMCommClient> &grpcClient);

    uint256_t getNativeBalance(const std::string& address);
    uint256_t getNativeNonce(const std::string& address);
    const std::unordered_map<std::string, dev::eth::TransactionBase>& getMempool() { return mempool; };

    // State changing functions

    // Process a new block from the network and update the local state.
    bool processNewBlock(Block &newBlock, std::unique_ptr<ChainHead>& chainHead);

    bool createNewBlock(std::unique_ptr<ChainHead>& chainHead);

    // State querying functions

    // Asks the state if a given transaction is valid, and add it to the mempool if it is.
    bool validateTransaction(dev::eth::TransactionBase &tx);

    // TEST ONLY FUNCTIONS.

    void addBalance(std::string &address);

    friend class Subnet;
};

#endif // STATE_H
