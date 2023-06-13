#ifndef STATE_H
#define STATE_H

#include "../contract/contractmanager.h"
#include "../utils/utils.h"
#include "../utils/db.h"
#include "storage.h"
#include "rdpos.h"

// TODO: We could possibly change the bool functions
// into a enum function, to be able to properly return each error case
// We need this in order to slash invalid rdPoS blocks.
/// Enum for labeling transaction validity.
enum TxInvalid { NotInvalid, InvalidNonce, InvalidBalance };

/**
 * Abstraction of the blockchain's state.
 * Responsible for maintaining the current blockchain state at the current block.
 */
class State {
  private:
    /// Pointer to the database.
    const std::unique_ptr<DB>& db;

    /// Pointer to the blockchain's storage.
    const std::unique_ptr<Storage>& storage;

    /// Pointer to the rdPoS object.
    const std::unique_ptr<rdPoS>& rdpos;

    /// Pointer to the P2P connection manager.
    const std::unique_ptr<P2P::ManagerNormal> &p2pManager;

    /// Pointer to the options singleton.
    const std::unique_ptr<Options>& options;

    /// Pointer to the contract manager.
    const std::unique_ptr<ContractManager> contractManager;

    // TODO: Add contract functionality to State after ContractManager is ready.

    /// Map with information about blockchain accounts (Address -> Account).
    std::unordered_map<Address, Account, SafeHash> accounts;

    /// TxBlock mempool.
    std::unordered_map<Hash, TxBlock, SafeHash> mempool;

    /// Mutex for managing read/write access to the state object.
    mutable std::shared_mutex stateMutex;

    /**
     * Verify if a transaction can be accepted within the current state.
     * @param tx The transaction to check.
     * @return An enum telling if the block is invalid or not.
     */
    TxInvalid validateTransactionInternal(const TxBlock& tx) const;

    /**
     * Process a transaction within a block. Called by processNextBlock().
     * If the process fails, any state change that this transaction would cause has to be reverted.
     * @param tx The transaction to process.
     */
    void processTransaction(const TxBlock& tx);

    /**
     * Update the mempool, removing transactions that are in the given block,
     * and leaving only valid transactions in it.
     * Called by processNewBlock(), used to filter the current mempool based
     * on transactions that have been accepted on the block, and verify if
     * transactions on the mempool are valid given the new state after processing
     * the block itself.
     * @param block The block to use for pruning transactions from the mempool.
     */
    void refreshMempool(const Block& block);

    /// Flag indicating whether the state is currently processing a payable contract function
    bool processingPayable = false;

  public:
    /**
     * Constructor.
     * @param db Pointer to the database.
     * @param storage Pointer to the blockchain's storage.
     * @param rdpos Pointer to the rdPoS object.
     * @param p2pManager Pointer to the P2P connection manager.
     * @param options Pointer to the options singleton.
     * @throw std::runtime_error on any database size mismatch.
     */
    State(
      const std::unique_ptr<DB>& db,
      const std::unique_ptr<Storage>& storage,
      const std::unique_ptr<rdPoS>& rdpos,
      const std::unique_ptr<P2P::ManagerNormal>& p2pManager,
      const std::unique_ptr<Options>& options
    );

    /// Destructor.
    ~State();

    /**
     * Get the native balance of an account in the state.
     * @param addr The address of the account to check.
     * @return The native account balance of the given address.
     */
    const uint256_t getNativeBalance(const Address& addr) const;

    /**
     * Get the native nonce of an account in the state.
     * @param addr The address of the account to check.
     * @return The native account nonce of the given address.
     */
    const uint64_t getNativeNonce(const Address& addr) const;

    /// Getter for `accounts`. Returns a copy.
    const std::unordered_map<Address, Account, SafeHash> getAccounts() const;

    /// Getter for `mempool`. Returns a copy.
    const std::unordered_map<Hash, TxBlock, SafeHash> getMempool() const;

    /// Get the mempool's current size.
    inline const size_t getMempoolSize() const { std::shared_lock (this->stateMutex); return mempool.size(); }

    /**
     * Validate the next block given the current state and its transactions.
     * Does NOT update the state.
     * The block will be rejected if there are invalid transactions in it
     * (e.g. invalid signature, insufficient balance, etc.).
     * @param block The block to validate.
     * @return `true` if the block is validated successfully, `false` otherwise.
     */
    bool validateNextBlock(const Block& block) const;

    /**
     * Process the next block given current state from the network.
     * DOES update the state.
     * Appends block to Storage after processing.
     * @param block The block to process.
     * @throw std::runtime_error if block is invalid.
     */
    void processNextBlock(Block&& block);

    /**
     * Fill a block with all transactions currently in the mempool.
     * DOES NOT FINALIZE THE BLOCK.
     * @param block The block to fill.
     */
    void fillBlockWithTransactions(Block& block) const;

    /**
     * Verify if a transaction can be accepted within the current state.
     * Calls validateTransactionInternal(), but locking the mutex in a shared manner.
     * @param tx The transaction to verify.
     * @return An enum telling if the transaction is valid or not.
     */
    TxInvalid validateTransaction(const TxBlock& tx) const;

    /**
     * Add a transaction to the mempool, if valid.
     * @param tx The transaction to add.
     * @return An enum telling if the transaction is valid or not.
     */
    TxInvalid addTx(TxBlock&& tx);

    /**
     * Add a Validator transaction to the rdPoS mempool, if valid.
     * @param tx The transaction to add.
     * @return `true` if transaction is valid, `false` otherwise.
     */
    bool addValidatorTx(const TxValidator& tx);

    /**
     * Check if a transaction is in the mempool.
     * @param txHash The transaction hash to check.
     * @return `true` if the transaction is in the mempool, `false` otherwise.
     */
    bool isTxInMempool(const Hash& txHash) const;

    /**
     * Get a transaction from the mempool.
     * @param txHash The transaction Hash.
     * @return A pointer to the transaction, or `nullptr` if not found.
     * We cannot directly copy the transaction, since TxBlock doesn't have a
     * default constructor, thus making it impossible to return
     * an "empty" transaction if the hash is not found in the mempool,
     * so we return a null pointer instead.
     */
    std::unique_ptr<TxBlock> getTxFromMempool(const Hash& txHash) const;

    /**
     * Add balance to a given account.
     * Used through HTTP RPC to add balance to a given address
     * NOTE: ONLY TO BE USED WITHIN THE TESTNET OF A GIVEN CHAIN.
     * THIS FUNCTION ALLOWS ANYONE TO GIVE THEMSELVES NATIVE TOKENS.
     * IF CALLING THIS FUNCTION WITHIN A MULTI-NODE NETWORK, YOU HAVE TO CALL
     * IT ON ALL NODES IN ORDER TO BE VALID.
     * @param addr The address to add balance to.
     */
    void addBalance(const Address& addr);

    /**
     * Simulate an `eth_call` to a contract.
     * @param callInfo Tuple with info about the call (from, to, gasLimit, gasPrice, value, data).
     * @return The return of the called function as a data string.
     */
    Bytes ethCall(const ethCallInfo& callInfo);

    /**
     * Estimate gas for callInfo in RPC.
     * Doesn't really "estimate" gas, but rather tells if the transaction is valid or not.
     * @param callInfo Tuple with info about the call (from, to, gasLimit, gasPrice, value, data).
     * @return `true` if the call is valid, `false` otherwise.
     */
    bool estimateGas(const ethCallInfo& callInfo);

    /**
     * Update the State's account balances after a contract call. Called by ContractManager.
     * @param payableMap A map of the accounts to update and their respective new balances.
     * @throw std::runtime_error on an attempt to change State while not processing a payable contract.
     */
    void processContractPayable(std::unordered_map<Address, uint256_t, SafeHash>& payableMap);

    /// Get a list of contract addresses and names.
    std::vector<std::pair<std::string, Address>> getContracts() const;

    /// the Manager Interface cannot use getNativeBalance. as it will call a lock with the mutex.
    friend class ContractManager::ContractManagerInterface;
};

#endif // STATE_H
