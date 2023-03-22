#ifndef STATE_H
#define STATE_H

#include "../utils/utils.h"
#include "../utils/db.h"
#include "storage.h"
#include "rdpos.h"

// TODO: We could possibly change the bool functions
// into a enum function, to be able to properly return each error case
// We need this in order to slash invalid rdPoS blocks.
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
    void processTransaction(const TxBlock& tx);

    /**
     * Free mempool from processing block transactions.
     * This function is called by processNewBlock and it is used to filter out our current
     * mempool based on transactions that have been accepted on the block, and verify if transactions
     * on the mempool are valid given the new State after processing the block.
     */

    bool refreshMempool();
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
     * Getter for accounts within the current State
     * @return accounts from copy
     */
    const std::unordered_map<Address, Account, SafeHash> getAccounts() const;

    /**
     * Getter for mempool
     * @return mempool copy
     */
    const std::unordered_map<Hash, TxBlock, SafeHash> getMempool() const;

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
     * Fill the block with all transactions with the current mempool.
     * DOES NOT FINALIZE THE BLOCK.
     */
    void fillBlockWithTransactions(Block& block) const;

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
     * IF CALLING THIS FUNCTION WITH A MULTI-NODE NETWORK, YOU HAVE TO CALL IT ON ALL NODES IN ORDER TO BE VALID.
     */
    void addBalance(const Address& addr);

};


#endif // STATE