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
    bool value_; ///< Value.
    mutable std::unique_ptr<bool> valuePtr_; ///< Pointer to the value. check() requires this to be mutable.

    /// Check if the pointer is initialized (and initialize it if not).
    inline void check() const override {
      if (valuePtr_ == nullptr) { valuePtr_ = std::make_unique<bool>(value_); }
    };

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param value The initial value. Defaults to `false`.
     */
    SafeBool(DynamicContract* owner, bool value = false)
      : SafeBase(owner), value_(false), valuePtr_(std::make_unique<bool>(value))
    {};

    /**
     * Empty constructor.
     * @param value The initial value. Defaults to `false`.
     */
    SafeBool(bool value = false)
      : SafeBase(nullptr), value_(false), valuePtr_(std::make_unique<bool>(value))
    {};

    /// Copy constructor.
    SafeBool(const SafeBool& other) : SafeBase(nullptr) {
      other.check();
      value_ = other.value_;
      valuePtr_ = std::make_unique<bool>(*other.valuePtr_);
    }

    /// Getter for the value. Returns the value from the pointer.
    inline const bool& get() const { check(); return *valuePtr_; };

    /// Explicit conversion operator used to get the value.
    explicit operator bool() const { check(); return *valuePtr_; }

    /**
     * Commit the value. Updates the value from the pointer, nullifies it and
     * unregisters the variable.
     */
    inline void commit() override {
      check(); value_ = *valuePtr_; valuePtr_ = nullptr; registered_ = false;
    };

    /// Revert the value. Nullifies the pointer and unregisters the variable.
    inline void revert() const override {
      valuePtr_ = nullptr; registered_ = false;
    };

    ///@{
    /** Assignment operator. */
    inline SafeBool& operator=(bool value) {
      check(); markAsUsed(); *valuePtr_ = value; return *this;
    }
    inline SafeBool& operator=(const SafeBool& other) {
      check(); markAsUsed(); *valuePtr_ = other.get(); return *this;
    }
    ///@}
};

#endif  // SAFEBOOL_H
