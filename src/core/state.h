/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STATE_H
#define STATE_H

#include "../contract/contract.h"

#include "rdpos.h" // set, boost/unordered/unordered_flat_map.hpp
#include "dump.h" // utils/db.h, storage.h -> utils/randomgen.h -> utils.h -> logger.h, (strings.h -> evmc/evmc.hpp), (libs/json.hpp -> boost/unordered/unordered_flat_map.hpp)

// TODO: We could possibly change the bool functions into an enum function,
// to be able to properly return each error case. We need this in order to slash invalid rdPoS blocks.

/// Next-block validation status codes.
enum class BlockValidationStatus { valid, invalidWrongHeight, invalidErroneous };

/// Abstraction of the blockchain's current state at the current block.
class State : public Dumpable, public Log::LogicalLocationProvider {
  protected: // TODO: those shouldn't be protected, plz refactor someday
    mutable std::shared_mutex stateMutex_;  ///< Mutex for managing read/write access to the state object.
    evmc_vm* vm_;  ///< Pointer to the EVMC VM.
    const Options& options_;  ///< Reference to the options singleton.
    Storage& storage_;  ///< Reference to the blockchain's storage.
    DumpManager dumpManager_; ///< The Dump Worker object
    DumpWorker dumpWorker_; ///< Dump Manager object
    P2P::ManagerNormal& p2pManager_;  ///< Reference to the P2P connection manager.
    rdPoS rdpos_; ///< rdPoS object (consensus).
    boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash> contracts_; ///< Map with information about blockchain contracts (Address -> Contract).
    boost::unordered_flat_map<StorageKey, Hash, SafeHash> vmStorage_; ///< Map with the storage of the EVM.
    boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash> accounts_; ///< Map with information about blockchain accounts (Address -> Account).
    boost::unordered_flat_map<Hash, TxBlock, SafeHash> mempool_; ///< TxBlock mempool.

    /**
     * Verify if a transaction can be accepted within the current state.
     * @param tx The transaction to check.
     * @return An enum telling if the block is invalid or not.
     */
    TxStatus validateTransactionInternal(const TxBlock& tx) const;

    /**
     * Validate the next block given the current state and its transactions. Does NOT update the state.
     * The block will be rejected if there are invalid transactions in it
     * (e.g. invalid signature, insufficient balance, etc.).
     * NOTE: This method does not perform synchronization.
     * @param block The block to validate.
     * @return A status code from BlockValidationStatus.
     */
    BlockValidationStatus validateNextBlockInternal(const FinalizedBlock& block) const;

    /**
     * Process a transaction within a block. Called by processNextBlock().
     * If the process fails, any state change that this transaction would cause has to be reverted.
     * @param tx The transaction to process.
     * @param blockHash The hash of the block being processed.
     * @param txIndex The index of the transaction inside the block that is being processed.
     * @param randomnessHash The hash of the previous block's randomness seed.
     */
    void processTransaction(const TxBlock& tx, const Hash& blockHash, const uint64_t& txIndex, const Hash& randomnessHash);

    /**
     * Update the mempool, remove transactions that are in the given block, and leave only valid transactions in it.
     * Called by processNewBlock(), used to filter the current mempool based on transactions that have been
     * accepted on the block, and verify if transactions on the mempool are valid given the new state after
     * processing the block itself.
     * @param block The block to use for pruning transactions from the mempool.
     */
    void refreshMempool(const FinalizedBlock& block);

    /**
     * Helper function that does a sanity check on all contracts in the accounts_ map.
     * Used exclusively by the constructor.
     * @param addr The address of the contract.
     * @param acc The account tied to the contract.
     * @throw DynamicException if any contract does not have code, or if an
     *        address that isn't a contract has any code.
     */
    void contractSanityCheck(const Address& addr, const Account& acc);

  public:
    /**
     * Constructor.
     * @param db Pointer to the database.
     * @param storage Pointer to the blockchain's storage.
     * @param p2pManager Pointer to the P2P connection manager.
     * @param snapshotHeight The block height to start from.
     * @param options Pointer to the options singleton.
     * @throw DynamicException on any database size mismatch.
     */
    State(const DB& db, Storage& storage, P2P::ManagerNormal& p2pManager, const uint64_t& snapshotHeight, const Options& options);

    ~State(); ///< Destructor.

    std::string getLogicalLocation() const override { return p2pManager_.getLogicalLocation(); } ///< Log instance from P2P

    // ----------------------------------------------------------------------
    // RDPOS WRAPPER FUNCTIONS
    // ----------------------------------------------------------------------

    ///@{
    /** Wrapper for the respective rdPoS function. */
    std::set<Validator> rdposGetValidators() const { std::shared_lock lock(this->stateMutex_); return this->rdpos_.getValidators(); }
    std::vector<Validator> rdposGetRandomList() const { std::shared_lock lock(this->stateMutex_); return this->rdpos_.getRandomList(); }
    size_t rdposGetMempoolSize() const { std::shared_lock lock(this->stateMutex_); return this->rdpos_.getMempoolSize(); }
    const boost::unordered_flat_map<Hash, TxValidator, SafeHash> rdposGetMempool() const { std::shared_lock lock(this->stateMutex_); return this->rdpos_.getMempool(); }
    const Hash& rdposGetBestRandomSeed() const { std::shared_lock lock(this->stateMutex_); return this->rdpos_.getBestRandomSeed(); }
    bool rdposGetIsValidator() const { std::shared_lock lock(this->stateMutex_); return this->rdpos_.getIsValidator(); }
    const uint32_t& rdposGetMinValidators() const { std::shared_lock lock(this->stateMutex_); return this->rdpos_.getMinValidators(); }
    void rdposClearMempool() { std::unique_lock lock(this->stateMutex_); return this->rdpos_.clearMempool(); }
    bool rdposValidateBlock(const FinalizedBlock& block) const { std::shared_lock lock(this->stateMutex_); return this->rdpos_.validateBlock(block); }
    Hash rdposProcessBlock(const FinalizedBlock& block) { std::unique_lock lock(this->stateMutex_); return this->rdpos_.processBlock(block); }
    TxStatus rdposAddValidatorTx(const TxValidator& tx) { std::unique_lock lock(this->stateMutex_); return this->rdpos_.addValidatorTx(tx); }
    void dumpStartWorker() { this->dumpWorker_.startWorker(); }
    void dumpStopWorker() { this->dumpWorker_.stopWorker(); }
    size_t getDumpManagerSize() const { std::shared_lock lock(this->stateMutex_); return this->dumpManager_.size(); }
    // Returns the block height of the dump and the time it took to serialize and dump to DB.
    std::tuple<uint64_t, uint64_t, uint64_t> saveToDB() const { return this->dumpManager_.dumpToDB(); }
    ///@}

    // ----------------------------------------------------------------------
    // STATE FUNCTIONS
    // ----------------------------------------------------------------------

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

    /**
     * Get a copy of the mempool (as a vector).
     * @return A vector with all transactions in the mempool.
     */
    std::vector<TxBlock> getMempool() const;

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
    bool validateNextBlock(const FinalizedBlock& block) const;

    /**
     * Process the next block given current state from the network. DOES update the state.
     * Appends block to Storage after processing.
     * @param block The block to process.
     * @throw DynamicException if block is invalid.
     */
    void processNextBlock(FinalizedBlock&& block);

    /**
     * Process the next block given current state from the network. DOES update the state.
     * Appends block to Storage after processing.
     * Does not throw an exception in case of block validation error.
     * @param block The block to process.
     * @return A status code from BlockValidationStatus.
     */
    BlockValidationStatus tryProcessNextBlock(FinalizedBlock&& block);

    /**
     * Verify if a transaction can be accepted within the current state.
     * Calls validateTransactionInternal(), but locks the mutex in a shared manner.
     * @param tx The transaction to verify.
     * @return An enum telling if the transaction is valid or not.
     */
    TxStatus validateTransaction(const TxBlock& tx) const;

    /**
     * Add a transaction to the mempool, if valid.
     * @param tx The transaction to add.
     * @return An enum telling if the transaction is valid or not.
     */
    TxStatus addTx(TxBlock&& tx);

    /**
     * Add a Validator transaction to the rdPoS mempool, if valid.
     * @param tx The transaction to add.
     * @return `true` if transaction is valid, `false` otherwise.
     */
    TxStatus addValidatorTx(const TxValidator& tx);

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
    Bytes ethCall(const evmc_message& callInfo);

    /**
     * Estimate gas for callInfo in RPC.
     * Doesn't really "estimate" gas, but rather tells if the transaction is valid or not.
     * @param callInfo Tuple with info about the call (from, to, gasLimit, gasPrice, value, data).
     * @return The used gas limit of the transaction.
     */
    int64_t estimateGas(const evmc_message& callInfo);

    /// Get a list of the C++ contract addresses and names.
    std::vector<std::pair<std::string, Address>> getCppContracts() const;

    /// Get a list of Addresss which are EVM contracts.
    std::vector<Address> getEvmContracts() const;

    DBBatch dump() const final; ///< State dumping function.

    /// Get the pending transactions from the mempool.
    auto getPendingTxs() const {
      std::shared_lock lock(this->stateMutex_);
      return this->mempool_;
    }

    /**
     * Get the code section of a given contract.
     * @param addr The address of the contract.
     * @return The code section as a raw bytes string.
     */
    Bytes getContractCode(const Address& addr) const;
};

#endif // STATE_H
