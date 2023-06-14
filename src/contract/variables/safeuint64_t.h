#ifndef SAFEUINT64_T_H
#define SAFEUINT64_T_H

#include "safebase.h"
#include <memory>

/**
 * Safe wrapper for a uint64_t variable.
 * This class is used to safely store a uint64_t variable within a contract.
 * @see SafeBase
 */
class SafeUint64_t : public SafeBase {
private:
  uint64_t value; ///< value for the uint64_t variable.
  // Check() require valuePtr to be mutable.
  mutable std::unique_ptr<uint64_t> valuePtr; ///< Pointer to the value.

  /**
   *@brief Check if the valuePtr is initialized. If not, initialize it.
   */
  inline void check() const override {
    if (valuePtr == nullptr) {
      valuePtr = std::make_unique<uint64_t>(value);
    }
  };

public:
  /**
   *@brief Constructor for SafeUint64_t.
   * Only variables built with this constructor will be registered within a
   * contract.
   *@param owner The contract that owns this variable.
   *@param value The initial value of the variable.
   */
  SafeUint64_t(DynamicContract *owner, const uint64_t &value = 0)
      : SafeBase(owner), value(0),
        valuePtr(std::make_unique<uint64_t>(value)){};

  /**
   *@brief Normal Constructor for SafeUint64_t.
   *@param value The initial value of the variable.
   */
  SafeUint64_t(const uint64_t &value = 0)
      : SafeBase(nullptr), value(0),
        valuePtr(std::make_unique<uint64_t>(value)){};

  /**
   *@brief Constructor used to create a SafeUint64_t from another SafeUint64_t
   *(copy constructor).
   *@param other The SafeUint64_t to copy.
   */
  SafeUint64_t(const SafeUint64_t &other) : SafeBase(nullptr) {
    other.check();
    value = 0;
    valuePtr = std::make_unique<uint64_t>(*other.valuePtr);
  };

  // Arithmetic Operators

  /**
   *@brief Addition operator to add two SafeUint64_t.
   *@param other The SafeUint64_t to add.
   *@return A new SafeUint64_t with the result of the addition.
   *@throws std::overflow_error if the result of the addition is greater than
   *the maximum value of uint64_t.
   */
  inline SafeUint64_t operator+(const SafeUint64_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint64_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint64_t(*valuePtr + other.get());
  };

  /**
   *@brief Addition operator to add a SafeUint64_t and a uint64_t.
   *@param other The uint64_t to add.
   *@return A new SafeUint64_t with the result of the addition.
   *@throws std::overflow_error if the result of the addition is greater than
   *the maximum value of uint64_t.
   */
  inline SafeUint64_t operator+(const uint64_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint64_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint64_t(*valuePtr + other);
  };

  /**
   *@brief Subtraction operator to subtract two SafeUint64_t.
   *@param other The SafeUint64_t to subtract.
   *@return A new SafeUint64_t with the result of the subtraction.
   *@throws std::underflow_error if the result of the subtraction is less than
   *zero.
   */
  inline SafeUint64_t operator-(const SafeUint64_t &other) const {
    check();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint64_t(*valuePtr - other.get());
  };

  /**
   *@brief Subtraction operator to subtract a SafeUint64_t and a uint64_t.
   *@param other The uint64_t to subtract.
   *@return A new SafeUint64_t with the result of the subtraction.
   *@throws std::underflow_error if the result of the subtraction is less than
   *zero.
   */
  inline SafeUint64_t operator-(const uint64_t &other) const {
    check();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint64_t(*valuePtr - other);
  };

  /**
   *@brief Multiplication operator to multiply two SafeUint64_t.
   *@param other The SafeUint64_t to multiply.
   *@return A new SafeUint64_t with the result of the multiplication.
   *@throws std::overflow_error if the result of the multiplication is greater
   *than the maximum value of uint64_t.
   *@throws std::domain_error if one of the SafeUint64_t is zero.
   */
  inline SafeUint64_t operator*(const SafeUint64_t &other) const {
    check();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint64_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint64_t(*valuePtr * other.get());
  };

  /**
   *@brief Multiplication operator to multiply a SafeUint64_t and a uint64_t.
   *@param other The uint64_t to multiply.
   *@return A new SafeUint64_t with the result of the multiplication.
   *@throws std::overflow_error if the result of the multiplication is greater
   *than the maximum value of uint64_t.
   *@throws std::domain_error if one of the SafeUint64_t is zero.
   */
  inline SafeUint64_t operator*(const uint64_t &other) const {
    check();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint64_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint64_t(*valuePtr * other);
  };

  /**
   *@brief Division operator to divide two SafeUint64_t.
   *@param other The SafeUint64_t to divide.
   *@return A new SafeUint64_t with the result of the division.
   *@throws std::domain_error if one of the SafeUint64_t is zero.
   */
  inline SafeUint64_t operator/(const SafeUint64_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint64_t(*valuePtr / other.get());
  };

  /**
   *@brief Division operator to divide a SafeUint64_t and a uint64_t.
   *@param other The uint64_t to divide.
   *@return A new SafeUint64_t with the result of the division.
   *@throws std::domain_error if one of the SafeUint64_t is zero.
   */
  inline SafeUint64_t operator/(const uint64_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint64_t(*valuePtr / other);
  };

  /**
   *@brief Modulo operator to get the remainder of two SafeUint64_t.
   *@param other The SafeUint64_t to get the remainder of.
   *@return A new SafeUint64_t with the remainder.
   *@throws std::domain_error if one of the SafeUint64_t is zero.
   */
  inline SafeUint64_t operator%(const SafeUint64_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint64_t(*valuePtr % other.get());
  };

  /**
   *@brief Modulo operator to get the remainder of a SafeUint64_t and a
   *uint64_t.
   *@param other The uint64_t to get the remainder of.
   *@return A new SafeUint64_t with the remainder.
   *@throws std::domain_error if one of the SafeUint64_t is zero.
   */
  inline SafeUint64_t operator%(const uint64_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint64_t(*valuePtr % other);
  };

  // Bitwise Operators

  /**
   *@brief Bitwise AND operator to perform a bitwise AND operation on two
   *SafeUint64_t.
   *@param other The SafeUint64_t to perform a bitwise AND operation with.
   *@return A new SafeUint64_t with the result of the bitwise AND operation.
   */
  inline SafeUint64_t operator&(const SafeUint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr & other.get());
  };

  /**
   *@brief Bitwise AND operator to perform a bitwise AND operation on a
   *SafeUint64_t and a uint64_t.
   *@param other The uint64_t to perform a bitwise AND operation with.
   *@return A new SafeUint64_t with the result of the bitwise AND operation.
   */
  inline SafeUint64_t operator&(const uint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr & other);
  };

  /**
   *@brief Bitwise OR operator to perform a bitwise OR operation on two
   *SafeUint64_t.
   *@param other The SafeUint64_t to perform a bitwise OR operation with.
   *@return A new SafeUint64_t with the result of the bitwise OR operation.
   */
  inline SafeUint64_t operator|(const SafeUint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr | other.get());
  };

  /**
   *@brief Bitwise OR operator to perform a bitwise OR operation on a
   *SafeUint64_t and a uint64_t.
   *@param other The uint64_t to perform a bitwise OR operation with.
   *@return A new SafeUint64_t with the result of the bitwise OR operation.
   */
  inline SafeUint64_t operator|(const uint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr | other);
  };

  /**
   *@brief Bitwise XOR operator to perform a bitwise XOR operation on two
   *SafeUint64_t.
   *@param other The SafeUint64_t to perform a bitwise XOR operation with.
   *@return A new SafeUint64_t with the result of the bitwise XOR operation.
   */
  inline SafeUint64_t operator^(const SafeUint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr ^ other.get());
  };

  /**
   *@brief Bitwise XOR operator to perform a bitwise XOR operation on a
   *SafeUint64_t and a uint64_t.
   *@param other The uint64_t to perform a bitwise XOR operation with.
   *@return A new SafeUint64_t with the result of the bitwise XOR operation.
   */
  inline SafeUint64_t operator^(const uint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr ^ other);
  };

  /**
   *@brief Left shift operator to shift the bits of a SafeUint64_t to the left
   *using another SafeUint64_t.
   *@param other The SafeUint64_t to shift the bits with.
   *@return A new SafeUint64_t with the shifted bits.
   */
  inline SafeUint64_t operator<<(const SafeUint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr << other.get());
  };

  /**
   *@brief Left shift operator to shift the bits of a SafeUint64_t to the left
   *using a uint64_t.
   *@param other The uint64_t to shift the bits with.
   *@return A new SafeUint64_t with the shifted bits.
   */
  inline SafeUint64_t operator<<(const uint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr << other);
  };

  /**
   *@brief Right shift operator to shift the bits of a SafeUint64_t to the right
   *using another SafeUint64_t.
   *@param other The SafeUint64_t to shift the bits with.
   *@return A new SafeUint64_t with the shifted bits.
   */
  inline SafeUint64_t operator>>(const SafeUint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr >> other.get());
  };

  /**
   *@brief Right shift operator to shift the bits of a SafeUint64_t to the right
   *using a uint64_t.
   *@param other The uint64_t to shift the bits with.
   *@return A new SafeUint64_t with the shifted bits.
   */
  inline SafeUint64_t operator>>(const uint64_t &other) const {
    check();
    return SafeUint64_t(*valuePtr >> other);
  };

  // Logical Operators

  /**
   *@brief Logical NOT operator to get the logical NOT of a SafeUint64_t.
   *@return A new SafeUint64_t with the logical NOT.
   */
  inline bool operator!() const {
    check();
    return !*valuePtr;
  };

  /**
   *@brief Logical AND operator to perform a logical AND operation on two
   *SafeUint64_t.
   *@param other The SafeUint64_t to perform a logical AND operation with.
   *@return A new SafeUint64_t with the result of the logical AND operation
   *(true if both are true).
   */
  inline bool operator&&(const SafeUint64_t &other) const {
    check();
    return *valuePtr && other.get();
  };

  /**
   *@brief Logical AND operator to perform a logical AND operation on a
   *SafeUint64_t and a uint64_t.
   *@param other The uint64_t to perform a logical AND operation with.
   *@return A new SafeUint64_t with the result of the logical AND operation
   *(true if both are true).
   */
  inline bool operator&&(const uint64_t &other) const {
    check();
    return *valuePtr && other;
  };

  /**
   *@brief Logical OR operator to perform a logical OR operation on two
   *SafeUint64_t.
   *@param other The SafeUint64_t to perform a logical OR operation with.
   *@return A new SafeUint64_t with the result of the logical OR operation (true
   *if either are true).
   */
  inline bool operator||(const SafeUint64_t &other) const {
    check();
    return *valuePtr || other.get();
  };

  /**
   *@brief Logical OR operator to perform a logical OR operation on a
   *SafeUint64_t and a uint64_t.
   *@param other The uint64_t to perform a logical OR operation with.
   *@return A new SafeUint64_t with the result of the logical OR operation (true
   *if either are true).
   */
  inline bool operator||(const uint64_t &other) const {
    check();
    return *valuePtr || other;
  };

  // Comparison Operators

  /**
   *@brief Equality operator to check if two SafeUint64_t are equal.
   *@param other The SafeUint64_t to check equality with.
   *@return True if the SafeUint64_t are equal.
   */
  inline bool operator==(const SafeUint64_t &other) const {
    check();
    return *valuePtr == other.get();
  };

  /**
   *@brief Equality operator to check if a SafeUint64_t and a uint64_t are
   *equal.
   *@param other The uint64_t to check equality with.
   *@return True if the SafeUint64_t and uint64_t are equal.
   */
  inline bool operator==(const uint64_t &other) const {
    check();
    return *valuePtr == other;
  };

  /**
   *@brief Inequality operator to check if two SafeUint64_t are not equal.
   *@param other The SafeUint64_t to check inequality with.
   *@return True if the SafeUint64_t are not equal.
   */
  inline bool operator!=(const SafeUint64_t &other) const {
    check();
    return *valuePtr != other.get();
  };

  /**
   *@brief Inequality operator to check if a SafeUint64_t and a uint64_t are not
   *equal.
   *@param other The uint64_t to check inequality with.
   *@return True if the SafeUint64_t and uint64_t are not equal.
   */
  inline bool operator!=(const uint64_t &other) const {
    check();
    return *valuePtr != other;
  };

  /**
   *@brief Less than operator to check if one SafeUint64_t is less than another.
   *@param other The SafeUint64_t to check if this SafeUint64_t is less than.
   *@return True if this SafeUint64_t is less than the other.
   */
  inline bool operator<(const SafeUint64_t &other) const {
    check();
    return *valuePtr < other.get();
  };

  /**
   *@brief Less than operator to check if a SafeUint64_t is less than a
   *uint64_t.
   *@param other The uint64_t to check if this SafeUint64_t is less than.
   *@return True if this SafeUint64_t is less than the other.
   */
  inline bool operator<(const uint64_t &other) const {
    check();
    return *valuePtr < other;
  };

  /**
   *@brief GLess than or equal to operator to check if one SafeUint64_t is less
   *than or equal to another.
   *@param other The SafeUint64_t to check if this SafeUint64_t is less than or
   *equal to.
   *@return True if this SafeUint64_t is less than or equal to the other.
   */
  inline bool operator<=(const SafeUint64_t &other) const {
    check();
    return *valuePtr <= other.get();
  };

  /**
   *@brief Less than or equal to operator to check if a SafeUint64_t is less
   *than or equal to a uint64_t.
   *@param other The uint64_t to check if this SafeUint64_t is less than or
   *equal to.
   *@return True if this SafeUint64_t is less than or equal to the other.
   */
  inline bool operator<=(const uint64_t &other) const {
    check();
    return *valuePtr <= other;
  };

  /**
   *@brief Greater than operator to check if one SafeUint64_t is greater than
   *another.
   *@param other The SafeUint64_t to check if this SafeUint64_t is greater than.
   *@return True if this SafeUint64_t is greater than the other.
   */
  inline bool operator>(const SafeUint64_t &other) const {
    check();
    return *valuePtr > other.get();
  };

  /**
   *@brief Greater than operator to check if a SafeUint64_t is greater than a
   *uint64_t.
   *@param other The uint64_t to check if this SafeUint64_t is greater than.
   *@return True if this SafeUint64_t is greater than the other.
   */
  inline bool operator>(const uint64_t &other) const {
    check();
    return *valuePtr > other;
  };

  /**
   *@brief Greater than or equal to operator to check if one SafeUint64_t is
   *greater than or equal to another.
   *@param other The SafeUint64_t to check if this SafeUint64_t is greater than
   *or equal to.
   *@return True if this SafeUint64_t is greater than or equal to the other.
   */
  inline bool operator>=(const SafeUint64_t &other) const {
    check();
    return *valuePtr >= other.get();
  };

  /**
   *@brief Greater than or equal to operator to check if a SafeUint64_t is
   *greater than or equal to a uint64_t.
   *@param other The uint64_t to check if this SafeUint64_t is greater than or
   *equal to.
   *@return True if this SafeUint64_t is greater than or equal to the other.
   */
  inline bool operator>=(const uint64_t &other) const {
    check();
    return *valuePtr >= other;
  };

  // Assignment Operators

  /**
   *@brief Assignment operator to set this SafeUint64_t to the value of another
   *SafeUint64_t.
   *@param other The SafeUint64_t to set this SafeUint64_t to.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    *valuePtr = other.get();
    return *this;
  };

  /**
   *@brief Assignment operator to set this SafeUint64_t to the value of a
   *uint64_t.
   *@param other The uint64_t to set this SafeUint64_t to.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr = other;
    return *this;
  };

  /**
   *@brief Addition assignment operator to add another SafeUint64_t to this
   *SafeUint64_t.
   *@param other The SafeUint64_t to add to this SafeUint64_t.
   *@return A reference to this SafeUint64_t.
   *@throws overflow_error if there is overflow.
   */
  inline SafeUint64_t &operator+=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint64_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other.get();
    return *this;
  };

  /**
   *@brief Addition assignment operator to add a uint64_t to this SafeUint64_t.
   *@param other The uint64_t to add to this SafeUint64_t.
   *@return A reference to this SafeUint64_t.
   *@throws overflow_error if there is overflow.
   */
  inline SafeUint64_t &operator+=(const uint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint64_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other;
    return *this;
  };

  /**
   *@brief Subtraction assignment operator to subtract another SafeUint64_t from
   *this SafeUint64_t.
   *@param other The SafeUint64_t to subtract from this SafeUint64_t.
   *@return A reference to this SafeUint64_t.
   *@throws underflow_error if there is underflow.
   */
  inline SafeUint64_t &operator-=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other.get();
    return *this;
  };

  /**
   *@brief Subtraction assignment operator to subtract a uint64_t from this
   *SafeUint64_t.
   *@param other The uint64_t to subtract from this SafeUint64_t.
   *@return A reference to this SafeUint64_t.
   *@throws underflow_error if there is underflow.
   */
  inline SafeUint64_t &operator-=(const uint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other;
    return *this;
  };

  /**
   *@brief Multiplication assignment operator to multiply this SafeUint64_t by
   *another SafeUint64_t.
   *@param other The SafeUint64_t to multiply this SafeUint64_t by.
   *@return A reference to this SafeUint64_t.
   *@throws overflow_error if there is overflow.
   *@throws domain_error if there is multiplication by zero.
   */
  inline SafeUint64_t &operator*=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint64_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other.get();
    return *this;
  };

  /**
   *@brief Multiplication assignment operator to multiply this SafeUint64_t by a
   *uint64_t.
   *@param other The uint64_t to multiply this SafeUint64_t by.
   *@return A reference to this SafeUint64_t.
   *@throws overflow_error if there is overflow.
   *@throws domain_error if there is multiplication by zero.
   */
  inline SafeUint64_t &operator*=(const uint64_t &other) {
    check();
    markAsUsed();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint64_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other;
    return *this;
  };

  /**
   *@brief Division assignment operator to divide this SafeUint64_t by another
   *SafeUint64_t.
   *@param other The SafeUint64_t to divide this SafeUint64_t by.
   *@return A reference to this SafeUint64_t.
   *@throws domain_error if there is division by zero.
   */
  inline SafeUint64_t &operator/=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other.get();
    return *this;
  };

  /**
   *@brief Division assignment operator to divide this SafeUint64_t by a
   *uint64_t.
   *@param other The uint64_t to divide this SafeUint64_t by.
   *@return A reference to this SafeUint64_t.
   *@throws domain_error if there is division by zero.
   */
  inline SafeUint64_t &operator/=(const uint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other;
    return *this;
  };

  /**
   *@brief Modulo assignment operator to modulo this SafeUint64_t by another
   *SafeUint64_t.
   *@param other The SafeUint64_t to modulo this SafeUint64_t by.
   *@return A reference to this SafeUint64_t.
   *@throws domain_error if there is modulo by zero.
   */
  inline SafeUint64_t &operator%=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other.get();
    return *this;
  };

  /**
   *@brief Modulo assignment operator to modulo this SafeUint64_t by a
   *uint64_t.
   *@param other The uint64_t to modulo this SafeUint64_t by.
   *@return A reference to this SafeUint64_t.
   *@throws domain_error if there is modulo by zero.
   */
  inline SafeUint64_t &operator%=(const uint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other;
    return *this;
  };

  /**
   *@brief Bitwise AND assignment operator to AND this SafeUint64_t with another
   *SafeUint64_t.
   *@param other The SafeUint64_t to AND this SafeUint64_t with.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator&=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other.get();
    return *this;
  };

  /**
   *@brief Bitwise AND assignment operator to AND this SafeUint64_t with a
   *uint64_t.
   *@param other The uint64_t to AND this SafeUint64_t with.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator&=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other;
    return *this;
  };

  /**
   *@brief Bitwise OR assignment operator to OR this SafeUint64_t with another
   *SafeUint64_t.
   *@param other The SafeUint64_t to OR this SafeUint64_t with.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator|=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other.get();
    return *this;
  };

  /**
   *@brief Bitwise OR assignment operator to OR this SafeUint64_t with a
   *uint64_t.
   *@param other The uint64_t to OR this SafeUint64_t with.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator|=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other;
    return *this;
  };

  /**
   *@brief Bitwise XOR assignment operator to XOR this SafeUint64_t with another
   *SafeUint64_t.
   *@param other The SafeUint64_t to XOR this SafeUint64_t with.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator^=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other.get();
    return *this;
  };

  /**
   *@brief Bitwise XOR assignment operator to XOR this SafeUint64_t with a
   *uint64_t.
   *@param other The uint64_t to XOR this SafeUint64_t with.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator^=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other;
    return *this;
  };

  /**
   *@brief Left shift assignment operator to shift this SafeUint64_t to the
   *left by another SafeUint64_t.
   *@param other The SafeUint64_t to shift this SafeUint64_t to the left by.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator<<=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    *valuePtr <<= other.get();
    return *this;
  };

  /**
   *@brief Left shift assignment operator to shift this SafeUint64_t to the
   *left by a uint64_t.
   *@param other The uint64_t to shift this SafeUint64_t to the left by.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator<<=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr <<= other;
    return *this;
  };

  /**
   *@brief Right shift assignment operator to shift this SafeUint64_t to the
   *right by another SafeUint64_t.
   *@param other The SafeUint64_t to shift this SafeUint64_t to the right by.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator>>=(const SafeUint64_t &other) {
    check();
    markAsUsed();
    *valuePtr >>= other.get();
    return *this;
  };

  /**
   *@brief Right shift assignment operator to shift this SafeUint64_t to the
   *right by a uint64_t.
   *@param other The uint64_t to shift this SafeUint64_t to the right by.
   *@return A reference to this SafeUint64_t.
   */
  inline SafeUint64_t &operator>>=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr >>= other;
    return *this;
  };

  // Increment and Decrement Operators

  /**
   *@brief Prefix increment operator.
   *@return A reference to this SafeUint64_t.
   *@throws std::overflow_error if incrementing this SafeUint64_t would result
   *in a value greater than max.
   */
  inline SafeUint64_t &operator++() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint64_t>::max()) {
      throw std::overflow_error("Overflow in increment operation");
    }
    ++*valuePtr;
    return *this;
  };

  /**
   *@brief Prefix decrement operator.
   *@return A reference to this SafeUint64_t.
   *@throws std::underflow_error if decrementing this SafeUint64_t would result
   *in a value less than min.
   */
  inline SafeUint64_t &operator--() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint64_t>::min()) {
      throw std::underflow_error("Underflow in increment operation");
    }
    --*valuePtr;
    return *this;
  };

  /**
  Getter function used to access the value of the SafeUint64_t.
  @return The value of the SafeUint64_t.
  */
  inline uint64_t get() const {
    check();
    return *valuePtr;
  };

  /**
  Commit function used to commit the value of the SafeUint64_t to the value
  pointed to.
  */
  inline void commit() override {
    check();
    value = *valuePtr;
    valuePtr = nullptr;
    registered = false;
  };

  /**
  Revert function used to revert the value of the SafeUint64_t (nullify it).
  */
  inline void revert() const override {
    valuePtr = nullptr;
    registered = false;
  };
};

#endif