#ifndef SAFEBOOL_H
#define SAFEBOOL_H

#include <memory>
#include "safebase.h"

class SafeBool : public SafeBase {
  private:
    bool value;
    mutable std::unique_ptr<bool> valuePtr;

    inline void check() const override { if (valuePtr == nullptr) { valuePtr = std::make_unique<bool>(value); } };
 public:

  /// Only Variables built with this constructor will be registered within a contract.
  SafeBool(DynamicContract* owner, bool value = false) : SafeBase(owner), value(false), valuePtr(std::make_unique<bool>(value)) {};

  SafeBool(bool value = false) : SafeBase(nullptr), value(false), valuePtr(std::make_unique<bool>(value)) {};
  SafeBool(const SafeBool& other) : SafeBase(nullptr) {
    other.check();
    value = other.value;
    valuePtr = std::make_unique<bool>(*other.valuePtr);
  }

  inline SafeBool& operator=(const SafeBool& other) { check(); markAsUsed(); *valuePtr = other.get(); return *this; }
  inline SafeBool& operator=(bool value) { check(); markAsUsed(); *valuePtr = value; return *this; }

  explicit operator bool() const { check(); return *valuePtr; }
  inline const bool& get() const { check(); return *valuePtr; };

  inline void commit() override { check(); value = *valuePtr; valuePtr = nullptr; registered = false; };
  inline void revert() const override { valuePtr = nullptr; registered = false; };
};














#endif // SAFEBOOL_H
