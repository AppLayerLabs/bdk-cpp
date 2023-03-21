#ifndef STATE_H
#define STATE_H

#include "../utils/utils.h"
#include "../utils/db.h"
#include "storage.h"
#include "rdpos.h"


class State {
  private:
    /// Pointer to DB.
    const std::unique_ptr<DB>& db;

    /// Pointer to Storage.
    const std::unique_ptr<Storage>& storage;

    /// Pointer to rdPoS.
    const std::unique_ptr<rdPoS>& rdpos;

    /// Pointer to P2P Manager
    const std::unique_ptr<P2P::ManagerNormal> &p2pManager;

    /// Pointer to ContractManager
    /// TODO: Add contract functionality to State after ContractManager is ready.

    /**
     * std::unordered_map containing information about the blockchain accounts
     * the map is expressed as following:
     * Address -> Account
     * Where Account is a struct containing balance and account nonce.
     */
    std::unordered_map<Address, Account, SafeHash> accounts;

    /// Normal (TxBlock) Transaction Mempool
    std::unordered_map<Hash, TxBlock, SafeHash> mempool;

    /// State mutex
    mutable std::shared_mutex stateMutex;

    /**
     * Process the transaction
     * To be called by State::processNextBlock
     * @param tx within a block
     * @return 'true' if transaction succeeded, 'false' if transaction failed.
     * when transaction fails, any state change that this transaction would cause has to be reverted
     */
    bool processTransaction(const TxBlock& tx);
  public:

    State(const std::unique_ptr<DB>& db,
          const std::unique_ptr<Storage>& storage,
          const std::unique_ptr<rdPoS>& rdpos,
          const std::unique_ptr<P2P::ManagerNormal>& p2pManager);

    ~State();

    /**
     * Get native account balance
     * @param addr
     * @return native account balance.
     */

    const uint256_t getNativeBalance(const Address& addr) const;

    /**
     * Get native account nonce
     * @param addr
     * @return native account nonce.
     */

    const uint256_t getNativeNonce(const Address& addr) const;

    /**
     * Getter for mempool
     * @return mempool copy
     */

    const std::unordered_map<Address, Account, SafeHash> getMempool() const;

    /**
     * Validate the next block given current state and its transactions. Does NOT update the state.
     * The block will be rejected if there are invalid transactions in it
     * (e.g. invalid signature, insufficient balance, etc.).
     * @param block The block to validate.
     * @return `true` if the block is validated successfully, `false` otherwise.
     */
    bool validateNextBlock(const Block& block) const;

    /**
     * Process the next block given current state from the network. DOES update the state.
     * Appends block to Storage after processing.
     * @param block The block to process.
     */
    void processNextBlock(Block&& block);


    /**
     * Create a new block with the block transactions filled with the current mempool.
     * Does not fill block TxValitador transactions, neither finalize the block or update State.
     * @return Block with transactions currently on mempool.
     */
    Block createNewBlock() const;

    /**
     * Verify if the transaction can be accepted within the current State.
     * @param tx The transaction
     */
    bool validateTransaction(const TxBlock& tx) const;

    /**
     * Add transaction to mempool if valid.
     * @param tx The transaction.
     */
    bool addTx(TxBlock&& tx);

    /**
     * This function is used through HTTP RPC to add balance to a given address
     * ONLY TO BE USED WITHIN TESTNET OF A GIVEN APP-CHAIN.
     * THIS FUNCTION ALLOWS ANYONE TO GIVE THEMSELVES NATIVE TOKENS
     */
    void addBalance(const Address& addr);

};


#endif // STATE