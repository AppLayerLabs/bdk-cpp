/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STATE_H
#define STATE_H

#include "../contract/contract.h"
#include "../contract/contractmanager.h"
#include "../utils/utils.h"
#include "../utils/db.h"
#include "storage.h"
#include "rdpos.h"

// TODO: We could possibly change the bool functions into an enum function,
// to be able to properly return each error case. We need this in order to slash invalid rdPoS blocks.

/// Enum for labeling transaction validity.
enum TxInvalid { NotInvalid, InvalidNonce, InvalidBalance };

/// Abstraction of the blockchain's current state at the current block.
class State {
  private:
    const Options& options_;  ///< Reference to the options singleton.
    DB& db_;  ///< Reference to the database.
    Storage& storage_;  ///< Reference to the blockchain's storage.
    P2P::ManagerNormal& p2pManager_;  ///< Reference to the P2P connection manager.
    rdPoS rdpos_; ///< rdPoS object (consensus).
    ContractManager contractManager_; ///< Contract Manager.
    std::unordered_map<Address, Account, SafeHash> accounts_; ///< Map with information about blockchain accounts (Address -> Account).
    std::unordered_map<Hash, TxBlock, SafeHash> mempool_; ///< TxBlock mempool.
    mutable std::shared_mutex stateMutex_;  ///< Mutex for managing read/write access to the state object.
    bool processingPayable_ = false;  ///< Indicates whether the state is currently processing a payable contract function.

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
     * @param blockHash The hash of the block being processed.
     * @param txIndex The index of the transaction inside the block that is being processed.
     */
    void processTransaction(const TxBlock& tx, const Hash& blockHash, const uint64_t& txIndex);

    /**
     * Update the mempool, remove transactions that are in the given block, and leave only valid transactions in it.
     * Called by processNewBlock(), used to filter the current mempool based on transactions that have been
     * accepted on the block, and verify if transactions on the mempool are valid given the new state after
     * processing the block itself.
     * @param block The block to use for pruning transactions from the mempool.
     */
    void refreshMempool(const Block& block);

  public:
    /**
     * Constructor.
     * @param db Pointer to the database.
     * @param storage Pointer to the blockchain's storage.
     * @param p2pManager Pointer to the P2P connection manager.
     * @param options Pointer to the options singleton.
     * @throw DynamicException on any database size mismatch.
     */
    State(DB& db, Storage& storage, P2P::ManagerNormal& p2pManager, const Options& options);

    ~State(); ///< Destructor.

    // ======================================================================
    // RDPOS WRAPPER FUNCTIONS
    // ======================================================================

    ///@{
    /**
     * Wrapper for the respective rdPoS function.
     * Returns a copy to prevent a possible segfault condition when returning a
     * const reference while using a mutex at the same time.
     */
    std::set<Validator> rdposGetValidators() const {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.getValidators();
    }
    std::vector<Validator> rdposGetRandomList() const {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.getRandomList();
    }
    std::unordered_map<Hash, TxValidator, SafeHash> rdposGetMempool() const {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.getMempool();
    }
    Hash rdposGetBestRandomSeed() const {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.getBestRandomSeed();
    }
    bool rdposGetIsValidator() const {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.getIsValidator();
    }
    uint32_t rdposGetMinValidators() const {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.getMinValidators();
    }
    bool rdposValidateBlock(const Block& block) const {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.validateBlock(block);
    }
    Hash rdposProcessBlock(const Block& block) {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.processBlock(block);
    }
    bool rdposAddValidatorTx(const TxValidator& tx) {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_); return this->rdpos_.addValidatorTx(tx);
    }
    ///@}

    // ======================================================================
    // STATE FUNCTIONS
    // ======================================================================

    /**
     * Get the native balance of an account in the state.
     * @param addr The address of the account to check.
     * @return The native account balance of the given address.
     */
    uint256_t getNativeBalance(const Address& addr) const;

    /**
     * Get the native nonce of an account in the state.
     * @param addr The address of the account to check.
     * @return The native account nonce of the given address.
     */
    uint64_t getNativeNonce(const Address& addr) const;

    std::unordered_map<Address, Account, SafeHash> getAccounts() const; ///< Getter for `accounts_`. Returns a copy.
    std::unordered_map<Hash, TxBlock, SafeHash> getMempool() const; ///< Getter for `mempool_`. Returns a copy.

    /// Get the mempool's current size.
    inline size_t getMempoolSize() const {
      std::shared_lock<std::shared_mutex> lock (this->stateMutex_);
      return this->mempool_.size();
    }

    /**
     * Validate the next block given the current state and its transactions. Does NOT update the state.
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
     * @throw DynamicException if block is invalid.
     */
    void processNextBlock(Block&& block);

    /**
     * Fill a block with all transactions currently in the mempool. DOES NOT FINALIZE THE BLOCK.
     * @param block The block to fill.
     */
    void fillBlockWithTransactions(Block& block) const;

    /**
     * Verify if a transaction can be accepted within the current state.
     * Calls validateTransactionInternal(), but locks the mutex in a shared manner.
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

    // TODO: remember this function is for testing purposes only,
    // it should probably be removed at some point before definitive release.
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
    Bytes ethCall(const ethCallInfo& callInfo) const;

    // TODO: This function should be considered 'const' as it doesn't change the state,
    // but it is not due to calling non-const contract functions. This should be fixed in the future
    // (even if we call non-const functions, the state is ALWAYS reverted to its original state after the call).
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
     * @throw DynamicException on an attempt to change State while not processing a payable contract.
     */
    void processContractPayable(const std::unordered_map<Address, uint256_t, SafeHash>& payableMap);

    /// Get a list of contract addresses and names.
    std::vector<std::pair<std::string, Address>> getContracts() const;

    /**
     * Get all the events emitted under the given inputs.
     * Parameters are defined when calling "eth_getLogs" on an HTTP request
     * (directly from the http/jsonrpc submodules, through handle_request() on httpparser).
     * They're supposed to be all "optional" at that point, but here they're
     * all required, even if all of them turn out to be empty.
     * @param fromBlock The initial block height to look for.
     * @param toBlock The final block height to look for.
     * @param address The address to look for. Defaults to empty (look for all available addresses).
     * @param topics The topics to filter by. Defaults to empty (look for all available topics).
     * @return A list of matching events.
     */
    std::vector<Event> getEvents(
      const uint64_t& fromBlock, const uint64_t& toBlock,
      const Address& address = Address(), const std::vector<Hash>& topics = {}
    ) const;

    /**
     * Overload of getEvents() for transaction receipts.
     * @param txHash The hash of the transaction to look for events.
     * @param blockIndex The height of the block to look for events.
     * @param txIndex The index of the transaction to look for events.
     * @return A list of matching events.
     */
    std::vector<Event> getEvents(
      const Hash& txHash, const uint64_t& blockIndex, const uint64_t& txIndex
    ) const;

    /// ContractManagerInterface cannot use getNativeBalance, as it will call a lock with the mutex.
    friend class ContractManagerInterface;
};

#endif // STATE_H
