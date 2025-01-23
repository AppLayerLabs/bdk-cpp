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

#include "typedefs.h"

class Blockchain;
class FinalizedBlock;

/// Abstraction of the blockchain's current state at the current block.
// FIXME/TODO: reimplement periodic state saving
class State : /*public Dumpable,*/ public Log::LogicalLocationProvider {
  private:
    Blockchain& blockchain_; ///< Parent Blockchain object

    DumpManager dumpManager_;
    //DumpWorker dumpWorker_; // FIXME

    // Machine state
    mutable std::shared_mutex stateMutex_; ///< Mutex for managing read/write access to the state object.
    uint64_t height_; ///< This is the simulation timestamp the State machine is at.
    uint64_t timeMicros_; ///< This is the wallclock timestamp the State machine is at, in microseconds since epoch.
    evmc_vm* vm_; ///< Pointer to the EVMC VM.
    boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash, SafeCompare> contracts_; ///< Map with information about blockchain contracts (Address -> Contract).
    boost::unordered_flat_map<StorageKey, Hash, SafeHash, SafeCompare> vmStorage_; ///< Map with the storage of the EVM.
    boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash, SafeCompare> accounts_; ///< Map with information about blockchain accounts (Address -> Account).
    MempoolModel mempoolModel_; ///< Mempool model that transparently tracks all transactions seen via validateTransactionInternal().

    /**
     * Helper that cleans up an entry from mempoolModel_.
     */
    void removeTxFromMempoolModel(const TxBlock& tx);

    /**
     * Helper that cleans up an entry from mempoolModel_ (faster).
     */
    void removeTxFromMempoolModel(const TxBlock& tx, MempoolModelIt& fromIt, MempoolModelNonceIt& nonceIt, MempoolModelHashIt& hashIt);

    /**
     * Doesn't acquire the state mutex.
     */
    bool validateTransactionInternal(const TxBlock& tx, bool affectsMempool, MempoolModel *mm = nullptr);

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

    /**
     * FIXME: snapshot saving should always be done in a NEW database dir ("snapshot file")
     * Should not even be named "DB". The database driver is an implementation detail of
     *   the feature, which is a state snapshotting tool. Should be named snapshot or checkpoint,
     *   not "DB".
     * This should be instead named something like:
     *    saveSnapshot()
     * Which is aware of its *current* height, and saves to a new database at that height.
     * Insead of constantly updating a snapshot DB dir whose height number is some
     *   arbitrary height number from the past (from when the node was instantiated.
     * 
     * FIXME/TODO: also implement a loadSnapshot(const uint64_t height = 0) that loads the
     *  saved snapshot at a given height, or if 0, load the latest one.
     * 
     * FIXME/TODO: other snapshot management functions: list, count, getLatestHeight,
     *    delete/prune, check snapshot integrity, etc.
     */
    void saveToDB() const { this->dumpManager_.dumpToDB(); }

    // ----------------------------------------------------------------------
    // STATE FUNCTIONS
    // ----------------------------------------------------------------------

    /**
     * Get the blockchain block height currently reflected by this machine state.
     * @return The current machine logical time (block height); 0 if genesis.
     */
    uint64_t getHeight() const {
      std::shared_lock<std::shared_mutex> lock(stateMutex_);
      return height_;
    }

    /**
     * Get the current wall-clock time associated with the machine state.
     * @return A microseconds timestamp (of the last processed block, or genesis timestamp).
     */
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
     * Add uint256_t("1000000000000000000000") (1,000 eth) tokens to an account.
     * Unit testing helper only; do NOT expose e.g. via RPC, not even for testing.
     * @param addr The account address to fund.
     */
    void addBalance(const Address& addr);

    /**
     * Set the balance of an account.
     * Unit testing helper only; do NOT expose e.g. via RPC, not even for testing.
     * @param addr The account address that will have its balance modified.
     * @param balance The new balance of the account.
     */
    void setBalance(const Address& addr, const uint256_t& balance);

    /**
     * Validate the next block given the current state and its transactions.
     * The block will be rejected if there are invalid transactions in it
     * (e.g. invalid signature, insufficient balance, etc.) or if its height
     * is not exactly the current state machine's height plus one.
     * @param block The block to validate.
     * @return `true` if the block is validated successfully, `false` otherwise.
     */
    bool validateNextBlock(const FinalizedBlock& block);

    /**
     * Apply a block to the current machine state (does NOT validate it first).
     * @param block The block to process.
     * @param succeeded Empty outparam vector of tx execution success status.
     * @param gasUsed Empty outparam vector of tx execution gas used.
     * @throw DynamicException on any error.
     */
    void processBlock(const FinalizedBlock& block, std::vector<bool>& succeeded, std::vector<uint64_t>& gasUsed);

    /**
     * Verify if a transaction can be accepted within the current state and mempool model.
     * If it is found to be valid, it will be added to the internal mempool model kept in RAM,
     * which will allow subsequent transactions from the same account but with the next nonces
     * in the account's nonce sequence to be found valid and included in the mempool as well.
     * Invalid transactions will either cause the mempool model to remove them (if affectsMempool)
     * or flag the tx entry in the mempool model as "ejected" (if !affectsMempool).
     * @param tx The transaction to verify.
     * @param affectsMempool If `true`, this is being called from CometBFT's CheckTx, meaning
     * the transaction will be evicted from the mempool if this method returns `false`. If `false`,
     * this means this method is being called internally and won't affect the CometBFT mempool.
     * @param mm Optional in/out param that, if set, causes validateTransaction() to operate
     * with the given MempoolModel instead.
     * @return `true` if the transaction is valid, `false` otherwise.
     */
    bool validateTransaction(const TxBlock& tx, bool affectsMempool, MempoolModel *mm = nullptr);

    /**
     * Simulate an `eth_call` to a contract.
     * @param callInfo Tuple with info about the call (from, to, gasLimit, gasPrice, value, data).
     * @return The return of the called function as a data string.
     */
    Bytes ethCall(EncodedStaticCallMessage& msg);

    /**
     * Estimate gas for callInfo in RPC.
     * Doesn't really "estimate" gas, but rather tells if the transaction is valid or not.
     * @param callInfo Tuple with info about the call (from, to, gasLimit, gasPrice, value, data).
     * @return The used gas limit of the transaction.
     */
    int64_t estimateGas(EncodedMessageVariant msg);

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
