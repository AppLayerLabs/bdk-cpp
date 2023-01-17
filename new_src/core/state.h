#ifndef STATE_H
#define STATE_H

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "block.h"
#include "blockChain.h"
#include "blockManager.h"
//#include "subnet.h" // TODO: fix circular dep
#include "../net/grpcclient.h"
#include "../utils/db.h"
#include "../utils/random.h"
#include "../utils/transaction.h"
#include "../utils/utils.h"

/**
 * Class for storing system state and shared blockchain inner variables.
 * e.g. coin/token balances, contract statuses, mempool, txs, block parsing, creation, etc.
 * Updates with blocks only, either creating one itself or receiving one from the network.
 */
class State {
  private:
    /**
     * List of native accounts, by address.
     * A "native account" is an account used to make normal transaction operations.
     */
    std::unordered_map<Address, Account, SafeHash> nativeAccounts;

    /**
     * Transaction mempool.
     * This differs from %BlockMempool because those are transactions not bound
     * to any block, whereas the transactions on %BlockMempool are already in
     * a block, waiting to be accepted or rejected.
     */
    std::unordered_map<Hash, Tx, SafeHash> mempool;

    /// Pointer to the database.
    const std::shared_ptr<DB> db;

    /// Pointer to the blockchain.
    const std::shared_ptr<BlockChain> chain;

    /// Pointer to the block mempool.
    const std::shared_ptr<BlockMempool> mempool;

    /// Pointer to the block manager.
    const std::shared_ptr<BlockManager> mgr;

    /// Pointer to the gRPC client.
    const std::shared_ptr<gRPCClient> grpcClient;

    /// Mutex for managing read/write access to the state.
    std::mutex stateLock;

    /// Random seed generator.
    RandomGen gen;

    /**
     * Save accounts from memory to database.
     * @return `true` if the state was saved successfully, `false` otherwise.
     */
    bool saveToDB();

    /**
     * Load accounts from database to memory.
     * @return `true` if the state was loaded successfully, `false` otherwise.
     */
    bool loadFromDB();

    /**
     * Process a new transaction from a given block (only used by `processNewBlock()`).
     * @param tx The transaction to process.
     * @return `true` if the transaction was processed successfully, `false` otherwise.
     */
    bool processNewTx(const Tx& tx);

  public:
    /**
     * Constructor. Automatically loads accounts from the database.
     * @param db Pointer to the database.
     * @param chain Pointer to the blockchain.
     * @param mempool Pointer to the blockchain's mempool.
     * @param mgr Pointer to the block manager.
     * @param grpcClient Pointer to the gRPC client.
     */
    State(
      const std::shared_ptr<DB>& db,
      const std::shared_ptr<BlockChain>& chain,
      const std::shared_ptr<BlockMempool>& mempool,
      const std::shared_ptr<BlockManager>& mgr,
      const std::shared_ptr<gRPCClient>& grpcClient
    ) : db(db), chain(chain), mempool(mempool), mgr(mgr), grpcClient(grpcClient) {
      this->loadFromDB();
    }

    /// Getter for `mempool`.
    const std::unordered_map<Hash, Tx, SafeHash>& getMempool() { return this->mempool; }

    /**
     * Get a native account's balance.
     * @param add The account's address.
     * @return The native account's current balance.
     */
    uint256_t getNativeBalance(const Address& add);

    /**
     * Get a native account's nonce.
     * @param add The account's address.
     * @return The native account's current nonce.
     */
    uint256_t getNativeNonce(const Address& add);

    /**
     * Validate a block and its transactions. Does NOT update the state.
     * The block will be rejected if there are invalid transactions in it
     * (e.g. invalid signature, insufficient balance, etc.).
     * @param block The block to validate.
     * @return `true` if the block is validated successfully, `false` otherwise.
     */
    bool validateNewBlock(const std::shared_ptr<const Block>& block);

    /**
     * Process a new block from the network. DOES update the state.
     * @param block The block to process.
     */
    void processNewBlock(const std::shared_ptr<const Block>&& block);

    /**
     * Create a new block. Does NOT update the state.
     * Uses either the preferred block, or the latest block if there's no preference.
     * @return A pointer to the new block, or `nullptr` if block creation fails.
     */
    const std::shared_ptr<const Block> createNewBlock();

    /**
     * Validate a transaction from inside a block.
     * Validation is done checking the following requirements:
     * - Replay protection
     * - Sender nonce matches
     * - Sender has enough balance
     * - Tx is not already in mempool
     * Does NOT update the state and does NOT move the tx to the mempool,
     * as it is already included in a block.
     * @param tx The transaction to validate.
     * @return `true` if the transaction is valid, `false` otherwise.
     */
    bool validateTxForBlock(const Tx& tx);

    /**
     * Validates a transaction from RPC.
     * Validation is done checking the following requirements:
     * - Replay protection
     * - Sender nonce matches
     * - Sender has enough balance
     * - Tx is not already in mempool
     * Does NOT update the state, but DOES move the tx to the mempool.
     * @param tx The transaction to validate.
     * @return An error code/message pair with the status of the validation.
     */
    const std::pair<int, string> validateTxForRPC(const Tx& tx);

    /**
     * Add a fixed amount of funds to an account.
     * @param add The address to add funds to.
     * TODO: FOR TESTING PURPOSES ONLY. This should be removed before release.
     */
    void addBalance(const Address& add);
};

#endif  // STATE_H
