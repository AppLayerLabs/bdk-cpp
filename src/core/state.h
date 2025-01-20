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
class FinalizedBlock;

/// Abstraction of the blockchain's current state at the current block.
class State : public Log::LogicalLocationProvider {
  private:
    Blockchain& blockchain_; ///< Parent Blockchain object

    // Machine state
    mutable std::shared_mutex stateMutex_; ///< Mutex for managing read/write access to the state object.
    uint64_t height_; ///< This is the simulation timestamp the State machine is at.
    uint64_t timeMicros_; ///< This is the wallclock timestamp the State machine is at, in microseconds since epoch.
    evmc_vm* vm_; ///< Pointer to the EVMC VM.
    boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash> contracts_; ///< Map with information about blockchain contracts (Address -> Contract).
    boost::unordered_flat_map<StorageKey, Hash, SafeHash> vmStorage_; ///< Map with the storage of the EVM.
    boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash> accounts_; ///< Map with information about blockchain accounts (Address -> Account).

    /**
     * Doesn't acquire the state mutex.
     */
    bool validateTransactionInternal(const TxBlock& tx) const;

    /**
     * FIXME/TODO:
     * - randomnessHash passed in will be just 0 and we don't have actual support for secure
     *   randomness. we need to implement a commit/reveal protocol between the validators
     *   of a block that runs in parallel generates a secure and signed random number that
     *   is used by processBlock() for that height (i.e. the CometBFT consensus is not
     *   actually aware of this; we just commit the hash of the random block on the next
     *   cometbft block, or maybe in the app_hash since it looks like the app_hash value
     *   goes into the same block that generates that app hash value (that is, the app_hash
     *   that is written in the header of a final/committed block is the hash of the state
     *   *after* processing that block! If so, app_hash is perfect for storing the hash
     *   of the secure, signed result of the random generation protocol for that round.
     *
     * Process a transaction within a block. Called by processNextBlock().
     * If the process fails, any state change that this transaction would cause has to be reverted.
     * @param tx The transaction to process.
     * @param blockHash The hash of the block being processed.
     * @param txIndex The index of the transaction inside the block that is being processed.
     * @param randomnessHash The hash of the previous block's randomness seed.
     * @param succeeded `true` if the transaction succeeded. `false` if it reverted.
     * @param gasUsed Amount of gas used by the transaction.
     */
    void processTransaction(
      const TxBlock& tx, const uint64_t& txIndex, const Hash& blockHash, const Hash& randomnessHash,
      bool& succeeded, uint64_t& gasUsed
    );

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

    std::string getLogicalLocation() const override;

    // ----------------------------------------------------------------------
    // STATE FUNCTIONS
    // ----------------------------------------------------------------------

    uint64_t getHeight() const {
      std::shared_lock<std::shared_mutex> lock(stateMutex_);
      return height_;
    }

    uint64_t getTimeMicros() const {
      std::shared_lock<std::shared_mutex> lock(stateMutex_);
      return timeMicros_;
    }

    /**
     * Get the native balance of an account in the current state.
     * @param addr The address of the account to check.
     * @return The native account balance of the given address.
     */
    uint256_t getNativeBalance(const Address& addr) const;

    /**
     * Get the native nonce of an account in the current state.
     * @param addr The address of the account to check.
     * @return The native account nonce of the given address.
     */
    uint64_t getNativeNonce(const Address& addr) const;

    /**
     * Validate the next block given the current state and its transactions.
     * The block will be rejected if there are invalid transactions in it
     * (e.g. invalid signature, insufficient balance, etc.) or if its height
     * is not exactly the current state machine's height plus one.
     * @param block The block to validate.
     * @return `true` if the block is validated successfully, `false` otherwise.
     */
    bool validateNextBlock(const FinalizedBlock& block) const;

    /**
     * Apply a block to the current machine state (does NOT validate it first).
     * @param block The block to process.
     * @param succeeded Empty outparam vector of tx execution success status.
     * @param gasUsed Empty outparam vector of tx execution gas used.
     * @throw DynamicException on any error.
     */
    void processBlock(const FinalizedBlock& block, std::vector<bool>& succeeded, std::vector<uint64_t>& gasUsed);

    /**
     * Verify if a transaction can be accepted within the current state.
     * @param tx The transaction to verify.
     * @return `true` if the transaction is valid, `false` otherwise.
     */
    bool validateTransaction(const TxBlock& tx) const;

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

    friend class Blockchain;
};

#endif // STATE_H
