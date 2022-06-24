#ifndef STATE_H
#define STATE_H

#include <vector>
#include <unordered_map>
#include "utils.h"
#include <web3cpp/ethcore/TransactionBase.h>

// The State Class.
// This class is used to store the state of the system, such as native balances.
// Contract status, mempool transactions, token balances, and the shared inner variables of the blockchain.
// The state only knows the latest block, it does not know the whole chain.
// State can only be updated with blocks, it can create the block itself or receive one from the network.
class State {
  private:
    std::unordered_map<std::string, uint256_t> nativeBalance;
    std::vector<dev::eth::TransactionBase> mempool;

  public:
    State();
    ~State();

    // Getters.
    uint256_t getNativeBalance(const std::string& address) { return nativeBalance[address]; };
    std::vector<dev::eth::TransactionBase> getMempool() { return mempool; };


    // Appenders.



};

#endif // STATE_H