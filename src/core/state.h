#ifndef STATE_H
#define STATE_H

#include <atomic>
#include <chrono>
#include <deque>
#include <unordered_map>
#include <vector>
#include <shared_mutex>

#include "block.h"
#include "chainHead.h"
#include "db.h"
#include "transaction.h"
#include "utils.h"
#if !IS_LOCAL_TESTS
#include "../net/grpcclient.h"
#endif

class VMCommClient; // Forward declaration.
class ChainTip;

/**
 * The State class is used to store the state of the system, such as
 * native balances, contract statuses, mempool transactions, token balances
 * and the shared inner variables of the blockchain.
 * State can only be updated with blocks, either by creating one itself
 * or receiving one from the network.
 */
class State {
  private:
    std::unordered_map<Address, Account> nativeAccount; // Address -> Account
    mutable std::unordered_map<std::string, Tx::Base> mempool; // Tx Hash (bytes) -> Tx
    mutable std::shared_mutex stateLock;

    // used to notify avalancheGo when to create new blocks.
    #if !IS_LOCAL_TESTS
      std::shared_ptr<VMCommClient> &grpcClient;
    #endif

    // Save accounts from memory to DB. Does a batch operation.
    bool saveState(std::shared_ptr<DBService> &dbServer);

    // Load accounts from DB to memory.
    bool loadState(std::shared_ptr<DBService> &dbServer);

    // Process a new transaction from a given block (only used by processNewBlock).
    // Not threadified, can be only called by one thread.
    bool processNewTransaction(const Tx::Base &tx);

  public:
    #if !IS_LOCAL_TESTS
      State(std::shared_ptr<DBService> &dbServer, std::shared_ptr<VMCommClient> &grpcClient);
    #else
      State(std::shared_ptr<DBService> &dbServer);
    #endif
    uint256_t getNativeBalance(const Address& address);
    uint256_t getNativeNonce(const Address& address);
    const std::unordered_map<std::string, Tx::Base>& getMempool() { return mempool; };

    // State changing functions

    // Validates if a given block is valid and the transactions within. Does *not* update the state.
    bool validateNewBlock(const Block &newBlock, const std::shared_ptr<const ChainHead>& chainHead) const;

    // Process a new block from the network and update the local state. to be called by chainTip.
    // The block is moved to this function because it will move the block into the chainHead if succeeds.
    void processNewBlock(const std::shared_ptr<const Block> &&newBlock, const std::shared_ptr<ChainHead>& chainHead);

    // Create a new block using setPreference or latest in case of not found. Does *not* update the state.
    const std::shared_ptr<const Block> createNewBlock(std::shared_ptr<ChainHead>& chainHead, std::shared_ptr<ChainTip> &chainTip) const;

    // State querying functions

    // Asks the state if a given transaction is valid, and add it to the mempool if it is.
    // TODO: Maybe split this function into two functions, one for actually verifying a tx inside a block, and one for adding it to mempool.
    // Transaction multithreadification can be achieved at the block and RPC level, as the expensive part is the Tx constructor which runs secp256k1.
    std::pair<int, std::string> validateTransaction(const Tx::Base &tx) const;

    // TEST ONLY FUNCTIONS.

    void addBalance(const Address &address);

    friend class Subnet;
};

#endif // STATE_H
