/*
Copyright (c) [2023-2024] [Sparq Network]

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

// Forward declarations.
class rdPoS;
class Storage;
class Block;
class State;

// "0x6fc5a2d6" -> Function for random tx
// "0xcfffe746" -> Function for random hash tx

/**
 * Abstraction of a validator, same as Address but different type.
 * Responsible for creating/signing/validating blocks.
 */
class Validator : public Address {
  public:
    /// Constructor.
    Validator(const Address& add) : Address(add) {}

    /// Copy constructor.
    Validator(const Validator& other) : Address(other.data_) {}

    /// Getter for the address.
    Address address() const { return Address(this->data_); }

    /// Copy assignment operator.
    Validator& operator=(const Validator& other) {
      this->data_ = other.data_;
      return *this;
    }
};

/// Worker class for rdPoS. This separates the class from the %rdPoS operation which runs the %rdPoS consensus.
class rdPoSWorker {
  private:
    /// Reference to the parent rdPoS object.
    rdPoS& rdpos_;

    /// Flag for stopping the worker thread.
    std::atomic<bool> stopWorker_ = false;

    /**
     * Future object for the worker thread.
     * Used to wait for the thread to finish after stopWorker_ is set to true.
     */
    std::future<bool> workerFuture_;

    /// Flag for knowing if the worker is ready to create a block.
    std::atomic<bool> canCreateBlock_ = false;

    /// Pointer to the latest block.
    std::shared_ptr<const Block> latestBlock_;

    /**
     * Check if the latest block has updated.
     * Does NOT update the latest block per se, this is done by workerLoop().
     * @return `true` if the latest block has been updated, `false` otherwise.
     */
    bool checkLatestBlock();

    /**
     * Entry function for the worker thread (runs the workerLoop() function).
     * @return `true` when done running.
     */
    bool workerLoop();

    /**
     * Wait for transactions to be added to the mempool and create a block by rdPoS consesus. Called by workerLoop().
     * TODO: this function should call State or Blockchain to let them know that we are ready to create a block.
     */
    void doBlockCreation();

    /**
     * Create a transaction by rdPoS consensus and broadcast it to the network.
     * @param nHeight The block height for the transaction.
     * @param me The Validator that will create the transaction.
     */
    void doTxCreation(const uint64_t& nHeight, const Validator& me);

  public:
    /**
     * Constructor.
     * @param rdpos Reference to the parent rdPoS object.
     */
    explicit rdPoSWorker(rdPoS& rdpos) : rdpos_(rdpos) {}

    /// Destructor. Automatically stops the worker thread if it's still running.
    ~rdPoSWorker() { this->stop(); }

    /// Getter for `canCreateBlock_`.
    const std::atomic<bool>& getCanCreateBlock() const { return this->canCreateBlock_; }

    /// Setter for `canCreateBlock_`.
    void blockCreated() { this->canCreateBlock_ = false; }

    void start(); ///< Start `workerFuture_` and `workerLoop()`. Should only be called after node is synced.
    void stop();  ///< Stop `workerFuture_` and `workerLoop()`.
};

/// Abstraction of the %rdPoS (Random Deterministic Proof of Stake) consensus algorithm.
class rdPoS : public BaseContract {
  private:
    const Options& options_;  ///< Reference to the options singleton.
    const Storage& storage_;  ///< Reference to the blockchain's storage.
    P2P::ManagerNormal& p2p_; ///< Reference to the P2P Manager (for sending/requesting TxValidators from other nodes).
    State& state_;  ///< Reference to the blockchain state.
    rdPoSWorker worker_;  ///< Worker object.
    std::set<Validator> validators_;  ///< Ordered list of rdPoS validators.
    std::vector<Validator> randomList_; ///< Shuffled version of the validator list, used at block creation/signing.
    std::unordered_map<Hash, TxValidator, SafeHash> validatorMempool_;  ///< Mempool for validator transactions.
    const PrivKey validatorKey_;  ///< Private key for operating a validator.
    const bool isValidator_ = false;  ///< Indicates whether node is a Validator.
    RandomGen randomGen_; ///< Randomness generator (for use in seeding).
    Hash bestRandomSeed_; ///< Best randomness seed (taken from the last block).
    mutable std::shared_mutex mutex_; ///< Mutex for managing read/write access to the class members.

    /**
     * Initializes the blockchain with the default information for rdPoS.
     * Called by the constructor if no previous blockchain is found.
     */
    void initializeBlockchain() const;

  public:
    /// Enum for Validator transaction functions.
    enum TxValidatorFunction { INVALID, RANDOMHASH, RANDOMSEED };

    /// Enum for transaction types.
    enum TxType { addValidator, removeValidator, randomHash, randomSeed };

    /// Minimum number of required Validators for creating and signing blocks.
    static const uint32_t minValidators = 4;

    /**
     * Constructor.
     * @param db Reference to the database.
     * @param storage Reference to the blockchain's storage.
     * @param p2p Reference to the P2P connection manager.
     * @param options Reference to the options singleton.
     * @param state Reference to the blockchain's state.
     * @throw DynamicException if there are no Validators registered in the database.
     */
    rdPoS(DB& db, const Storage& storage, P2P::ManagerNormal& p2p, const Options& options, State& state);

    ~rdPoS() override;  ///< Destructor.

    ///@{
    /** Getter. */
    const std::set<Validator>& getValidators() const {
      std::shared_lock lock(this->mutex_); return this->validators_;
    }
    const std::vector<Validator>& getRandomList() const {
      std::shared_lock lock(this->mutex_); return this->randomList_;
    }
    const std::unordered_map<Hash, TxValidator, SafeHash> getMempool() const {
      // Return is NOT a reference because the inner map can be changed.
      std::shared_lock lock(this->mutex_); return this->validatorMempool_;
    }
    const Hash& getBestRandomSeed() const { std::shared_lock lock(this->mutex_); return this->bestRandomSeed_; }
    bool getIsValidator() const { return this->isValidator_; }
    UPubKey getValidatorUPubKey() const { return Secp256k1::toUPub(this->validatorKey_); }
    ///@}

    /**
     * Check if a given Address is a Validator.
     * @param add The address to check.
     * @return `true` if address is in the Validator list, `false` otherwise.
     */
    bool isValidatorAddress(const Address& add) const {
      std::shared_lock lock(this->mutex_); return validators_.contains(Validator(add));
    }

    /// Clear the mempool.
    void clearMempool() { std::unique_lock lock(this->mutex_); this->validatorMempool_.clear(); }

    /**
     * Validate a block.
     * @param block The block to validate.
     * @return `true` if the block is properly validated, `false` otherwise.
     */
    bool validateBlock(const Block& block) const;

    /**
     * Process a block. Should be called from State, after a block is validated but before it is added to Storage.
     * @param block The block to process.
     * @return The new randomness seed to be used for the next block.
     * @throw DynamicException if block is not finalized.
     */
    Hash processBlock(const Block& block);

    /**
     * Sign a block using the Validator's private key.
     * @param block The block to sign.
     */
    void signBlock(Block& block);

    /**
     * Add a Validator transaction to the mempool.
     * Should ONLY be called by the State, as it locks the current state mutex,
     * not allowing a race condition of adding transactions that are not for the current block height.
     * @param tx The transaction to add.
     * @return `true` if the transaction was added, `false` if invalid otherwise.
     */
    bool addValidatorTx(const TxValidator& tx);

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

    const std::atomic<bool>& canCreateBlock() const;  ///< Check if a block can be created by rdPoSWorker.
    void startrdPoSWorker();  ///< Start the rdPoSWorker.
    void stoprdPoSWorker(); ///< Stop the rdPoSWorker.
    friend rdPoSWorker; ///< Worker class is a friend.
};

#endif // RDPOS_H
