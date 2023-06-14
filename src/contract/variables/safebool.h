#ifndef SAFEBOOL_H
#define SAFEBOOL_H

#include "safebase.h"
#include <memory>

/**
 * Safe wrapper for a bool variable.
 * This class is used to safely store a bool variable within a contract.
 * @see SafeBase
 */
class SafeBool : public SafeBase {
private:
  bool value;                             ///< value for the bool variable.
  mutable std::unique_ptr<bool> valuePtr; ///< Pointer to the value.

  /**
   * Check if the valuePtr is initialized.
   * If not, initialize it.
   */
  inline void check() const override {
    if (valuePtr == nullptr) {
      valuePtr = std::make_unique<bool>(value);
    }
  };

public:
  /**
   * Constructor.
   * Only Variables built with this constructor will be registered within a
   * contract.
   * @param owner The contract that owns this variable.
   * @param value The bool initial value (default: false).
   */
  SafeBool(DynamicContract *owner, bool value = false)
      : SafeBase(owner), value(false),
        valuePtr(std::make_unique<bool>(value)){};

  /**
   * Normal Constructor.
   * @param value The bool initial value (default: false).
   */
  SafeBool(bool value = false)
      : SafeBase(nullptr), value(false),
        valuePtr(std::make_unique<bool>(value)){};

  /**
   * Constructor used to create a SafeBool from another SafeBool (copy
   * constructor).
   * @param other The SafeBool to copy.
   */
  SafeBool(const SafeBool &other) : SafeBase(nullptr) {
    other.check();
    value = other.value;
    valuePtr = std::make_unique<bool>(*other.valuePtr);
  }

  /**
   * Assignment operator used to copy a SafeBool.
   * @param other The SafeBool to copy.
   * @return A reference to this SafeBool.
   */
  inline SafeBool &operator=(const SafeBool &other) {
    check();
    markAsUsed();
    *valuePtr = other.get();
    return *this;
  }

  /**
   * Assignment operator used to set the value of a SafeBool.
   * @param value The value to set.
   * @return A reference to this SafeBool.
   */
  inline SafeBool &operator=(bool value) {
    check();
    markAsUsed();
    *valuePtr = value;
    return *this;
  }

  /**
   * Explicit conversion operator used to get the value of a SafeBool.
   * @return The value of the SafeBool.
   */
  explicit operator bool() const {
    check();
    return *valuePtr;
  }

  /**
   * Getter for the value of the SafeBool.
   * @return The value of the SafeBool.
   */
  inline const bool &get() const {
    check();
    return *valuePtr;
  };

  /**
   *  Commit the value of the SafeBool, i.e. update the value of the SafeBool to
   * the value of the pointer.
   */
  inline void commit() override {
    check();
    value = *valuePtr;
    valuePtr = nullptr;
    registered = false;
  };

  /**
   * Revert the value of the SafeBool, i.e. nullify the pointer.
   */
  inline void revert() const override {
    valuePtr = nullptr;
    registered = false;
  };
};

#endif // SAFEBOOL_H
