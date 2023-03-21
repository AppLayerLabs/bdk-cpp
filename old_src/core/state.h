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
#include "blockmanager.h"
#include "../utils/db.h"
#include "../utils/random.h"
#include "../utils/transaction.h"
#include "../utils/utils.h"
#include "../net/grpcclient.h"

class VMCommClient; // Forward declaration.
class ChainTip;
class BlockManager;

/**
 * The State class is used to store the state of the system, such as
 * native balances, contract statuses, mempool transactions, token balances
 * and the shared inner variables of the blockchain.
 * State can only be updated with blocks, either by creating one itself
 * or receiving one from the network.
 */
class State {
  private:
    std::unordered_map<Address, Account, SafeHash> nativeAccount; // Address -> Account
    mutable std::unordered_map<Hash, Tx::Base, SafeHash> mempool; // Tx Hash (bytes) -> Tx
    // TODO: improve mempool structure (e.g. verify txs in mempool not included in a block after accepting another block, as we need to keep every tx valid)
    mutable std::shared_mutex stateLock;
    
    // Used to notify AvalancheGo when creating new blocks.
    std::shared_ptr<VMCommClient> &grpcClient;

    // Save accounts from memory to DB. Does a batch operation.
    bool saveState(std::shared_ptr<DBService> &dbServer);

    // Load accounts from DB to memory.
    bool loadState(std::shared_ptr<DBService> &dbServer);

    // Process a new transaction from a given block (only used by processNewBlock).
    // Not threadified, can be only called by one thread.
    bool processNewTransaction(const Tx::Base &tx);

    // Used by transactions that require randomness (none at the moment)
    RandomGen gen;

  public:

    State(std::shared_ptr<DBService> &dbServer, std::shared_ptr<VMCommClient> &grpcClient);

    uint256_t getNativeBalance(const Address& address);
    uint256_t getNativeNonce(const Address& address);
    const std::unordered_map<Hash, Tx::Base, SafeHash>& getMempool() const { return mempool; };

    // State changing functions

    // Validates if a given block is valid and the transactions within. Does *not* update the state.
    bool validateNewBlock(const std::shared_ptr<const Block> &newBlock, const std::shared_ptr<const ChainHead>& chainHead, const std::shared_ptr<const BlockManager>& blockManager) const;

    // Process a new block from the network and update the local state. to be called by chainTip.
    // The block is moved to this function because it will move the block into the chainHead if succeeds.
    void processNewBlock(const std::shared_ptr<const Block> &&newBlock, const std::shared_ptr<ChainHead>& chainHead, const std::shared_ptr<BlockManager> &blockManager);

    // Create a new block using setPreference or latest in case of not found. Does *not* update the state.
    const std::shared_ptr<const Block> createNewBlock(const std::shared_ptr<ChainHead>& chainHead, const std::shared_ptr<ChainTip> &chainTip, const std::shared_ptr<BlockManager> &blockManager) const;

    // State querying functions

    // Validates a transaction inside a block, does not update the state. If returns false block is considered invalid.
    // Does **not** move tx, as it is already included in a given block.
    bool validateTransactionForBlock(const Tx::Base &tx) const;

    // Validates a transaction from RPC, moving the transaction to the mempool. does not update the state, returns error handling for RPC.
    // Add transaction to mempool if valid. moves tx to itself.
    std::pair<int, std::string> validateTransactionForRPC(const Tx::Base &tx) const;

    // TEST ONLY FUNCTIONS.

    void addBalance(const Address &address);

    friend class Subnet;

};

#endif // STATE_H
