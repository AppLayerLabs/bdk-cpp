/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

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
    Address address_; ///< Value.
    mutable std::unique_ptr<Address> addressPtr_; ///< Pointer to the value. check() requires this to be mutable.

    /// Check if the pointer is initialized (and initialize it if not).
    inline void check() const override {
      if (addressPtr_ == nullptr) addressPtr_ = std::make_unique<Address>(address_);
    };

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param address The initial value. Defaults to an empty address.
     */
    SafeAddress(DynamicContract* owner, const Address& address = Address())
      : SafeBase(owner), address_(Address()), addressPtr_(std::make_unique<Address>(address))
    {};

    /**
     * Empty constructor.
     * @param address The initial value. Defaults to an empty address.
     */
    explicit SafeAddress(const Address& address = Address())
      : SafeBase(nullptr), address_(Address()), addressPtr_(std::make_unique<Address>(address))
    {};

    /// Copy constructor.
    SafeAddress(const SafeAddress& other) : SafeBase(nullptr) {
      other.check(); addressPtr_ = std::make_unique<Address>(*other.addressPtr_);
    }

    /// Getter for the value. Returns the value from the pointer.
    inline const Address& get() const { check(); return *addressPtr_; };

    /// Commit the value. Updates the value from the pointer and nullifies it.
    inline void commit() override {
      check();
      address_ = *addressPtr_;
      addressPtr_ = nullptr;
    };

    /// Revert the value. Nullifies the pointer.
    inline void revert() const override { addressPtr_ = nullptr; };

    /// Assignment operator.
    inline Address& operator=(const Address& address) {
      check();
      markAsUsed();
      *addressPtr_ = address;
      return *addressPtr_;
    };

    /// Assignment operator.
    inline Address& operator=(const SafeAddress& other) {
      check();
      markAsUsed();
      *addressPtr_ = other.get();
      return *addressPtr_;
    };

    /// Equality operator.
    inline bool operator==(const Address& other) const {
      check(); return (*addressPtr_ == other);
    }

    /// Equality operator.
    inline bool operator==(const SafeAddress& other) const {
      check(); return (*addressPtr_ == other.get());
    }
};

#endif  // SAFEADDRESS_H
