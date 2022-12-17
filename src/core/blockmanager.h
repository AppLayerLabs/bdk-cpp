#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H

#include <shared_mutex>

#include "../utils/utils.h"
#include "../utils/db.h"
#include "../utils/random.h"
#include "../utils/secp256k1Wrapper.h"
#include "../utils/transaction.h"
#include "../contract/contract.h"
#include "chainHead.h"
#include "../net/P2PManager.h"
#include "../net/grpcclient.h"

// Forward declaration
class Block;


class Validator {
  private:
    const Address _address;
  public:
    Validator() = default;
    Validator(const Address &address) : _address(address) {}
    Validator(const Address&& address) : _address(std::move(address)) {}

    Validator(const Validator& other) : _address(other._address) {}
    Validator(Validator&& other) noexcept : _address(std::move(other._address)) {}
    ~Validator() = default;

    const Address& get() const { return _address; };
    const std::string hex() const { return _address.hex(); }

    bool operator==(const Validator& other) const {
      return _address == other._address;
    }

    bool operator!=(const Validator& other) const {
      return _address != other._address;
    }
};


/*
BlockManager contract: 0x0000000000000000626c6f636b4d616e61676572
BlockManager is also considered a contract, but it is not located under contracts folder because it remains part of the core protocol of Sparq
TODO: BlockManager currently considers that all nodes are online and in sync in the network.
      for this first version, that is okay, but we need to start handling the cases where nodes went down or not in sync.
TODO: Implement sentinels
*/
class BlockManager : public Contract {
  private:
    std::vector<Validator> validatorsList;
    std::vector<std::reference_wrapper<Validator>> randomList;
    std::unordered_map<Hash, Tx::Validator, SafeHash> validatorMempool;
    const std::shared_ptr<const ChainHead> chainHead;
    const std::shared_ptr<P2PManager> p2pmanager;
    const std::shared_ptr<VMCommClient> grpcClient;
    void loadFromDB(std::shared_ptr<DBService> &db);
    mutable std::shared_mutex managerLock;

    const PrivKey _validatorPrivKey;
    const bool _isValidator = false;
    bool _isValidatorThreadRunning = false;
    bool shuffle();

    void validatorLoop();

    RandomGen gen;
  public:
    enum TransactionTypes { addValidator, removeValidator, randomHash, randomSeed};
    static const uint32_t minValidators = 4;
    BlockManager(std::shared_ptr<DBService> &db, 
                 const std::shared_ptr<const ChainHead> chainHead, 
                 const std::shared_ptr<P2PManager> p2pmanager, 
                 const std::shared_ptr<VMCommClient> grpcClient,
                 const Address &address, 
                 const Address &owner);
    
    BlockManager(std::shared_ptr<DBService> &db, 
                 const std::shared_ptr<const ChainHead> chainHead, 
                 const std::shared_ptr<P2PManager> p2pmanager, 
                 const std::shared_ptr<VMCommClient> grpcClient,
                 const PrivKey& privKey, 
                 const Address &address, 
                 const Address &owner);

    bool isValidator(const Validator &validator) const;
    void saveToDB(std::shared_ptr<DBService> &db) const;
    // Validates a given block using current randomList
    bool validateBlock(const std::shared_ptr<const Block> &block) const;
    // Process the block, and returns the new given Hash for RandomGen.
    Hash processBlock(const std::shared_ptr<const Block> &block) const;

    // Add the validator transaction to the blockManager mempool.
    void addValidatorTx(const Tx::Validator& tx);

    // Parse tx list and returns the new given uint256_t for RandomGen.
    // DOES NOT VALIDATE!
    static Hash parseTxListSeed(const std::unordered_map<uint64_t, Tx::Validator, SafeHash> &transactions);
    // TX FUNCTIONS.

    // Get the functor of the transaction, throws if invalid.
    static TransactionTypes getTransactionType(const Tx::Validator &tx);

    void startValidatorThread();

    std::unordered_map<Hash, Tx::Validator, SafeHash> getMempoolCopy();

    std::vector<std::reference_wrapper<Validator>> getRandomListCopy();
    friend class State; 
};
#endif  // BLOCKMANAGER_H
