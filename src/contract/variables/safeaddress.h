#ifndef SAFEADDRESS_H
#define SAFEADDRESS_H

#include "../../utils/strings.h"

class SafeAddress {
  private:
    Address address;
    mutable std::unique_ptr<Address> addressPtr;

    inline void check() const { if (addressPtr == nullptr) { addressPtr = std::make_unique<Address>(address); } };
  public:
    SafeAddress(const Address& address = Address()) : address(Address()), addressPtr(std::make_unique<Address>(address)) {};
    SafeAddress(const SafeAddress& other) {
      check();
      address = other.address;
      addressPtr = std::make_unique<Address>(*other.addressPtr);
    }

    /// Assignment operators
    inline Address& operator=(const Address& address) { check(); *addressPtr = address; return *addressPtr; };
    inline Address& operator=(const SafeAddress& other) { check(); *addressPtr = other.get(); return *addressPtr; };

    /// Equality operator.
    inline bool operator==(const SafeAddress& other) const { check(); return (*addressPtr == other.get()); }
    inline bool operator==(const Address& other) const { check(); return (*addressPtr == other); }

    /// Inquality operator.
    inline bool operator!=(const SafeAddress& other) const { check(); return (*addressPtr != other.get()); }
    inline bool operator!=(const Address& other) const { check(); return (*addressPtr != other); }

    inline const Address& get() const { check(); return *addressPtr; };
    inline void commit() { check(); address = *addressPtr; addressPtr = nullptr; };
    inline void revert() { addressPtr = nullptr; };
};




#endif // SAFEADDRESS_H