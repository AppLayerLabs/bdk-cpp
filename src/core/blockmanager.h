#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H

#include <shared_mutex>
#include "utils.h"
#include "db.h"


class Validator {
  private:
    // TODO: We could possibly use secp256k1 compressed pubkeys.
    std::string _pubkey;

  public:
    Validator() = default;

    Validator(const std::string &pubkey) { _pubkey = pubkey; }

    Validator(std::string&& pubkey) { _pubkey = std::move(pubkey); }

    Validator(const std::string_view &pubkey) { _pubkey = pubkey; }

    Validator(const Validator& other) { this->_pubkey = other._pubkey; }

    Validator(Validator&& other) noexcept :_pubkey(std::move(other._pubkey)) {}

    ~Validator() { this->_pubkey = ""; }

    void operator=(const std::string_view& address) { this->_pubkey = address; }
    void operator=(const std::string& address) { this->_pubkey = address; }
    void operator=(std::string&& address) { this->_pubkey = std::move(address); }
    Validator& operator=(Validator&& other) { this->_pubkey = std::move(other._pubkey); return *this;}
    const std::string& get() const { return _pubkey; };
    const std::string hex() const { return Utils::bytesToHex(_pubkey); }
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
#endif