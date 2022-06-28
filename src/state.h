#ifndef STATE_H
#define STATE_H

#include <vector>
#include <unordered_map>
#include <include/web3cpp/ethcore/TransactionBase.h>
#include <atomic>
#include <deque>

#include "utils.h"
#include "block.h"
#include "db.h"
#include "chainHead.h"

// The State Class.
// This class is used to store the state of the system, such as native balances.
// Contract status, mempool transactions, token balances, and the shared inner variables of the blockchain.
// State can only be updated with blocks, it can create the block itself or receive one from the network.

struct Account {
  uint256_t balance = 0;
  uint32_t nonce = 0;
};

class State {
  private:
    std::unordered_map<std::string, Account> nativeAccount;             // Address -> Account
    std::unordered_map<std::string, dev::eth::TransactionBase> mempool; // Tx Hash -> Tx 
    bool saveState(std::shared_ptr<DBService>  &dbServer);
    bool loadState(std::shared_ptr<DBService>  &dbServer);
    std::unordered_map<std::string, dev::eth::TransactionBase> latestConfirmedTransactions;
    std::mutex stateLock;

  public:
    State(std::shared_ptr<DBService> &dbServer);

    uint256_t getNativeBalance(const std::string& address) { return nativeAccount[address].balance; };
    uint256_t getNativeNonce(const std::string& address) { return nativeAccount[address].balance; };
    const std::unordered_map<std::string, dev::eth::TransactionBase>& getMempool() { return mempool; };

    // State changing functions
    // Process a new block from the network and update the local state.
    bool processNewBlock(Block &newBlock, std::unique_ptr<ChainHead>& chainHead);
    // Process a new transaction from a given block (only used by processNewBlock).
    bool processNewTransaction(dev::eth::TransactionBase &tx, std::unique_ptr<ChainHead>& chainHead);
    bool createNewBlock(std::unique_ptr<ChainHead>& chainHead);

    // State querying functions
    // Asks the state if a given transaction is valid, add to mempool if it is.
    bool validateTransaction(dev::eth::TransactionBase &tx);
};

#endif // STATE_H