#ifndef SAFEBOOL_H
#define SAFEBOOL_H

#include <memory>


class SafeBool {
  private:
    bool value;
    mutable std::unique_ptr<bool> valuePtr;

    inline void check() const { if (valuePtr == nullptr) { valuePtr = std::make_unique<bool>(value); } };
 public:
  SafeBool(bool value = false) : value(false), valuePtr(std::make_unique<bool>(value)) {};
  SafeBool(const SafeBool& other) {
    other.check();
    value = other.value;
    valuePtr = std::make_unique<bool>(*other.valuePtr);
  }

  inline SafeBool& operator=(const SafeBool& other) { check(); *valuePtr = other.get(); return *this; }
  inline SafeBool& operator=(bool value) { check(); *valuePtr = value; return *this; }

  explicit operator bool() const { check(); return *valuePtr; }
  inline const bool& get() const { check(); return *valuePtr; };

  inline void commit() { check(); value = *valuePtr; valuePtr = nullptr; };
  inline void revert() { valuePtr = nullptr; };
};














#endif // SAFEBOOL_H
