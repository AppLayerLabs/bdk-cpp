#ifndef RDPOS_H
#define RDPOS_H

#include <mutex>

#include "../contract/contract.h"
#include "../net/p2pmanager.h"
#include "../utils/block.h"
#include "../utils/db.h"
#include "../utils/randomgen.h"
#include "../utils/safehash.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"

#include "state.h"
#include "storage.h"

/**
 * Abstraction of a validator.
 * A validator is a node that validates blocks and transactions in the network.
 */
class Validator {
  private:
    Address add;  ///< The validator's address.

  public:
    /// Constructor.
    Validator(const Address& add) : add(add) {}

    /// Move constructor.
    Validator(const Address&& add) : add(std::move(add)) {}

    /// Copy constructor.
    Validator(const Validator& other) : add(other.add) {}

    /// Move constructor.
    Validator(Validator&& other) noexcept : add(std::move(other.add)) {}

    /// Getter for `add`.
    const Address& get() { return this->add; }

    /// Getter for `add`, but returns the address in hex format.
    const std::string hex() { return this->add.hex(); }

    /// Equality operator. Checks both addresses.
    bool operator==(const Validator& other) { return add == other.add; }

    /// Inequality operator. Checks both addresses.
    bool operator!=(const Validator& other) { return add != other.add; }
};

/**
 * Implementation of rdPoS (Random Deterministic Proof of Stake).
 * Also known as "block manager", as it manages block creation and congestion.
 * Also considered a contract, but remains part of the core protocol.
 */
class rdPoS : public Contract {
  private:
    /// List of known Validator nodes.
    std::vector<Validator> validatorList;

    /// Shuffled version of `validatorList`, used at block creation/signing.
    std::vector<std::reference_wrapper<Validator>> randomList;

    /// Mempool for Validator transactions.
    std::unordered_map<Hash, TxValidator, SafeHash> validatorMempool;

    /// Private key of the Validator node running the block manager contract.
    PrivKey validatorPrivKey;

    /// Indicates whether the current running node is a Validator or not.
    bool isValidator = false;

    /// Indicates whether the validator thread is running or not. See `validatorLoop()`.
    bool isValidatorThreadRunning = false;

    /// Mutex that manages read/write access to the block manager.
    std::mutex lock;

    /// Random seed generator.
    RandomGen gen;

    /// Pointer to the database.
    const std::shared_ptr<DB> db;

    /// Pointer to the blockchain history.
    const std::shared_ptr<Storage> storage;

    /// Pointer to the P2P connection manager.
    const std::shared_ptr<P2PManager> p2p;

    /**
     * Load Validator nodes from the database.
     * Validators are stored as a list - 8 bytes for index and 32 bytes for public key.
     * Throws on error.
     */
    void loadFromDB();

    /**
     * Shuffle Validator order for the next block.
     * @return `true` on success, `false` on failure.
     */
    bool shuffle();

    /**
     * Function loop for the Validator to do several things.
     * Runs in a thread. Also known as "the implementation of rdPoS". Steps are:
     * - Checks if the node is a Validator that wil create hash txs or block txs
     * - If block txs, it'll be the one to sign and broadcast the block to the network
     * - If hash txs, it'll be the one to provide signatures for creating the block
     * - Asks for txs from other Validator nodes and checks if both are valid
     */
    void validatorLoop();

  public:
    /// Enum for transaction types.
    enum TxType { addValidator, removeValidator, randomHash, randomSeed };

    /// Minimum number of required Validators for creating and signing blocks.
    static const uint32_t minValidators = 4;

    /**
     * Constructor.
     * @param db Pointer to the database.
     * @param storage Pointer to the blockchain history.
     * @param p2p Pointer to the P2P connection manager.
     * @param add The address where the block manager will be deployed as a smart contract.
     * @param owner The owner address of the block manager contract.
     * @param privKey (optional) Private key of the Validator.
     *                If set, the current running node will become a Validator.
     */
    rdPoS(
      const std::shared_ptr<DB>& db, const std::shared_ptr<Storage>& storage,
      const std::shared_ptr<P2PManager>& p2p, const Address& add,
      const Address& owner, const PrivKey& privKey = Hash(uint256_t(0))
    ) : db(db), storage(storage), p2p(p2p), gen(Hash()), Contract(add, owner)
    {
      if (privKey != Hash(uint256_t(0))) {
        this->isValidator = true; this->validatorPrivKey = privKey;
      }
      this->loadFromDB();
      Utils::logToDebug(Log::rdpos, __func__, std::string("Loaded ")
        + std::to_string(validatorList.size()) + std::string(" validators")
      );
      for (Validator& v : validatorList) Utils::logToDebug(
        Log::rdpos, __func__, std::string("Validator: ") + v.hex()
      );
    }

    /// Getter for `validatorMempool`. Returns a copy, not the original.
    std::unordered_map<Hash, TxValidator, SafeHash> getMempoolCopy() {
      this->lock.lock();
      auto ret = this->validatorMempool;
      this->lock.unlock();
      return ret;
    }

    /// Getter for `randomList`. Returns a copy, not the original.
    std::vector<std::reference_wrapper<Validator>> getRandomListCopy() {
      this->lock.lock();
      auto ret = this->randomList;
      this->lock.unlock();
      return ret;
    }

    /**
     * Check if a given Validator is in the list of known validator nodes.
     * @param val The Validator to check.
     * @return `true` if the Validator is known, `false` otherwise.
     */
    bool validatorIsKnown(const Validator& val);

    /// Save the current Validator nodes list to the database.
    void saveToDB();

    /**
     * Validate a block.
     * @param block The block to validate.
     * @return `true` if the block is properly validated, `false` otherwise.
     */
    bool validateBlock(const std::shared_ptr<const Block>& block);

    /**
     * Process a block.
     * @param block The block to process.
     * @return The new randomness seed to be used for the next block.
     */
    Hash processBlock(const std::shared_ptr<const Block>& block);

    /**
     * Add a Validator transaction to the mempool.
     * @param tx The transaction to add.
     */
    void addValidatorTx(const TxValidator& tx);

    /**
     * Finalize a block. See %Block for more details.
     * @param block The block to finalize.
     */
    inline void finalizeBlock(const std::shared_ptr<Block> block) {
      block->finalize(this->validatorPrivKey);
    }

    /**
     * Parse a transaction list.
     * Does NOT validate any of the transactions.
     * @param txs The list of transactions to parse.
     * @return The new randomness seed to be used for the next block.
     */
    static Hash parseTxSeedList(const std::unordered_map<uint64_t, TxValidator, SafeHash> txs);

    /// Runs `validatorLoop()` inside the Validator thread.
    void startValidatorThread();
};

#endif  // RDPOS_H
