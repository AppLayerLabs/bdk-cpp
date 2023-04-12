#ifndef SAFEADDRESS_H
#define SAFEADDRESS_H

#include "../../utils/strings.h"
#include "safebase.h"

class SafeAddress : public SafeBase {
  private:
    Address address;
    mutable std::unique_ptr<Address> addressPtr;

    inline void check() const override { if (addressPtr == nullptr) { addressPtr = std::make_unique<Address>(address); } };
  public:
    /// Only Variables built with this constructor will be registered within a contract.
    SafeAddress(Contract* owner, const Address& address = Address()) : SafeBase(owner), address(address), addressPtr(std::make_unique<Address>(address)) {};

    SafeAddress(const Address& address = Address()) : SafeBase(nullptr), address(Address()), addressPtr(std::make_unique<Address>(address)) {};
    SafeAddress(const SafeAddress& other) : SafeBase(nullptr) {
      check();
      address = other.address;
      addressPtr = std::make_unique<Address>(*other.addressPtr);
    }

    /// Assignment operators
    inline Address& operator=(const Address& address) { check();  markAsUsed(); *addressPtr = address; return *addressPtr; };
    inline Address& operator=(const SafeAddress& other) { check();  markAsUsed(); *addressPtr = other.get(); return *addressPtr; };

    /// Equality operator.
    inline bool operator==(const SafeAddress& other) const { check(); return (*addressPtr == other.get()); }
    inline bool operator==(const Address& other) const { check(); return (*addressPtr == other); }

    /// Inquality operator.
    inline bool operator!=(const SafeAddress& other) const { check(); return (*addressPtr != other.get()); }
    inline bool operator!=(const Address& other) const { check(); return (*addressPtr != other); }

    inline const Address& get() const { check(); return *addressPtr; };
    inline void commit() override { check(); address = *addressPtr; addressPtr = nullptr; };
    inline void revert() const override { addressPtr = nullptr; };
};




#endif // SAFEADDRESS_H