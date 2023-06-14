#ifndef SAFEUINT160_T_H
#define SAFEUINT160_T_H

#include <memory>

#include <boost/multiprecision/cpp_int.hpp>

#include "safebase.h"

// Typedef for uint160_t.
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

/**
 * Safe wrapper for a uint160 variable.
 * Used to safely store a uint160 within a contract.
 * @see SafeBase
 */
class SafeUint160_t : public SafeBase {
  private:
    uint160_t value; ///< Value.
    mutable std::unique_ptr<uint160_t> valuePtr; ///< Pointer to the value. check() requires this to be mutable.

    /// Check if the pointer is initialized (and initialize it if not).
    inline void check() const override {
      if (valuePtr == nullptr) valuePtr = std::make_unique<uint160_t>(value);
    };

  public:
    /**
     * Constructor.
     * @param owner The contract that owns the variable.
     * @param value The initial value. Defaults to 0.
     */
    SafeUint160_t(DynamicContract* owner, const uint160_t& value = 0)
      : SafeBase(owner), value(0), valuePtr(std::make_unique<uint160_t>(value))
    {};

    /**
     * Empty constructor.
     * @param value The initial value. Defaults to 0.
     */
    SafeUint160_t(const uint160_t& value = 0)
      : SafeBase(nullptr), value(0), valuePtr(std::make_unique<uint160_t>(value))
    {};

    /// Copy constructor.
    SafeUint160_t(const SafeUint160_t& other) : SafeBase(nullptr) {
      other.check(); value = 0; valuePtr = std::make_unique<uint160_t>(*other.valuePtr);
    };

    /// Getter for the value. Returns the value from the pointer.
    inline uint160_t get() const { check(); return *valuePtr; };

    /**
     * Commit the value. Updates the value from the pointer, nullifies it and
     * unregisters the variable.
     */
    inline void commit() override { check(); value = *valuePtr; valuePtr = nullptr; registered = false; };

    /// Revert the value. Nullifies the pointer and unregisters the variable.
    inline void revert() const override { valuePtr = nullptr; registered = false; };

    // ====================
    // Arithmetic operators
    // ====================

    /**
     * Addition operator.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t operator+(const SafeUint160_t& other) const {
      check();
      if (*valuePtr > std::numeric_limits<uint160_t>::max() - other.get()) {
        throw std::overflow_error("Overflow in addition operation");
      }
      return SafeUint160_t(*valuePtr + other.get());
    };

    /**
     * Addition operator.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t operator+(const uint160_t& other) const {
      check();
      if (*valuePtr > std::numeric_limits<uint160_t>::max() - other) {
        throw std::overflow_error("Overflow in addition operation");
      }
      return SafeUint160_t(*valuePtr + other);
    };

    /**
     * Addition operator.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t operator+(const uint64_t& other) const {
      check();
      if (*valuePtr > std::numeric_limits<uint160_t>::max() - other) {
        throw std::overflow_error("Overflow in addition operation");
      }
      return SafeUint160_t(*valuePtr + other);
    };

    /**
     * Subtraction operator.
     * @throw std::underflow_error if an underflow happens.
     */
    inline SafeUint160_t operator-(const SafeUint160_t& other) const {
      check();
      if (*valuePtr < other.get()) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      return SafeUint160_t(*valuePtr - other.get());
    };

    /**
     * Subtraction operator.
     * @throw std::underflow_error if an underflow happens.
     */
    inline SafeUint160_t operator-(const uint160_t& other) const {
      check();
      if (*valuePtr < other) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      return SafeUint160_t(*valuePtr - other);
    };

    /**
     * Subtraction operator.
     * @throw std::underflow_error if an underflow happens.
     */
    inline SafeUint160_t operator-(const uint64_t& other) const {
      check();
      if (*valuePtr < other) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      return SafeUint160_t(*valuePtr - other);
    };

    /**
     * Multiplication operator.
     * @throw std::domain_error if one of the operands is zero.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t operator*(const SafeUint160_t& other) const {
      check();
      if (other.get() == 0 || *valuePtr == 0) throw std::domain_error("Multiplication by zero");
      if (*valuePtr > std::numeric_limits<uint160_t>::max() / other.get()) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      return SafeUint160_t(*valuePtr * other.get());
    };

    /**
     * Multiplication operator.
     * @throw std::domain_error if one of the operands is zero.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t operator*(const uint160_t& other) const {
      check();
      if (other == 0 || *valuePtr == 0) throw std::domain_error("Multiplication by zero");
      if (*valuePtr > std::numeric_limits<uint160_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      return SafeUint160_t(*valuePtr * other);
    };

    /**
     * Multiplication operator.
     * @throw std::domain_error if one of the operands is zero.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t operator*(const uint64_t& other) const {
      check();
      if (other == 0 || *valuePtr == 0) throw std::domain_error("Multiplication by zero");
      if (*valuePtr > std::numeric_limits<uint160_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      return SafeUint160_t(*valuePtr * other);
    };

    /**
     * Division operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t operator/(const SafeUint160_t& other) const {
      check();
      if (*valuePtr == 0 || other.get() == 0) throw std::domain_error("Division by zero");
      return SafeUint160_t(*valuePtr / other.get());
    };

    /**
     * Division operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t operator/(const uint160_t& other) const {
      check();
      if (*valuePtr == 0 || other == 0) throw std::domain_error("Division by zero");
      return SafeUint160_t(*valuePtr / other);
    };

    /**
     * Division operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t operator/(const uint64_t& other) const {
      check();
      if (*valuePtr == 0 || other == 0) throw std::domain_error("Division by zero");
      return SafeUint160_t(*valuePtr / other);
    };

    /**
     * Modulo operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t operator%(const SafeUint160_t& other) const {
      check();
      if (*valuePtr == 0 || other.get() == 0) throw std::domain_error("Modulo by zero");
      return SafeUint160_t(*valuePtr % other.get());
    };

    /**
     * Modulo operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t operator%(const uint160_t& other) const {
      check();
      if (*valuePtr == 0 || other == 0) throw std::domain_error("Modulo by zero");
      return SafeUint160_t(*valuePtr % other);
    };

    /**
     * Modulo operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t operator%(const uint64_t& other) const {
      check();
      if (*valuePtr == 0 || other == 0) throw std::domain_error("Modulo by zero");
      return SafeUint160_t(*valuePtr % other);
    };

    // =================
    // Bitwise operators
    // =================

    /// Bitwise AND operator.
    inline SafeUint160_t operator&(const SafeUint160_t& other) const {
      check(); return SafeUint160_t(*valuePtr & other.get());
    };

    /// Bitwise AND operator.
    inline SafeUint160_t operator&(const uint160_t& other) const {
      check(); return SafeUint160_t(*valuePtr & other);
    };

    /// Bitwise AND operator.
    inline SafeUint160_t operator&(const uint64_t& other) const {
      check(); return SafeUint160_t(*valuePtr & other);
    };

    /// Bitwise OR operator.
    inline SafeUint160_t operator|(const SafeUint160_t& other) const {
      check(); return SafeUint160_t(*valuePtr | other.get());
    };

    /// Bitwise OR operator.
    inline SafeUint160_t operator|(const uint160_t& other) const {
      check(); return SafeUint160_t(*valuePtr | other);
    };

    /// Bitwise OR operator.
    inline SafeUint160_t operator|(const uint64_t& other) const {
      check(); return SafeUint160_t(*valuePtr | other);
    };

    /// Bitwise XOR operator.
    inline SafeUint160_t operator^(const SafeUint160_t& other) const {
      check(); return SafeUint160_t(*valuePtr ^ other.get());
    };

    /// Bitwise XOR operator.
    inline SafeUint160_t operator^(const uint160_t& other) const {
      check(); return SafeUint160_t(*valuePtr ^ other);
    };

    /// Bitwise XOR operator.
    inline SafeUint160_t operator^(const uint64_t& other) const {
      check(); return SafeUint160_t(*valuePtr ^ other);
    };

    // =================
    // Logical operators
    // =================

    /// Logical NOT operator.
    inline bool operator!() const { check(); return !*valuePtr; };

    /// Logical AND operator.
    inline bool operator&&(const SafeUint160_t& other) const {
      check(); return *valuePtr && other.get();
    };

    /// Logical AND operator.
    inline bool operator&&(const uint160_t& other) const {
      check(); return *valuePtr && other;
    };

    /// Logical AND operator.
    inline bool operator&&(const uint64_t& other) const {
      check(); return *valuePtr && other;
    };

    /// Logical OR operator.
    inline bool operator||(const SafeUint160_t& other) const {
      check(); return *valuePtr || other.get();
    };

    /// Logical OR operator.
    inline bool operator||(const uint160_t& other) const {
      check(); return *valuePtr || other;
    };

    /// Logical OR operator.
    inline bool operator||(const uint64_t& other) const {
      check(); return *valuePtr || other;
    };

    // ====================
    // Comparison operators
    // ====================

    /// Equality operator.
    inline bool operator==(const SafeUint160_t& other) const {
      check(); return *valuePtr == other.get();
    };

    /// Equality operator.
    inline bool operator==(const uint160_t& other) const {
      check(); return *valuePtr == other;
    };

    /// Equality operator.
    inline bool operator==(const uint64_t& other) const {
      check(); return *valuePtr == other;
    };

    /// Inequality operator.
    inline bool operator!=(const SafeUint160_t& other) const {
      check(); return *valuePtr != other.get();
    };

    /// Inequality operator.
    inline bool operator!=(const uint160_t& other) const {
      check(); return *valuePtr != other;
    };

    /// Inequality operator.
    inline bool operator!=(const uint64_t& other) const {
      check(); return *valuePtr != other;
    };

    /// Lesser operator.
    inline bool operator<(const SafeUint160_t& other) const {
      check(); return *valuePtr < other.get();
    };

    /// Lesser operator.
    inline bool operator<(const uint160_t& other) const {
      check(); return *valuePtr < other;
    };

    /// Lesser operator.
    inline bool operator<(const uint64_t& other) const {
      check(); return *valuePtr < other;
    };

    /// Lesser-or-equal operator.
    inline bool operator<=(const SafeUint160_t& other) const {
      check(); return *valuePtr <= other.get();
    };

    /// Lesser-or-equal operator.
    inline bool operator<=(const uint160_t& other) const {
      check(); return *valuePtr <= other;
    };

    /// Lesser-or-equal operator.
    inline bool operator<=(const uint64_t& other) const {
      check(); return *valuePtr <= other;
    };

    /// Greater operator.
    inline bool operator>(const SafeUint160_t& other) const {
      check(); return *valuePtr > other.get();
    };

    /// Greater operator.
    inline bool operator>(const uint160_t& other) const {
      check(); return *valuePtr > other;
    };

    /// Greater operator.
    inline bool operator>(const uint64_t& other) const {
      check(); return *valuePtr > other;
    };

    /// Greater-or-equal operator.
    inline bool operator>=(const SafeUint160_t& other) const {
      check(); return *valuePtr >= other.get();
    };

    /// Greater-or-equal operator.
    inline bool operator>=(const uint160_t& other) const {
      check(); return *valuePtr >= other;
    };

    /// Greater-or-equal operator.
    inline bool operator>=(const uint64_t& other) const {
      check(); return *valuePtr >= other;
    };

    // ====================
    // Assignment operators
    // ====================

    /// Assignment operator.
    inline SafeUint160_t& operator=(const SafeUint160_t& other) {
      check(); markAsUsed(); *valuePtr = other.get(); return *this;
    };

    /// Assignment operator.
    inline SafeUint160_t& operator=(const uint160_t& other) {
      check(); markAsUsed(); *valuePtr = other; return *this;
    };

    /// Assignment operator.
    inline SafeUint160_t& operator=(const uint64_t& other) {
      check(); markAsUsed(); *valuePtr = other; return *this;
    };

    /**
     * Addition assignment operator.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t& operator+=(const SafeUint160_t& other) {
      check();
      markAsUsed();
      if (*valuePtr > std::numeric_limits<uint160_t>::max() - other.get()) {
        throw std::overflow_error("Overflow in addition operation");
      }
      *valuePtr += other.get();
      return *this;
    };

    /**
     * Addition assignment operator.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t& operator+=(const uint160_t& other) {
      check();
      markAsUsed();
      if (*valuePtr > std::numeric_limits<uint160_t>::max() - other) {
        throw std::overflow_error("Overflow in addition operation");
      }
      *valuePtr += other;
      return *this;
    };

    /**
     * Addition assignment operator.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t& operator+=(const uint64_t& other) {
      check();
      markAsUsed();
      if (*valuePtr > std::numeric_limits<uint160_t>::max() - other) {
        throw std::overflow_error("Overflow in addition operation");
      }
      *valuePtr += other;
      return *this;
    };

    /**
     * Subtraction assignment operator.
     * @throw std::underflow_error if an underflow happens.
     */
    inline SafeUint160_t& operator-=(const SafeUint160_t& other) {
      check();
      markAsUsed();
      if (*valuePtr < other.get()) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      *valuePtr -= other.get();
      return *this;
    };

    /**
     * Subtraction assignment operator.
     * @throw std::underflow_error if an underflow happens.
     */
    inline SafeUint160_t& operator-=(const uint160_t& other) {
      check();
      markAsUsed();
      if (*valuePtr < other) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      *valuePtr -= other;
      return *this;
    };

    /**
     * Subtraction assignment operator.
     * @throw std::underflow_error if an underflow happens.
     */
    inline SafeUint160_t& operator-=(const uint64_t& other) {
      check();
      markAsUsed();
      if (*valuePtr < other) {
        throw std::underflow_error("Underflow in subtraction operation");
      }
      *valuePtr -= other;
      return *this;
    };

    /**
     * Multiplication assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t& operator*=(const SafeUint160_t& other) {
      check();
      markAsUsed();
      if (other.get() == 0 || *valuePtr == 0) throw std::domain_error("Multiplication by zero");
      if (*valuePtr > std::numeric_limits<uint160_t>::max() / other.get()) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      *valuePtr *= other.get();
      return *this;
    };

    /**
     * Multiplication assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t& operator*=(const uint160_t& other) {
      check();
      markAsUsed();
      if (other == 0 || *valuePtr == 0) throw std::domain_error("Multiplication by zero");
      if (*valuePtr > std::numeric_limits<uint160_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      *valuePtr *= other;
      return *this;
    };

    /**
     * Multiplication assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t& operator*=(const uint64_t& other) {
      check();
      markAsUsed();
      if (other == 0 || *valuePtr == 0) throw std::domain_error("Multiplication by zero");
      if (*valuePtr > std::numeric_limits<uint160_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation");
      }
      *valuePtr *= other;
      return *this;
    };

    /**
     * Division assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t& operator/=(const SafeUint160_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other.get() == 0) throw std::domain_error("Division by zero");
      *valuePtr /= other.get();
      return *this;
    };

    /**
     * Division assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t& operator/=(const uint160_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other == 0) throw std::domain_error("Division by zero");
      *valuePtr /= other;
      return *this;
    };

    /**
     * Division assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t& operator/=(const uint64_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other == 0) throw std::domain_error("Division by zero");
      *valuePtr /= other;
      return *this;
    };

    /**
     * Modulo assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t& operator%=(const SafeUint160_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other.get() == 0) throw std::domain_error("Modulo by zero");
      *valuePtr %= other.get();
      return *this;
    };

    /**
     * Modulo assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t& operator%=(const uint160_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other == 0) throw std::domain_error("Modulo by zero");
      *valuePtr %= other;
      return *this;
    };

    /**
     * Modulo assignment operator.
     * @throw std::domain_error if one of the operands is zero.
     */
    inline SafeUint160_t& operator%=(const uint64_t& other) {
      check();
      markAsUsed();
      if (*valuePtr == 0 || other == 0) throw std::domain_error("Modulo by zero");
      *valuePtr %= other;
      return *this;
    };

    /// Bitwise AND assignment operator.
    inline SafeUint160_t& operator&=(const SafeUint160_t& other) {
      check(); markAsUsed(); *valuePtr &= other.get(); return *this;
    };

    /// Bitwise AND assignment operator.
    inline SafeUint160_t& operator&=(const uint160_t& other) {
      check(); markAsUsed(); *valuePtr &= other; return *this;
    };

    /// Bitwise AND assignment operator.
    inline SafeUint160_t& operator&=(const uint64_t& other) {
      check(); markAsUsed(); *valuePtr &= other; return *this;
    };

    /// Bitwise OR assignment operator.
    inline SafeUint160_t& operator|=(const SafeUint160_t& other) {
      check(); markAsUsed(); *valuePtr |= other.get(); return *this;
    };

    /// Bitwise OR assignment operator.
    inline SafeUint160_t& operator|=(const uint160_t& other) {
      check(); markAsUsed(); *valuePtr |= other; return *this;
    };

    /// Bitwise OR assignment operator.
    inline SafeUint160_t& operator|=(const uint64_t& other) {
      check(); markAsUsed(); *valuePtr |= other; return *this;
    };

    /// Bitwise XOR assignment operator.
    inline SafeUint160_t& operator^=(const SafeUint160_t& other) {
      check(); markAsUsed(); *valuePtr ^= other.get(); return *this;
    };

    /// Bitwise XOR assignment operator.
    inline SafeUint160_t& operator^=(const uint160_t& other) {
      check(); markAsUsed(); *valuePtr ^= other; return *this;
    };

    /// Bitwise XOR assignment operator.
    inline SafeUint160_t& operator^=(const uint64_t& other) {
      check(); markAsUsed(); *valuePtr ^= other; return *this;
    };

    // =================================
    // Increment and decrement operators
    // =================================

    /**
     * Increment operator.
     * @throw std::overflow_error if an overflow happens.
     */
    inline SafeUint160_t& operator++() {
      check();
      markAsUsed();
      if (*valuePtr == std::numeric_limits<uint160_t>::max()) {
        throw std::overflow_error("Overflow in increment operation");
      }
      ++*valuePtr;
      return *this;
    };

    /**
     * Decrement operator.
     * @throw std::underflow_error if an underflow happens.
     */
    inline SafeUint160_t& operator--() {
      check();
      markAsUsed();
      if (*valuePtr == std::numeric_limits<uint160_t>::min()) {
        throw std::underflow_error("Underflow in decrement operation");
      }
      --*valuePtr;
      return *this;
    };
};

#endif  // SAFEUINT160_T_H