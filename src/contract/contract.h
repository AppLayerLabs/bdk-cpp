#ifndef CONTRACT_H
#define CONTRACT_H

#include <cstdint>
#include <string>

#include "../utils/db.h"
#include "../utils/utils.h"

class Contract {
  private:
    const Address _address;
    const Address _owner;
    static Hash currentBlockHash;
    static uint64_t currentBlockHeight;
    static uint64_t currentBlockTime;
    // Setters
    static void setCurrentBlockHash(const Hash &hash) { currentBlockHash = hash; }
    static void setCurrentBlockHeight(const uint64_t &height) { currentBlockHeight = height; }
    static void setCurrentBlockTime(const uint64_t &time) { currentBlockTime = time; }
  public:
  
    Contract(const Address &address, const Address &owner) : _address(address), _owner(owner) {}

    // Getters
    const Address& address() const { return this->_address; }
    const Address& owner() const { return this->_owner; }

    friend class State;
};

#endif  // CONTRACT_H
