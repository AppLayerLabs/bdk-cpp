#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H

#include <shared_mutex>

#include "utils.h"
#include "db.h"
#include "random.h"

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

    const std::string& get() const { return _address.get(); };
    const std::string hex() const { return Utils::bytesToHex(_address.get()); }
};


/*
BlockManager contract: 0x0000000000000000626c6f636b4d616e61676572

*/
class BlockManager {
  private:
    std::vector<Validator> validatorsList;
    void loadFromDB(std::shared_ptr<DBService> &db);
    mutable std::shared_mutex managerLock;

  public:
    BlockManager(std::shared_ptr<DBService> &db);
    bool isValidator(const Validator &validator) const;
    void saveToDB(std::shared_ptr<DBService> &db);
};

#endif  // BLOCKMANAGER_H
