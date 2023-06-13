#ifndef SAFEBOOL_H
#define SAFEBOOL_H

#include <memory>

#include "safebase.h"

/**
 * Safe wrapper for a bool variable.
 * Used to safely store a bool within a contract.
 * @see SafeBase
 */
class SafeBool : public SafeBase {
  private:
    bool value; ///< Value.
    mutable std::unique_ptr<bool> valuePtr; ///< Pointer to the value. check() requires this to be mutable.

    /// Check if the pointer is initialized (and initialize it if not).
    inline void check() const override {
      if (valuePtr == nullptr) { valuePtr = std::make_unique<bool>(value); }
    };

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param value The initial value. Defaults to `false`.
     */
    SafeBool(DynamicContract* owner, bool value = false)
      : SafeBase(owner), value(false), valuePtr(std::make_unique<bool>(value))
    {};

    /**
     * Empty constructor.
     * @param value The initial value. Defaults to `false`.
     */
    SafeBool(bool value = false)
      : SafeBase(nullptr), value(false), valuePtr(std::make_unique<bool>(value))
    {};

    /// Copy constructor.
    SafeBool(const SafeBool& other) : SafeBase(nullptr) {
      other.check();
      value = other.value;
      valuePtr = std::make_unique<bool>(*other.valuePtr);
    }

    /// Getter for the value. Returns the value from the pointer.
    inline const bool& get() const { check(); return *valuePtr; };

    /// Explicit conversion operator used to get the value.
    explicit operator bool() const { check(); return *valuePtr; }

    /**
     * Commit the value. Updates the value from the pointer, nullifies it and
     * unregisters the variable.
     */
    inline void commit() override {
      check();
      value = *valuePtr;
      valuePtr = nullptr;
      registered = false;
    };

    /// Revert the value. Nullifies the pointer and unregisters the variable.
    inline void revert() const override {
      valuePtr = nullptr;
      registered = false;
    };

    /// Assignment operator.
    inline SafeBool& operator=(bool value) {
      check();
      markAsUsed();
      *valuePtr = value;
      return *this;
    }

    /// Assignment operator.
    inline SafeBool& operator=(const SafeBool& other) {
      check();
      markAsUsed();
      *valuePtr = other.get();
      return *this;
    }
};

#endif  // SAFEBOOL_H
