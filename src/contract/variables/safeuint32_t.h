#ifndef SAFEUINT32_T_H
#define SAFEUINT32_T_H

#include "safebase.h"
#include <memory>

/**
 * Safe wrapper for a uint32_t variable.
 * This class is used to safely store a uint32_t variable within a contract.
 * @see SafeBase
 */
class SafeUint32_t : public SafeBase {
private:
  uint32_t value; ///< value for the uint32_t variable.
  // Check() require valuePtr to be mutable.
  mutable std::unique_ptr<uint32_t> valuePtr; ///< Pointer to the value.

  /**
   @brief Check if the valuePtr is initialized. If not, initialize it.
   */
  inline void check() const override {
    if (valuePtr == nullptr) {
      valuePtr = std::make_unique<uint32_t>(value);
    }
  };

public:
  /**
   *@brief Constructor for SafeUint32_t.
   * Only variables built with this constructor will be registered within a
   * contract.
   *@param owner The contract that owns this variable.
   *@param value The initial value of the variable.
   */
  SafeUint32_t(DynamicContract *owner, const uint32_t &value = 0)
      : SafeBase(owner), value(0),
        valuePtr(std::make_unique<uint32_t>(value)){};

  /**
   *@brief Normal Constructor for SafeUint32_t.
   *@param value The initial value of the variable.
   */
  SafeUint32_t(const uint32_t &value = 0)
      : SafeBase(nullptr), value(0),
        valuePtr(std::make_unique<uint32_t>(value)){};

  /**
   *@brief Constructor used to create a SafeUint32_t from another SafeUint32_t
   *(copy constructor).
   *@param other The SafeUint32_t to copy.
   */
  SafeUint32_t(const SafeUint32_t &other) : SafeBase(nullptr) {
    other.check();
    value = 0;
    valuePtr = std::make_unique<uint32_t>(*other.valuePtr);
  };

  // Arithmetic Operators

  /**
   * @brief Addition operator to add two SafeUint32_t.
   * @param other The SafeUint32_t to add.
   * @return A new SafeUint32_t with the result of the addition.
   * @throws std::overflow_error if the result of the addition is greater than
   * the maximum value of uint32_t.
   */
  inline SafeUint32_t operator+(const SafeUint32_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint32_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint32_t(*valuePtr + other.get());
  };

  /**
   * @brief Addition operator to add a SafeUint32_t and a uint32_t.
   * @param other The uint32_t to add.
   * @return A new SafeUint32_t with the result of the addition.
   * @throws std::overflow_error if the result of the addition is greater than
   * the maximum value of uint32_t.
   */
  inline SafeUint32_t operator+(const uint32_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint32_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint32_t(*valuePtr + other);
  };

  /**
   * @brief Subtraction operator to subtract two SafeUint32_t.
   * @param other The SafeUint32_t to subtract.
   * @return A new SafeUint32_t with the result of the subtraction.
   * @throws std::underflow_error if the result of the subtraction is less than
   * zero.
   */
  inline SafeUint32_t operator-(const SafeUint32_t &other) const {
    check();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint32_t(*valuePtr - other.get());
  };

  /**
   * @brief Subtraction operator to subtract a SafeUint32_t and a uint32_t.
   * @param other The uint32_t to subtract.
   * @return A new SafeUint32_t with the result of the subtraction.
   * @throws std::underflow_error if the result of the subtraction is less than
   * zero.
   */
  inline SafeUint32_t operator-(const uint32_t &other) const {
    check();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint32_t(*valuePtr - other);
  };

  /**
   * @brief Multiplication operator to multiply two SafeUint32_t.
   * @param other The SafeUint32_t to multiply.
   * @return A new SafeUint32_t with the result of the multiplication.
   * @throws std::overflow_error if the result of the multiplication is greater
   * than the maximum value of uint32_t.
   * @throws std::domain_error if one of the SafeUint32_t is zero.
   */
  inline SafeUint32_t operator*(const SafeUint32_t &other) const {
    check();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint32_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint32_t(*valuePtr * other.get());
  };

  /**
   * @brief Multiplication operator to multiply a SafeUint32_t and a uint32_t.
   * @param other The uint32_t to multiply.
   * @return A new SafeUint32_t with the result of the multiplication.
   * @throws std::overflow_error if the result of the multiplication is greater
   * than the maximum value of uint32_t.
   * @throws std::domain_error if one of the SafeUint32_t is zero.
   */
  inline SafeUint32_t operator*(const uint32_t &other) const {
    check();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint32_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint32_t(*valuePtr * other);
  };

  /**
   * @brief Division operator to divide two SafeUint32_t.
   * @param other The SafeUint32_t to divide.
   * @return A new SafeUint32_t with the result of the division.
   * @throws std::domain_error if one of the SafeUint32_t is zero.
   */
  inline SafeUint32_t operator/(const SafeUint32_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint32_t(*valuePtr / other.get());
  };

  /**
   * @brief Division operator to divide a SafeUint32_t and a uint32_t.
   * @param other The uint32_t to divide.
   * @return A new SafeUint32_t with the result of the division.
   * @throws std::domain_error if one of the SafeUint32_t is zero.
   */
  inline SafeUint32_t operator/(const uint32_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint32_t(*valuePtr / other);
  };

  /**
   * @brief Modulo operator to calculate the remainder of the division of two
   * SafeUint32_t.
   * @param other The SafeUint32_t to calculate the remainder with.
   * @return A new SafeUint32_t with the result of the modulo operation.
   * @throws std::domain_error if one of the SafeUint32_t is zero.
   */
  inline SafeUint32_t operator%(const SafeUint32_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint32_t(*valuePtr % other.get());
  };

  /**
   * @brief Modulo operator to calculate the remainder of the division of a
   * SafeUint32_t and a uint32_t.
   * @param other The uint32_t to calculate the remainder with.
   * @return A new SafeUint32_t with the result of the modulo operation.
   * @throws std::domain_error if one of the SafeUint32_t is zero.
   */
  inline SafeUint32_t operator%(const uint32_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint32_t(*valuePtr % other);
  };

  // Bitwise Operators

  /**
   * @brief Bitwise AND operator to perform a bitwise AND operation on two
   * SafeUint32_t.
   * @param other The SafeUint32_t to perform the bitwise AND operation with.
   * @return A new SafeUint32_t with the result of the bitwise AND operation.
   */
  inline SafeUint32_t operator&(const SafeUint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr & other.get());
  };

  /**
   * @brief Bitwise AND operator to perform a bitwise AND operation on a
   * SafeUint32_t and a uint32_t.
   * @param other The uint32_t to perform the bitwise AND operation with.
   * @return A new SafeUint32_t with the result of the bitwise AND operation.
   */
  inline SafeUint32_t operator&(const uint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr & other);
  };

  /**
   * @brief Bitwise OR operator to perform a bitwise OR operation on two
   * SafeUint32_t.
   * @param other The SafeUint32_t to perform the bitwise OR operation with.
   * @return A new SafeUint32_t with the result of the bitwise OR operation.
   */
  inline SafeUint32_t operator|(const SafeUint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr | other.get());
  };

  /**
   * @brief Bitwise OR operator to perform a bitwise OR operation on a
   * SafeUint32_t and a uint32_t.
   * @param other The uint32_t to perform the bitwise OR operation with.
   * @return A new SafeUint32_t with the result of the bitwise OR operation.
   */
  inline SafeUint32_t operator|(const uint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr | other);
  };

  /**
   * @brief Bitwise XOR operator to perform a bitwise XOR operation on two
   * SafeUint32_t.
   * @param other The SafeUint32_t to perform the bitwise XOR operation with.
   * @return A new SafeUint32_t with the result of the bitwise XOR operation.
   */
  inline SafeUint32_t operator^(const SafeUint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr ^ other.get());
  };

  /**
   * @brief Bitwise XOR operator to perform a bitwise XOR operation on a
   * SafeUint32_t and a uint32_t.
   * @param other The uint32_t to perform the bitwise XOR operation with.
   * @return A new SafeUint32_t with the result of the bitwise XOR operation.
   */
  inline SafeUint32_t operator^(const uint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr ^ other);
  };

  /**
   * @brief Left shift operator to shift the bits of a SafeUint32_t to the left
   * by the amount specified by another SafeUint32_t.
   * @param other The SafeUint32_t specifying the amount of bits to shift.
   * @return A new SafeUint32_t with the result of the left shift operation.
   */
  inline SafeUint32_t operator<<(const SafeUint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr << other.get());
  };

  /**
   * @brief Left shift operator to shift the bits of a SafeUint32_t to the left
   * by the amount specified by a uint32_t.
   * @param other The uint32_t specifying the amount of bits to shift.
   * @return A new SafeUint32_t with the result of the left shift operation.
   */
  inline SafeUint32_t operator<<(const uint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr << other);
  };

  /**
   * @brief Right shift operator to shift the bits of a SafeUint32_t to the
   * right by the amount specified by another SafeUint32_t.
   * @param other The SafeUint32_t specifying the amount of bits to shift.
   * @return A new SafeUint32_t with the result of the right shift operation.
   */
  inline SafeUint32_t operator>>(const SafeUint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr >> other.get());
  };

  /**
   * @brief Right shift operator to shift the bits of a SafeUint32_t to the
   * right by the amount specified by a uint32_t.
   * @param other The uint32_t specifying the amount of bits to shift.
   * @return A new SafeUint32_t with the result of the right shift operation.
   */
  inline SafeUint32_t operator>>(const uint32_t &other) const {
    check();
    return SafeUint32_t(*valuePtr >> other);
  };

  // Logical Operators

  /**
   * @brief Logical NOT operator to perform a logical NOT operation on a
   * SafeUint32_t.
   * @return The value of the SafeUint32_t after the logical NOT operation.
   */
  inline bool operator!() const {
    check();
    return !*valuePtr;
  };

  /**
   * @brief Logical AND operator to perform a logical AND operation on two
   * SafeUint32_t.
   * @param other The SafeUint32_t to perform the logical AND operation with.
   * @return The result of the logical AND operation (true if both SafeUint32_t
   * are true, false otherwise)
   */
  inline bool operator&&(const SafeUint32_t &other) const {
    check();
    return *valuePtr && other.get();
  };

  /**
   * @brief Logical AND operator to perform a logical AND operation on a
   * SafeUint32_t and a uint32_t.
   * @param other The uint32_t to perform the logical AND operation with.
   * @return The result of the logical AND operation (true if both SafeUint32_t
   * and uint32_t are true, false otherwise)
   */
  inline bool operator&&(const uint32_t &other) const {
    check();
    return *valuePtr && other;
  };

  /**
   * @brief Logical OR operator to perform a logical OR operation on two
   * SafeUint32_t.
   * @param other The SafeUint32_t to perform the logical OR operation with.
   * @return The result of the logical OR operation (true if either SafeUint32_t
   * is true, false otherwise)
   */
  inline bool operator||(const SafeUint32_t &other) const {
    check();
    return *valuePtr || other.get();
  };

  /**
   * @brief Logical OR operator to perform a logical OR operation on a
   * SafeUint32_t and a uint32_t.
   * @param other The uint32_t to perform the logical OR operation with.
   * @return The result of the logical OR operation (true if either SafeUint32_t
   * or uint32_t is true, false otherwise)
   */
  inline bool operator||(const uint32_t &other) const {
    check();
    return *valuePtr || other;
  };

  // Comparison Operators

  /**
   * @brief Equality operator to check if two SafeUint32_t are equal.
   * @param other The SafeUint32_t to compare against.
   * @return True if the two SafeUint32_t are equal, false otherwise.
   */
  inline bool operator==(const SafeUint32_t &other) const {
    check();
    return *valuePtr == other.get();
  };

  /**
   * @brief Equality operator to check if a SafeUint32_t and a uint32_t are
   * equal.
   * @param other The uint32_t to compare against.
   * @return True if the SafeUint32_t and uint32_t are equal, false otherwise.
   */
  inline bool operator==(const uint32_t &other) const {
    check();
    return *valuePtr == other;
  };

  /**
   * @brief Inequality operator to check if two SafeUint32_t are not equal.
   * @param other The SafeUint32_t to compare against.
   * @return True if the two SafeUint32_t are not equal, false otherwise.
   */
  inline bool operator!=(const SafeUint32_t &other) const {
    check();
    return *valuePtr != other.get();
  };

  /**
   * @brief Inequality operator to check if a SafeUint32_t and a uint32_t are
   * not equal.
   * @param other The uint32_t to compare against.
   * @return True if the SafeUint32_t and uint32_t are not equal, false
   * otherwise.
   */
  inline bool operator!=(const uint32_t &other) const {
    check();
    return *valuePtr != other;
  };

  /**
   * @brief Less than operator to check if one SafeUint32_t is less than
   * another.
   * @param other The SafeUint32_t to compare against.
   * @return True if the first SafeUint32_t is less than the second
   * SafeUint32_t, false otherwise.
   */
  inline bool operator<(const SafeUint32_t &other) const {
    check();
    return *valuePtr < other.get();
  };

  /**
   * @brief Less than operator to check if a SafeUint32_t is less than a
   * uint32_t.
   * @param other The uint32_t to compare against.
   * @return True if the SafeUint32_t is less than the uint32_t, false
   * otherwise.
   */
  inline bool operator<(const uint32_t &other) const {
    check();
    return *valuePtr < other;
  };

  /**
   * @brief Less than or equal to operator to check if one SafeUint32_t is less
   * than or equal to another.
   * @param other The SafeUint32_t to compare against.
   * @return True if the first SafeUint32_t is less than or equal to the second
   * SafeUint32_t, false otherwise.
   */
  inline bool operator<=(const SafeUint32_t &other) const {
    check();
    return *valuePtr <= other.get();
  };

  /**
   * @brief Less than or equal to operator to check if a SafeUint32_t is less
   * than or equal to a uint32_t.
   * @param other The uint32_t to compare against.
   * @return True if the SafeUint32_t is less than or equal to the uint32_t,
   * false otherwise.
   */
  inline bool operator<=(const uint32_t &other) const {
    check();
    return *valuePtr <= other;
  };

  /**
   * @brief Greater than operator to check if one SafeUint32_t is greater than
   * another.
   * @param other The SafeUint32_t to compare against.
   * @return True if the first SafeUint32_t is greater than the second
   * SafeUint32_t, false otherwise.
   */
  inline bool operator>(const SafeUint32_t &other) const {
    check();
    return *valuePtr > other.get();
  };

  /**
   * @brief Greater than operator to check if a SafeUint32_t is greater than a
   * uint32_t.
   * @param other The uint32_t to compare against.
   * @return True if the SafeUint32_t is greater than the uint32_t, false
   * otherwise.
   */
  inline bool operator>(const uint32_t &other) const {
    check();
    return *valuePtr > other;
  };

  /**
   * @brief Greater than or equal to operator to check if one SafeUint32_t is
   * greater than or equal to another.
   * @param other The SafeUint32_t to compare against.
   * @return True if the first SafeUint32_t is greater than or equal to the
   * second SafeUint32_t, false otherwise.
   */
  inline bool operator>=(const SafeUint32_t &other) const {
    check();
    return *valuePtr >= other.get();
  };

  /**
   * @brief Greater than or equal to operator to check if a SafeUint32_t is
   * greater than or equal to a uint32_t.
   * @param other The uint32_t to compare against.
   * @return True if the SafeUint32_t is greater than or equal to the uint32_t,
   * false otherwise.
   */
  inline bool operator>=(const uint32_t &other) const {
    check();
    return *valuePtr >= other;
  };

  // Assignment Operators

  /**
   * @brief Assignment operator to assign a SafeUint32_t to another
   * SafeUint32_t.
   * @param other The SafeUint32_t to assign to the SafeUint32_t.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    *valuePtr = other.get();
    return *this;
  };

  /**
   * @brief Assignment operator to assign a uint32_t to a SafeUint32_t.
   * @param other The uint32_t to assign to the SafeUint32_t.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator=(const uint32_t &other) {
    check();
    markAsUsed();
    *valuePtr = other;
    return *this;
  };

  /**
   * @brief Addition assignment operator to add a SafeUint32_t to a
   * SafeUint32_t.
   * @param other The SafeUint32_t to add to the SafeUint32_t.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::overflow_error if there is an overflow.
   */
  inline SafeUint32_t &operator+=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint32_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other.get();
    return *this;
  };

  /**
   * @brief Addition assignment operator to add a uint32_t to a SafeUint32_t.
   * @param other The uint32_t to add to the SafeUint32_t.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::overflow_error if there is an overflow.
   */
  inline SafeUint32_t &operator+=(const uint32_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint32_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other;
    return *this;
  };

  /**
   * @brief Subtraction assignment operator to subtract a SafeUint32_t from a
   * SafeUint32_t.
   * @param other The SafeUint32_t to subtract from the SafeUint32_t.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::underflow_error if there is an underflow.
   */
  inline SafeUint32_t &operator-=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other.get();
    return *this;
  };

  /**
   * @brief Subtraction assignment operator to subtract a uint32_t from a
   * SafeUint32_t.
   * @param other The uint32_t to subtract from the SafeUint32_t.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::underflow_error if there is an underflow.
   */
  inline SafeUint32_t &operator-=(const uint32_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other;
    return *this;
  };

  /**
   * @brief Multiplication assignment operator to multiply a SafeUint32_t by a
   * SafeUint32_t.
   * @param other The SafeUint32_t to multiply the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::overflow_error if there is an overflow.
   * @throws std::domain_error if there is a multiplication by zero.
   */
  inline SafeUint32_t &operator*=(const SafeUint32_t &other) {
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

  /**
   * @brief Multiplication assignment operator to multiply a SafeUint32_t by a
   * uint32_t.
   * @param other The uint32_t to multiply the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::overflow_error if there is an overflow.
   * @throws std::domain_error if there is a multiplication by zero.
   */
  inline SafeUint32_t &operator*=(const uint32_t &other) {
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

  /**
   * @brief Division assignment operator to divide a SafeUint32_t by a
   * SafeUint32_t.
   * @param other The SafeUint32_t to divide the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::domain_error if there is a division by zero.
   */
  inline SafeUint32_t &operator/=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other.get();
    return *this;
  };

  /**
   * @brief Division assignment operator to divide a SafeUint32_t by a
   * uint32_t.
   * @param other The uint32_t to divide the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::domain_error if there is a division by zero.
   */
  inline SafeUint32_t &operator/=(const uint32_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other;
    return *this;
  };

  /**
   * @brief Modulo assignment operator to modulo a SafeUint32_t by a
   * SafeUint32_t.
   * @param other The SafeUint32_t to modulo the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::domain_error if there is a modulo by zero.
   */
  inline SafeUint32_t &operator%=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other.get();
    return *this;
  };

  /**
   * @brief Modulo assignment operator to modulo a SafeUint32_t by a
   * uint32_t.
   * @param other The uint32_t to modulo the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   * @throws std::domain_error if there is a modulo by zero.
   */
  inline SafeUint32_t &operator%=(const uint32_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other;
    return *this;
  };

  /**
   * @brief Bitwise AND assignment operator to AND a SafeUint32_t by a
   * SafeUint32_t.
   * @param other The SafeUint32_t to AND the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator&=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other.get();
    return *this;
  };

  /**
   * @brief Bitwise AND assignment operator to AND a SafeUint32_t by a
   * uint32_t.
   * @param other The uint32_t to AND the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator&=(const uint32_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other;
    return *this;
  };

  /**
   * @brief Bitwise OR assignment operator to OR a SafeUint32_t by a
   * SafeUint32_t.
   * @param other The SafeUint32_t to OR the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator|=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other.get();
    return *this;
  };

  /**
   * @brief Bitwise OR assignment operator to OR a SafeUint32_t by a
   * uint32_t.
   * @param other The uint32_t to OR the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator|=(const uint32_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other;
    return *this;
  };

  /**
   * @brief Bitwise XOR assignment operator to XOR a SafeUint32_t by a
   * SafeUint32_t.
   * @param other The SafeUint32_t to XOR the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator^=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other.get();
    return *this;
  };

  /**
   * @brief Bitwise XOR assignment operator to XOR a SafeUint32_t by a
   * uint32_t.
   * @param other The uint32_t to XOR the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator^=(const uint32_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other;
    return *this;
  };

  /**
   * @brief Left shift assignment operator to left shift a SafeUint32_t by a
   * SafeUint32_t.
   * @param other The SafeUint32_t to left shift the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator<<=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    *valuePtr <<= other.get();
    return *this;
  };

  /**
   * @brief Left shift assignment operator to left shift a SafeUint32_t by a
   * uint32_t.
   * @param other The uint32_t to left shift the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator<<=(const uint32_t &other) {
    check();
    markAsUsed();
    *valuePtr <<= other;
    return *this;
  };

  /**
   * @brief Right shift assignment operator to right shift a SafeUint32_t by a
   * SafeUint32_t.
   * @param other The SafeUint32_t to right shift the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator>>=(const SafeUint32_t &other) {
    check();
    markAsUsed();
    *valuePtr >>= other.get();
    return *this;
  };

  /**
   * @brief Right shift assignment operator to right shift a SafeUint32_t by a
   * uint32_t.
   * @param other The uint32_t to right shift the SafeUint32_t by.
   * @return A reference to the SafeUint32_t after assignment.
   */
  inline SafeUint32_t &operator>>=(const uint32_t &other) {
    check();
    markAsUsed();
    *valuePtr >>= other;
    return *this;
  };

  // Increment and Decrement Operators

  /**
   * @brief Prefix increment operator.
   * @return A reference to the SafeUint32_t after incrementing itself.
   * @throws std::overflow_error if incrementing would result in exceeding the
   */
  inline SafeUint32_t &operator++() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint32_t>::max()) {
      throw std::overflow_error("Overflow in increment operation");
    }
    ++*valuePtr;
    return *this;
  };

  /**
   * @brief Prefix decrement operator.
   * @return A reference to the SafeUint32_t after decrementing itself.
   * @throws std::underflow_error if decrementing would result in exceeding the
   */
  inline SafeUint32_t &operator--() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint32_t>::min()) {
      throw std::underflow_error("Underflow in increment operation");
    }
    --*valuePtr;
    return *this;
  };

  // get is used to properly get the value of a variable within another
  // SafeUint32_t (we need to call check!)

  /**
  Getter function used to access the value of the SafeUint32_t.
  @return The value of the SafeUint32_t.
  */
  inline uint32_t get() const {
    check();
    return *valuePtr;
  };

  /**
  Commit function used to commit the value of the SafeUint32_t to the value
  pointed to.
  */
  inline void commit() override {
    check();
    value = *valuePtr;
    valuePtr = nullptr;
    registered = false;
  };

  /**
  Revert function used to revert the value of the SafeUint32_t (nullify it).
  */
  inline void revert() const override {
    valuePtr = nullptr;
    registered = false;
  };
};

#endif