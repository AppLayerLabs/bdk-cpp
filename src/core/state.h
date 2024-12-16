/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STATE_H
#define STATE_H

#include <shared_mutex>
#include <boost/unordered/unordered_flat_map.hpp>

#include "../contract/contract.h"

#include "../utils/logger.h"
#include "../utils/safehash.h"

class Blockchain;

/// Abstraction of the blockchain's current state at the current block.
class State : public Log::LogicalLocationProvider {
  protected: // TODO: those shouldn't be protected, plz refactor someday
    Blockchain& blockchain_; ///< Parent Blockchain object
    mutable std::shared_mutex stateMutex_; ///< Mutex for managing read/write access to the state object.
    evmc_vm* vm_; ///< Pointer to the EVMC VM.
    boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash> contracts_; ///< Map with information about blockchain contracts (Address -> Contract).
    boost::unordered_flat_map<StorageKey, Hash, SafeHash> vmStorage_; ///< Map with the storage of the EVM.
    boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash> accounts_; ///< Map with information about blockchain accounts (Address -> Account).
    boost::unordered_flat_map<Hash, TxBlock, SafeHash> mempool_; ///< TxBlock mempool.

    /**
     * Verify if a transaction can be accepted within the current state.
     * @param tx The transaction to check.
     * @return An enum telling if the block is invalid or not.
     */
    // FIXME: transaction validation
    //TxStatus validateTransactionInternal(const TxBlock& tx) const;

    /**
     * Validate the next block given the current state and its transactions. Does NOT update the state.
     * The block will be rejected if there are invalid transactions in it
     * (e.g. invalid signature, insufficient balance, etc.).
     * NOTE: This method does not perform synchronization.
     * @param block The block to validate.
     * @return A status code from BlockValidationStatus.
     */
    // FIXME: block validation
    //BlockValidationStatus validateNextBlockInternal(const FinalizedBlock& block) const;

    /**
     * Process a transaction within a block. Called by processNextBlock().
     * If the process fails, any state change that this transaction would cause has to be reverted.
     * @param tx The transaction to process.
     * @param blockHash The hash of the block being processed.
     * @param txIndex The index of the transaction inside the block that is being processed.
     * @param randomnessHash The hash of the previous block's randomness seed.
     */
    // FIXME: execute a transaction
    //void processTransaction(const TxBlock& tx, const Hash& blockHash, const uint64_t& txIndex, const Hash& randomnessHash);

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
     * @throw DynamicException on any database size mismatch.
     */
    State(Blockchain& blockchain);

    ~State(); ///< Destructor.

    std::string getLogicalLocation() const override { return ""; } // FIXME

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
    // no need
    //std::vector<TxBlock> getMempool() const;

    /**
     * Validate the next block given the current state and its transactions. Does NOT update the state.
     * The block will be rejected if there are invalid transactions in it
     * (e.g. invalid signature, insufficient balance, etc.).
     * @param block The block to validate.
     * @return `true` if the block is validated successfully, `false` otherwise.
     */
    // FIXME
    //bool validateNextBlock(const FinalizedBlock& block) const;

    /**
     * Process the next block given current state from the network. DOES update the state.
     * Appends block to Storage after processing.
     * @param block The block to process.
     * @throw DynamicException if block is invalid.
     */
    // FIXME
    //void processNextBlock(FinalizedBlock&& block);

    /**
     * Process the next block given current state from the network. DOES update the state.
     * Appends block to Storage after processing.
     * Does not throw an exception in case of block validation error.
     * @param block The block to process.
     * @return A status code from BlockValidationStatus.
     */
    // FIXME
    //BlockValidationStatus tryProcessNextBlock(FinalizedBlock&& block);

    /**
     * Verify if a transaction can be accepted within the current state.
     * Calls validateTransactionInternal(), but locks the mutex in a shared manner.
     * @param tx The transaction to verify.
     * @return An enum telling if the transaction is valid or not.
     */
    // FIXME
    //TxStatus validateTransaction(const TxBlock& tx) const;

    /**
     * REMOVE?
     * 
     * Add a transaction to the mempool, if valid.
     * @param tx The transaction to add.
     * @return An enum telling if the transaction is valid or not.
     */
    // FIXME
    //TxStatus addTx(TxBlock&& tx);

    /**
     * REMOVE?
     * 
     * Check if a transaction is in the mempool.
     * @param txHash The transaction hash to check.
     * @return `true` if the transaction is in the mempool, `false` otherwise.
     */
    bool isTxInMempool(const Hash& txHash) const;

    /**
     * REMOVED
     * Use Comet::txCache_ and/or Comet::checkTransaction() instead
     *
     * Get a transaction from the mempool.
     * @param txHash The transaction Hash.
     * @return A pointer to the transaction, or `nullptr` if not found.
     * We cannot directly copy the transaction, since TxBlock doesn't have a
     * default constructor, thus making it impossible to return
     * an "empty" transaction if the hash is not found in the mempool,
     * so we return a null pointer instead.
     */
    //std::unique_ptr<TxBlock> getTxFromMempool(const Hash& txHash) const;

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

    //DBBatch dump() const final; ///< State dumping function.

    /**
     * Get the code section of a given contract.
     * @param addr The address of the contract.
     * @return The code section as a raw bytes string.
     */
    Bytes getContractCode(const Address& addr) const;
};

#endif // STATE_H
