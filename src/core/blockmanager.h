#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H

#include <shared_mutex>

#include "utils.h"
#include "db.h"

class Validator {
  private:
    // TODO: We could possibly use secp256k1 compressed pubkeys.
    StringContainer<64> _pubkey;

  public:
    Validator() = default;
    Validator(const std::string &pubkey) { _pubkey = StringContainer<64>(pubkey); }
    Validator(std::string&& pubkey) { _pubkey = std::move(StringContainer<64>(pubkey)); }
    Validator(const std::string_view &pubkey) { _pubkey = StringContainer<64>(pubkey); }
    Validator(const Validator& other) { this->_pubkey = StringContainer<64>(other._pubkey); }
    Validator(Validator&& other) noexcept :_pubkey(std::move(StringContainer<64>(other._pubkey))) {}
    ~Validator() = default;

    void operator=(const std::string_view& address) { this->_pubkey = StringContainer<64>(address); }
    void operator=(const std::string& address) { this->_pubkey = StringContainer<64>(address); }
    void operator=(std::string&& address) { this->_pubkey = std::move(StringContainer<64>(address)); }
    Validator& operator=(Validator&& other) { this->_pubkey = std::move(StringContainer<64>(other._pubkey)); return *this; }
    const std::string& get() const { return _pubkey.get(); };
    const std::string hex() const { return Utils::bytesToHex(_pubkey.get()); }
};

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
