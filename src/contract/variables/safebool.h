/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEBOOL_H
#define SAFEBOOL_H

#include <memory>

#include "safebase.h"

/**
 * Safe wrapper for a bool variable. Used to safely store a bool within a contract.
 * @see SafeBase
 */
class SafeBool : public SafeBase {
  private:
    bool value_; ///< Current ("original") value.
    bool copy_; ///< Previous ("temporary") value. Not a pointer because bool is trivial and only takes 1 byte, while a pointer takes 8 bytes.

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param value The initial value. Defaults to `false`.
     */
    SafeBool(DynamicContract* owner, bool value = false) : SafeBase(owner), value_(value), copy_(value) {}

    /**
     * Empty constructor.
     * @param value The initial value. Defaults to `false`.
     */
    SafeBool(bool value = false) : SafeBase(nullptr), value_(value), copy_(value) {}

    /// Copy constructor.
    SafeBool(const SafeBool& other) : SafeBase(nullptr), value_(other.value_), copy_(other.copy_) {}

    /// Getter for the CURRENT value.
    inline const bool& get() const { return this->value_; }

    /// Explicit conversion operator.
    explicit operator bool() const { return this->value_; }

    ///@{
    /** Assignment operator. Assigns only the CURRENT value. */
    inline SafeBool& operator=(bool value) { markAsUsed(); this->value_ = value; return *this; }
    inline SafeBool& operator=(const SafeBool& other) { markAsUsed(); this->value_ = other.get(); return *this; }
    ///@}

    ///@{
    /** Equality operator. Checks only the CURRENT value. */
    inline bool operator==(const bool& other) const { return (this->value_ == other); }
    inline bool operator==(const SafeBool& other) const { return (this->value_ == other.get()); }
    ///@}

    /// Commit the value.
    inline void commit() override { this->copy_ = this->value_; this->registered_ = false; }

    /// Revert the value.
    inline void revert() override { this->value_ = this->copy_; this->registered_ = false; }
};

#endif  // SAFEBOOL_H
