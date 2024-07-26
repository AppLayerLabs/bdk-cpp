/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef RDPOS_H
#define RDPOS_H

#include "../contract/contract.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/safehash.h"
#include "../utils/randomgen.h"
#include "../utils/options.h"
#include "../net/p2p/managernormal.h"

#include <optional>
#include <shared_mutex>
#include <set>
#include <boost/unordered/unordered_flat_map.hpp>

// Forward declarations.
class rdPoS;
class Storage;
class State;

// "0x6fc5a2d6" -> Function for random tx
// "0xcfffe746" -> Function for random hash tx

/// Enum for labeling transaction status.
enum class TxStatus {
  ValidNew,
  ValidExisting,
  InvalidNonce,       // Tx only
  InvalidBalance,     // Tx only
  InvalidUnexpected,  // ValidatorTx only
  InvalidDuplicate,   // ValidatorTx only
  InvalidRedundant    // ValidatorTx only
};

inline bool isTxStatusValid(const TxStatus& txStatus) { return txStatus <= TxStatus::ValidExisting; }

/**
 * Abstraction of a validator, same as Address but different type.
 * Responsible for creating/signing/validating blocks.
 */
class Validator : public Address {
  public:
    /// Constructor.
    Validator(const Address& add) : Address(add) {}

    /// Getter for the address.
    Address address() const { return Address(*this); }
};

/// Abstraction of the %rdPoS (Random Deterministic Proof of Stake) consensus algorithm.
class rdPoS : public BaseContract, public Log::LogicalLocationProvider {
  private:
    const Options& options_;  ///< Reference to the options singleton.
    const Storage& storage_;  ///< Reference to the blockchain's storage.
    P2P::ManagerNormal& p2p_; ///< Reference to the P2P Manager (for sending/requesting TxValidators from other nodes).
    std::set<Validator> validators_;  ///< Ordered list of rdPoS validators.
    std::vector<Validator> randomList_; ///< Shuffled version of the validator list, used at block creation/signing.
    boost::unordered_flat_map<Hash, TxValidator, SafeHash> validatorMempool_;  ///< Mempool for validator transactions.
    const PrivKey validatorKey_;  ///< Private key for operating a validator.
    const bool isValidator_ = false;  ///< Indicates whether node is a Validator.
    RandomGen randomGen_; ///< Randomness generator (for use in seeding).
    Hash bestRandomSeed_; ///< Best randomness seed (taken from the last block).
    const uint32_t minValidators_; ///< Minimum required number of Validators for creating and signing blocks.

    /**
     * Helper function that does a sanity check on a given pair of Validator txs.
     * Used exclusively by validateBlock().
     * @param hashTx A Validator's hash tx.
     * @param seedTx A Validator's seed tx.
     * @return `true` if tx data is valid, `false` otherwise.
     */
    bool validateBlockTxSanityCheck(const TxValidator& hashTx, const TxValidator& seedTx) const;
    
  public:
    /// Enum for Validator transaction functions.
    enum class TxValidatorFunction { INVALID, RANDOMHASH, RANDOMSEED };

    /// Enum for transaction types.
    enum class TxType { addValidator, removeValidator, randomHash, randomSeed };

    /**
     * Constructor.
     * @param db Reference to the database.
     * @param storage Reference to the blockchain's storage.
     * @param p2p Reference to the P2P connection manager.
     * @param options Reference to the options singleton.
     * @throw DynamicException if there are no Validators registered in the database.
     */
    rdPoS(const DB& db, DumpManager& manager, const Storage& storage, P2P::ManagerNormal& p2p, const Options& options);

    ~rdPoS() override;  ///< Destructor.

    std::string getLogicalLocation() const override { return p2p_.getLogicalLocation(); } ///< Log instance from P2P

    ///@{
    /** Getter. */
    std::set<Validator> getValidators() const { return this->validators_; }
    std::vector<Validator> getRandomList() const { return this->randomList_; }
    const boost::unordered_flat_map<Hash, TxValidator, SafeHash> getMempool() const { return this->validatorMempool_; }
    size_t getMempoolSize() const { return this->validatorMempool_.size(); }
    const Hash& getBestRandomSeed() const { return this->bestRandomSeed_; }
    bool getIsValidator() const { return this->isValidator_; }
    UPubKey getValidatorUPubKey() const { return Secp256k1::toUPub(this->validatorKey_); }
    const uint32_t& getMinValidators() const { return this->minValidators_; }
    ///@}

    /**
     * Check if a given Address is a Validator.
     * @param add The address to check.
     * @return `true` if address is in the Validator list, `false` otherwise.
     */
    bool isValidatorAddress(const Address& add) const { return validators_.contains(Validator(add)); }

    /**
     * Validate a block.
     * @param block The block to validate.
     * @return `true` if the block is properly validated, `false` otherwise.
     */
    bool validateBlock(const FinalizedBlock& block) const;

    /**
     * Process a block. Should be called from State, after a block is validated but before it is added to Storage.
     * @param block The block to process.
     * @return The new randomness seed to be used for the next block.
     * @throw DynamicException if block is not finalized.
     */
    Hash processBlock(const FinalizedBlock& block);

    /**
     * Add a Validator transaction to the mempool.
     * Should ONLY be called by the State, as it locks the current state mutex,
     * not allowing a race condition of adding transactions that are not for the current block height.
     * @param tx The transaction to add.
     * @return `true` if the transaction was added, `false` if invalid otherwise.
     */
    TxStatus addValidatorTx(const TxValidator& tx);

    /**
     * Parse a Validator transaction list.
     * Does NOT validate any of the block rdPoS transactions.
     * @param txs The list of transactions to parse.
     * @return The new randomness of given transaction set.
     */
    static Hash parseTxSeedList(const std::vector<TxValidator>& txs);

    /**
     * Get a function from a given Validator transaction, based on ABI.
     * @param tx The transaction to parse.
     * @return The function type.
     */
    static TxValidatorFunction getTxValidatorFunction(const TxValidator& tx);

    /**
     * Clear the mempool
     * Used by tests
     */
    void clearMempool() { this->validatorMempool_.clear(); }

    /// Dump overriden function.
    DBBatch dump() const override;
};

#endif // RDPOS_H
