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
#include "comet.h"

/// Mempool model to help validate multiple txs with same from account and various nonce values.
using MempoolModel =
  std::unordered_map<
    Address, // From Address -->
    std::map<
      uint64_t, // Nonce (of from address) -->
      std::unordered_map<
        Hash, // Transaction hash (for a from address and nonce) -->
        std::pair<
          uint256_t, // Minimum balance (max fee) required by this transaction.
          bool // Eject? `true` if tx should be removed from the mempool on next CheckTx().
        >
      , SafeHash>
    >
  , SafeHash>;

using MempoolModelIt = MempoolModel::iterator;

using MempoolModelNonceIt =
  std::map<
    uint64_t,
    std::unordered_map<
      Hash,
      std::pair<
        uint256_t,
        bool>
      , SafeHash>
  , SafeHash>::iterator;

using MempoolModelHashIt =
  std::unordered_map<
    Hash,
    std::pair<
      uint256_t,
      bool
    >
  , SafeHash>::iterator;

#if defined(BUILD_TESTS) && defined(BUILD_BENCHMARK_TESTS)
// Forward declaration. Used only for benchmarking purposes.
class SDKTestSuite;
#endif
class Blockchain;
class FinalizedBlock;
class SystemContract;

/// A CometBFT validator set that becomes active at a given height
class ValidatorSet {
  private:
    uint64_t height_; ///< Height of the first block that this validator set actually votes on (is active, in effect).
    std::vector<CometValidatorUpdate> validators_; ///< A CometBFT validator set.
    std::unordered_map<Address, uint64_t, SafeHash> validatorAddrs_; /// Map of CometBFT validator address to index in validators_.
  public:
    uint64_t& getHeight() { return height_; }
    const std::vector<CometValidatorUpdate>& getValidators() { return validators_; }
    Address validatorCometAddressToEthAddress(const Address& validatorCometAddress) {
      auto it = validatorAddrs_.find(validatorCometAddress);
      if (it == validatorAddrs_.end()) {
        return {};
      }
      const uint64_t& validatorIndex = it->second;
      if (validatorIndex >= validators_.size()) {
        throw DynamicException("Blockchain::validatorCometAddressToEthAddress() returned an index not in validators_.");
      }
      const CometValidatorUpdate& v = validators_[validatorIndex];
      PubKey pubKey(v.publicKey); // Compressed key (33 bytes)
      return Secp256k1::toAddress(pubKey); // Generate Eth address from validator pub key
    }
    ValidatorSet(const uint64_t& height, const std::vector<CometValidatorUpdate>& validators)
      : height_(height), validators_(validators)
    {
      for (int i = 0; i < validators_.size(); ++i) {
        const CometValidatorUpdate& v = validators_[i];
        Bytes cometAddrBytes = Comet::getCometAddressFromPubKey(v.publicKey);
        Address cometAddr(cometAddrBytes);
        validatorAddrs_[cometAddr] = i;
      }
    }
};

/// Abstraction of the blockchain's current state at the current block.
class State : public Log::LogicalLocationProvider {
  private:
    Blockchain& blockchain_; ///< Parent Blockchain object
    MempoolModel mempoolModel_; ///< Mempool model that transparently tracks all transactions seen via validateTransactionInternal().
    static std::once_flag stateRegisterContractsFlag; ///< Ensures contract registration happens only once

    // Binary state (dynamic infrastructure for the machine side that is independent of a blockchain)
    CreateContractFuncsType createContractFuncs_; ///< Functions to create contracts.

    // Machine state
    mutable std::shared_mutex stateMutex_; ///< Mutex for managing read/write access to the state object.
    uint64_t height_; ///< This is the simulation timestamp the State machine is at.
    uint64_t timeMicros_; ///< This is the wallclock timestamp the State machine is at, in microseconds since epoch.
    evmc_vm* vm_; ///< Pointer to the EVMC VM.
    ContractsContainerType contracts_; ///< Map with information about blockchain contracts (Address -> Contract).
    boost::unordered_flat_map<StorageKey, Hash, SafeHash, SafeCompare> vmStorage_; ///< Map with the storage of the EVM.
    boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash, SafeCompare> accounts_; ///< Map with information about blockchain accounts (Address -> Account).
    int currentValidatorSet_ = -1; ///< Index in validatorSets_ of the currently active validator set, -1 means none.
    std::deque<ValidatorSet> validatorSets_; ///< More recent set in front, oldest in back.

    /**
     * A new validator set is elected in the governance contract, so update the State with it.
     * @param newValidatorSet Validator set that will become active in current height + 2 or, if we are in
     * genesis state, the genesis validator set that becomes the first validator set (instantly active at
     * starting height).
     */
    void setValidators(const std::vector<CometValidatorUpdate>& newValidatorSet);

    /**
     * Helper for static contract registration step.
     */
    static void registerContracts();

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

    /**
     * Get the system contract. Does not acquire the stateMutex_.
     * @return Pointer to SystemContract, or nullptr if SystemContract not instantiated (no initChain() or loadSnapshot() yet).
     */
    SystemContract* getSystemContractInternal();

    #if defined(BUILD_TESTS) && defined(BUILD_BENCHMARK_TESTS)
    // Process a transaction directly without having to put it in a block.
    // FOR TESTING PURPOSES ONLY. DO NOT COMPILE FOR PRODUCTION
    void benchCall(const TxBlock& tx);
    friend class SDKTestSuite;  // It's the only class that uses this function
    #endif

  public:
    State(Blockchain& blockchain); ///< Constructor.

    ~State(); ///< Destructor.

    std::string getLogicalLocation() const override; ///< Log helper.

    /**
     * Get the validator set that is currently in effect.
     * @param validatorSet Outparam with the validator set in effect.
     * @param height Outparam with the block height in which this validator set took effect
     * (past or present height) or will take effect (future height).
     */
    void getValidatorSet(std::vector<CometValidatorUpdate>& validatorSet, uint64_t& height);

    /**
     * Given a CometBFT validator address, which is backed by a secp256k1 validator
     * private key (due to how the Comet driver configures CometBFT to use secp256k1
     * keys), look up the current validator list, find the validator private key given
     * the CometBFT validator address, then generate an Eth validator address from
     * that found private key.
     * @param validatorCometAddress The CometBFT address of one of the active validators.
     * @return The translation of the CometBFT address into the corresponding Eth address.
     */
    Address validatorCometAddressToEthAddress(Address validatorCometAddress);

    /**
     * Get the number of user-deployed contracts.
     * @return Number of user-deployed contracts.
     */
    size_t getUserContractsSize();

    /**
     * Get the system contract.
     * @return Pointer to SystemContract, or nullptr if SystemContract not instantiated (no initChain() or loadSnapshot() yet).
     */
    SystemContract* getSystemContract();

    /**
     * Resets the machine state to its default, bare-minimum state.
     * This wipes accounts_, contracts_, vmStorage_ and then inserts the
     * ContractManager in accounts_ and contracts_.
     * @param height Value to initialize height to (default: 0).
     * @param timeMicros Value to initialize timeMicros to (default: 0).
     */
    void resetState(uint64_t height = 0, uint64_t timeMicros = 0);

    /**
     * Initializes the state to genesis state.
     * Genesis state is known only after the ABCI InitChain call.
     * @param initialHeight Genesis height (0 is the default start-from-scratch, no-blocks value).
     * @param initialTimeEpochSeconds Genesis timestamp in seconds since epoch.
     * @param initialValidators Validator set at genesis.
     * @param genesisSnapshot Optional snapshot directory location with genesis state to load ("" if none).
     */
    void initChain(
      uint64_t initialHeight, uint64_t initialTimeEpochSeconds,
      const std::vector<CometValidatorUpdate>& initialValidators,
      std::string genesisSnapshot = ""
    );

    /**
     * Write the entire consensus machine state held in RAM to persistent storage.
     * May throw on errors.
     * @param where New speedb directory name the snapshot will be written to.
     */
    void saveSnapshot(const std::string& where);

    /**
     * Read the entire consensus machine state held in persistent storage to RAM.
     * May throw on errors.
     * @param where Existing speedb directory name the snapshot will be read from.
     * @param genesisSnapshot `true` if loading a genesis snapshot, `false` otherwise.
     */
    void loadSnapshot(const std::string& where, bool genesisSnapshot = false);

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
     * Set the balance of an account.
     * Unit testing helper only; do NOT expose e.g. via RPC, not even for testing.
     * @param addr The account address that will have its balance modified.
     * @param balance The new balance of the account.
     */
    void setBalance(const Address& addr, const uint256_t& balance);

    /**
     * Apply a block to the current machine state (does NOT validate it first).
     * @param block The block to process.
     * @param succeeded Empty outparam vector of tx execution success status.
     * @param gasUsed Empty outparam vector of tx execution gas used.
     * @param validatorUpdates Empty outparam filled in with (optional) validator updates for CometBFT.
     * @throw DynamicException on any error.
     */
    void processBlock(
      const FinalizedBlock& block, std::vector<bool>& succeeded, std::vector<uint64_t>& gasUsed,
      std::vector<CometValidatorUpdate>& validatorUpdates
    );

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

    /**
     * Get the code section of a given contract.
     * @param addr The address of the contract.
     * @return The code section as a raw bytes string.
     */
    Bytes getContractCode(const Address& addr) const;

    friend class Blockchain;
};

#endif // STATE_H
