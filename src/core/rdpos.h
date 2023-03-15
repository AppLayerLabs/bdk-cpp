#ifndef RDPOS_H
#define RDPOS_H

#include "../contract/contract.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/safehash.h"
#include "../utils/randomgen.h"
#include "../net/p2p/p2pmanagerbase.h"

#include <optional>
#include <shared_mutex>

class rdPoSWorker;
class Storage;
class Block; 

class Validator : public Address {
  public:
    using Address::operator==; // Inherit == operator.
    using Address::operator!=; // Inherit != operator.
    using Address::operator<;  // Inherit < operator.
    using Address::operator<=; // Inherit <= operator.
    using Address::operator>;  // Inherit > operator.
    using Address::operator>=; // Inherit >= operator.

    /// Constructor.
    Validator(const Address& add) : Address(add) {}

    /// Move constructor.
    Validator(const Address&& add) : Address(std::move(add)) {}

    /// Copy constructor.
    Validator(const Validator& other) : Address(other.data, true) {}

    /// Move constructor.
    Validator(Validator&& other) noexcept : Address(std::move(other.data), true) {}

    /// Copy assignment operator.
    Validator& operator=(const Validator& other) {
      this->data = other.data;
      return *this;
    }

    /// Move assignment operator.
    Validator& operator=(Validator&& other) noexcept {
      this->data = std::move(other.data);
      return *this;
    }
};

class rdPoS : public Contract {
  private:
    ///< Ordered list of validators.
    std::set<Validator> validators;

    /// Shuffled version of `validatorList`, used at block creation/signing.
    std::vector<Validator> randomList;

    /// Mempool for validator Transactions.
    std::unordered_map<Hash, TxValidator, SafeHash> validatorMempool;

    /// Private Key for operating a validator, optional.
    std::optional<PrivKey> validatorKey;
    bool isValidator = false;

    /// worker for rdPoS. 
    const std::unique_ptr<rdPoSWorker> worker;

    /// Randomness Generator
    RandomGen randomGen;  

    /// Best randomness seed (taken from the last block)
    Hash bestRandomSeed;

    /// mutex for class members
    mutable std::shared_mutex mutex;

    /// Pointer to Storage
    const std::unique_ptr<Storage>& storage;

    /// Pointer to P2P Manager (for sending/requesting TxValidators from other nodes)
    const std::unique_ptr<P2P::ManagerBase>& p2p;

    /// Initializes the blockchain with the default information for rdPoS.
    /// This function is called by the constructor if no previous blockchain is found.
    void initializeBlockchain();

  public:
    /**
     * Constructor.
     * @param db Pointer to the database.
     * @param chainId The chain ID.
     * @param storage Pointer to the blockchain history.
     * @param p2p Pointer to the P2P Manager.
     * @param validatorKey The private key of the validator, if any.
     */

    rdPoS(const std::unique_ptr<DB>& db, 
          const uint64_t& chainId,
          const std::unique_ptr<Storage>& storage,
          const std::unique_ptr<P2P::ManagerBase>& p2p,
          const PrivKey& validatorKey = PrivKey());

    ~rdPoS();
    
    /// Enum for transaction types.
    enum TxType { addValidator, removeValidator, randomHash, randomSeed };

    /// Minimum number of required Validators for creating and signing blocks.
    static const uint32_t minValidators = 4;

    /// Getter for validators, not a reference because the inner set can be changed.
    const std::set<Validator> getValidators() const { std::shared_lock lock(this->mutex); return validators; }

    /// Getter for randomList, not a reference because the inner vector can be changed.
    const std::vector<Validator> getRandomList() const { std::shared_lock lock(this->mutex); return randomList; }

    /// Getter for mempool, not a reference because the inner map can be changed.
    const std::unordered_map<Hash, TxValidator, SafeHash> getMempool() const { std::shared_lock lock(this->mutex); return validatorMempool; }

    /// Getter for bestRandomSeed.
    const Hash getBestRandomSeed() const { std::shared_lock lock(this->mutex); return bestRandomSeed; }

    /// Check if a given address is a validator
    const bool isValidatorAddress(const Address& add) const { std::shared_lock lock(this->mutex); return validators.contains(Validator(add)); }

    /**
     * Validate a block.
     * @param block The block to validate.
     * @return `true` if the block is properly validated, `false` otherwise.
     */
    bool validateBlock(const Block& block);

    /**
     * Process a block.
     * @param block The block to process.
     * @return The new randomness seed to be used for the next block.
     */
    Hash processBlock(const Block& block);

    /**
     * Add a Validator transaction to the mempool.
     * @param tx The transaction to add.
     */
    void addValidatorTx(const TxValidator& tx);
 
    /**
     * Finalize a block. See %Block for more details.
     * @param block The block to finalize.
     */
    inline void finalizeBlock(Block& block);

    /**
     * Parse a transaction list.
     * Does NOT validate any of the block rdPoS transactions.
     * @param txs The list of transactions to parse.
     * @return The new randomness of given transaction set.
     */
    static Hash parseTxSeedList(const std::vector<TxValidator>& txs); 

    friend rdPoSWorker;
};

// Worker Class for rdPoS. This separate the class from the rdPoS operation which runs rdPoS.
class rdPoSWorker {
  private:
    rdPoS& rdpos;
  public:
    rdPoSWorker(rdPoS& rdpos) : rdpos(rdpos) {}
};




#endif // RDPOS_H