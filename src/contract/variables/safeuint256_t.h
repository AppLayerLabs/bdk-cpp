#ifndef SAFEUINT256_T_H
#define SAFEUINT256_T_H

#include "safebase.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <memory>

using uint256_t =
    boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
        256, 256, boost::multiprecision::unsigned_magnitude,
        boost::multiprecision::cpp_int_check_type::checked, void>>;

/**
 * Safe wrapper for a uint256_t variable.
 * This class is used to safely store a uint256_t variable within a contract.
 * @see SafeBase
 */
class SafeUint256_t : public SafeBase {
private:
  uint256_t value; ///< value for the uint256_t variable.
  // Check() require valuePtr to be mutable.
  mutable std::unique_ptr<uint256_t> valuePtr; ///< Pointer to the value.

  /**
   *@brief Check if the valuePtr is initialized.
   * If not, initialize it.
   */
  inline void check() const override {
    if (valuePtr == nullptr) {
      valuePtr = std::make_unique<uint256_t>(value);
    }
  };

public:
  /**
   *@brief Constructor for SafeUint256_t.
   * Only variables built with this constructor will be registered within a
   * contract.
   *@param owner The contract that owns this variable.
   *@param value The initial value of the variable.
   */
  SafeUint256_t(DynamicContract *owner, const uint256_t &value = 0)
      : SafeBase(owner), value(0),
        valuePtr(std::make_unique<uint256_t>(value)){};

  /**
   *@brief Normal Constructor for SafeUint256_t.
   *@param value The initial value of the variable.
   */
  SafeUint256_t(const uint256_t &value = 0)
      : SafeBase(nullptr), value(0),
        valuePtr(std::make_unique<uint256_t>(value)){};

  /**
   *@brief Constructor used to create a SafeUint256_t from another SafeUint256_t
   *(copy constructor).
   *@param other The SafeUint256_t to copy.
   */
  SafeUint256_t(const SafeUint256_t &other) : SafeBase(nullptr) {
    other.check();
    value = 0;
    valuePtr = std::make_unique<uint256_t>(*other.valuePtr);
  };

  // Arithmetic Operators

  /**
   *@brief Addition operator to add two SafeUint256_t.
   *@param other The SafeUint256_t to add.
   *@return A new SafeUint256_t with the result of the addition.
   *@throw std::overflow_error if the result of the addition is greater than
   *the maximum value of uint256_t.
   */
  inline SafeUint256_t operator+(const SafeUint256_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint256_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint256_t(*valuePtr + other.get());
  };

  /**
   *@brief Addition operator to add a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to add.
   *@return A new SafeUint256_t with the result of the addition.
   *@throw std::overflow_error if the result of the addition is greater than
   *the maximum value of uint256_t.
   */
  inline SafeUint256_t operator+(const uint256_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint256_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint256_t(*valuePtr + other);
  };

  /**
   *@brief Addition operator to add a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to add.
   *@return A new SafeUint256_t with the result of the addition.
   *@throw std::overflow_error if the result of the addition is greater than
   *the maximum value of uint256_t.
   */
  inline SafeUint256_t operator+(const uint64_t &other) const {
    check();
    if (*valuePtr > std::numeric_limits<uint256_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    return SafeUint256_t(*valuePtr + other);
  };

  /**
   *@brief Subtraction operator to subtract two SafeUint256_t.
   *@param other The SafeUint256_t to subtract.
   *@return A new SafeUint256_t with the result of the subtraction.
   *@throw std::underflow_error if the result of the subtraction is less than
   *zero.
   */
  inline SafeUint256_t operator-(const SafeUint256_t &other) const {
    check();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint256_t(*valuePtr - other.get());
  };

  /**
   *@brief Subtraction operator to subtract a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to subtract.
   *@return A new SafeUint256_t with the result of the subtraction.
   *@throw std::underflow_error if the result of the subtraction is less than
   *zero.
   */
  inline SafeUint256_t operator-(const uint256_t &other) const {
    check();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint256_t(*valuePtr - other);
  };

  /**
   *@brief Subtraction operator to subtract a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to subtract.
   *@return A new SafeUint256_t with the result of the subtraction.
   *@throw std::underflow_error if the result of the subtraction is less than
   *zero.
   */
  inline SafeUint256_t operator-(const uint64_t &other) const {
    check();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    return SafeUint256_t(*valuePtr - other);
  };

  /**
   *@brief Multiplication operator to multiply two SafeUint256_t.
   *@param other The SafeUint256_t to multiply.
   *@return A new SafeUint256_t with the result of the multiplication.
   *@throw std::overflow_error if the result of the multiplication is greater
   *than the maximum value of uint256_t.
   *@throw std::domain_error if one of the SafeUint256_t is zero.
   */
  inline SafeUint256_t operator*(const SafeUint256_t &other) const {
    check();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint256_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint256_t(*valuePtr * other.get());
  };

  /**
   *@brief Multiplication operator to multiply a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to multiply.
   *@return A new SafeUint256_t with the result of the multiplication.
   *@throw std::overflow_error if the result of the multiplication is greater
   *than the maximum value of uint256_t.
   *@throw std::domain_error if one of the SafeUint256_t is zero.
   */
  inline SafeUint256_t operator*(const uint256_t &other) const {
    check();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint256_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint256_t(*valuePtr * other);
  };

  /**
   *@brief Multiplication operator to multiply a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to multiply.
   *@return A new SafeUint256_t with the result of the multiplication.
   *@throw std::overflow_error if the result of the multiplication is greater
   *than the maximum value of uint256_t.
   *@throw std::domain_error if one of the SafeUint256_t is zero.
   */
  inline SafeUint256_t operator*(const uint64_t &other) const {
    check();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint256_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    return SafeUint256_t(*valuePtr * other);
  };

  /**
   *@brief Division operator to divide two SafeUint256_t.
   *@param other The SafeUint256_t to divide.
   *@return A new SafeUint256_t with the result of the division.
   *@throw std::domain_error if one of the SafeUint256_t is zero.
   *@throw std::domain_error if the result of the division is zero.
   */
  inline SafeUint256_t operator/(const SafeUint256_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint256_t(*valuePtr / other.get());
  };

  /**
   *@brief Division operator to divide a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to divide.
   *@return A new SafeUint256_t with the result of the division.
   *@throw std::domain_error if one of the uints is zero.
   *@throw std::domain_error if the result of the division is zero.
   */
  inline SafeUint256_t operator/(const uint256_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint256_t(*valuePtr / other);
  };

  /**
   *@brief Division operator to divide a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to divide.
   *@return A new SafeUint256_t with the result of the division.
   *@throw std::domain_error if one of the uints is zero.
   *@throw std::domain_error if the result of the division is zero.
   */
  inline SafeUint256_t operator/(const uint64_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    return SafeUint256_t(*valuePtr / other);
  };

  /**
   *@brief Modulo operator to calculate the remainder of the division of two
   *SafeUint256_t.
   *@param other The SafeUint256_t to divide.
   *@return A new SafeUint256_t with the result of the modulo operation.
   *@throw std::domain_error if one of the SafeUint256_t is zero.
   */
  inline SafeUint256_t operator%(const SafeUint256_t &other) const {
    check();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint256_t(*valuePtr % other.get());
  };

  /**
   *@brief Modulo operator to calculate the remainder of the division of a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to divide.
   *@return A new SafeUint256_t with the result of the modulo operation.
   *@throw std::domain_error if one of the SafeUint256_t is zero.
   */
  inline SafeUint256_t operator%(const uint256_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint256_t(*valuePtr % other);
  };

  /**
   *@brief Modulo operator to calculate the remainder of the division of a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to divide.
   *@return A new SafeUint256_t with the result of the modulo operation.
   *@throw std::domain_error if one of the SafeUint256_t is zero.
   */
  inline SafeUint256_t operator%(const uint64_t &other) const {
    check();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    return SafeUint256_t(*valuePtr % other);
  };

  // Bitwise Operators

  /**
   *@brief Bitwise AND operator to perform a bitwise AND operation on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the bitwise AND operation with.
   *@return A new SafeUint256_t with the result of the bitwise AND operation.
   */
  inline SafeUint256_t operator&(const SafeUint256_t &other) const {
    check();
    return SafeUint256_t(*valuePtr & other.get());
  };

  /**
   *@brief Bitwise AND operator to perform a bitwise AND operation on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the bitwise AND operation with.
   *@return A new SafeUint256_t with the result of the bitwise AND operation.
   */
  inline SafeUint256_t operator&(const uint256_t &other) const {
    check();
    return SafeUint256_t(*valuePtr & other);
  };

  /**
   *@brief Bitwise AND operator to perform a bitwise AND operation on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the bitwise AND operation with.
   *@return A new SafeUint256_t with the result of the bitwise AND operation.
   */
  inline SafeUint256_t operator&(const uint64_t &other) const {
    check();
    return SafeUint256_t(*valuePtr & other);
  };

  /**
   *@brief Bitwise OR operator to perform a bitwise OR operation on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the bitwise OR operation with.
   *@return A new SafeUint256_t with the result of the bitwise OR operation.
   */
  inline SafeUint256_t operator|(const SafeUint256_t &other) const {
    check();
    return SafeUint256_t(*valuePtr | other.get());
  };

  /**
   *@brief Bitwise OR operator to perform a bitwise OR operation on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the bitwise OR operation with.
   *@return A new SafeUint256_t with the result of the bitwise OR operation.
   */
  inline SafeUint256_t operator|(const uint256_t &other) const {
    check();
    return SafeUint256_t(*valuePtr | other);
  };

  /**
   *@brief Bitwise OR operator to perform a bitwise OR operation on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the bitwise OR operation with.
   *@return A new SafeUint256_t with the result of the bitwise OR operation.
   */
  inline SafeUint256_t operator|(const uint64_t &other) const {
    check();
    return SafeUint256_t(*valuePtr | other);
  };

  /**
   *@brief Bitwise XOR operator to perform a bitwise XOR operation on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the bitwise XOR operation with.
   *@return A new SafeUint256_t with the result of the bitwise XOR operation.
   */
  inline SafeUint256_t operator^(const SafeUint256_t &other) const {
    check();
    return SafeUint256_t(*valuePtr ^ other.get());
  };

  /**
   *@brief Bitwise XOR operator to perform a bitwise XOR operation on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the bitwise XOR operation with.
   *@return A new SafeUint256_t with the result of the bitwise XOR operation.
   */
  inline SafeUint256_t operator^(const uint256_t &other) const {
    check();
    return SafeUint256_t(*valuePtr ^ other);
  };

  /**
   *@brief Bitwise XOR operator to perform a bitwise XOR operation on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the bitwise XOR operation with.
   *@return A new SafeUint256_t with the result of the bitwise XOR operation.
   */
  inline SafeUint256_t operator^(const uint64_t &other) const {
    check();
    return SafeUint256_t(*valuePtr ^ other);
  };

  // Logical Operators

  /**
   *@brief Logical NOT operator to perform a logical NOT operation on a
   *SafeUint256_t.
   *@return AThe result of the logical NOT operation.
   */
  inline bool operator!() const {
    check();
    return !*valuePtr;
  };

  /**
   *@brief Logical AND operator to perform a logical AND operation on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the logical AND operation with.
   *@return The result of the logical AND operation (true if both are true,
   *false otherwise).
   */
  inline bool operator&&(const SafeUint256_t &other) const {
    check();
    return *valuePtr && other.get();
  };

  /**
   *@brief Logical AND operator to perform a logical AND operation on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the logical AND operation with.
   *@return The result of the logical AND operation (true if both are true,
   *false otherwise).
   */
  inline bool operator&&(const uint256_t &other) const {
    check();
    return *valuePtr && other;
  };

  /**
   *@brief Logical AND operator to perform a logical AND operation on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the logical AND operation with.
   *@return The result of the logical AND operation (true if both are true,
   *false otherwise).
   */
  inline bool operator&&(const uint64_t &other) const {
    check();
    return *valuePtr && other;
  };

  /**
   *@brief Logical OR operator to perform a logical OR operation on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the logical OR operation with.
   *@return The result of the logical OR operation (true if either is true,
   *false otherwise).
   */
  inline bool operator||(const SafeUint256_t &other) const {
    check();
    return *valuePtr || other.get();
  };

  /**
   *@brief Logical OR operator to perform a logical OR operation on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the logical OR operation with.
   *@return The result of the logical OR operation (true if either is true,
   *false otherwise).
   */
  inline bool operator||(const uint256_t &other) const {
    check();
    return *valuePtr || other;
  };

  /**
   *@brief Logical OR operator to perform a logical OR operation on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the logical OR operation with.
   *@return The result of the logical OR operation (true if either is true,
   *false otherwise).
   */
  inline bool operator||(const uint64_t &other) const {
    check();
    return *valuePtr || other;
  };

  // Comparison Operators

  /**
   *@brief Equality operator to perform an equality comparison on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the equality comparison with.
   *@return The result of the equality comparison (true if equal, false
   *otherwise).
   */
  inline bool operator==(const SafeUint256_t &other) const {
    check();
    return *valuePtr == other.get();
  };

  /**
   *@brief Equality operator to perform an equality comparison on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the equality comparison with.
   *@return The result of the equality comparison (true if equal, false
   *otherwise).
   */
  inline bool operator==(const uint256_t &other) const {
    check();
    return *valuePtr == other;
  };

  /**
   *@brief Equality operator to perform an equality comparison on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the equality comparison with.
   *@return The result of the equality comparison (true if equal, false
   *otherwise).
   */
  inline bool operator==(const uint64_t &other) const {
    check();
    return *valuePtr == other;
  };

  /**
   *@brief Inequality operator to perform an inequality comparison on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the inequality comparison with.
   *@return The result of the inequality comparison (true if not equal, false
   *otherwise).
   */
  inline bool operator!=(const SafeUint256_t &other) const {
    check();
    return *valuePtr != other.get();
  };

  /**
   *@brief Inequality operator to perform an inequality comparison on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the inequality comparison with.
   *@return The result of the inequality comparison (true if not equal, false
   *otherwise).
   */
  inline bool operator!=(const uint256_t &other) const {
    check();
    return *valuePtr != other;
  };

  /**
   *@brief Inequality operator to perform an inequality comparison on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the inequality comparison with.
   *@return The result of the inequality comparison (true if not equal, false
   *otherwise).
   */
  inline bool operator!=(const uint64_t &other) const {
    check();
    return *valuePtr != other;
  };

  /**
   *@brief Less than operator to perform a less than comparison on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the less than comparison with.
   *@return The result of the less than comparison (true if less than, false
   *otherwise).
   */
  inline bool operator<(const SafeUint256_t &other) const {
    check();
    return *valuePtr < other.get();
  };

  /**
   *@brief Less than operator to perform a less than comparison on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the less than comparison with.
   *@return The result of the less than comparison (true if less than, false
   *otherwise).
   */
  inline bool operator<(const uint256_t &other) const {
    check();
    return *valuePtr < other;
  };

  /**
   *@brief Less than operator to perform a less than comparison on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the less than comparison with.
   *@return The result of the less than comparison (true if less than, false
   *otherwise).
   */
  inline bool operator<(const uint64_t &other) const {
    check();
    return *valuePtr < other;
  };

  /**
   *@brief Less than or equal to operator to perform a less than or equal to
   *comparison on two SafeUint256_t.
   *@param other The SafeUint256_t to perform the less than or equal to
   *comparison with.
   *@return The result of the less than or equal to comparison (true if less
   *than or equal to, false otherwise).
   */
  inline bool operator<=(const SafeUint256_t &other) const {
    check();
    return *valuePtr <= other.get();
  };

  /**
   *@brief Less than or equal to operator to perform a less than or equal to
   *comparison on a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the less than or equal to comparison
   *with.
   *@return The result of the less than or equal to comparison (true if less
   *than or equal to, false otherwise).
   */
  inline bool operator<=(const uint256_t &other) const {
    check();
    return *valuePtr <= other;
  };

  /**
   *@brief Less than or equal to operator to perform a less than or equal to
   *comparison on a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the less than or equal to comparison
   *with.
   *@return The result of the less than or equal to comparison (true if less
   *than or equal to, false otherwise).
   */
  inline bool operator<=(const uint64_t &other) const {
    check();
    return *valuePtr <= other;
  };

  /**
   *@brief Greater than operator to perform a greater than comparison on two
   *SafeUint256_t.
   *@param other The SafeUint256_t to perform the greater than comparison with.
   *@return The result of the greater than comparison (true if greater than,
   *false otherwise).
   */
  inline bool operator>(const SafeUint256_t &other) const {
    check();
    return *valuePtr > other.get();
  };

  /**
   *@brief Greater than operator to perform a greater than comparison on a
   *SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the greater than comparison with.
   *@return The result of the greater than comparison (true if greater than,
   *false otherwise).
   */
  inline bool operator>(const uint256_t &other) const {
    check();
    return *valuePtr > other;
  };

  /**
   *@brief Greater than operator to perform a greater than comparison on a
   *SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the greater than comparison with.
   *@return The result of the greater than comparison (true if greater than,
   *false otherwise).
   */
  inline bool operator>(const uint64_t &other) const {
    check();
    return *valuePtr > other;
  };

  /**
   *@brief Greater than or equal to operator to perform a greater than or equal
   *to comparison on two SafeUint256_t.
   *@param other The SafeUint256_t to perform the greater than or equal to
   *comparison with.
   *@return The result of the greater than or equal to comparison (true if
   *greater than or equal to, false otherwise).
   */
  inline bool operator>=(const SafeUint256_t &other) const {
    check();
    return *valuePtr >= other.get();
  };

  /**
   *@brief Greater than or equal to operator to perform a greater than or equal
   *to comparison on a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform the greater than or equal to
   *comparison with.
   *@return The result of the greater than or equal to comparison (true if
   *greater than or equal to, false otherwise).
   */
  inline bool operator>=(const uint256_t &other) const {
    check();
    return *valuePtr >= other;
  };

  /**
   *@brief Greater than or equal to operator to perform a greater than or equal
   *to comparison on a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform the greater than or equal to
   *comparison with.
   *@return The result of the greater than or equal to comparison (true if
   *greater than or equal to, false otherwise).
   */
  inline bool operator>=(const uint64_t &other) const {
    check();
    return *valuePtr >= other;
  };

  // Assignment Operators

  /**
   *@brief Assignment operator to assign a SafeUint256_t to another
   *SafeUint256_t.
   *@param other The SafeUint256_t to assign to the SafeUint256_t.
   *@return The SafeUint256_t after assignment.
   */
  inline SafeUint256_t &operator=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    *valuePtr = other.get();
    return *this;
  };

  /**
   *@brief Assignment operator to assign a uint256_t to a SafeUint256_t.
   *@param other The uint256_t to assign to the SafeUint256_t.
   *@return The SafeUint256_t after assignment.
   */
  inline SafeUint256_t &operator=(const uint256_t &other) {
    check();
    markAsUsed();
    *valuePtr = other;
    return *this;
  };

  /**
   *@brief Assignment operator to assign a uint64_t to a SafeUint256_t.
   *@param other The uint64_t to assign to the SafeUint256_t.
   *@return The SafeUint256_t after assignment.
   */
  inline SafeUint256_t &operator=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr = other;
    return *this;
  };

  /**
   *@brief Addition assignment operator to add a SafeUint256_t to another
   *SafeUint256_t.
   *@param other The SafeUint256_t to add to the SafeUint256_t.
   *@return The SafeUint256_t after addition.
   *@throws std::overflow_error if the addition operation results in an
   *overflow.
   */
  inline SafeUint256_t &operator+=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint256_t>::max() - other.get()) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other.get();
    return *this;
  };

  /**
   *@brief Addition assignment operator to add a uint256_t to a SafeUint256_t.
   *@param other The uint256_t to add to the SafeUint256_t.
   *@return The SafeUint256_t after addition.
   *@throws std::overflow_error if the addition operation results in an
   *overflow.
   */
  inline SafeUint256_t &operator+=(const uint256_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint256_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other;
    return *this;
  };

  /**
   *@brief Addition assignment operator to add a uint64_t to a SafeUint256_t.
   *@param other The uint64_t to add to the SafeUint256_t.
   *@return The SafeUint256_t after addition.
   *@throws std::overflow_error if the addition operation results in an
   *overflow.
   */
  inline SafeUint256_t &operator+=(const uint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr > std::numeric_limits<uint256_t>::max() - other) {
      throw std::overflow_error("Overflow in addition operation");
    }
    *valuePtr += other;
    return *this;
  };

  /**
   *@brief Subtraction assignment operator to subtract a SafeUint256_t from
   *another SafeUint256_t.
   *@param other The SafeUint256_t to subtract from the SafeUint256_t.
   *@return The SafeUint256_t after subtraction.
   *@throws std::underflow_error if the subtraction operation results in an
   *underflow.
   */
  inline SafeUint256_t &operator-=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other.get()) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other.get();
    return *this;
  };

  /**
   *@brief Subtraction assignment operator to subtract a uint256_t from a
   *SafeUint256_t.
   *@param other The uint256_t to subtract from the SafeUint256_t.
   *@return The SafeUint256_t after subtraction.
   *@throws std::underflow_error if the subtraction operation results in an
   *underflow.
   */
  inline SafeUint256_t &operator-=(const uint256_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other;
    return *this;
  };

  /**
   *@brief Subtraction assignment operator to subtract a uint64_t from a
   *SafeUint256_t.
   *@param other The uint64_t to subtract from the SafeUint256_t.
   *@return The SafeUint256_t after subtraction.
   *@throws std::underflow_error if the subtraction operation results in an
   *underflow.
   */
  inline SafeUint256_t &operator-=(const uint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr < other) {
      throw std::underflow_error("Underflow in subtraction operation");
    }
    *valuePtr -= other;
    return *this;
  };

  /**
   *@brief Multiplication assignment operator to multiply a SafeUint256_t by
   *another SafeUint256_t.
   *@param other The SafeUint256_t to multiply the SafeUint256_t by.
   *@return The SafeUint256_t after multiplication.
   *@throws std::overflow_error if the multiplication operation results in an
   *overflow.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator*=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    if (other.get() == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint256_t>::max() / other.get()) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other.get();
    return *this;
  };

  /**
   *@brief Multiplication assignment operator to multiply a SafeUint256_t by a
   *uint256_t.
   *@param other The uint256_t to multiply the SafeUint256_t by.
   *@return The SafeUint256_t after multiplication.
   *@throws std::overflow_error if the multiplication operation results in an
   *overflow.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator*=(const uint256_t &other) {
    check();
    markAsUsed();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint256_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other;
    return *this;
  };

  /**
   *@brief Multiplication assignment operator to multiply a SafeUint256_t by a
   *uint64_t.
   *@param other The uint64_t to multiply the SafeUint256_t by.
   *@return The SafeUint256_t after multiplication.
   *@throws std::overflow_error if the multiplication operation results in an
   *overflow.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator*=(const uint64_t &other) {
    check();
    markAsUsed();
    if (other == 0 || *valuePtr == 0) {
      throw std::domain_error("Multiplication by zero");
    }
    if (*valuePtr > std::numeric_limits<uint256_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation");
    }
    *valuePtr *= other;
    return *this;
  };

  /**
   *@brief Division assignment operator to divide a SafeUint256_t by another
   *SafeUint256_t.
   *@param other The SafeUint256_t to divide the SafeUint256_t by.
   *@return The SafeUint256_t after division.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator/=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other.get();
    return *this;
  };

  /**
   *@brief Division assignment operator to divide a SafeUint256_t by a
   *uint256_t.
   *@param other The uint256_t to divide the SafeUint256_t by.
   *@return The SafeUint256_t after division.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator/=(const uint256_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other;
    return *this;
  };

  /**
   *@brief Division assignment operator to divide a SafeUint256_t by a
   *uint64_t.
   *@param other The uint64_t to divide the SafeUint256_t by.
   *@return The SafeUint256_t after division.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator/=(const uint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Division by zero");
    }
    *valuePtr /= other;
    return *this;
  };

  /**
   *@brief Modulo assignment operator to calculate the remainder of a
   *SafeUint256_t divided by another SafeUint256_t.
   *@param other The SafeUint256_t to divide the SafeUint256_t by.
   *@return The SafeUint256_t after modulo.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator%=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other.get() == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other.get();
    return *this;
  };

  /**
   *@brief Modulo assignment operator to calculate the remainder of a
   *SafeUint256_t divided by a uint256_t.
   *@param other The uint256_t to divide the SafeUint256_t by.
   *@return The SafeUint256_t after modulo.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator%=(const uint256_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other;
    return *this;
  };

  /**
   *@brief Modulo assignment operator to calculate the remainder of a
   *SafeUint256_t divided by a uint64_t.
   *@param other The uint64_t to divide the SafeUint256_t by.
   *@return The SafeUint256_t after modulo.
   *@throws std::domain_error if the SafeUint256_t or other is zero.
   */
  inline SafeUint256_t &operator%=(const uint64_t &other) {
    check();
    markAsUsed();
    if (*valuePtr == 0 || other == 0) {
      throw std::domain_error("Modulo by zero");
    }
    *valuePtr %= other;
    return *this;
  };

  /**
   *@brief Bitwise AND assignment operator to perform a bitwise AND operation
   *between two SafeUint256_t.
   *@param other The SafeUint256_t to perform a bitwise AND with.
   *@return The SafeUint256_t after the bitwise AND operation.
   */
  inline SafeUint256_t &operator&=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other.get();
    return *this;
  };

  /**
   *@brief Bitwise AND assignment operator to perform a bitwise AND operation
   *between a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform a bitwise AND with.
   *@return The SafeUint256_t after the bitwise AND operation.
   */
  inline SafeUint256_t &operator&=(const uint256_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other;
    return *this;
  };

  /**
   *@brief Bitwise AND assignment operator to perform a bitwise AND operation
   *between a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform a bitwise AND with.
   *@return The SafeUint256_t after the bitwise AND operation.
   */
  inline SafeUint256_t &operator&=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr &= other;
    return *this;
  };

  /**
   *@brief Bitwise OR assignment operator to perform a bitwise OR operation
   *between two SafeUint256_t.
   *@param other The SafeUint256_t to perform a bitwise OR with.
   *@return The SafeUint256_t after the bitwise OR operation.
   */
  inline SafeUint256_t &operator|=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other.get();
    return *this;
  };

  /**
   *@brief Bitwise OR assignment operator to perform a bitwise OR operation
   *between a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform a bitwise OR with.
   *@return The SafeUint256_t after the bitwise OR operation.
   */
  inline SafeUint256_t &operator|=(const uint256_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other;
    return *this;
  };

  /**
   *@brief Bitwise OR assignment operator to perform a bitwise OR operation
   *between a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform a bitwise OR with.
   *@return The SafeUint256_t after the bitwise OR operation.
   */
  inline SafeUint256_t &operator|=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr |= other;
    return *this;
  };

  /**
   *@brief Bitwise XOR assignment operator to perform a bitwise XOR operation
   *between two SafeUint256_t.
   *@param other The SafeUint256_t to perform a bitwise XOR with.
   *@return The SafeUint256_t after the bitwise XOR operation.
   */
  inline SafeUint256_t &operator^=(const SafeUint256_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other.get();
    return *this;
  };

  /**
   *@brief Bitwise XOR assignment operator to perform a bitwise XOR operation
   *between a SafeUint256_t and a uint256_t.
   *@param other The uint256_t to perform a bitwise XOR with.
   *@return The SafeUint256_t after the bitwise XOR operation.
   */
  inline SafeUint256_t &operator^=(const uint256_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other;
    return *this;
  };

  /**
   *@brief Bitwise XOR assignment operator to perform a bitwise XOR operation
   *between a SafeUint256_t and a uint64_t.
   *@param other The uint64_t to perform a bitwise XOR with.
   *@return The SafeUint256_t after the bitwise XOR operation.
   */
  inline SafeUint256_t &operator^=(const uint64_t &other) {
    check();
    markAsUsed();
    *valuePtr ^= other;
    return *this;
  };

  // Increment and Decrement Operators

  /**
   *@brief Prefix increment operator to increment a SafeUint256_t by 1.
   *@return The SafeUint256_t after the increment.
   *@throws std::overflow_error if the SafeUint256_t is already at its maximum
   *value.
   */
  inline SafeUint256_t &operator++() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint256_t>::max()) {
      throw std::overflow_error("Overflow in increment operation");
    }
    ++*valuePtr;
    return *this;
  };

  /**
   *@brief Prefix decrement operator to decrement a SafeUint256_t by 1.
   *@return The SafeUint256_t after the decrement.
   *@throws std::underflow_error if the SafeUint256_t is already at its minimum
   *value.
   */
  inline SafeUint256_t &operator--() {
    check();
    markAsUsed();
    if (*valuePtr == std::numeric_limits<uint256_t>::min()) {
      throw std::underflow_error("Underflow in increment operation");
    }
    --*valuePtr;
    return *this;
  };

  // get is used to properly get the value of a variable within another
  // SafeUint256_t (we need to call check!)

  /**
  Getter function used to access the value of the SafeUint256_t.
  @return The value of the SafeUint256_t.
  */
  inline uint256_t get() const {
    check();
    return *valuePtr;
  };

  /**
 Commit function used to commit the value of the SafeUint256_t to the value
 pointed to.
 */
  inline void commit() override {
    value = *valuePtr;
    valuePtr = nullptr;
    registered = false;
  };

  /**
  Revert function used to revert the value of the SafeUint256_t (nullify it).
  */
  inline void revert() const override {
    valuePtr = nullptr;
    registered = false;
  };
};

#endif // SAFEUINT256_T_H
