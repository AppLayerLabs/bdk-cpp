/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEINT_T_H
#define SAFEINT_T_H

#include <memory>
#include <boost/multiprecision/cpp_int.hpp>
#include "safebase.h"

/**
* Template for the type of an int with the given size.
* @tparam Size The size of the int.
*/
template <int Size>
struct IntType {
    /**
    * The type of the int.
    */
    using type = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<Size, Size, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
};

/**
* Template specialization for int8_t.
*/
template <>
struct IntType<8> {
    using type = int8_t; ///< int8_t type.
};

/**
* Template specialization for int16_t.
*/
template <>
struct IntType<16> {
    using type = int16_t;  ///< int16_t type.
};

/**
* Template specialization for int32_t.
*/
template <>
struct IntType<32> {
    using type = int32_t; ///< int32_t type.
};

/**
* Template specialization for int64_t.
*/
template <>
struct IntType<64> {
    using type = int64_t; ///< int64_t type.
};

/**
* SafeInt_t class template.
* @tparam Size The size of the int.
*/
template <int Size>
class SafeInt_t : public SafeBase {
private:
    using int_t = typename IntType<Size>::type; ///< The type of the int.
    int_t value_; ///< The value of the int.
    mutable std::unique_ptr<int_t> valuePtr_; ///< The pointer to the value of the int.

    /**
    * Check if the value is registered_ and if not, register it.
    */
    inline void check() const override {
        if (valuePtr_ == nullptr) valuePtr_ = std::make_unique<int_t>(value_);
    };

public:
    static_assert(Size >= 8 && Size <= 256 && Size % 8 == 0, "Size must be between 8 and 256 and a multiple of 8.");

    /**
    * Constructor.
    * @param owner The DynamicContract that owns this variable.
    * @param value The initial value of the variable.
    */
    SafeInt_t(DynamicContract* owner, const int_t& value = 0)
      : SafeBase(owner), value_(0), valuePtr_(std::make_unique<int_t>(value))
    {};

    /**
    * Constructor.
    * @param value The initial value of the variable.
    */
    SafeInt_t(const int_t& value = 0)
      : SafeBase(nullptr), value_(0), valuePtr_(std::make_unique<int_t>(value))
    {};

    /**
    * Copy constructor.
    * @param other The SafeInt_t to copy.
    */
    SafeInt_t(const SafeInt_t<Size>& other) : SafeBase(nullptr) {
      other.check(); value_ = 0; valuePtr_ = std::make_unique<int_t>(*other.valuePtr_);
    };

    /**
    * Getter for the value.
    * @return The value.
    */
    inline int_t get() const { check(); return *valuePtr_; };

    /**
    * Commit the value.
    */
    inline void commit() override { check(); value_ = *valuePtr_; valuePtr_ = nullptr; registered_ = false; };

    /**
    * Revert the value.
    */
    inline void revert() const override { valuePtr_ = nullptr; registered_ = false; };

    /**
    * Addition operator.
    * @param other The SafeInt_t to add.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @return A new SafeInt_t with the result of the addition.
    */
    inline SafeInt_t<Size> operator+(const SafeInt_t<Size>& other) const {
        check();
        if ((other.get() > 0) && (*valuePtr_ > std::numeric_limits<int_t>::max() - other.get())) {
            throw std::overflow_error("Overflow in addition operation.");
        }
        if ((other.get() < 0) && (*valuePtr_ < std::numeric_limits<int_t>::min() - other.get())) {
            throw std::underflow_error("Underflow in addition operation.");
        }
        return SafeInt_t<Size>(*valuePtr_ + other.get());
    }

    /**
    * Addition operator.
    * @param other The int_t to add.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @return A new SafeInt_t with the result of the addition.
    */
    inline SafeInt_t<Size> operator+(const int_t& other) const {
        check();
        if ((other > 0) && (*valuePtr_ > std::numeric_limits<int_t>::max() - other)) {
            throw std::overflow_error("Overflow in addition operation.");
        }
        if ((other < 0) && (*valuePtr_ < std::numeric_limits<int_t>::min() - other)) {
            throw std::underflow_error("Underflow in addition operation.");
        }
        return SafeInt_t<Size>(*valuePtr_ + other);
    }

    /**
    * Subtraction operator.
    * @param other The SafeInt_t to subtract.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @return A new SafeInt_t with the result of the subtraction.
    */
    inline SafeInt_t<Size> operator-(const SafeInt_t<Size>& other) const {
        check();
        if ((other.get() < 0) && (*valuePtr_ > std::numeric_limits<int_t>::max() + other.get())) {
            throw std::overflow_error("Overflow in subtraction operation.");
        }
        if ((other.get() > 0) && (*valuePtr_ < std::numeric_limits<int_t>::min() + other.get())) {
            throw std::underflow_error("Underflow in subtraction operation.");
        }
        return SafeInt_t<Size>(*valuePtr_ - other.get());
    }

    /**
    * Subtraction operator.
    * @param other The int_t to subtract.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @return A new SafeInt_t with the result of the subtraction.
    */
    inline SafeInt_t<Size> operator-(const int_t& other) const {
        check();
        if ((other < 0) && (*valuePtr_ > std::numeric_limits<int_t>::max() + other)) {
            throw std::overflow_error("Overflow in subtraction operation.");
        }
        if ((other > 0) && (*valuePtr_ < std::numeric_limits<int_t>::min() + other)) {
            throw std::underflow_error("Underflow in subtraction operation.");
        }
        return SafeInt_t<Size>(*valuePtr_ - other);
    }

    /**
    * Multiplication operator.
    * @param other The SafeInt_t to multiply.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @throw std::domain_error if multiplying by 0.
    * @return A new SafeInt_t with the result of the multiplication.
    */
    inline SafeInt_t<Size> operator*(const SafeInt_t<Size>& other) const {
        check();
        if (*valuePtr_ == 0 || other.get() == 0) {
            throw std::domain_error("Multiplication by zero.");
        }
        if (*valuePtr_ > std::numeric_limits<int_t>::max() / other.get()) {
            throw std::overflow_error("Overflow in multiplication operation.");
        }
        if (*valuePtr_ < std::numeric_limits<int_t>::min() / other.get()) {
            throw std::underflow_error("Underflow in multiplication operation.");
        }
        return SafeInt_t<Size>(*valuePtr_ * other.get());
    }

    /**
    * Multiplication operator.
    * @param other The int_t to multiply.
    * @throw std::overflow_error if an overflow happens.
    * @throw std::underflow_error if an underflow happens.
    * @throw std::domain_error if multiplying by 0.
    * @return A new SafeInt_t with the result of the multiplication.
    */
    inline SafeInt_t<Size> operator*(const int_t& other) const {
        check();
        if (*valuePtr_ == 0 || other == 0) {
            throw std::domain_error("Multiplication by zero.");
        }
        if (*valuePtr_ > std::numeric_limits<int_t>::max() / other) {
            throw std::overflow_error("Overflow in multiplication operation.");
        }
        if (*valuePtr_ < std::numeric_limits<int_t>::min() / other) {
            throw std::underflow_error("Underflow in multiplication operation.");
        }
        return SafeInt_t<Size>(*valuePtr_ * other);
    }

    /**
    * Division operator.
    * @param other The SafeInt_t to divide.
    * @throw std::domain_error if the other value is zero.
    * @throw std::overflow_error if division results in overflow.
    * @return A new SafeInt_t with the result of the division.
    */
    inline SafeInt_t<Size> operator/(const SafeInt_t<Size>& other) const {
        check();
        if (other.get() == 0) throw std::domain_error("Division by zero");

        // Handling the edge case where dividing the smallest negative number by -1 causes overflow
        if (*valuePtr_ == std::numeric_limits<int_t>::min() && other.get() == -1) {
            throw std::overflow_error("Overflow in division operation.");
        }

        return SafeInt_t<Size>(*valuePtr_ / other.get());
    }

    /**
    * Division operator.
    * @param other The int_t to divide.
    * @throw std::domain_error if the other value is zero.
    * @throw std::overflow_error if division results in overflow.
    * @return A new SafeInt_t with the result of the division.
    */
    inline SafeInt_t<Size> operator/(const int_t& other) const {
        check();
        if (other == 0) throw std::domain_error("Division by zero");

        // Handling the edge case where dividing the smallest negative number by -1 causes overflow
        if (*valuePtr_ == std::numeric_limits<int_t>::min() && other == -1) {
            throw std::overflow_error("Overflow in division operation.");
        }

        return SafeInt_t<Size>(*valuePtr_ / other);
    }

    /**
    * Modulus operator.
    * @param other The SafeInt_t to take the modulus by.
    * @throw std::domain_error if the other value is zero.
    * @return A new SafeInt_t with the result of the modulus.
    */
    inline SafeInt_t<Size> operator%(const SafeInt_t<Size>& other) const {
        check();
        if (other.get() == 0) throw std::domain_error("Modulus by zero");

        return SafeInt_t<Size>(*valuePtr_ % other.get());
    }

    /**
    * Modulus operator.
    * @param other The int_t to take the modulus by.
    * @throw std::domain_error if the other value is zero.
    * @return A new SafeInt_t with the result of the modulus.
    */
    inline SafeInt_t<Size> operator%(const int_t& other) const {
        check();
        if (other == 0) throw std::domain_error("Modulus by zero");

        return SafeInt_t<Size>(*valuePtr_ % other);
    }

    /**
    * Bitwise AND operator.
    * @param other The SafeInt_t to AND.
    * @return A new SafeInt_t with the result of the AND.
    */
    inline SafeInt_t<Size> operator&(const SafeInt_t<Size>& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ & other.get());
    }

    /**
    * Bitwise AND operator.
    * @param other The int_t to AND.
    * @return A new SafeInt_t with the result of the AND.
    */
    inline SafeInt_t<Size> operator&(const int_t& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ & other);
    }

    /**
    * Bitwise OR operator.
    * @param other The SafeInt_t to OR.
    * @return A new SafeInt_t with the result of the OR.
    */
    inline SafeInt_t<Size> operator|(const SafeInt_t<Size>& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ | other.get());
    }

    /**
    * Bitwise OR operator.
    * @param other The int_t to OR.
    * @return A new SafeInt_t with the result of the OR.
    */
    inline SafeInt_t<Size> operator|(const int_t& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ | other);
    }

    /**
    * Bitwise XOR operator.
    * @param other The SafeInt_t to XOR.
    * @return A new SafeInt_t with the result of the XOR.
    */
    inline SafeInt_t<Size> operator^(const SafeInt_t<Size>& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ ^ other.get());
    }

    /**
    * Bitwise XOR operator.
    * @param other The int_t to XOR.
    * @return A new SafeInt_t with the result of the XOR.
    */
    inline SafeInt_t<Size> operator^(const int_t& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ ^ other);
    }

    /**
    * Left shift operator.
    * @param other The SafeInt_t indicating the number of positions to shift.
    * @return A new SafeInt_t with the result of the shift.
    */
    inline SafeInt_t<Size> operator<<(const SafeInt_t<Size>& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ << other.get());
    }

    /**
    * Left shift operator.
    * @param other The int_t indicating the number of positions to shift.
    * @return A new SafeInt_t with the result of the shift.
    */
    inline SafeInt_t<Size> operator<<(const int_t& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ << other);
    }

    /**
    * Right shift operator.
    * @param other The SafeInt_t indicating the number of positions to shift.
    * @return A new SafeInt_t with the result of the shift.
    */
    inline SafeInt_t<Size> operator>>(const SafeInt_t<Size>& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ >> other.get());
    }

    /**
    * Right shift operator.
    * @param other The int_t indicating the number of positions to shift.
    * @return A new SafeInt_t with the result of the shift.
    */
    inline SafeInt_t<Size> operator>>(const int_t& other) const {
        check();
        return SafeInt_t<Size>(*valuePtr_ >> other);
    }

    /**
    * Logical NOT operator.
    * @return True if the value is zero, false otherwise.
    */
    inline bool operator!() const {
        check();
        return (!*valuePtr_);
    }

    /**
    * Logical AND operator.
    * @param other The SafeInt_t to AND.
    * @return True if both values are non-zero, false otherwise.
    */
    inline bool operator&&(const SafeInt_t<Size>& other) const {
        check();
        return (*valuePtr_ && other.get());
    }

    /**
    * Logical AND operator.
    * @param other The int_t to AND.
    * @return True if both values are non-zero, false otherwise.
    */
    inline bool operator&&(const int_t& other) const {
        check();
        return (*valuePtr_ && other);
    }

    /**
    * Logical OR operator.
    * @param other The SafeInt_t to OR.
    * @return True if either value is non-zero, false otherwise.
    */
    inline bool operator||(const SafeInt_t<Size>& other) const {
        check();
        return (*valuePtr_ || other.get());
    }

    /**
    * Logical OR operator.
    * @param other The int_t to OR.
    * @return True if either value is non-zero, false otherwise.
    */
    inline bool operator||(const int_t& other) const {
        check();
        return (*valuePtr_ || other);
    }

    /**
    * Equality operator.
    * @param other The SafeInt_t to compare to.
    * @return True if the values are equal, false otherwise.
    */
    inline bool operator==(const SafeInt_t<Size>& other) const {
        check();
        return (*valuePtr_ == other.get());
    }

    /**
    * Equality operator.
    * @param other The int_t to compare to.
    * @return True if the values are equal, false otherwise.
    */
    inline bool operator==(const int_t& other) const {
        check();
        return (*valuePtr_ == other);
    }

    /**
    * Inequality operator.
    * @param other The SafeInt_t to compare to.
    * @return True if the values are not equal, false otherwise.
    */
    inline bool operator!=(const SafeInt_t<Size>& other) const {
        check();
        return (*valuePtr_ != other.get());
    }

    /**
    * Inequality operator.
    * @param other The int_t to compare to.
    * @return True if the values are not equal, false otherwise.
    */
    inline bool operator!=(const int_t& other) const {
        check();
        return (*valuePtr_ != other);
    }

    /**
    * Less than operator.
    * @param other The SafeInt_t to compare to.
    * @return True if the value is less than the other value, false otherwise.
    */
    inline bool operator<(const SafeInt_t<Size>& other) const {
        check();
        return (*valuePtr_ < other.get());
    }

    /**
    * Less than operator.
    * @param other The int_t to compare to.
    * @return True if the value is less than the other value, false otherwise.
    */
    inline bool operator<(const int_t& other) const {
        check();
        return (*valuePtr_ < other);
    }

    /**
    * Less than or equal to operator.
    * @param other The SafeInt_t to compare to.
    * @return True if the value is less than or equal to the other value, false otherwise.
    */
    inline bool operator<=(const SafeInt_t<Size>& other) const {
        check();
        return (*valuePtr_ <= other.get());
    }

    /**
    * Less than or equal to operator.
    * @param other The int_t to compare to.
    * @return True if the value is less than or equal to the other value, false otherwise.
    */
    inline bool operator<=(const int_t& other) const {
        check();
        return (*valuePtr_ <= other);
    }

    /**
    * Greater than operator.
    * @param other The SafeInt_t to compare to.
    * @return True if the value is greater than the other value, false otherwise.
    */
    inline bool operator>(const SafeInt_t<Size>& other) const {
        check();
        return (*valuePtr_ > other.get());
    }

    /**
    * Greater than operator.
    * @param other The int_t to compare to.
    * @return True if the value is greater than the other value, false otherwise.
    */
    inline bool operator>(const int_t& other) const {
        check();
        return (*valuePtr_ > other);
    }

    /**
    * Greater than or equal to operator.
    * @param other The SafeInt_t to compare to.
    * @return True if the value is greater than or equal to the other value, false otherwise.
    */
    inline bool operator>=(const SafeInt_t<Size>& other) const {
        check();
        return (*valuePtr_ >= other.get());
    }

    /**
    * Greater than or equal to operator.
    * @param other The int_t to compare to.
    * @return True if the value is greater than or equal to the other value, false otherwise.
    */
    inline bool operator>=(const int_t& other) const {
        check();
        return (*valuePtr_ >= other);
    }

    /**
    * Assignment operator.
    * @param other The SafeInt_t to assign.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator=(const SafeInt_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ = other.get();
        return *this;
    }

    /**
    * Assignment operator.
    * @param other The int_t to assign.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator=(const int_t& other) {
        check();
        markAsUsed();
        *valuePtr_ = other;
        return *this;
    }

    /**
    * Addition assignment operator.
    * @param other The SafeInt_t to add.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator+=(const SafeInt_t<Size>& other) {
        check();
        if ((other.get() > 0) && (*valuePtr_ > std::numeric_limits<int_t>::max() - other.get())) {
            throw std::overflow_error("Overflow in addition assignment operation.");
        }
        if ((other.get() < 0) && (*valuePtr_ < std::numeric_limits<int_t>::min() - other.get())) {
            throw std::underflow_error("Underflow in addition assignment operation.");
        }
        markAsUsed();
        *valuePtr_ += other.get();
        return *this;
    }

    /**
    * Addition assignment operator.
    * @param other The int_t to add.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator+=(const int_t& other) {
        check();
        if ((other > 0) && (*valuePtr_ > std::numeric_limits<int_t>::max() - other)) {
            throw std::overflow_error("Overflow in addition assignment operation.");
        }
        if ((other < 0) && (*valuePtr_ < std::numeric_limits<int_t>::min() - other)) {
            throw std::underflow_error("Underflow in addition assignment operation.");
        }
        markAsUsed();
        *valuePtr_ += other;
        return *this;
    }

    /**
    * Subtraction assignment operator.
    * @param other The SafeInt_t to subtract.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator-=(const SafeInt_t<Size>& other) {
        check();
        if ((other.get() < 0) && (*valuePtr_ > std::numeric_limits<int_t>::max() + other.get())) {
            throw std::overflow_error("Overflow in subtraction assignment operation.");
        }
        if ((other.get() > 0) && (*valuePtr_ < std::numeric_limits<int_t>::min() + other.get())) {
            throw std::underflow_error("Underflow in subtraction assignment operation.");
        }
        markAsUsed();
        *valuePtr_ -= other.get();
        return *this;
    }

    /**
    * Subtraction assignment operator.
    * @param other The int_t to subtract.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator-=(const int_t& other) {
        check();
        if ((other < 0) && (*valuePtr_ > std::numeric_limits<int_t>::max() + other)) {
            throw std::overflow_error("Overflow in subtraction assignment operation.");
        }
        if ((other > 0) && (*valuePtr_ < std::numeric_limits<int_t>::min() + other)) {
            throw std::underflow_error("Underflow in subtraction assignment operation.");
        }
        markAsUsed();
        *valuePtr_ -= other;
        return *this;
    }

    /**
    * Multiplication assignment operator.
    * @param other The SafeInt_t to multiply.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator*=(const SafeInt_t<Size>& other) {
        check();
        if (*valuePtr_ > std::numeric_limits<int_t>::max() / other.get()) {
            throw std::overflow_error("Overflow in multiplication assignment operation.");
        }
        if (*valuePtr_ < std::numeric_limits<int_t>::min() / other.get()) {
            throw std::underflow_error("Underflow in multiplication assignment operation.");
        }
        markAsUsed();
        *valuePtr_ *= other.get();
        return *this;
    }

    /**
    * Multiplication assignment operator.
    * @param other The int_t to multiply.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator*=(const int_t& other) {
        check();
        if (*valuePtr_ > std::numeric_limits<int_t>::max() / other) {
            throw std::overflow_error("Overflow in multiplication assignment operation.");
        }
        if (*valuePtr_ < std::numeric_limits<int_t>::min() / other) {
            throw std::underflow_error("Underflow in multiplication assignment operation.");
        }
        markAsUsed();
        *valuePtr_ *= other;
        return *this;
    }

    /**
    * Division assignment operator.
    * @param other The SafeInt_t to divide.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator/=(const SafeInt_t<Size>& other) {
        check();
        if (other.get() == 0) throw std::domain_error("Division assignment by zero.");

        // Handling the edge case where dividing the smallest negative number by -1 causes overflow
        if (*valuePtr_ == std::numeric_limits<int_t>::min() && other.get() == -1) {
            throw std::overflow_error("Overflow in division assignment operation.");
        }

        markAsUsed();
        *valuePtr_ /= other.get();
        return *this;
    }

    /**
    * Division assignment operator.
    * @param other The int_t to divide.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator/=(const int_t& other) {
        check();
        if (other == 0) throw std::domain_error("Division assignment by zero.");

        // Handling the edge case where dividing the smallest negative number by -1 causes overflow
        if (*valuePtr_ == std::numeric_limits<int_t>::min() && other == -1) {
            throw std::overflow_error("Overflow in division assignment operation.");
        }

        markAsUsed();
        *valuePtr_ /= other;
        return *this;
    }

    /**
    * Modulus assignment operator.
    * @param other The SafeInt_t to take the modulus by.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator%=(const SafeInt_t<Size>& other) {
        check();
        if (other.get() == 0) throw std::domain_error("Modulus assignment by zero.");
        markAsUsed();
        *valuePtr_ %= other.get();
        return *this;
    }

    /**
    * Modulus assignment operator.
    * @param other The int_t to take the modulus by.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator%=(const int_t& other) {
        check();
        if (other == 0) throw std::domain_error("Modulus assignment by zero.");
        markAsUsed();
        *valuePtr_ %= other;
        return *this;
    }

    /**
    * Bitwise AND assignment operator.
    * @param other The SafeInt_t to AND.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator&=(const SafeInt_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ &= other.get();
        return *this;
    }

    /**
    * Bitwise AND assignment operator.
    * @param other The int_t to AND.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator&=(const int_t& other) {
        check();
        markAsUsed();
        *valuePtr_ &= other;
        return *this;
    }

    /**
    * Bitwise OR assignment operator.
    * @param other The SafeInt_t to OR.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator|=(const SafeInt_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ |= other.get();
        return *this;
    }

    /**
    * Bitwise OR assignment operator.
    * @param other The int_t to OR.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator|=(const int_t& other) {
        check();
        markAsUsed();
        *valuePtr_ |= other;
        return *this;
    }

    /**
    * Bitwise XOR assignment operator.
    * @param other The SafeInt_t to XOR.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator^=(const SafeInt_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ ^= other.get();
        return *this;
    }

    /**
    * Bitwise XOR assignment operator.
    * @param other The int_t to XOR.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator^=(const int_t& other) {
        check();
        markAsUsed();
        *valuePtr_ ^= other;
        return *this;
    }

    /**
    * Left shift assignment operator.
    * @param other The SafeInt_t indicating the number of positions to shift.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator<<=(const SafeInt_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ <<= other.get();
        return *this;
    }

    /**
    * Left shift assignment operator.
    * @param other The int_t indicating the number of positions to shift.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator<<=(const int_t& other) {
        check();
        markAsUsed();
        *valuePtr_ <<= other;
        return *this;
    }

    /**
    * Right shift assignment operator.
    * @param other The SafeInt_t indicating the number of positions to shift.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator>>=(const SafeInt_t<Size>& other) {
        check();
        markAsUsed();
        *valuePtr_ >>= other.get();
        return *this;
    }

    /**
    * Right shift assignment operator.
    * @param other The int_t indicating the number of positions to shift.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator>>=(const int_t& other) {
        check();
        markAsUsed();
        *valuePtr_ >>= other;
        return *this;
    }

    /**
    * Prefix increment operator.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator++() {
        check();
        if (*valuePtr_ == std::numeric_limits<int_t>::max()) {
            throw std::overflow_error("Overflow in prefix increment operation.");
        }
        markAsUsed();
        ++(*valuePtr_);
        return *this;
    }

    /**
    * Postfix increment operator.
    * @return A new SafeInt_t with the value of this SafeInt_t before the increment.
    */
    inline SafeInt_t<Size> operator++(int) {
        check();
        if (*valuePtr_ == std::numeric_limits<int_t>::max()) {
            throw std::overflow_error("Overflow in postfix increment operation.");
        }
        markAsUsed();
        SafeInt_t<Size> temp(*valuePtr_);
        ++(*valuePtr_);
        return temp;
    }

    /**
    * Prefix decrement operator.
    * @return A reference to this SafeInt_t.
    */
    inline SafeInt_t<Size>& operator--() {
        check();
        if (*valuePtr_ == std::numeric_limits<int_t>::min()) {
            throw std::underflow_error("Underflow in prefix decrement operation.");
        }
        markAsUsed();
        --(*valuePtr_);
        return *this;
    }

    /**
    * Postfix decrement operator.
    * @return A new SafeInt_t with the value of this SafeInt_t before the decrement.
    */
    inline SafeInt_t<Size> operator--(int) {
        check();
        if (*valuePtr_ == std::numeric_limits<int_t>::min()) {
            throw std::underflow_error("Underflow in postfix decrement operation.");
        }
        markAsUsed();
        SafeInt_t<Size> temp(*valuePtr_);
        --(*valuePtr_);
        return temp;
    }

}; // class SafeInt_t

#endif // SAFEINT_T_H
