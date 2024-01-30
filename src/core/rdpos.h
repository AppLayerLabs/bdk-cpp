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
class rdPoSWorker;
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
    Validator(const Validator& other) : Address(other.data_)  {}

    /**
     * Getter for the address.
     * @return The address.
     */
    const Address address() const { return Address(this->data_); }

    /// Copy assignment operator.
    Validator& operator=(const Validator& other) {
      this->data_ = other.data_;
      return *this;
    }
};

/// Abstraction of the %rdPoS (Random Deterministic Proof of Stake) consensus algorithm.
class rdPoS : public BaseContract {
  private:
    /// Pointer to the blockchain's storage.
    const std::unique_ptr<Storage>& storage_;

    /// Pointer to the P2P Manager (for sending/requesting TxValidators from other nodes).
    const std::unique_ptr<P2P::ManagerNormal>& p2p_;

    /// Pointer to the blockchain state.
    const std::unique_ptr<State>& state_;

    /// Pointer to the options singleton.
    const std::unique_ptr<Options>& options_;

    /// Pointer to the worker object.
    const std::unique_ptr<rdPoSWorker> worker_;

    /// Ordered list of rdPoS validators.
    std::set<Validator> validators_;

    /// Shuffled version of the validator list, used at block creation/signing.
    std::vector<Validator> randomList_;

    /// Mempool for validator transactions.
    std::unordered_map<Hash, TxValidator, SafeHash> validatorMempool_;

    /// Private key for operating a validator.
    const PrivKey validatorKey_;

    /// Indicated whether this node is a Validator or not.
    const bool isValidator_ = false;

    /// Randomness generator (for use in seeding).
    RandomGen randomGen_;

    /// Best randomness seed (taken from the last block).
    Hash bestRandomSeed_;

    /// Mutex for managing read/write access to the class members.
    mutable std::shared_mutex mutex_;

    /**
     * Initializes the blockchain with the default information for rdPoS.
     * Called by the constructor if no previous blockchain is found.
     */
    void initializeBlockchain();

  public:
    /// Enum for Validator transaction functions.
    enum TxValidatorFunction { INVALID, RANDOMHASH, RANDOMSEED };

    /**
     * Constructor.
     * @param db Pointer to the database.
     * @param storage Pointer to the blockchain's storage.
     * @param p2p Pointer to the P2P connection manager.
     * @param options Pointer to the options singleton.
     * @param state Pointer to the blockchain's state.
     * @throw std::runtime_error if there are no Validators registered in the database.
     */
    rdPoS(
      const std::unique_ptr<DB>& db, const std::unique_ptr<Storage>& storage,
      const std::unique_ptr<P2P::ManagerNormal>& p2p,
      const std::unique_ptr<Options>& options, const std::unique_ptr<State>& state
    );

    /// Destructor.
    ~rdPoS() override;

    /// Enum for transaction types.
    enum TxType { addValidator, removeValidator, randomHash, randomSeed };

    /// Minimum number of required Validators for creating and signing blocks.
    static const uint32_t minValidators = 4;

    /// Getter for `validators`. Not a reference because the inner set can be changed.
    const std::set<Validator> getValidators() const { std::shared_lock lock(this->mutex_); return validators_; }

    /// Getter for `randomList`. Not a reference because the inner vector can be changed.
    const std::vector<Validator> getRandomList() const { std::shared_lock lock(this->mutex_); return randomList_; }

    /// Getter for `mempool`. Not a reference because the inner map can be changed.
    const std::unordered_map<Hash, TxValidator, SafeHash> getMempool() const { std::shared_lock lock(this->mutex_); return validatorMempool_; }

    /// Getter for `bestRandomSeed`.
    const Hash getBestRandomSeed() const { std::shared_lock lock(this->mutex_); return this->bestRandomSeed_; }

    /// Getter for `isValidator`.
    const bool getIsValidator() const { return this->isValidator_; }

    /// Getter for `validatorKey`, converted to an uncompressed public key.
    const UPubKey getValidatorUPubKey() const { return Secp256k1::toUPub(this->validatorKey_); }

    /**
     * Check if a given Address is a Validator.
     * @param add The address to check.
     * @return `true` if address is in the validator list, `false` otherwise.
     */
    const bool isValidatorAddress(const Address& add) const { std::shared_lock lock(this->mutex_); return validators_.contains(Validator(add)); }

    /// Clear the mempool.
    void clearMempool() { std::unique_lock lock(this->mutex_); this->validatorMempool_.clear(); }

    /**
     * Validate a block.
     * @param block The block to validate.
     * @return `true` if the block is properly validated, `false` otherwise.
     */
    bool validateBlock(const Block& block) const;

    /**
     * Process a block.
     * Should be called from State, after a block is validated and before it is added to Storage.
     * @param block The block to process.
     * @return The new randomness seed to be used for the next block.
     * @throw std::runtime_error if block is not finalized.
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
     * not allowing a race condition of adding transactions that are not for
     * the current block height.
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

    /**
     * Check if a block can be created by rdPoSWorker.
     * @return `true` if a block can be created, `false` otherwise.
     */
    const std::atomic<bool>& canCreateBlock() const;

    /// Start the rdPoSWorker.
    void startrdPoSWorker();

    /// Stop the rdPoSWorker.
    void stoprdPoSWorker();

    /// Worker class is a friend.
    friend rdPoSWorker;
};

/**
 * Worker class for rdPoS.
 * This separates the class from the %rdPoS operation which runs the %rdPoS consensus.
 */
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
     * Does NOT update latestBlock per se, this is done by workerLoop().
     * @return `true` if latest block has been updated, `false` otherwise.
     */
    bool checkLatestBlock();

    /**
     * Entry function for the worker thread (runs the workerLoop() function).
     * TODO: document return
     */
    bool workerLoop();

    /**
     * Wait for transactions to be added to the mempool and create a block by rdPoS consesus.
     * Called by workerLoop().
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
    rdPoSWorker(rdPoS& rdpos) : rdpos_(rdpos) {}

    /**
     * Destructor.
     * Automatically stops the worker thread if it's still running.
     */
    ~rdPoSWorker() { this->stop(); }

    /// Getter for `canCreateBlock_`.
    const std::atomic<bool>& getCanCreateBlock() const { return this->canCreateBlock_; }

    /// Setter for `canCreateBlock_`.
    void blockCreated() { this->canCreateBlock_ = false; }

    /**
     * Start workerFuture_ and workerLoop.
     * Should only be called after node is synced.
     */
    void start();

    /// Stop workerFuture_ and workerLoop.
    void stop();
};

#endif // RDPOS_H
