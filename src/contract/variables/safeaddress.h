/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEADDRESS_H
#define SAFEADDRESS_H

#include "../../utils/strings.h"

#include "safebase.h"

/**
 * Safe wrapper for an Address variable. Used to safely store an Address within a contract.
 * @see SafeBase, Address
 */
class SafeAddress : public SafeBase {
  private:
    Address value_; ///< Current ("original") value.
    std::unique_ptr<Address> copy_; ///< Previous ("temporary") value.

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param address The initial value. Defaults to an empty address.
     */
    SafeAddress(DynamicContract* owner, const Address& address = Address())
      : SafeBase(owner), value_(address), copy_(nullptr) {}

    /**
     * Empty constructor.
     * @param address The initial value. Defaults to an empty address.
     */
    explicit SafeAddress(const Address& address = Address())
      : SafeBase(nullptr), value_(address), copy_(nullptr) {}

    /// Copy constructor. Only copies the CURRENT value.
    SafeAddress(const SafeAddress& other) : SafeBase(nullptr), value_(other.value_), copy_(nullptr) {}

    /// Getter for the CURRENT value.
    inline const Address& get() const { return this->value_; }

    ///@{
    /** Assignment operator. Assigns only the CURRENT value. */
    inline SafeAddress& operator=(const Address& address) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<Address>(this->value_);
      markAsUsed(); this->value_ = address; return *this;
    };
    inline SafeAddress& operator=(const SafeAddress& other) {
      if (this->copy_ == nullptr) this->copy_ = std::make_unique<Address>(this->value_);
      markAsUsed(); this->value_ = other.get(); return *this;
    };
    ///@}

    ///@{
    /** Equality operator. Checks only the CURRENT value. */
    inline bool operator==(const Address& other) const { return (this->value_ == other); }
    inline bool operator==(const SafeAddress& other) const { return (this->value_ == other.get()); }
    ///@}

    /// Commit the value.
    inline void commit() override { this->copy_ = nullptr; this->registered_ = false; };

    /// Revert the value.
    inline void revert() override { this->value_ = *this->copy_; this->copy_ = nullptr; this->registered_ = false; };
};

#endif  // SAFEADDRESS_H
