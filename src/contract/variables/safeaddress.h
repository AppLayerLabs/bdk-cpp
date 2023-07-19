#ifndef SAFEADDRESS_H
#define SAFEADDRESS_H

#include "../../utils/strings.h"
#include "safebase.h"

/**
 * Safe wrapper for an Address variable.
 * Used to safely store an Address within a contract.
 * @see SafeBase
 * @see Address
 */
class SafeAddress : public SafeBase {
  private:
    Address address; ///< Value.
    mutable std::unique_ptr<Address> addressPtr; ///< Pointer to the value. check() requires this to be mutable.

    /// Check if the pointer is initialized (and initialize it if not).
    inline void check() const override {
      if (addressPtr == nullptr) addressPtr = std::make_unique<Address>(address);
    };

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param address The initial value. Defaults to an empty address.
     */
    SafeAddress(DynamicContract* owner, const Address& address = Address())
      : SafeBase(owner), address(address), addressPtr(std::make_unique<Address>(address))
    {};

    /**
     * Empty constructor.
     * @param address The initial value. Defaults to an empty address.
     */
    SafeAddress(const Address& address = Address())
      : SafeBase(nullptr), address(Address()), addressPtr(std::make_unique<Address>(address))
    {};

    /// Copy constructor.
    SafeAddress(const SafeAddress& other) : SafeBase(nullptr) {
      check();
      address = other.address;
      addressPtr = std::make_unique<Address>(*other.addressPtr);
    }

    /// Getter for the value. Returns the value from the pointer.
    inline const Address& get() const { check(); return *addressPtr; };

    /// Commit the value. Updates the value from the pointer and nullifies it.
    inline void commit() override {
      check();
      address = *addressPtr;
      addressPtr = nullptr;
    };

    /// Revert the value. Nullifies the pointer.
    inline void revert() const override { addressPtr = nullptr; };

    /// Assignment operator.
    inline Address& operator=(const Address& address) {
      check();
      markAsUsed();
      *addressPtr = address;
      return *addressPtr;
    };

    /// Assignment operator.
    inline Address& operator=(const SafeAddress& other) {
      check();
      markAsUsed();
      *addressPtr = other.get();
      return *addressPtr;
    };

    /// Equality operator.
    inline bool operator==(const Address& other) const {
      check(); return (*addressPtr == other);
    }

    /// Equality operator.
    inline bool operator==(const SafeAddress& other) const {
      check(); return (*addressPtr == other.get());
    }

    /// Inequality operator.
    inline bool operator!=(const Address& other) const {
      check(); return (*addressPtr != other);
    }

    /// Inequality operator.
    inline bool operator!=(const SafeAddress& other) const {
      check(); return (*addressPtr != other.get());
    }
};

#endif  // SAFEADDRESS_H
