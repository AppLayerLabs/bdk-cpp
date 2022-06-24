#ifndef STATE_H
#define STATE_H

#include <vector>
#include <unordered_map>
#include <include/web3cpp/ethcore/TransactionBase.h>
#include <atomic>

#include "utils.h"
#include "block.h"
#include "db.h"

// The State Class.
// This class is used to store the state of the system, such as native balances.
// Contract status, mempool transactions, token balances, and the shared inner variables of the blockchain.
// The state only knows the latest block, it does not know the whole chain.
// State can only be updated with blocks, it can create the block itself or receive one from the network.

struct Account {
  uint256_t balance = 0;
  uint32_t nonce = 0;
};

class State {
  private:
    std::unordered_map<std::string, Account> nativeAccount;
    std::vector<dev::eth::TransactionBase> mempool;
    bool saveState(std::shared_ptr<DBService>  &dbServer);
    bool loadState(std::shared_ptr<DBService>  &dbServer);
    Block latest;

  public:
    State(std::shared_ptr<DBService> &dbServer);

    uint256_t getNativeBalance(const std::string& address) { return nativeAccount[address].balance; };
    uint256_t getNativeNonce(const std::string& address) { return nativeAccount[address].balance; };
    std::vector<dev::eth::TransactionBase> getMempool() { return mempool; };
    Block getLatest() { return latest; };

    // State changing functions
    // Process a new block from the network and update the local state.
    bool processNewBlock(Block &newBlock);
    // Process a new transaction from a given block (only used by processNewBlock).
    bool processNewTransaction(dev::eth::TransactionBase &tx);
    bool createNewBlock();

    // State querying functions
    // Asks the state if a given transaction is valid.
    bool validateTransaction(dev::eth::TransactionBase &tx);
};

#endif // STATE_H