#ifndef SAFEUINT8_T_H
#define SAFEUINT8_T_H

#include <memory>


class SafeUint8_t {
  private:
    uint8_t value;
    /// Check() require valuePtr to be mutable.
    mutable std::unique_ptr<uint8_t> valuePtr;
    inline void check() const { if (valuePtr == nullptr) { valuePtr = std::make_unique<uint8_t>(value); } };
  public:

    SafeUint8_t(const uint8_t& value = 0) : value(0), valuePtr(std::make_unique<uint8_t>(value)) {};
    SafeUint8_t(const SafeUint8_t& other) {
      other.check();
      value = 0;
      valuePtr = std::make_unique<uint8_t>(*other.valuePtr);
    };

    /// Arithmetic Operators
    inline SafeUint8_t operator+(const SafeUint8_t& other) const {
      check();
      if (*valuePtr > std::numeric_limits<uint8_t>::max() - other.get()) {
        throw std::overflow_error("Overflow in addition operation");
      }
      return SafeUint8_t(*valuePtr + other.get());
    };

    inline SafeUint8_t operator+(const uint8_t& other) const {
      check();
      if (*valuePtr > std::numeric_limits<uint8_t>::max() - other) {
        throw std::overflow_error("Overflow in addition operation");
      }
      return SafeUint8_t(*valuePtr + other);
    };

    inline SafeUint8_t operator-(const SafeUint8_t& other) const {
      check();
      if (*valuePtr < other.get()) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      return SafeUint8_t(*valuePtr - other.get());
    };

    inline SafeUint8_t operator-(const uint8_t& other) const {
      check();
      if (*valuePtr < other) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      return SafeUint8_t(*valuePtr - other);
    };

    inline SafeUint8_t operator* (const SafeUint8_t& other) const {
      check();
      if (other.get() == 0 || *valuePtr == 0) {
        throw std::domain_error("Multiplication by zero");
      }
      if (*valuePtr > std::numeric_limits<uint8_t>::max() / other.get()) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      return SafeUint8_t(*valuePtr * other.get());
    };

    inline SafeUint8_t operator* (const uint8_t& other) const {
      check();
      if (other == 0 || *valuePtr == 0) {
        throw std::domain_error("Multiplication by zero");
      }
      if (*valuePtr > std::numeric_limits<uint8_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      return SafeUint8_t(*valuePtr * other);
    };

    inline SafeUint8_t operator/ (const SafeUint8_t& other) const {
      check();
      if (*valuePtr == 0 || other.get() == 0) {
        throw std::domain_error("Division by zero");
      }
      return SafeUint8_t(*valuePtr / other.get());
    };

    inline SafeUint8_t operator/ (const uint8_t& other) const {
      check();
      if (*valuePtr == 0 || other == 0) {
        throw std::domain_error("Division by zero");
      }
      return SafeUint8_t(*valuePtr / other);
    };

    inline SafeUint8_t operator% (const SafeUint8_t& other) const {
      check();
      if (*valuePtr == 0 || other.get() == 0) {
        throw std::domain_error("Modulo by zero");
      }
      return SafeUint8_t(*valuePtr % other.get());
    };
    inline SafeUint8_t operator% (const uint8_t& other) const {
      check();
      if (*valuePtr == 0 || other == 0) {
        throw std::domain_error("Modulo by zero");
      }
      return SafeUint8_t(*valuePtr % other);
    };
    /// Bitwise Operators
    inline SafeUint8_t operator& (const SafeUint8_t& other) const { check(); return SafeUint8_t(*valuePtr & other.get()); };
    inline SafeUint8_t operator& (const uint8_t& other) const { check(); return SafeUint8_t(*valuePtr & other); };
    inline SafeUint8_t operator| (const SafeUint8_t& other) const { check(); return SafeUint8_t(*valuePtr | other.get()); };
    inline SafeUint8_t operator| (const uint8_t& other) const { check(); return SafeUint8_t(*valuePtr | other); };
    inline SafeUint8_t operator^ (const SafeUint8_t& other) const { check(); return SafeUint8_t(*valuePtr ^ other.get()); };
    inline SafeUint8_t operator^ (const uint8_t& other) const { check(); return SafeUint8_t(*valuePtr ^ other); };
    inline SafeUint8_t operator<< (const SafeUint8_t& other) const { check(); return SafeUint8_t(*valuePtr << other.get()); };
    inline SafeUint8_t operator<< (const uint8_t& other) const { check(); return SafeUint8_t(*valuePtr << other); };
    inline SafeUint8_t operator>> (const SafeUint8_t& other) const { check(); return SafeUint8_t(*valuePtr >> other.get()); };
    inline SafeUint8_t operator>> (const uint8_t& other) const { check(); return SafeUint8_t(*valuePtr >> other); };
    /// Logical Operators
    inline bool operator!() const { check(); return !*valuePtr; };
    inline bool operator&&(const SafeUint8_t& other) const { check(); return *valuePtr && other.get(); };
    inline bool operator&&(const uint8_t& other) const { check(); return *valuePtr && other; };
    inline bool operator||(const SafeUint8_t& other) const { check(); return *valuePtr || other.get(); };
    inline bool operator||(const uint8_t& other) const { check(); return *valuePtr || other; };
    /// Comparison Operators
    inline bool operator==(const SafeUint8_t& other) const { check(); return *valuePtr == other.get(); };
    inline bool operator==(const uint8_t& other) const { check(); return *valuePtr == other; };
    inline bool operator!=(const SafeUint8_t& other) const { check(); return *valuePtr != other.get(); };
    inline bool operator!=(const uint8_t& other) const { check(); return *valuePtr != other; };
    inline bool operator<(const SafeUint8_t& other) const { check(); return *valuePtr < other.get(); };
    inline bool operator<(const uint8_t& other) const { check(); return *valuePtr < other; };
    inline bool operator<=(const SafeUint8_t& other) const { check(); return *valuePtr <= other.get(); };
    inline bool operator<=(const uint8_t& other) const { check(); return *valuePtr <= other; };
    inline bool operator>(const SafeUint8_t& other) const { check(); return *valuePtr > other.get(); };
    inline bool operator>(const uint8_t& other) const { check(); return *valuePtr > other; };
    inline bool operator>=(const SafeUint8_t& other) const { check(); return *valuePtr >= other.get(); };
    inline bool operator>=(const uint8_t& other) const { check(); return *valuePtr >= other; };
    /// Assignment Operators
    inline SafeUint8_t& operator= (const SafeUint8_t& other) { check(); *valuePtr = other.get(); return *this; };
    inline SafeUint8_t& operator= (const uint8_t& other) { check(); *valuePtr = other; return *this; };

    inline SafeUint8_t& operator+= (const SafeUint8_t& other) {
      check();
      if (*valuePtr > std::numeric_limits<uint8_t>::max() - other.get()) {
        throw std::overflow_error("Overflow in addition operation");
      }
      *valuePtr += other.get();
      return *this;
    };

    inline SafeUint8_t& operator+= (const uint8_t& other) {
      check();
      if (*valuePtr > std::numeric_limits<uint8_t>::max() - other) {
        throw std::overflow_error("Overflow in addition operation");
      }
      *valuePtr += other;
      return *this;
    };

    inline SafeUint8_t& operator-= (const SafeUint8_t& other) {
      check();
      if (*valuePtr < other.get()) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      *valuePtr -= other.get();
      return *this;
    };

    inline SafeUint8_t& operator-= (const uint8_t& other) {
      check();
      if (*valuePtr < other) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      *valuePtr -= other;
      return *this;
    };

    inline SafeUint8_t& operator*= (const SafeUint8_t& other) {
      check();
      if (other.get() == 0 || *valuePtr == 0) {
        throw std::domain_error("Multiplication by zero");
      }
      if (*valuePtr > std::numeric_limits<uint8_t>::max() / other.get()) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      *valuePtr *= other.get();
      return *this;
    };

    inline SafeUint8_t& operator*= (const uint8_t& other) {
      check();
      if (other == 0 || *valuePtr == 0) {
        throw std::domain_error("Multiplication by zero");
      }
      if (*valuePtr > std::numeric_limits<uint8_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      *valuePtr *= other;
      return *this;
    };

    inline SafeUint8_t& operator/= (const SafeUint8_t& other) {
      check();
      if (*valuePtr == 0 || other.get() == 0) {
        throw std::domain_error("Division by zero");
      }
      *valuePtr /= other.get();
      return *this;
    };

    inline SafeUint8_t& operator/= (const uint8_t& other) {
      check();
      if (*valuePtr == 0 || other == 0) {
        throw std::domain_error("Division by zero");
      }
      *valuePtr /= other;
      return *this;
    };

    inline SafeUint8_t& operator%= (const SafeUint8_t& other) {
      check();
      if (*valuePtr == 0 || other.get() == 0) {
        throw std::domain_error("Modulo by zero");
      }
      *valuePtr %= other.get();
      return *this;
    };

    inline SafeUint8_t& operator%= (const uint8_t& other) {
      check();
      if (*valuePtr == 0 || other == 0) {
        throw std::domain_error("Modulo by zero");
      }
      *valuePtr %= other;
      return *this;
    };
    inline SafeUint8_t& operator&= (const SafeUint8_t& other) { check(); *valuePtr &= other.get(); return *this; };
    inline SafeUint8_t& operator&= (const uint8_t& other) { check(); *valuePtr &= other; return *this; };
    inline SafeUint8_t& operator|= (const SafeUint8_t& other) { check(); *valuePtr |= other.get(); return *this; };
    inline SafeUint8_t& operator|= (const uint8_t& other) { check(); *valuePtr |= other; return *this; };
    inline SafeUint8_t& operator^= (const SafeUint8_t& other) { check(); *valuePtr ^= other.get(); return *this; };
    inline SafeUint8_t& operator^= (const uint8_t& other) { check(); *valuePtr ^= other; return *this; };
    inline SafeUint8_t& operator<<= (const SafeUint8_t& other) { check(); *valuePtr <<= other.get(); return *this; };
    inline SafeUint8_t& operator<<= (const uint8_t& other) { check(); *valuePtr <<= other; return *this; };
    inline SafeUint8_t& operator>>= (const SafeUint8_t& other) { check(); *valuePtr >>= other.get(); return *this; };
    inline SafeUint8_t& operator>>= (const uint8_t& other) { check(); *valuePtr >>= other; return *this; };
    /// Increment and Decrement Operators
    inline SafeUint8_t& operator++() {
      check();
      if (*valuePtr == std::numeric_limits<uint8_t>::max()) {
        throw std::overflow_error("Overflow in increment operation");
      }
      ++*valuePtr;
      return *this;
    };

    inline SafeUint8_t& operator--() {
      check();
      if (*valuePtr == std::numeric_limits<uint8_t>::min()) {
        throw std::underflow_error("Underflow in increment operation");
      }
      --*valuePtr;
      return *this;
    };

    /// get is used to properly get the value of a variable within another SafeUint8_t (we need to call check!)
    inline uint8_t get() const { check(); return *valuePtr; };
    inline void commit() { check(); value = *valuePtr; valuePtr = nullptr; };
    inline void revert() { valuePtr = nullptr; };
};

#endif