#ifndef SAFEUINT32_T_H
#define SAFEUINT32_T_H

#include <memory>
#include "safebase.h"

class SafeUint32_t : public SafeBase {
  private:
    uint32_t value;
    /// Check() require valuePtr to be mutable.
    mutable std::unique_ptr<uint32_t> valuePtr;
    inline void check() const override { if (valuePtr == nullptr) { valuePtr = std::make_unique<uint32_t>(value); } };
  public:

     /// Only Variables built with this constructor will be registered within a contract.
     SafeUint32_t(DynamicContract* owner, const uint32_t& value = 0) : SafeBase(owner), value(0), valuePtr(std::make_unique<uint32_t>(value)) {};

     SafeUint32_t(const uint32_t& value = 0) : SafeBase(nullptr), value(0), valuePtr(std::make_unique<uint32_t>(value)) {};
     SafeUint32_t(const SafeUint32_t& other) : SafeBase(nullptr) {
       other.check();
       value = 0;
       valuePtr = std::make_unique<uint32_t>(*other.valuePtr);
     };

    /// Arithmetic Operators
    inline SafeUint32_t operator+(const SafeUint32_t& other) const {
      check();
      if (*valuePtr > std::numeric_limits<uint32_t>::max() - other.get()) {
        throw std::overflow_error("Overflow in addition operation");
      }
      return SafeUint32_t(*valuePtr + other.get());
    };

    inline SafeUint32_t operator+(const uint32_t& other) const {
      check();
      if (*valuePtr > std::numeric_limits<uint32_t>::max() - other) {
        throw std::overflow_error("Overflow in addition operation");
      }
      return SafeUint32_t(*valuePtr + other);
    };

    inline SafeUint32_t operator-(const SafeUint32_t& other) const {
      check();
      if (*valuePtr < other.get()) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      return SafeUint32_t(*valuePtr - other.get());
    };

    inline SafeUint32_t operator-(const uint32_t& other) const {
      check();
      if (*valuePtr < other) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      return SafeUint32_t(*valuePtr - other);
    };

    inline SafeUint32_t operator* (const SafeUint32_t& other) const {
      check();
      if (other.get() == 0 || *valuePtr == 0) {
        throw std::domain_error("Multiplication by zero");
      }
      if (*valuePtr > std::numeric_limits<uint32_t>::max() / other.get()) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      return SafeUint32_t(*valuePtr * other.get());
    };

    inline SafeUint32_t operator* (const uint32_t& other) const {
      check();
      if (other == 0 || *valuePtr == 0) {
        throw std::domain_error("Multiplication by zero");
      }
      if (*valuePtr > std::numeric_limits<uint32_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      return SafeUint32_t(*valuePtr * other);
    };

    inline SafeUint32_t operator/ (const SafeUint32_t& other) const {
      check();
      if (*valuePtr == 0 || other.get() == 0) {
        throw std::domain_error("Division by zero");
      }
      return SafeUint32_t(*valuePtr / other.get());
    };

    inline SafeUint32_t operator/ (const uint32_t& other) const {
      check();
      if (*valuePtr == 0 || other == 0) {
        throw std::domain_error("Division by zero");
      }
      return SafeUint32_t(*valuePtr / other);
    };

    inline SafeUint32_t operator% (const SafeUint32_t& other) const {
      check();
      if (*valuePtr == 0 || other.get() == 0) {
        throw std::domain_error("Modulo by zero");
      }
      return SafeUint32_t(*valuePtr % other.get());
    };
    inline SafeUint32_t operator% (const uint32_t& other) const {
      check();
      if (*valuePtr == 0 || other == 0) {
        throw std::domain_error("Modulo by zero");
      }
      return SafeUint32_t(*valuePtr % other);
    };
    /// Bitwise Operators
    inline SafeUint32_t operator& (const SafeUint32_t& other) const { check(); return SafeUint32_t(*valuePtr & other.get()); };
    inline SafeUint32_t operator& (const uint32_t& other) const { check(); return SafeUint32_t(*valuePtr & other); };
    inline SafeUint32_t operator| (const SafeUint32_t& other) const { check(); return SafeUint32_t(*valuePtr | other.get()); };
    inline SafeUint32_t operator| (const uint32_t& other) const { check(); return SafeUint32_t(*valuePtr | other); };
    inline SafeUint32_t operator^ (const SafeUint32_t& other) const { check(); return SafeUint32_t(*valuePtr ^ other.get()); };
    inline SafeUint32_t operator^ (const uint32_t& other) const { check(); return SafeUint32_t(*valuePtr ^ other); };
    inline SafeUint32_t operator<< (const SafeUint32_t& other) const { check(); return SafeUint32_t(*valuePtr << other.get()); };
    inline SafeUint32_t operator<< (const uint32_t& other) const { check(); return SafeUint32_t(*valuePtr << other); };
    inline SafeUint32_t operator>> (const SafeUint32_t& other) const { check(); return SafeUint32_t(*valuePtr >> other.get()); };
    inline SafeUint32_t operator>> (const uint32_t& other) const { check(); return SafeUint32_t(*valuePtr >> other); };
    /// Logical Operators
    inline bool operator!() const { check(); return !*valuePtr; };
    inline bool operator&&(const SafeUint32_t& other) const { check(); return *valuePtr && other.get(); };
    inline bool operator&&(const uint32_t& other) const { check(); return *valuePtr && other; };
    inline bool operator||(const SafeUint32_t& other) const { check(); return *valuePtr || other.get(); };
    inline bool operator||(const uint32_t& other) const { check(); return *valuePtr || other; };
    /// Comparison Operators
    inline bool operator==(const SafeUint32_t& other) const { check(); return *valuePtr == other.get(); };
    inline bool operator==(const uint32_t& other) const { check(); return *valuePtr == other; };
    inline bool operator!=(const SafeUint32_t& other) const { check(); return *valuePtr != other.get(); };
    inline bool operator!=(const uint32_t& other) const { check(); return *valuePtr != other; };
    inline bool operator<(const SafeUint32_t& other) const { check(); return *valuePtr < other.get(); };
    inline bool operator<(const uint32_t& other) const { check(); return *valuePtr < other; };
    inline bool operator<=(const SafeUint32_t& other) const { check(); return *valuePtr <= other.get(); };
    inline bool operator<=(const uint32_t& other) const { check(); return *valuePtr <= other; };
    inline bool operator>(const SafeUint32_t& other) const { check(); return *valuePtr > other.get(); };
    inline bool operator>(const uint32_t& other) const { check(); return *valuePtr > other; };
    inline bool operator>=(const SafeUint32_t& other) const { check(); return *valuePtr >= other.get(); };
    inline bool operator>=(const uint32_t& other) const { check(); return *valuePtr >= other; };
    /// Assignment Operators
    inline SafeUint32_t& operator= (const SafeUint32_t& other) { check(); markAsUsed(); *valuePtr = other.get(); return *this; };
    inline SafeUint32_t& operator= (const uint32_t& other) { check(); markAsUsed(); *valuePtr = other; return *this; };

    inline SafeUint32_t& operator+= (const SafeUint32_t& other) {
      check();
      markAsUsed();
      if (*valuePtr > std::numeric_limits<uint32_t>::max() - other.get()) {
        throw std::overflow_error("Overflow in addition operation");
      }
      *valuePtr += other.get();
      return *this;
    };

    inline SafeUint32_t& operator+= (const uint32_t& other) {
      check();
      markAsUsed();
      if (*valuePtr > std::numeric_limits<uint32_t>::max() - other) {
        throw std::overflow_error("Overflow in addition operation");
      }
      *valuePtr += other;
      return *this;
    };

    inline SafeUint32_t& operator-= (const SafeUint32_t& other) {
      check();
      markAsUsed();
      if (*valuePtr < other.get()) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      *valuePtr -= other.get();
      return *this;
    };

    inline SafeUint32_t& operator-= (const uint32_t& other) {
      check();
      markAsUsed();
      if (*valuePtr < other) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      *valuePtr -= other;
      return *this;
    };

    inline SafeUint32_t& operator*= (const SafeUint32_t& other) {
      check();
      markAsUsed();
      if (other.get() == 0 || *valuePtr == 0) {
        throw std::domain_error("Multiplication by zero");
      }
      if (*valuePtr > std::numeric_limits<uint32_t>::max() / other.get()) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      *valuePtr *= other.get();
      return *this;
    };

    inline SafeUint32_t& operator*= (const uint32_t& other) {
      check();
      markAsUsed();
      if (other == 0 || *valuePtr == 0) {
        throw std::domain_error("Multiplication by zero");
      }
      if (*valuePtr > std::numeric_limits<uint32_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      *valuePtr *= other;
      return *this;
    };

    inline SafeUint32_t& operator/= (const SafeUint32_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other.get() == 0) {
        throw std::domain_error("Division by zero");
      }
      *valuePtr /= other.get();
      return *this;
    };

    inline SafeUint32_t& operator/= (const uint32_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other == 0) {
        throw std::domain_error("Division by zero");
      }
      *valuePtr /= other;
      return *this;
    };

    inline SafeUint32_t& operator%= (const SafeUint32_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other.get() == 0) {
        throw std::domain_error("Modulo by zero");
      }
      *valuePtr %= other.get();
      return *this;
    };

    inline SafeUint32_t& operator%= (const uint32_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other == 0) {
        throw std::domain_error("Modulo by zero");
      }
      *valuePtr %= other;
      return *this;
    };
    inline SafeUint32_t& operator&= (const SafeUint32_t& other) { check(); markAsUsed(); *valuePtr &= other.get(); return *this; };
    inline SafeUint32_t& operator&= (const uint32_t& other) { check(); markAsUsed(); *valuePtr &= other; return *this; };
    inline SafeUint32_t& operator|= (const SafeUint32_t& other) { check(); markAsUsed(); *valuePtr |= other.get(); return *this; };
    inline SafeUint32_t& operator|= (const uint32_t& other) { check(); markAsUsed(); *valuePtr |= other; return *this; };
    inline SafeUint32_t& operator^= (const SafeUint32_t& other) { check(); markAsUsed(); *valuePtr ^= other.get(); return *this; };
    inline SafeUint32_t& operator^= (const uint32_t& other) { check(); markAsUsed(); *valuePtr ^= other; return *this; };
    inline SafeUint32_t& operator<<= (const SafeUint32_t& other) { check(); markAsUsed(); *valuePtr <<= other.get(); return *this; };
    inline SafeUint32_t& operator<<= (const uint32_t& other) { check(); markAsUsed(); *valuePtr <<= other; return *this; };
    inline SafeUint32_t& operator>>= (const SafeUint32_t& other) { check(); markAsUsed(); *valuePtr >>= other.get(); return *this; };
    inline SafeUint32_t& operator>>= (const uint32_t& other) { check(); markAsUsed(); *valuePtr >>= other; return *this; };
    /// Increment and Decrement Operators
    inline SafeUint32_t& operator++() {
      check();
      markAsUsed();
      if (*valuePtr == std::numeric_limits<uint32_t>::max()) {
        throw std::overflow_error("Overflow in increment operation");
      }
      ++*valuePtr;
      return *this;
    };

    inline SafeUint32_t& operator--() {
      check();
      markAsUsed();
      if (*valuePtr == std::numeric_limits<uint32_t>::min()) {
        throw std::underflow_error("Underflow in increment operation");
      }
      --*valuePtr;
      return *this;
    };

    /// get is used to properly get the value of a variable within another SafeUint32_t (we need to call check!)
    inline uint32_t get() const { check(); return *valuePtr; };
    inline void commit() override { check(); value = *valuePtr; valuePtr = nullptr; registered = false; };
    inline void revert() const override { valuePtr = nullptr; registered = false; };
};

#endif