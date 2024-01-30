/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEUINT_T_H
#define SAFEUINT_T_H

#include <memory>
#include <boost/multiprecision/cpp_int.hpp>
#include "safebase.h"

/**
* Template for the type of a uint with the given size.
* @tparam Size The size of the uint.
*/
template <int Size>
struct UintType {
    /// The type of the uint with the given size.
    using type = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<Size, Size, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
};

/**
* Specialization for the type of a uint with 8 bits.
*/
template <>
struct UintType<8> {
    using type = uint8_t; ///< Type of the uint with 8 bits.
};

/**
* Specialization for the type of a uint with 16 bits.
*/
template <>
struct UintType<16> {
    using type = uint16_t; ///< Type of the uint with 16 bits.
};

/**
* Specialization for the type of a uint with 32 bits.
*/
template <>
struct UintType<32> {
    using type = uint32_t; ///< Type of the uint with 32 bits.
};

/**
* Specialization for the type of a uint with 64 bits.
*/
template <>
struct UintType<64> {
    using type = uint64_t; ///< Type of the uint with 64 bits.
};

/**
 * Template for a safe wrapper for a uint variable.
 */
template <int Size>
class SafeUint_t : public SafeBase {
private:
    using uint_t = typename UintType<Size>::type; ///< Type of the uint.
    uint_t value_; ///< The value.
    mutable std::unique_ptr<uint_t> valuePtr_; ///< Pointer to the value.

    inline void check() const override {
        if (valuePtr_ == nullptr) valuePtr_ = std::make_unique<uint_t>(value_);
    };

public:
    static_assert(Size >= 8 && Size <= 256 && Size % 8 == 0, "Size must be between 8 and 256 and a multiple of 8.");

    /**
     * Constructor.
     * @param owner The owner of the variable.
     * @param value The initial value.
     */
    SafeUint_t(DynamicContract* owner, const uint_t& value = 0)
      : SafeBase(owner), value_(0), valuePtr_(std::make_unique<uint_t>(value))
    {};

    /**
     * Constructor.
     * @param value The initial value.
     */
    SafeUint_t(const uint_t& value = 0)
      : SafeBase(nullptr), value_(0), valuePtr_(std::make_unique<uint_t>(value))
    {};

    /**
     * Copy constructor.
     * @param other The SafeUint_t to copy.
     */
    SafeUint_t(const SafeUint_t<Size>& other) : SafeBase(nullptr) {
      other.check(); value_ = 0; valuePtr_ = std::make_unique<uint_t>(*other.valuePtr_);
    };

    /**
    * Getter for the value.
    * @return The value.
    */
    inline uint_t get() const { check(); return *valuePtr_; };
    
    /**
    * Commit the value.
    */
    inline void commit() override { check(); value_ = *valuePtr_; valuePtr_ = nullptr; registered_ = false; };
    
    /**
    * Revert the value.
    */
    inline void revert() const override { valuePtr_ = nullptr; registered_ = false; };

    // ====================
    // Arithmetic operators
    // ====================

    /**
    * Addition operator.
    * @param other The SafeUint_t to add.
    * @throw std::overflow_error if an overflow happens.
    * @return A new SafeUint_t with the result of the addition.
    */
    inline SafeUint_t<Size> operator+(const SafeUint_t<Size>& other) const {
        check();
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() - other.get())
        {
            throw std::overflow_error("Overflow in addition operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ + other.get());
    }

    /**
    * Addition operator.
    * @param other The uint_t to add.
    * @throw std::overflow_error if an overflow happens.
    * @return A new SafeUint_t with the result of the addition.
    */
    inline SafeUint_t<Size> operator+(const uint_t& other) const {
        check();
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() - other)
        {
            throw std::overflow_error("Overflow in addition operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ + other);
    }

    /**
    * Addition operator.
    * @param other The int to add.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @return A new SafeUint_t with the result of the addition.
    */
    inline SafeUint_t<Size> operator+(const int& other) const {
        check();
        if (other < 0) {
            if (*valuePtr_ < static_cast<uint_t>(-other)) {
                throw std::underflow_error("Underflow in addition operation.");
            }
        } else {
            if (*valuePtr_ > std::numeric_limits<uint_t>::max() - other) {
                throw std::overflow_error("Overflow in addition operation.");
            }
        }
        return SafeUint_t<Size>(*valuePtr_ + other);
    }

    /**
    * Addition operator.
    * @param other The uint_t to add.
    * @throw std::overflow_error if an overflow happens.
    * @return A new SafeUint_t with the result of the addition.
    */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type 
    operator+(const uint_t& other) const {
        check();
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() - other)
        {
            throw std::overflow_error("Overflow in addition operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ + other);
    }


    /**
     * Subtraction operator.
     * @param other The SafeUint_t to subtract.
     * @throw std::underflow_error if an underflow happens.
     * @return A new SafeUint_t with the result of the subtraction.
     */
    inline SafeUint_t<Size> operator-(const SafeUint_t<Size>& other) const {
        check();
        if (*valuePtr_ < other.get())
        {
            throw std::underflow_error("Underflow in subtraction operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ - other.get());
    }

    /**
     * Subtraction operator.
     * @param other The uint_t to subtract.
     * @throw std::underflow_error if an underflow happens.
     * @return A new SafeUint_t with the result of the subtraction.
     */
    inline SafeUint_t<Size> operator-(const uint_t& other) const {
        check();
        if (*valuePtr_ < other)
        {
            throw std::underflow_error("Underflow in subtraction operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ - other);
    }

    /**
    * Subtraction operator.
    * @param other The uint_t to subtract.
    * @throw std::underflow_error if an underflow happens.
    * @return A new SafeUint_t with the result of the subtraction.
    */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type 
    operator-(const uint_t& other) const {
        check();
        if (*valuePtr_ < other)
        {
            throw std::underflow_error("Underflow in subtraction operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ - other);
    }

    /**
    * Subtraction operator.
    * @param other The int to subtract.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @return A new SafeUint_t with the result of the subtraction.
    */
    inline SafeUint_t<Size> operator-(const int& other) const {
        check();
        if (other > 0) {
            if (*valuePtr_ < static_cast<uint_t>(other)) {
                throw std::underflow_error("Underflow in subtraction operation.");
            }
        } else {
            if (*valuePtr_ > std::numeric_limits<uint_t>::max() + other) {
                throw std::overflow_error("Overflow in subtraction operation.");
            }
        }
        return SafeUint_t<Size>(*valuePtr_ - other);
    }


    /**
     * Multiplication operator.
     * @param other The SafeUint_t to multiply.
     * @throw std::overflow_error if an overflow happens.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeUint_t with the result of the multiplication.
     */
    inline SafeUint_t<Size> operator*(const SafeUint_t<Size>& other) const {
        check();
        if (other.get() == 0 || *valuePtr_ == 0) throw std::domain_error("Multiplication by zero");
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() / other.get())
        {
            throw std::overflow_error("Overflow in multiplication operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ * other.get());
    }

    /**
     * Multiplication operator.
     * @param other The uint_t to multiply.
     * @throw std::overflow_error if an overflow happens.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeUint_t with the result of the multiplication.
     */

    inline SafeUint_t<Size> operator*(const uint_t& other) const {
        check();
        if (other == 0 || *valuePtr_ == 0) throw std::domain_error("Multiplication by zero");
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() / other)
        {
            throw std::overflow_error("Overflow in multiplication operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ * other);
    }

   /**
    * Multiplication operator.
    * @param other The uint_t to multiply.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::domain_error if the other value is zero.
    * @return A new SafeUint_t with the result of the multiplication.
    */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type
    operator*(const uint_t& other) const {
        check();
        if (other == 0 || *valuePtr_ == 0) throw std::domain_error("Multiplication by zero");
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() / other)
        {
            throw std::overflow_error("Overflow in multiplication operation.");
        }
        return SafeUint_t<Size>(*valuePtr_ * other);
    }

    /**
    * Multiplication operator.
    * @param other The int to multiply with.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @return A new SafeUint_t with the result of the multiplication.
    */
    inline SafeUint_t<Size> operator*(const int& other) const {
        check();
        if (other == 0 || *valuePtr_ == 0) throw std::domain_error("Multiplication by zero");
        
        if (other < 0) {
            throw std::underflow_error("Underflow in multiplication operation.");
        } else {
            if (*valuePtr_ > std::numeric_limits<uint_t>::max() / other) {
                throw std::overflow_error("Overflow in multiplication operation.");
            }
        }
        return SafeUint_t<Size>(*valuePtr_ * other);
    }


    /**
     * Division operator.
     * @param other The SafeUint_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeUint_t with the result of the division.
     */
    inline SafeUint_t<Size> operator/(const SafeUint_t<Size>& other) const {
        check();
        if (*valuePtr_ == 0 || other.get() == 0) throw std::domain_error("Division by zero");
        return SafeUint_t<Size>(*valuePtr_ / other.get());
    }

    /**
     * Division operator.
     * @param other The uint_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeUint_t with the result of the division.
     */
    inline SafeUint_t<Size> operator/(const uint_t& other) const {
        check();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Division by zero");
        return SafeUint_t<Size>(*valuePtr_ / other);
    }

    /**
    * Division operator.
    * @param other The uint_t to divide.
    * @throw std::domain_error if the other value is zero.
    * @return A new SafeUint_t with the result of the division.
    */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type
    operator/(const uint_t& other) const {
        check();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Division by zero");
        return SafeUint_t<Size>(*valuePtr_ / other);
    }


    /**
    * Division operator.
    * @param other The int to divide by.
    * @throw std::domain_error if the other value is zero.
    * @return A new SafeUint_t with the result of the division.
    */
    inline SafeUint_t<Size> operator/(const int& other) const {
        check();
        if (other == 0) throw std::domain_error("Division by zero");

        // Division by a negative number results in a negative result,
        // which cannot be represented in an unsigned integer.
        if (other < 0) throw std::domain_error("Division by a negative number");

        return SafeUint_t<Size>(*valuePtr_ / other);
    }


    /**
     * Modulus operator.
     * @param other The SafeUint_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeUint_t with the result of the modulus.
     */
    inline SafeUint_t<Size> operator%(const SafeUint_t<Size>& other) const {
        check();
        if (*valuePtr_ == 0 || other.get() == 0) throw std::domain_error("Modulus by zero");
        return SafeUint_t<Size>(*valuePtr_ % other.get());
    }

    /**
     * Modulus operator.
     * @param other The uint_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeUint_t with the result of the modulus.
     */
    inline SafeUint_t<Size> operator%(const uint_t& other) const {
        check();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Modulus by zero");
        return SafeUint_t<Size>(*valuePtr_ % other);
    }

    /**
     * Modulus operator.
     * @param other The uint64_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeUint_t with the result of the modulus.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type
    operator%(const uint64_t& other) const {
        check();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Modulus by zero");
        return SafeUint_t<Size>(*valuePtr_ % other);
    }

    /**
     * Modulo operator.
     * @param other The int to modulo.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeUint_t with the result of the modulo.
     */
    inline SafeUint_t<Size> operator%(const int& other) const {
        check();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Modulo by zero");
        return SafeUint_t<Size>(*valuePtr_ % static_cast<uint_t>(other));
    }

    // =================
    // Bitwise operators
    // =================

    /**
     * Bitwise AND operator.
     * @param other The SafeUint_t to AND.
     * @return A new SafeUint_t with the result of the AND.
     */
    inline SafeUint_t<Size> operator&(const SafeUint_t<Size>& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ & other.get());
    }

    /**
     * Bitwise AND operator.
     * @param other The uint_t to AND.
     * @return A new SafeUint_t with the result of the AND.
     */
    inline SafeUint_t<Size> operator&(const uint_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ & other);
    }

    /**
     * Bitwise AND operator.
     * @param other The uint64_t to AND.
     * @return A new SafeUint_t with the result of the AND.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type
    operator&(const uint64_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ & other);
    }

     /**
     * Bitwise AND operator.
     * @param other The int to AND.
     * @return A new SafeUint_t with the result of the AND.
     */
    inline SafeUint_t<Size> operator&(const int& other) const {
        check();
        if (other < 0) throw std::domain_error("Bitwise AND with a negative number");
        return SafeUint_t<Size>(*valuePtr_ & static_cast<uint_t>(other));
    }

    /**
     * Bitwise OR operator.
     * @param other The SafeUint_t to OR.
     * @return A new SafeUint_t with the result of the OR.
     */
    inline SafeUint_t<Size> operator|(const SafeUint_t<Size>& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ | other.get());
    }

    /**
     * Bitwise OR operator.
     * @param other The uint_t to OR.
     * @return A new SafeUint_t with the result of the OR.
     */
    inline SafeUint_t<Size> operator|(const uint_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ | other);
    }

    /**
     * Bitwise OR operator.
     * @param other The uint64_t to OR.
     * @return A new SafeUint_t with the result of the OR.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type
    operator|(const uint64_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ | other);
    }

    /**
     * Bitwise OR operator.
     * @param other The int to OR.
     * @return A new SafeUint_t with the result of the OR.
     */
    inline SafeUint_t<Size> operator|(const int& other) const {
        check();
        if (other < 0) throw std::domain_error("Bitwise OR with a negative number");
        return SafeUint_t<Size>(*valuePtr_ | static_cast<uint_t>(other));
    }

    /**
     * Bitwise XOR operator.
     * @param other The SafeUint_t to XOR.
     * @return A new SafeUint_t with the result of the XOR.
     */
    inline SafeUint_t<Size> operator^(const SafeUint_t<Size>& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ ^ other.get());
    }

    /**
     * Bitwise XOR operator.
     * @param other The uint_t to XOR.
     * @return A new SafeUint_t with the result of the XOR.
     */
    inline SafeUint_t<Size> operator^(const uint_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ ^ other);
    }

    /**
     * Bitwise XOR operator.
     * @param other The uint64_t to XOR.
     * @return A new SafeUint_t with the result of the XOR.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type
    operator^(const uint64_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ ^ other);
    }

    /**
     * Bitwise XOR operator.
     * @param other The int to XOR.
     * @return A new SafeUint_t with the result of the XOR.
     */
    inline SafeUint_t<Size> operator^(const int& other) const {
        check();
        if (other < 0) throw std::domain_error("Bitwise XOR with a negative number");
        return SafeUint_t<Size>(*valuePtr_ ^ static_cast<uint_t>(other));
    }

    /**
     * Left shift operator.
     * @param other The SafeUint_t to shift.
     * @return A new SafeUint_t with the result of the shift.
     */
    inline SafeUint_t<Size> operator<<(const SafeUint_t<Size>& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ << other.get());
    }

    /**
     * Left shift operator.
     * @param other The uint_t to shift.
     * @return A new SafeUint_t with the result of the shift.
     */
    inline SafeUint_t<Size> operator<<(const uint_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ << other);
    }

    /**
     * Left shift operator.
     * @param other The uint64_t to shift.
     * @return A new SafeUint_t with the result of the shift.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type
    operator<<(const uint64_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ << other);
    }

    /**
     * Left shift operator.
     * @param other The int to shift.
     * @return A new SafeUint_t with the result of the shift.
     */
    inline SafeUint_t<Size> operator<<(const int& other) const {
        check();
        if (other < 0) throw std::domain_error("Bitwise left shift with a negative number");
        return SafeUint_t<Size>(*valuePtr_ << other);
    }

    /**
     * Right shift operator.
     * @param other The SafeUint_t to shift.
     * @return A new SafeUint_t with the result of the shift.
     */
    inline SafeUint_t<Size> operator>>(const SafeUint_t<Size>& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ >> other.get());
    }

    /**
     * Right shift operator.
     * @param other The uint_t to shift.
     * @return A new SafeUint_t with the result of the shift.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>>::type
    operator>>(const uint_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ >> other);
    }

    /**
     * Right shift operator.
     * @param other The uint64_t to shift.
     * @return A new SafeUint_t with the result of the shift.
     */
    inline SafeUint_t<Size> operator>>(const uint64_t& other) const {
        check();
        return SafeUint_t<Size>(*valuePtr_ >> other);
    }

    /**
     * Right shift operator.
     * @param other The int to shift.
     * @return A new SafeUint_t with the result of the shift.
     */
    inline SafeUint_t<Size> operator>>(const int& other) const {
        check();
        if (other < 0) throw std::domain_error("Bitwise right shift with a negative number");
        return SafeUint_t<Size>(*valuePtr_ >> other);
    }

    // =================
    // Logical operators
    // =================

    /**
     * Logical NOT operator.
     * @return True if the value is zero, false otherwise.
     */
    inline bool operator!() const {
        check();
        return !(*valuePtr_);
    }

    /**
     * Logical AND operator.
     * @param other The SafeUint_t to AND.
     * @return True if both values are not zero, false otherwise.
     */
    inline bool operator&&(const SafeUint_t<Size>& other) const {
        check();
        return *valuePtr_ && other.get();
    }

    /**
     * Logical AND operator.
     * @param other The uint_t to AND.
     * @return True if both values are not zero, false otherwise.
     */
    inline bool operator&&(const uint_t& other) const {
        check();
        return *valuePtr_ && other;
    }

    /**
    * Logical AND operator.
    * @param other The uint_t to AND.
    * @return True if both values are not zero, false otherwise.
    */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, bool>::type
    operator&&(const uint_t& other) const {
        check();
        return *valuePtr_ && other;
    }


    /**
     * Logical OR operator.
     * @param other The SafeUint_t to OR.
     * @return True if at least one value is not zero, false otherwise.
     */
    inline bool operator||(const SafeUint_t<Size>& other) const {
        check();
        return *valuePtr_ || other.get();
    }

    /**
     * Logical OR operator.
     * @param other The uint_t to OR.
     * @return True if at least one value is not zero, false otherwise.
     */
    inline bool operator||(const uint_t& other) const {
        check();
        return *valuePtr_ || other;
    }

    /**
     * Logical OR operator.
     * @param other The uint64_t to OR.
     * @return True if at least one value is not zero, false otherwise.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, bool>::type
    operator||(const uint64_t& other) const {
        check();
        return *valuePtr_ || other;
    }

    // ====================
    // Comparison operators
    // ====================

    /**
     * Equality operator.
     * @param other The SafeUint_t to compare.
     * @return True if both values are equal, false otherwise.
     */
    inline bool operator==(const SafeUint_t<Size>& other) const {
        check();
        return *valuePtr_ == other.get();
    }

    /**
     * Equality operator.
     * @param other The uint_t to compare.
     * @return True if both values are equal, false otherwise.
     */
    inline bool operator==(const uint_t& other) const {
        check();
        return *valuePtr_ == other;
    }

    /**
     * Equality operator.
     * @param other The uint64_t to compare.
     * @return True if both values are equal, false otherwise.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, bool>::type
    operator==(const uint64_t& other) const {
        check();
        return *valuePtr_ == other;
    }

    /**
    * Equality operator.
    * @param other The int to compare.
    * @return True if both values are equal, false otherwise.
    */
    inline bool operator==(const int& other) const {
        check();
        if (other < 0) {
            return false;  // unsigned value cannot be equal to negative int
        }
        return *valuePtr_ == static_cast<uint_t>(other);
    }


    /**
     * Inequality operator.
     * @param other The SafeUint_t to compare.
     * @return True if both values are not equal, false otherwise.
     */
    inline bool operator!=(const SafeUint_t<Size>& other) const {
        check();
        return *valuePtr_ != other.get();
    }

    /**
     * Inequality operator.
     * @param other The uint_t to compare.
     * @return True if both values are not equal, false otherwise.
     */
    inline bool operator!=(const uint_t& other) const {
        check();
        return *valuePtr_ != other;
    }

    /**
     * Inequality operator.
     * @param other The uint64_t to compare.
     * @return True if both values are not equal, false otherwise.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, bool>::type
    operator!=(const uint64_t& other) const {
        check();
        return *valuePtr_ != other;
    }

    /**
     * Less than operator.
     * @param other The SafeUint_t to compare.
     * @return True if the value is less than the other value, false otherwise.
     */
    inline bool operator<(const SafeUint_t<Size>& other) const {
        check();
        return *valuePtr_ < other.get();
    }

    /**
     * Less than operator.
     * @param other The uint_t to compare.
     * @return True if the value is less than the other value, false otherwise.
     */
    inline bool operator<(const uint_t& other) const {
        check();
        return *valuePtr_ < other;
    }

    /**
     * Less than operator.
     * @param other The uint64_t to compare.
     * @return True if the value is less than the other value, false otherwise.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, bool>::type
    operator<(const uint64_t& other) const {
        check();
        return *valuePtr_ < other;
    }

    /**
     * Less than or equal operator.
     * @param other The SafeUint_t to compare.
     * @return True if the value is less than or equal to the other value, false otherwise.
     */
    inline bool operator<=(const SafeUint_t<Size>& other) const {
        check();
        return *valuePtr_ <= other.get();
    }

    /**
     * Less than or equal operator.
     * @param other The uint_t to compare.
     * @return True if the value is less than or equal to the other value, false otherwise.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, bool>::type
    operator<=(const uint_t& other) const {
        check();
        return *valuePtr_ <= other;
    }

    /**
     * Less than or equal operator.
     * @param other The uint64_t to compare.
     * @return True if the value is less than or equal to the other value, false otherwise.
     */
    inline bool operator<=(const uint64_t& other) const {
        check();
        return *valuePtr_ <= other;
    }

    /**
     * Greater than operator.
     * @param other The SafeUint_t to compare.
     * @return True if the value is greater than the other value, false otherwise.
     */
    inline bool operator>(const SafeUint_t<Size>& other) const {
        check();
        return *valuePtr_ > other.get();
    }

    /**
     * Greater than operator.
     * @param other The uint_t to compare.
     * @return True if the value is greater than the other value, false otherwise.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, bool>::type
    operator>(const uint_t& other) const {
        check();
        return *valuePtr_ > other;
    }

    /**
     * Greater than operator.
     * @param other The uint64_t to compare.
     * @return True if the value is greater than the other value, false otherwise.
     */
    inline bool operator>(const uint64_t& other) const {
        check();
        return *valuePtr_ > other;
    }

    /**
     * Greater than or equal operator.
     * @param other The SafeUint_t to compare.
     * @return True if the value is greater than or equal to the other value, false otherwise.
     */
    inline bool operator>=(const SafeUint_t<Size>& other) const {
        check();
        return *valuePtr_ >= other.get();
    }

    /**
     * Greater than or equal operator.
     * @param other The uint_t to compare.
     * @return True if the value is greater than or equal to the other value, false otherwise.
     */
    inline bool operator>=(const uint_t& other) const {
        check();
        return *valuePtr_ >= other;
    }

    /**
     * Greater than or equal operator.
     * @param other The uint64_t to compare.
     * @return True if the value is greater than or equal to the other value, false otherwise.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, bool>::type
    operator>=(const uint64_t& other) const {
        check();
        return *valuePtr_ >= other;
    }

    // ====================
    // Assignment operators
    // ====================

    /**
     * Assignment operator.
     * @param other The SafeUint_t to assign.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ = other.get();
        return *this;
    }

    /**
     * Assignment operator.
     * @param other The uint_t to assign.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator=(const uint_t& other) {
        check();
        markAsUsed();
        *valuePtr_ = other;
        return *this;
    }

   /**
    * Assignment operator.
    * @param other The uint_t to assign.
    * @return A reference to this SafeUint_t.
    */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator=(const uint_t& other) {
        check();
        markAsUsed();
        *valuePtr_ = other;
        return *this;
    }

    /**
    * Assignment operator.
    * @param other The int to assign.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator=(const int& other) {
        check();
        if (other < 0) {
            throw std::domain_error("Cannot assign negative value to SafeUint_t");
        }
        markAsUsed();
        *valuePtr_ = static_cast<uint_t>(other);
        return *this;
    }


    /**
     * Addition assignment operator.
     * @param other The SafeUint_t to add.
     * @throw std::overflow_error if an overflow happens.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator+=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() - other.get())
        {
            throw std::overflow_error("Overflow in addition assignment operation.");
        }
        *valuePtr_ += other.get();
        return *this;
    }

    /**
     * Addition assignment operator.
     * @param other The uint_t to add.
     * @throw std::overflow_error if an overflow happens.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator+=(const uint_t& other) {
        check();
        markAsUsed();
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() - other)
        {
            throw std::overflow_error("Overflow in addition assignment operation.");
        }
        *valuePtr_ += other;
        return *this;
    }

    /**
     * Addition assignment operator.
     * @param other The uint64_t to add.
     * @throw std::overflow_error if an overflow happens.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator+=(const uint64_t& other) {
        check();
        markAsUsed();
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() - other)
        {
            throw std::overflow_error("Overflow in addition assignment operation.");
        }
        *valuePtr_ += other;
        return *this;
    }

    /**
    * Addition assignment operator.
    * @param other The int to add.
    * @throw std::overflow_error if an overflow happens.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator+=(const int& other) {
        check();
        markAsUsed();
        if (other < 0 || static_cast<uint64_t>(other) > std::numeric_limits<uint_t>::max() - *valuePtr_)
        {
            throw std::overflow_error("Overflow in addition assignment operation.");
        }
        *valuePtr_ += static_cast<uint_t>(other);
        return *this;
    }


    /**
     * Subtraction assignment operator.
     * @param other The SafeUint_t to subtract.
     * @throw std::underflow_error if an underflow happens.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator-=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        if (*valuePtr_ < other.get())
        {
            throw std::underflow_error("Underflow in subtraction assignment operation.");
        }
        *valuePtr_ -= other.get();
        return *this;
    }

    /**
     * Subtraction assignment operator.
     * @param other The uint_t to subtract.
     * @throw std::underflow_error if an underflow happens.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator-=(const uint_t& other) {
        check();
        markAsUsed();
        if (*valuePtr_ < other)
        {
            throw std::underflow_error("Underflow in subtraction assignment operation.");
        }
        *valuePtr_ -= other;
        return *this;
    }

    /**
     * Subtraction assignment operator.
     * @param other The uint64_t to subtract.
     * @throw std::underflow_error if an underflow happens.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator-=(const uint64_t& other) {
        check();
        markAsUsed();
        if (*valuePtr_ < other)
        {
            throw std::underflow_error("Underflow in subtraction assignment operation.");
        }
        *valuePtr_ -= other;
        return *this;
    }

    /**
    * Subtraction assignment operator.
    * @param other The int to subtract.
    * @throw std::underflow_error if an underflow happens.
    * @throw std::invalid_argument if other is negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator-=(const int& other) {
        check();
        markAsUsed();
        if (other < 0) {
            throw std::invalid_argument("Cannot subtract a negative value.");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        if (*valuePtr_ < other_uint)
        {
            throw std::underflow_error("Underflow in subtraction assignment operation.");
        }
        *valuePtr_ -= other_uint;
        return *this;
    }

    /**
     * Multiplication assignment operator.
     * @param other The SafeUint_t to multiply.
     * @throw std::overflow_error if an overflow happens.
     * @throw std::domain_error if the other value is zero.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator*=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        if (other.get() == 0 || *valuePtr_ == 0) throw std::domain_error("Multiplication assignment by zero");
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() / other.get())
        {
            throw std::overflow_error("Overflow in multiplication assignment operation.");
        }
        *valuePtr_ *= other.get();
        return *this;
    }

    /**
     * Multiplication assignment operator.
     * @param other The uint_t to multiply.
     * @throw std::overflow_error if an overflow happens.
     * @throw std::domain_error if the other value is zero.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator*=(const uint_t& other) {
        check();
        markAsUsed();
        if (other == 0 || *valuePtr_ == 0) throw std::domain_error("Multiplication assignment by zero");
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() / other)
        {
            throw std::overflow_error("Overflow in multiplication assignment operation.");
        }
        *valuePtr_ *= other;
        return *this;
    }

    /**
    * Multiplication assignment operator.
    * @param other The int to multiply.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::domain_error if the other value is zero or negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator*=(const int& other) {
        check();
        markAsUsed();
        if (other < 0) {
            throw std::invalid_argument("Cannot multiply by a negative value.");
        }
        if (other == 0 || *valuePtr_ == 0) {
            throw std::domain_error("Multiplication assignment by zero");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        if (*valuePtr_ > std::numeric_limits<uint_t>::max() / other_uint)
        {
            throw std::overflow_error("Overflow in multiplication assignment operation.");
        }
        *valuePtr_ *= other_uint;
        return *this;
    }


    /**
     * Division assignment operator.
     * @param other The SafeUint_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator/=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        if (*valuePtr_ == 0 || other.get() == 0) throw std::domain_error("Division assignment by zero");
        *valuePtr_ /= other.get();
        return *this;
    }

    /**
     * Division assignment operator.
     * @param other The uint_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator/=(const uint_t& other) {
        check();
        markAsUsed();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Division assignment by zero");
        *valuePtr_ /= other;
        return *this;
    }

    /**
     * Division assignment operator.
     * @param other The uint64_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator/=(const uint64_t& other) {
        check();
        markAsUsed();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Division assignment by zero");
        *valuePtr_ /= other;
        return *this;
    }

    /**
    * Division assignment operator.
    * @param other The int to divide by.
    * @throw std::domain_error if the other value is zero or negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator/=(const int& other) {
        check();
        markAsUsed();
        if (other <= 0) {
            throw std::invalid_argument("Cannot divide by a non-positive value.");
        }
        if (*valuePtr_ == 0) {
            throw std::domain_error("Division assignment by zero");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        *valuePtr_ /= other_uint;
        return *this;
    }


    /**
     * Modulus assignment operator.
     * @param other The SafeUint_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator%=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        if (*valuePtr_ == 0 || other.get() == 0) throw std::domain_error("Modulus assignment by zero");
        *valuePtr_ %= other.get();
        return *this;
    }

    /**
     * Modulus assignment operator.
     * @param other The uint_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator%=(const uint_t& other) {
        check();
        markAsUsed();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Modulus assignment by zero");
        *valuePtr_ %= other;
        return *this;
    }

    /**
     * Modulus assignment operator.
     * @param other The uint64_t to divide.
     * @throw std::domain_error if the other value is zero.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator%=(const uint64_t& other) {
        check();
        markAsUsed();
        if (*valuePtr_ == 0 || other == 0) throw std::domain_error("Modulus assignment by zero");
        *valuePtr_ %= other;
        return *this;
    }

    /**
    * Modulus assignment operator.
    * @param other The int to divide by.
    * @throw std::domain_error if the other value is zero or negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator%=(const int& other) {
        check();
        markAsUsed();
        if (other <= 0) {
            throw std::invalid_argument("Cannot modulus by a non-positive value.");
        }
        if (*valuePtr_ == 0) {
            throw std::domain_error("Modulus assignment by zero");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        *valuePtr_ %= other_uint;
        return *this;
    }


    /**
     * Bitwise AND assignment operator.
     * @param other The SafeUint_t to AND.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator&=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ &= other.get();
        return *this;
    }

    /**
     * Bitwise AND assignment operator.
     * @param other The uint_t to AND.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator&=(const uint_t& other) {
        check();
        markAsUsed();
        *valuePtr_ &= other;
        return *this;
    }

    /**
     * Bitwise AND assignment operator.
     * @param other The uint64_t to AND.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator&=(const uint64_t& other) {
        check();
        markAsUsed();
        *valuePtr_ &= other;
        return *this;
    }

    /**
    * Bitwise AND assignment operator.
    * @param other The int to AND with.
    * @throw std::invalid_argument if the other value is negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator&=(const int& other) {
        check();
        markAsUsed();
        if (other < 0) {
            throw std::invalid_argument("Cannot perform bitwise operation with a negative value.");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        *valuePtr_ &= other_uint;
        return *this;
    }


    /**
     * Bitwise OR assignment operator.
     * @param other The SafeUint_t to OR.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator|=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ |= other.get();
        return *this;
    }

    /**
     * Bitwise OR assignment operator.
     * @param other The uint_t to OR.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator|=(const uint_t& other) {
        check();
        markAsUsed();
        *valuePtr_ |= other;
        return *this;
    }

    /**
     * Bitwise OR assignment operator.
     * @param other The uint64_t to OR.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator|=(const uint64_t& other) {
        check();
        markAsUsed();
        *valuePtr_ |= other;
        return *this;
    }

    /**
    * Bitwise OR assignment operator.
    * @param other The int to OR with.
    * @throw std::invalid_argument if the other value is negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator|=(const int& other) {
        check();
        markAsUsed();
        if (other < 0) {
            throw std::invalid_argument("Cannot perform bitwise operation with a negative value.");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        *valuePtr_ |= other_uint;
        return *this;
    }

    /**
     * Bitwise XOR assignment operator.
     * @param other The SafeUint_t to XOR.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator^=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ ^= other.get();
        return *this;
    }

    /**
     * Bitwise XOR assignment operator.
     * @param other The uint_t to XOR.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator^=(const uint_t& other) {
        check();
        markAsUsed();
        *valuePtr_ ^= other;
        return *this;
    }

    /**
     * Bitwise XOR assignment operator.
     * @param other The uint64_t to XOR.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator^=(const uint64_t& other) {
        check();
        markAsUsed();
        *valuePtr_ ^= other;
        return *this;
    }

    /**
    * Bitwise XOR assignment operator.
    * @param other The int to XOR with.
    * @throw std::invalid_argument if the other value is negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator^=(const int& other) {
        check();
        markAsUsed();
        if (other < 0) {
            throw std::invalid_argument("Cannot perform bitwise operation with a negative value.");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        *valuePtr_ ^= other_uint;
        return *this;
    }

    /**
     * Left shift assignment operator.
     * @param other The SafeUint_t to shift.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator<<=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ <<= other.get();
        return *this;
    }

    /**
     * Left shift assignment operator.
     * @param other The uint_t to shift.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator<<=(const uint_t& other) {
        check();
        markAsUsed();
        *valuePtr_ <<= other;
        return *this;
    }

    /**
     * Left shift assignment operator.
     * @param other The uint64_t to shift.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator<<=(const uint64_t& other) {
        check();
        markAsUsed();
        *valuePtr_ <<= other;
        return *this;
    }

    /**
    * Left shift assignment operator.
    * @param other The int to shift by.
    * @throw std::invalid_argument if the other value is negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator<<=(const int& other) {
        check();
        markAsUsed();
        if (other < 0) {
            throw std::invalid_argument("Cannot perform bitwise operation with a negative value.");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        *valuePtr_ <<= other_uint;
        return *this;
    }

    /**
     * Right shift assignment operator.
     * @param other The SafeUint_t to shift.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator>>=(const SafeUint_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ >>= other.get();
        return *this;
    }

    /**
     * Right shift assignment operator.
     * @param other The uint_t to shift.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator>>=(const uint_t& other) {
        check();
        markAsUsed();
        *valuePtr_ >>= other;
        return *this;
    }

    /**
     * Right shift assignment operator.
     * @param other The uint64_t to shift.
     * @return A reference to this SafeUint_t.
     */
    template<typename T = uint_t>
    inline typename std::enable_if<!std::is_same<T, uint64_t>::value, SafeUint_t<Size>&>::type
    operator>>=(const uint64_t& other) {
        check();
        markAsUsed();
        *valuePtr_ >>= other;
        return *this;
    }

    /**
    * Right shift assignment operator.
    * @param other The int to shift by.
    * @throw std::invalid_argument if the other value is negative.
    * @return A reference to this SafeUint_t.
    */
    inline SafeUint_t<Size>& operator>>=(const int& other) {
        check();
        markAsUsed();
        if (other < 0) {
            throw std::invalid_argument("Cannot perform bitwise operation with a negative value.");
        }
        uint_t other_uint = static_cast<uint_t>(other);
        *valuePtr_ >>= other_uint;
        return *this;
    }

    // =================================
    // Increment and decrement operators
    // =================================

    /**
     * Prefix increment operator.
     * @throw std::overflow_error if an overflow happens.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator++() {
        check();
        markAsUsed();
        if (*valuePtr_ == std::numeric_limits<uint_t>::max())
        {
            throw std::overflow_error("Overflow in prefix increment operation.");
        }
        ++(*valuePtr_);
        return *this;
    }

    /**
     * Postfix increment operator.
     * @throw std::overflow_error if an overflow happens.
     * @return A new SafeUint_t with the value before the increment.
     */
    inline SafeUint_t<Size> operator++(int) {
        check();
        markAsUsed();
        if (*valuePtr_ == std::numeric_limits<uint_t>::max())
        {
            throw std::overflow_error("Overflow in postfix increment operation.");
        }
        SafeUint_t<Size> tmp(*valuePtr_);
        ++(*valuePtr_);
        return tmp;
    }

    /**
     * Prefix decrement operator.
     * @throw std::underflow_error if an underflow happens.
     * @return A reference to this SafeUint_t.
     */
    inline SafeUint_t<Size>& operator--() {
        check();
        markAsUsed();
        if (*valuePtr_ == 0)
        {
            throw std::underflow_error("Underflow in prefix decrement operation.");
        }
        --(*valuePtr_);
        return *this;
    }

    /**
     * Postfix decrement operator.
     * @throw std::underflow_error if an underflow happens.
     * @return A new SafeUint_t with the value before the decrement.
     */
    inline SafeUint_t<Size> operator--(int) {
        check();
        markAsUsed();
        if (*valuePtr_ == 0)
        {
            throw std::underflow_error("Underflow in postfix decrement operation.");
        }
        SafeUint_t<Size> tmp(*valuePtr_);
        --(*valuePtr_);
        return tmp;
    }
};

#endif // SAFEUINT_T_H
