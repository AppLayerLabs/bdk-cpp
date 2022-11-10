#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H

#include <shared_mutex>

#include "block.h"
#include "../utils/utils.h"
#include "../utils/db.h"
#include "../utils/random.h"
#include "../utils/secp256k1Wrapper.h"
#include "../contract/contract.h"

class Validator {
  private:
    const Address _address;
  public:
    Validator() = default;
    Validator(const std::string &pubkey, const bool &fromRPC) : _address(pubkey, fromRPC) {}
    Validator(std::string&& pubkey, const bool &fromRPC) : _address(std::move(pubkey), fromRPC) {}

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
    void loadFromDB(std::shared_ptr<DBService> &db);
    mutable std::shared_mutex managerLock;

    const Hash _validatorPrivKey;
    const bool _isValidator = false;
    bool shuffle();
  public:
    static const uint32_t minValidators = 4;
    BlockManager(std::shared_ptr<DBService> &db,const Address &address, const Address &owner);
    BlockManager(std::shared_ptr<DBService> &db, const Hash& privKey, const Address &address, const Address &owner);
    bool isValidator(const Validator &validator) const;
    void saveToDB(std::shared_ptr<DBService> &db) const;
    // Validates a given block using current randomList
    bool validateBlock(const std::shared_ptr<const Block> &block) const;
    // Process the block, and returns the new given uint256_t for RandomGen.
    uint256_t processBlock(const std::shared_ptr<const Block> &block) const;

    // TX FUNCTIONS.

    friend class State; 
};
#endif  // BLOCKMANAGER_H
