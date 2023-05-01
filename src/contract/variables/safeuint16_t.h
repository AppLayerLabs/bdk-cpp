#ifndef SAFEUINT16_T_H
#define SAFEUINT16_T_H

#include "safebase.h"
#include <memory>

/**
 *Safe wrapper for uint16_t variables.
 *This class is used to safely store a uint8_t variable within a contract.
 *@see SafeBase
 */

class SafeUint16_t : public SafeBase {
private:
  uint16_t value; ///< The actual value of the variable.
  // Check() require valuePtr to be mutable.
  mutable std::unique_ptr<uint16_t>
      valuePtr; ///< Pointer to the value of the variable.

  /**
   *@brief Checks if the valuePtr is initialized, if not, it initializes it.
   */
  inline void check() const override {
    if (valuePtr == nullptr) {
      valuePtr = std::make_unique<uint16_t>(value);
    }
  };

public:
  /**
   *@brief Constructor for SafeUint16_t.
   * Only variables built with this constructor will be registered within a
   * contract.
   *@param owner The contract that owns this variable.
   *@param value The initial value of the variable.
   */
  SafeUint16_t(DynamicContract *owner, const uint16_t &value = 0)
      : SafeBase(owner), value(0),
        valuePtr(std::make_unique<uint16_t>(value)){};

  /**
   *@brief Default constructor for SafeUint16_t.
   *@param value The initial value of the variable (defaults to 0).
   */
  SafeUint16_t(const uint16_t &value = 0)
      : SafeBase(nullptr), value(0),
        valuePtr(std::make_unique<uint16_t>(value)){};

  /**
   *@brief Copy constructor for SafeUint16_t.
   *@param other The other SafeUint16_t that will be copied.
   */
  SafeUint16_t(const SafeUint16_t &other) : SafeBase(nullptr) {
    other.check();
    value = 0;
    valuePtr = std::make_unique<uint16_t>(*other.valuePtr);
  };

  // Arithmetic Operators

  /**
   *@brief Addition operator for sum between two SafeUint16_t.
   *@param other The other SafeUint16_t that will be added to this.
   *@return A new SafeUint16_t containing the result of the addition.
   *@throws std::overflow_error if the result of the addition overflows.
   */
  inline SafeUint16_t operator+(const SafeUint16_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint16_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint16_t(*valuePtr + other.get());
  };

  /**
   *@brief Addition operator for sum between a SafeUint16_t and a uint16_t.
   *@param other The uint16_t that will be added to this.
   *@return A new SafeUint16_t containing the result of the addition.
   *@throws std::overflow_error if the result of the addition overflows.
   */
  inline SafeUint16_t operator+(const uint16_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint16_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint16_t(*valuePtr + other);
  };

  /**
   *@brief Subtraction operator for subtraction between two SafeUint16_t.
   *@param other The other SafeUint16_t that will be subtracted to this.
   *@return A new SafeUint16_t containing the result of the subtraction.
   *@throws std::underflow_error if the result of the subtraction underflows.
   */
  inline SafeUint16_t operator-(const SafeUint16_t &other) const {
    check();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint16_t(*valuePtr - other.get());
  };

  /**
   *@brief Subtraction operator for subtraction between a SafeUint16_t and a
   *uint16_t.
   *@param other The uint16_t that will be subtracted to this.
   *@return A new SafeUint16_t containing the result of the subtraction.
   *@throws std::underflow_error if the result of the subtraction underflows.
   */
  inline SafeUint16_t operator-(const uint16_t &other) const {
    check();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint16_t(*valuePtr - other);
  };

  /**
   *@brief Multiplication operator for multiplication between two SafeUint16_t.
   *@param other The other SafeUint16_t that will be multiplied to this.
   *@return A new SafeUint16_t containing the result of the multiplication.
   *@throws std::overflow_error if the result of the multiplication overflows.
   *@throws std::domain_error if one of the operands is zero.
   */
  inline SafeUint16_t operator*(const SafeUint16_t &other) const {
    check();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint16_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint16_t(*valuePtr * other.get());
  };

  /**
   *@brief Multiplication operator for multiplication between a SafeUint16_t and
   *a uint16_t.
   *@param other The uint16_t that will be multiplied to this.
   *@return A new SafeUint16_t containing the result of the multiplication.
   *@throws std::overflow_error if the result of the multiplication overflows.
   *@throws std::domain_error if one of the operands is zero.
   */
  inline SafeUint16_t operator*(const uint16_t &other) const {
    check();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint16_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint16_t(*valuePtr * other);
  };

  /**
   *@brief Division operator for division between two SafeUint16_t.
   *@param other The other SafeUint16_t that will divide this.
   *@return A new SafeUint16_t containing the result of the division.
   *@throws std::domain_error if one of the operands is zero.
   */
  inline SafeUint16_t operator/(const SafeUint16_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint16_t(*valuePtr / other.get());
  };

  /**
   *@brief Division operator for division between a SafeUint16_t and a uint16_t.
   *@param other The uint16_t that will divide this.
   *@return A new SafeUint16_t containing the result of the division.
   *@throws std::domain_error if one of the operands is zero.
   */
  inline SafeUint16_t operator/(const uint16_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint16_t(*valuePtr / other);
  };

  /**
   *@brief Modulo operator for modulo between two SafeUint16_t.
   *@param other The other SafeUint16_t that will modulo this.
   *@return A new SafeUint16_t containing the result of the modulo.
   *@throws std::domain_error if one of the operands is zero.
   */
  inline SafeUint16_t operator%(const SafeUint16_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint16_t(*valuePtr % other.get());
  };

  /**
   *@brief Modulo operator for modulo between a SafeUint16_t and a uint16_t.
   *@param other The uint16_t that will modulo this.
   *@return A new SafeUint16_t containing the result of the modulo.
   *@throws std::domain_error if one of the operands is zero.
   */
  inline SafeUint16_t operator%(const uint16_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint16_t(*valuePtr % other);
  };

  // Bitwise Operators

  /**
   *@brief Bitwise AND operator for bitwise AND between two SafeUint16_t.
   *@param other The other SafeUint16_t that will be AND'ed to this.
   *@return A new SafeUint16_t containing the result of the bitwise AND.
   */
  inline SafeUint16_t operator&(const SafeUint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr & other.get());
  };

  /**
   *@brief Bitwise AND operator for bitwise AND between a SafeUint16_t and a
   *uint16_t.
   *@param other The uint16_t that will be AND'ed to this.
   *@return A new SafeUint16_t containing the result of the bitwise AND.
   */
  inline SafeUint16_t operator&(const uint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr & other);
  };

  /**
   *@brief Bitwise OR operator for bitwise OR between two SafeUint16_t.
   *@param other The other SafeUint16_t that will be OR'ed to this.
   *@return A new SafeUint16_t containing the result of the bitwise OR.
   */
  inline SafeUint16_t operator|(const SafeUint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr | other.get());
  };

  /**
   *@brief Bitwise OR operator for bitwise OR between a SafeUint16_t and a
   *uint16_t.
   *@param other The uint16_t that will be OR'ed to this.
   *@return A new SafeUint16_t containing the result of the bitwise OR.
   */
  inline SafeUint16_t operator|(const uint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr | other);
  };

  /**
   *@brief Bitwise XOR operator for bitwise XOR between two SafeUint16_t.
   *@param other The other SafeUint16_t that will be XOR'ed to this.
   *@return A new SafeUint16_t containing the result of the bitwise XOR.
   */
  inline SafeUint16_t operator^(const SafeUint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr ^ other.get());
  };

  /**
   *@brief Bitwise XOR operator for bitwise XOR between a SafeUint16_t and a
   *uint16_t.
   *@param other The uint16_t that will be XOR'ed to this.
   *@return A new SafeUint16_t containing the result of the bitwise XOR.
   */
  inline SafeUint16_t operator^(const uint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr ^ other);
  };

  /**
   *@brief Left shift operator used to shift the bits of a SafeUint16_t to the
  left by the amount specified by another SafeUint16_t.
   *@param other The other SafeUint16_t that will determine the number of bits
  to shift by.
    *@return A new SafeUint16_t containing the result of the left shift.
    */
  inline SafeUint16_t operator<<(const SafeUint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr << other.get());
  };

  /**
   *@brief Left shift operator used to shift the bits of a SafeUint16_t to the
  left by the amount specified by a uint16_t.
    *@param other The uint16_t that will determine the number of bits to shift
  by.
    *@return A new SafeUint16_t containing the result of the left shift.
    */
  inline SafeUint16_t operator<<(const uint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr << other);
  };

  /**
   *@brief Right shift operator used to shift the bits of a SafeUint16_t to the
  right by the amount specified by another SafeUint16_t.
    *@param other The other SafeUint16_t that will determine the number of bits
  to shift by.
    *@return A new SafeUint16_t containing the result of the right shift.
    */
  inline SafeUint16_t operator>>(const SafeUint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr >> other.get());
  };

  /**
   *@brief Right shift operator used to shift the bits of a SafeUint16_t to the
  right by the amount specified by a uint16_t.
    *@param other The uint16_t that will determine the number of bits to shift
  by.
    *@return A new SafeUint16_t containing the result of the right shift.
    */
  inline SafeUint16_t operator>>(const uint16_t &other) const {
    check();
    return SafeUint16_t(*valuePtr >> other);
  };

  // Logical Operators

  /**
   *@brief Logical NOT operator used to get the logical negation of a
  SafeUint16_t.
    *@return The logical negation of the SafeUint16_t.
    */
  inline bool operator!() const {
    check();
    return !*valuePtr;
  };

  /**
   *@brief Logical AND operator used to logically AND a SafeUint16_t with
  another.
    *@param other The other SafeUint16_t that will be logically AND'ed to this.
    *@return The result of the logical AND (true if both SafeUint16_t's are
  true)
    */
  inline bool operator&&(const SafeUint16_t &other) const {
    check();
    return *valuePtr && other.get();
  };

  /**
   *@brief Logical AND operator used to logically AND a SafeUint16_t with a
  uint16_t.
    *@param other The uint16_t that will be logically AND'ed to this.
    *@return The result of the logical AND (true if both SafeUint16_t's are
  true)
    */
  inline bool operator&&(const uint16_t &other) const {
    check();
    return *valuePtr && other;
  };

  /**
   *@brief Logical OR operator used to logically OR a SafeUint16_t with another.
   *@param other The other SafeUint16_t that will be logically OR'ed to this.
   *@return The result of the logical OR (true if either SafeUint16_t is true)
   */
  inline bool operator||(const SafeUint16_t &other) const {
    check();
    return *valuePtr || other.get();
  };

  /**
   *@brief Logical OR operator used to logically OR a SafeUint16_t with a
  uint16_t.
    *@param other The uint16_t that will be logically OR'ed to this.
    *@return The result of the logical OR (true if either SafeUint16_t is true)
    */
  inline bool operator||(const uint16_t &other) const {
    check();
    return *valuePtr || other;
  };

  // Comparison Operators

  /**
   *@brief Equality operator used to check if a SafeUint16_t is equal to
  another.
    *@param other The other SafeUint16_t that will be compared to this.
    *@return True if the SafeUint16_t's are equal, false otherwise.
    */
  inline bool operator==(const SafeUint16_t &other) const {
    check();
    return *valuePtr == other.get();
  };

  /**
   *@brief Equality operator used to check if a SafeUint16_t is equal to a
  uint16_t.
    *@param other The uint16_t that will be compared to this.
    *@return True if the SafeUint16_t's are equal, false otherwise.
    */
  inline bool operator==(const uint16_t &other) const {
    check();
    return *valuePtr == other;
  };

  /**
   *@brief Inequality operator used to check if a SafeUint16_t is not equal to
  another.
    *@param other The other SafeUint16_t that will be compared to this.
    *@return True if the SafeUint16_t's are not equal, false otherwise.
    */
  inline bool operator!=(const SafeUint16_t &other) const {
    check();
    return *valuePtr != other.get();
  };

  /**
   *@brief Inequality operator used to check if a SafeUint16_t is not equal to a
  uint16_t.
    *@param other The uint16_t that will be compared to this.
    *@return True if the SafeUint16_t's are not equal, false otherwise.
    */
  inline bool operator!=(const uint16_t &other) const {
    check();
    return *valuePtr != other;
  };

  /**
   *@brief Less than operator used to check if a SafeUint16_t is less than
  another.
    *@param other The other SafeUint16_t that will be compared to this.
    *@return True if the SafeUint16_t is less than the other, false otherwise.
    */
  inline bool operator<(const SafeUint16_t &other) const {
    check();
    return *valuePtr < other.get();
  };

  /**
   *@brief Less than operator used to check if a SafeUint16_t is less than a
  uint16_t.
    *@param other The uint16_t that will be compared to this.
    *@return True if the SafeUint16_t is less than the other, false otherwise.
    */
  inline bool operator<(const uint16_t &other) const {
    check();
    return *valuePtr < other;
  };

  /**
   *@brief Less than or equal to operator used to check if a SafeUint16_t is
  less than or equal to another.
    *@param other The other SafeUint16_t that will be compared to this.
    *@return True if the SafeUint16_t is less than or equal to the other, false
  otherwise.
    */
  inline bool operator<=(const SafeUint16_t &other) const {
    check();
    return *valuePtr <= other.get();
  };

  /**
   *@brief Less than or equal to operator used to check if a SafeUint16_t is
  less than or equal to a uint16_t.
    *@param other The uint16_t that will be compared to this.
    *@return True if the SafeUint16_t is less than or equal to the other, false
  otherwise.
    */
  inline bool operator<=(const uint16_t &other) const {
    check();
    return *valuePtr <= other;
  };

  /**
   *@brief Greater than operator used to check if a SafeUint16_t is greater than
  another.
    *@param other The other SafeUint16_t that will be compared to this.
    *@return True if the SafeUint16_t is greater than the other, false
  otherwise.
    */
  inline bool operator>(const SafeUint16_t &other) const {
    check();
    return *valuePtr > other.get();
  };

  /**
   *@brief Greater than operator used to check if a SafeUint16_t is greater than
  a uint16_t.
    *@param other The uint16_t that will be compared to this.
    *@return True if the SafeUint16_t is greater than the other, false
  otherwise.
    */
  inline bool operator>(const uint16_t &other) const {
    check();
    return *valuePtr > other;
  };

  /**
   *@brief Greater than or equal to operator used to check if a SafeUint16_t is
  greater than or equal to another.
    *@param other The other SafeUint16_t that will be compared to this.
    *@return True if the SafeUint16_t is greater than or equal to the other,
  false otherwise.
    */
  inline bool operator>=(const SafeUint16_t &other) const {
    check();
    return *valuePtr >= other.get();
  };

  /**
   *@brief Greater than or equal to operator used to check if a SafeUint16_t is
  greater than or equal to a uint16_t.
    *@param other The uint16_t that will be compared to this.
    *@return True if the SafeUint16_t is greater than or equal to the other,
  false otherwise.
    */
  inline bool operator>=(const uint16_t &other) const {
    check();
    return *valuePtr >= other;
  };

  // Assignment Operators

  /**
   *@brief This operator assigns the value of another SafeUint16_t to this.
   *@param other The other SafeUint16_t that will be assigned to this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    *valuePtr = other.get();
    return *this;
  };

  /**
   *@brief This operator assigns the value of a uint16_t to this.
   *@param other The uint16_t that will be assigned to this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator=(const uint16_t &other) {
    check();
    markAsUsed();
    *valuePtr = other;
    return *this;
  };

  /**
   *@brief Addition assignment operator adds a SafeUint16_t to this.
    *@param other The other SafeUint16_t that will be added to this.
    *@return A reference to this SafeUint16_t.
    @throws std::overflow_error if there is an overflow.
    */
  inline SafeUint16_t &operator+=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint16_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other.get();
    return *this;
  };

  /**
   *@brief Addition assignment operator adds a uint16_t to this.
    *@param other The uint16_t that will be added to this.
    *@return A reference to this SafeUint16_t.
    @throws std::overflow_error if there is an overflow.
    */
  inline SafeUint16_t &operator+=(const uint16_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint16_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other;
    return *this;
  };

  /**
   *@brief Subtraction assignment operator subtracts a SafeUint16_t from this.
    *@param other The other SafeUint16_t that will be subtracted from this.
    *@return A reference to this SafeUint16_t.
    @throws std::underflow_error if there is an underflow.
    */
  inline SafeUint16_t &operator-=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other.get();
    return *this;
  };

  /**
   *@brief Subtraction assignment operator subtracts a uint16_t from this.
    *@param other The uint16_t that will be subtracted from this.
    *@return A reference to this SafeUint16_t.
    @throws std::underflow_error if there is an underflow.
    */
  inline SafeUint16_t &operator-=(const uint16_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other;
    return *this;
  };

  /**
   *@brief Multiplication assignment operator multiplies this by a SafeUint16_t.
    *@param other The other SafeUint16_t that will be multiplied by this.
    *@return A reference to this SafeUint16_t.
    @throws std::overflow_error if there is an overflow.
    @throws std::domain_error if there is a multiplication by zero.
    */
  inline SafeUint16_t &operator*=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint16_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other.get();
    return *this;
  };

  /**
   *@brief Multiplication assignment operator multiplies this by a uint16_t.
    *@param other The uint16_t that will be multiplied by this.
    *@return A reference to this SafeUint16_t.
    @throws std::overflow_error if there is an overflow.
    @throws std::domain_error if there is a multiplication by zero.
    */
  inline SafeUint16_t &operator*=(const uint16_t &other) {
    check();
    markAsUsed();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint16_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other;
    return *this;
  };

  /**
   *@brief Division assignment operator divides this by a SafeUint16_t.
    *@param other The other SafeUint16_t that will divide this.
    *@return A reference to this SafeUint16_t.
    @throws std::domain_error if there is a division by zero.
    */
  inline SafeUint16_t &operator/=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other.get();
    return *this;
  };

  /**
   *@brief Division assignment operator divides this by a uint16_t.
    *@param other The uint16_t that will divide this.
    *@return A reference to this SafeUint16_t.
    @throws std::domain_error if there is a division by zero.
    */
  inline SafeUint16_t &operator/=(const uint16_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other;
    return *this;
  };

  /**
   *@brief Modulo assignment operator performs a modulo operation with this and
   *another SafeUint16_t.
    *@param other The other SafeUint16_t that will modulo this.
    *@return A reference to this SafeUint16_t.
    @throws std::domain_error if there is a modulo by zero.
    */
  inline SafeUint16_t &operator%=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other.get();
    return *this;
  };

  /**
   *@brief Modulo assignment operator performs a modulo operation with this and
   *a uint16_t.
    *@param other The uint16_t that will modulo this.
    *@return A reference to this SafeUint16_t.
    @throws std::domain_error if there is a modulo by zero.
    */
  inline SafeUint16_t &operator%=(const uint16_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other;
    return *this;
  };

  /**
   *@brief Bitwise AND assignment operator performs a bitwise AND operation
   *between this and another SafeUint16_t.
   *@param other The other SafeUint16_t that will be ANDed with this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator&=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other.get();
    return *this;
  };

  /**
   *@brief Bitwise AND assignment operator performs a bitwise AND operation
   *between this and a uint16_t.
   *@param other The uint16_t that will be ANDed with this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator&=(const uint16_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other;
    return *this;
  };

  /**
   *@brief Bitwise OR assignment operator performs a bitwise OR operation
   *between this and another SafeUint16_t.
   *@param other The other SafeUint16_t that will be ORed with this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator|=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other.get();
    return *this;
  };

  /**
   *@brief Bitwise OR assignment operator performs a bitwise OR operation
   *between this and a uint16_t.
   *@param other The uint16_t that will be ORed with this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator|=(const uint16_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other;
    return *this;
  };

  /**
   *@brief Bitwise XOR assignment operator performs a bitwise XOR operation
   *between this and another SafeUint16_t.
   *@param other The other SafeUint16_t that will be XORed with this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator^=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other.get();
    return *this;
  };

  /**
   *@brief Bitwise XOR assignment operator performs a bitwise XOR operation
   *between this and a uint16_t.
   *@param other The uint16_t that will be XORed with this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator^=(const uint16_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other;
    return *this;
  };

  /**
   *@brief Left shift assignment operator shifts this by another SafeUint16_t.
   *@param other The other SafeUint16_t that will shift this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator<<=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    *valuePtr <<= other.get();
    return *this;
  };

  /**
   *@brief Left shift assignment operator shifts this by a uint16_t.
   *@param other The uint16_t that will shift this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator<<=(const uint16_t &other) {
    check();
    markAsUsed();
    *valuePtr <<= other;
    return *this;
  };

  /**
   *@brief Right shift assignment operator shifts this by another SafeUint16_t.
   *@param other The other SafeUint16_t that will shift this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator>>=(const SafeUint16_t &other) {
    check();
    markAsUsed();
    *valuePtr >>= other.get();
    return *this;
  };

  /**
   *@brief Right shift assignment operator shifts this by a uint16_t.
   *@param other The uint16_t that will shift this.
   *@return A reference to this SafeUint16_t.
   */
  inline SafeUint16_t &operator>>=(const uint16_t &other) {
    check();
    markAsUsed();
    *valuePtr >>= other;
    return *this;
  };

  // Increment and Decrement Operators

  /**
   *@brief Prefix increment operator increments this by 1.
   *@return A reference to this SafeUint16_t.
   *@throws std::overflow_error if there is an overflow.
   */
  inline SafeUint16_t &operator++() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint16_t>::max()) {
      throw std::overflow_error("Overflow in increment operation");
    }
    ++*valuePtr;
    return *this;
  };

  /**
   *@brief Prefix decrement operator decrements this by 1.
   *@return A reference to this SafeUint16_t.
   *@throws std::underflow_error if there is an underflow.
   */
  inline SafeUint16_t &operator--() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint16_t>::min()) {
      throw std::underflow_error("Underflow in increment operation");
    }
    --*valuePtr;
    return *this;
  };

  /**
  Getter function used to access the value of the SafeUint16_t.
  @return The value of the SafeUint16_t.
  */
  inline uint16_t get() const {
    check();
    return *valuePtr;
  };

  /**
  Commit function used to commit the value of the afeUint16_t to the value
  pointed to.
  */
  inline void commit() override {
    check();
    value = *valuePtr;
    valuePtr = nullptr;
    registered = false;
  };

  /**
  Revert function used to revert the value of the SafeUint16_t (nullify it)
  */
  inline void revert() const override {
    valuePtr = nullptr;
    registered = false;
  };
};

#endif