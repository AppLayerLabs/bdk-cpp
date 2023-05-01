#ifndef SAFEUINT8_T_H
#define SAFEUINT8_T_H

#include "safebase.h"
#include <memory>

/**
 * Safe wrapper for a uint8_t variable.
 * This class is used to safely store a uint8_t variable within a contract.
 * @see SafeBase
 */
class SafeUint8_t : public SafeBase {
private:
  uint8_t value; ///< value for the uint8_t variable.
  /// Check() require valuePtr to be mutable.
  mutable std::unique_ptr<uint8_t> valuePtr; ///< Pointer to the value.

  /**
  @brief Check if the valuePtr is initialized.
  If not, initialize it.
  */
  inline void check() const override {
    if (valuePtr == nullptr) {
      valuePtr = std::make_unique<uint8_t>(value);
    }
  };

public:
  /** Only Variables built with this constructor will be registered within a
  contract.
  @param owner The contract that owns this variable.
  @param value The uint8_t initial value (default: 0).
  */
  SafeUint8_t(DynamicContract *owner, const uint8_t &value = 0)
      : SafeBase(owner), value(0), valuePtr(std::make_unique<uint8_t>(value)){};

  /**
  @brief Default Constructor.
  @param value The uint8_t initial value (default: 0).
  */
  SafeUint8_t(const uint8_t &value = 0)
      : SafeBase(nullptr), value(0),
        valuePtr(std::make_unique<uint8_t>(value)){};

  /**
  @brief Constructor used to create a SafeUint8_t from another SafeUint8_t (copy
  constructor).
  @param other The SafeUint8_t to copy.
  */
  SafeUint8_t(const SafeUint8_t &other) : SafeBase(nullptr) {
    other.check();
    value = 0;
    valuePtr = std::make_unique<uint8_t>(*other.valuePtr);
  };

  /**
  @brief Sum operator used to add two SafeUint8_t.
  @param other The SafeUint8_t to add.
  @return A new SafeUint8_t with the result of the sum.
  @throws std::overflow_error if the sum overflows.
  */
  inline SafeUint8_t operator+(const SafeUint8_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint8_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint8_t(*valuePtr + other.get());
  };

  /**
  @brief Sum operator used to add a SafeUint8_t and a uint8_t.
  @param other The uint8_t to add.
  @return A new SafeUint8_t with the result of the sum.
  @throws std::overflow_error if the sum overflows.
  */
  inline SafeUint8_t operator+(const uint8_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint8_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint8_t(*valuePtr + other);
  };

  /**
  @brief Subtraction operator used to subtract two SafeUint8_t.
  @param other The SafeUint8_t to subtract.
  @return A new SafeUint8_t with the result of the subtraction.
  @throws std::underflow_error if the subtraction underflows.
  */
  inline SafeUint8_t operator-(const SafeUint8_t &other) const {
    check();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint8_t(*valuePtr - other.get());
  };

  /**
  @brief Subtraction operator used to subtract a SafeUint8_t and a uint8_t.
  @param other The uint8_t to subtract.
  @return A new SafeUint8_t with the result of the subtraction.
  @throws std::underflow_error if the subtraction underflows.
  */
  inline SafeUint8_t operator-(const uint8_t &other) const {
    check();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint8_t(*valuePtr - other);
  };

  /**
  @brief Multiplication operator used to multiply two SafeUint8_t.
  @param other The SafeUint8_t to multiply.
  @return A new SafeUint8_t with the result of the multiplication.
  @throws std::overflow_error if the multiplication overflows.
  @throws std::domain_error if one of the operands is zero.
  */
  inline SafeUint8_t operator*(const SafeUint8_t &other) const {
    check();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint8_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint8_t(*valuePtr * other.get());
  };

  /**
  @brief Multiplication operator used to multiply a SafeUint8_t and a uint8_t.
  @param other The uint8_t to multiply.
  @return A new SafeUint8_t with the result of the multiplication.
  @throws std::overflow_error if the multiplication overflows.
  @throws std::domain_error if one of the operands is zero.
  */
  inline SafeUint8_t operator*(const uint8_t &other) const {
    check();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint8_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint8_t(*valuePtr * other);
  };

  /**
  @brief Division operator used to divide two SafeUint8_t.
  @param other The SafeUint8_t to divide.
  @return A new SafeUint8_t with the result of the division.
  @throws std::domain_error if one of the operands is zero.
  @throws std::domain_error if the division has a remainder.
  */
  inline SafeUint8_t operator/(const SafeUint8_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint8_t(*valuePtr / other.get());
  };

  /**
  @brief Division operator used to divide a SafeUint8_t and a uint8_t.
  @param other The uint8_t to divide.
  @return A new SafeUint8_t with the result of the division.
  @throws std::domain_error if one of the operands is zero.
  */
  inline SafeUint8_t operator/(const uint8_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint8_t(*valuePtr / other);
  };

  /**
  @brief Modulo operator used to calculate the remainder of the division of two
  SafeUint8_t.
  @param other The SafeUint8_t to divide.
  @return A new SafeUint8_t with the remainder of the division.
  @throws std::domain_error if one of the operands is zero.
  */
  inline SafeUint8_t operator%(const SafeUint8_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint8_t(*valuePtr % other.get());
  };

  /**
  @brief Modulo operator used to calculate the remainder of the division of a
  SafeUint8_t and a uint8_t.
  @param other The uint8_t to divide.
  @return A new SafeUint8_t with the remainder of the division.
  @throws std::domain_error if one of the operands is zero.
  */
  inline SafeUint8_t operator%(const uint8_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint8_t(*valuePtr % other);
  };

  /**
  @brief Bitwise AND operator used to perform a bitwise AND operation on two
  SafeUint8_t.
  @param other The SafeUint8_t to perform the bitwise AND operation with.
  @return A new SafeUint8_t with the result of the bitwise AND operation.
  */
  inline SafeUint8_t operator&(const SafeUint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr & other.get());
  };

  /**
  @brief Bitwise AND operator used to perform a bitwise AND operation on a
  SafeUint8_t and a uint8_t.
  @param other The uint8_t to perform the bitwise AND operation with.
  @return A new SafeUint8_t with the result of the bitwise AND operation.
  */
  inline SafeUint8_t operator&(const uint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr & other);
  };

  /**
  @brief Bitwise OR operator used to perform a bitwise OR operation on two
  SafeUint8_t.
  @param other The SafeUint8_t to perform the bitwise OR operation with.
  @return A new SafeUint8_t with the result of the bitwise OR operation.
  */
  inline SafeUint8_t operator|(const SafeUint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr | other.get());
  };

  /**
  @brief Bitwise OR operator used to perform a bitwise OR operation on a
  SafeUint8_t and a uint8_t.
  @param other The uint8_t to perform the bitwise OR operation with.
  @return A new SafeUint8_t with the result of the bitwise OR operation.
  */
  inline SafeUint8_t operator|(const uint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr | other);
  };

  /**
  @brief Bitwise XOR operator used to perform a bitwise XOR operation on two
  SafeUint8_t.
  @param other The SafeUint8_t to perform the bitwise XOR operation with.
  @return A new SafeUint8_t with the result of the bitwise XOR operation.
  */
  inline SafeUint8_t operator^(const SafeUint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr ^ other.get());
  };

  /**
  @brief Bitwise XOR operator used to perform a bitwise XOR operation on a
  SafeUint8_t and a uint8_t.
  @param other The uint8_t to perform the bitwise XOR operation with.
  @return A new SafeUint8_t with the result of the bitwise XOR operation.
  */
  inline SafeUint8_t operator^(const uint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr ^ other);
  };

  /**
  @brief Left shift operator used to shift the bits of a SafeUint8_t to the
  left by the amount specified by another SafeUint8_t.
  @param other The SafeUint8_t specifying the amount to shift by.
  @return A new SafeUint8_t with the bits shifted by the specified amount.
  */
  inline SafeUint8_t operator<<(const SafeUint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr << other.get());
  };

  /**
  @brief Left shift operator used to shift the bits of a SafeUint8_t to the
  left by the amount specified by a uint8_t.
  @param other The uint8_t specifying the amount to shift by.
  @return A new SafeUint8_t with the bits shifted by the specified amount.
  */
  inline SafeUint8_t operator<<(const uint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr << other);
  };

  /**
  @brief Right shift operator used to shift the bits of a SafeUint8_t to the
  right by the amount specified by another SafeUint8_t.
  @param other The SafeUint8_t specifying the amount to shift by.
  @return A new SafeUint8_t with the bits shifted by the specified amount.
  */
  inline SafeUint8_t operator>>(const SafeUint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr >> other.get());
  };

  /**
  @brief Right shift operator used to shift the bits of a SafeUint8_t to the
  right by the amount specified by a uint8_t.
  @param other The uint8_t specifying the amount to shift by.
  @return A new SafeUint8_t with the bits shifted by the specified amount.
  */
  inline SafeUint8_t operator>>(const uint8_t &other) const {
    check();
    return SafeUint8_t(*valuePtr >> other);
  };

  /**
  @brief NOT operator used to perform a bitwise NOT operation on a SafeUint8_t.
  @return A new SafeUint8_t with the result of the bitwise NOT operation.
  */
  inline bool operator!() const {
    check();
    return !*valuePtr;
  };

  /**
  @brief AND operator used to perform a logical AND operation on two
  SafeUint8_t.
  @param other The SafeUint8_t to perform the logical AND operation with.
  @return A new SafeUint8_t with the result of the logical AND operation.
  */
  inline bool operator&&(const SafeUint8_t &other) const {
    check();
    return *valuePtr && other.get();
  };

  /**
  @brief AND operator used to perform a logical AND operation on a SafeUint8_t
  and a uint8_t.
  @param other The uint8_t to perform the logical AND operation with.
  @return A new SafeUint8_t with the result of the logical AND operation.
  */
  inline bool operator&&(const uint8_t &other) const {
    check();
    return *valuePtr && other;
  };

  /**
  @brief OR operator used to perform a logical OR operation on two
  SafeUint8_t.
  @param other The SafeUint8_t to perform the logical OR operation with.
  @return A new SafeUint8_t with the result of the logical OR operation.
  */
  inline bool operator||(const SafeUint8_t &other) const {
    check();
    return *valuePtr || other.get();
  };

  /**
  @brief OR operator used to perform a logical OR operation on a SafeUint8_t
  and a uint8_t.
  @param other The uint8_t to perform the logical OR operation with.
  @return A new SafeUint8_t with the result of the logical OR operation.
  */
  inline bool operator||(const uint8_t &other) const {
    check();
    return *valuePtr || other;
  };

  /**
  @brief Equality operator used to compare two SafeUint8_t.
  @param other The SafeUint8_t to compare with.
  @return True if the two SafeUint8_t are equal.
  */
  inline bool operator==(const SafeUint8_t &other) const {
    check();
    return *valuePtr == other.get();
  };

  /**
  @brief Equality operator used to compare a SafeUint8_t with a uint8_t.
  @param other The uint8_t to compare with.
  @return True if the two values are equal.
  */
  inline bool operator==(const uint8_t &other) const {
    check();
    return *valuePtr == other;
  };

  /**
  @brief Inequality operator used to compare two SafeUint8_t.
  @param other The SafeUint8_t to compare with.
  @return True if the two SafeUint8_t are not equal.
  */
  inline bool operator!=(const SafeUint8_t &other) const {
    check();
    return *valuePtr != other.get();
  };

  /**
  @brief Inequality operator used to compare a SafeUint8_t with a uint8_t.
  @param other The uint8_t to compare with.
  @return True if the two values are not equal.
  */
  inline bool operator!=(const uint8_t &other) const {
    check();
    return *valuePtr != other;
  };

  /**
  @brief Less than operator used to compare two SafeUint8_t.
  @param other The SafeUint8_t to compare with.
  @return True if the first SafeUint8_t is less than the second.
  */
  inline bool operator<(const SafeUint8_t &other) const {
    check();
    return *valuePtr < other.get();
  };

  /**
  @brief Less than operator used to compare a SafeUint8_t with a uint8_t.
  @param other The uint8_t to compare with.
  @return True if the first value is less than the second.
  */
  inline bool operator<(const uint8_t &other) const {
    check();
    return *valuePtr < other;
  };

  /**
  @brief Less than or equal to operator used to compare two SafeUint8_t.
  @param other The SafeUint8_t to compare with.
  @return True if the first SafeUint8_t is less than or equal to the second.
  */
  inline bool operator<=(const SafeUint8_t &other) const {
    check();
    return *valuePtr <= other.get();
  };

  /**
  @brief Less than or equal to operator used to compare a SafeUint8_t with a
  uint8_t.
  @param other The uint8_t to compare with.
  @return True if the first value is less than or equal to the second.
  */
  inline bool operator<=(const uint8_t &other) const {
    check();
    return *valuePtr <= other;
  };

  /**
  @brief Greater than operator used to compare two SafeUint8_t.
  @param other The SafeUint8_t to compare with.
  @return True if the first SafeUint8_t is greater than the second.
  */
  inline bool operator>(const SafeUint8_t &other) const {
    check();
    return *valuePtr > other.get();
  };

  /**
  @brief Greater than operator used to compare a SafeUint8_t with a uint8_t.
  @param other The uint8_t to compare with.
  @return True if the first value is greater than the second.
  */
  inline bool operator>(const uint8_t &other) const {
    check();
    return *valuePtr > other;
  };

  /**
  @brief Greater than or equal to operator used to compare two SafeUint8_t.
  @param other The SafeUint8_t to compare with.
  @return True if the first SafeUint8_t is greater than or equal to the second.
  */
  inline bool operator>=(const SafeUint8_t &other) const {
    check();
    return *valuePtr >= other.get();
  };

  /**
  @brief Greater than or equal to operator used to compare a SafeUint8_t with a
  uint8_t.
  @param other The uint8_t to compare with.
  @return True if the first value is greater than or equal to the second.
  */
  inline bool operator>=(const uint8_t &other) const {
    check();
    return *valuePtr >= other;
  };

  /**
  @brief Assignment operator used to set a SafeUint8_t equal to a SafeUint8_t.
  @param other The SafeUint8_t to set equal to.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    *valuePtr = other.get();
    return *this;
  };

  /**
  @brief Assignment operator used to set a SafeUint8_t equal to a uint8_t.
  @param other The uint8_t to set equal to.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator=(const uint8_t &other) {
    check();
    markAsUsed();
    *valuePtr = other;
    return *this;
  };

  /**
  @brief Plus assignment operator used to add a SafeUint8_t to a SafeUint8_t.
  @param other The SafeUint8_t to add to the first one.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::overflow_error if the operation results in an overflow.
  */
  inline SafeUint8_t &operator+=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint8_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other.get();
    return *this;
  };

  /**
  @brief Plus assignment operator used to add a uint8_t to a SafeUint8_t.
  @param other The uint8_t to add to the first one.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::overflow_error if the operation results in an overflow.
  */
  inline SafeUint8_t &operator+=(const uint8_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint8_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other;
    return *this;
  };

  /**
  @brief Minus assignment operator used to subtract a SafeUint8_t from a
  SafeUint8_t.
  @param other The SafeUint8_t to subtract from the first one.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::underflow_error if the operation results in an underflow.
  */
  inline SafeUint8_t &operator-=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other.get();
    return *this;
  };

  /**
  @brief Minus assignment operator used to subtract a uint8_t from a
  SafeUint8_t.
  @param other The uint8_t to subtract from the first one.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::underflow_error if the operation results in an underflow.
  */
  inline SafeUint8_t &operator-=(const uint8_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other;
    return *this;
  };

  /**
  @brief Multiplication assignment operator used to multiply a SafeUint8_t by a
  SafeUint8_t.
  @param other The SafeUint8_t to multiply the first one by.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::overflow_error if the operation results in an overflow.
  @throws std::domain_error if the operation results in a multiplication by
  */
  inline SafeUint8_t &operator*=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint8_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other.get();
    return *this;
  };

  /**
  @brief Multiplication assignment operator used to multiply a SafeUint8_t by a
  uint8_t.
  @param other The uint8_t to multiply the first one by.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::overflow_error if the operation results in an overflow.
  @throws std::domain_error if the operation results in a multiplication by
  */
  inline SafeUint8_t &operator*=(const uint8_t &other) {
    check();
    markAsUsed();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint8_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other;
    return *this;
  };

  /**
  @brief Division assignment operator used to divide a SafeUint8_t by a
  SafeUint8_t.
  @param other The SafeUint8_t to divide the first one by.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::domain_error if the operation results in a division by zero.
  */
  inline SafeUint8_t &operator/=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other.get();
    return *this;
  };

  /**
  @brief Division assignment operator used to divide a SafeUint8_t by a uint8_t.
  @param other The uint8_t to divide the first one by.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::domain_error if the operation results in a division by zero.
  */
  inline SafeUint8_t &operator/=(const uint8_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other;
    return *this;
  };

  /**
  @brief Modulo assignment operator used to modulo a SafeUint8_t by a
  SafeUint8_t.
  @param other The SafeUint8_t to modulo the first one by.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::domain_error if the operation results in a modulo by zero.
  */
  inline SafeUint8_t &operator%=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other.get();
    return *this;
  };

  /**
  @brief Modulo assignment operator used to modulo a SafeUint8_t by a uint8_t.
  @param other The uint8_t to modulo the first one by.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::domain_error if the operation results in a modulo by zero.
  */
  inline SafeUint8_t &operator%=(const uint8_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other;
    return *this;
  };

  /**
  @brief Bitwise AND assignment operator used to perform a bitwise AND
  operation on a SafeUint8_t and a SafeUint8_t.
  @param other The SafeUint8_t to AND the first one with.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator&=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other.get();
    return *this;
  };

  /**
  @brief Bitwise AND assignment operator used to perform a bitwise AND
  operation on a SafeUint8_t and a uint8_t.
  @param other The uint8_t to AND the first one with.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator&=(const uint8_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other;
    return *this;
  };

  /**
  @brief Bitwise OR assignment operator used to perform a bitwise OR operation
  on a SafeUint8_t and a SafeUint8_t.
  @param other The SafeUint8_t to OR the first one with.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator|=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other.get();
    return *this;
  };

  /**
  @brief Bitwise OR assignment operator used to perform a bitwise OR operation
  on a SafeUint8_t and a uint8_t.
  @param other The uint8_t to OR the first one with.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator|=(const uint8_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other;
    return *this;
  };

  /**
  @brief Bitwise XOR assignment operator used to perform a bitwise XOR
  operation on a SafeUint8_t and a SafeUint8_t.
  @param other The SafeUint8_t to XOR the first one with.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator^=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other.get();
    return *this;
  };

  /**
  @brief Bitwise XOR assignment operator used to perform a bitwise XOR
  operation on a SafeUint8_t and a uint8_t.
  @param other The uint8_t to XOR the first one with.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator^=(const uint8_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other;
    return *this;
  };

  /**
  @brief Shift left assignment operator used to perform a bitwise shift left
  operation on a SafeUint8_t and a SafeUint8_t.
  @param other The SafeUint8_t to shift the first one by.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator<<=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    *valuePtr <<= other.get();
    return *this;
  };

  /**
  @brief Shift left assignment operator used to perform a bitwise shift left
  operation on a SafeUint8_t and a uint8_t.
  @param other The uint8_t to shift the first one by.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator<<=(const uint8_t &other) {
    check();
    markAsUsed();
    *valuePtr <<= other;
    return *this;
  };

  /**
  @brief Shift right assignment operator used to perform a bitwise shift right
  operation on a SafeUint8_t and a SafeUint8_t.
  @param other The SafeUint8_t to shift the first one by.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator>>=(const SafeUint8_t &other) {
    check();
    markAsUsed();
    *valuePtr >>= other.get();
    return *this;
  };

  /**
  @brief Shift right assignment operator used to perform a bitwise shift right
  operation on a SafeUint8_t and a uint8_t.
  @param other The uint8_t to shift the first one by.
  @return A reference to the SafeUint8_t that was modified.
  */
  inline SafeUint8_t &operator>>=(const uint8_t &other) {
    check();
    markAsUsed();
    *valuePtr >>= other;
    return *this;
  };

  /** Prefix increment operator used to increment a SafeUint8_t by 1.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::overflow_error if the SafeUint8_t is already at its maximum
  */
  inline SafeUint8_t &operator++() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint8_t>::max()) {
      throw std::overflow_error("Overflow in increment operation");
    }
    ++*valuePtr;
    return *this;
  };

  /** Prefix decrement operator used to decrement a SafeUint8_t by 1.
  @return A reference to the SafeUint8_t that was modified.
  @throws std::underflow_error if the SafeUint8_t is already at its minimum
  */
  inline SafeUint8_t &operator--() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint8_t>::min()) {
      throw std::underflow_error("Underflow in increment operation");
    }
    --*valuePtr;
    return *this;
  };

  /**
  Getter function used to access the value of the SafeUint8_t.
  @return The value of the SafeUint8_t.
  */
  inline uint8_t get() const {
    check();
    return *valuePtr;
  };

  /**
  Commit function used to commit the value of the SafeUint8_t to the value
  pointed to.
  */
  inline void commit() override {
    check();
    value = *valuePtr;
    valuePtr = nullptr;
    registered = false;
  };

  /**
  Revert function used to revert the value of the SafeUint8_t (nullify it).
  */
  inline void revert() const override {
    valuePtr = nullptr;
    registered = false;
  };
};

#endif