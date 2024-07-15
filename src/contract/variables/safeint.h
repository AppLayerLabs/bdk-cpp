/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SAFEINT_T_H
#define SAFEINT_T_H

#include <boost/multiprecision/cpp_int.hpp>
#include "safebase.h"

/**
 * Template for the type of an int with the given size.
 * @tparam Size The size of the int.
 */
template <int Size> struct IntType {
  /// The type of the int.
  using type = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
    Size, Size, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void
  >>;
};

/// Template specialization for int8_t.
template <> struct IntType<8> {
  using type = int8_t; ///< int8_t type.
};

/// Template specialization for int16_t.
template <> struct IntType<16> {
  using type = int16_t;  ///< int16_t type.
};

/// Template specialization for int32_t.
template <> struct IntType<32> {
  using type = int32_t; ///< int32_t type.
};

/// Template specialization for int64_t.
template <> struct IntType<64> {
  using type = int64_t; ///< int64_t type.
};

/**
 * Safe wrapper for an int_t variable.
 * @tparam Size The size of the int.
 */
template <int Size> class SafeInt_t : public SafeBase {
  private:
    using int_t = typename IntType<Size>::type; ///< The type of the int.
    int_t value_; ///< Current ("original") value.
    int_t copy_; ///< Previous ("temporary") value.

  public:
    static_assert(Size >= 8 && Size <= 256 && Size % 8 == 0, "Size must be between 8 and 256 and a multiple of 8.");

    /**
     * Constructor.
     * @param value The initial value of the variable. Defaults to 0.
     */
    explicit SafeInt_t(const int_t& value = 0) : SafeBase(nullptr), value_(value), copy_(value) {}

    /**
     * Constructor with owner.
     * @param owner The DynamicContract that owns this variable.
     * @param value The initial value of the variable. Defaults to 0.
     */
    explicit SafeInt_t(DynamicContract* owner, const int_t& value = 0) : SafeBase(owner), value_(value), copy_(value) {}

    /// Copy constructor. Only copies the CURRENT value.
    SafeInt_t(const SafeInt_t<Size>& other) : SafeBase(nullptr), value_(other.value_), copy_(other.value_) {}

    /// Getter for the value.
    inline const int_t& get() const { return this->value_; }

    ///@{
    /**
     * Addition operator.
     * @param other The integer to add.
     * @throw std::overflow_error if an overflow happens.
     * @throw std::underflow_error if an underflow happens.
     * @return A new SafeInt_t with the result of the addition.
     */
    inline SafeInt_t<Size> operator+(const int_t& other) const {
      if ((other > 0) && (this->value_ > std::numeric_limits<int_t>::max() - other)) {
        throw std::overflow_error("Overflow in addition operation.");
      }
      if ((other < 0) && (this->value_ < std::numeric_limits<int_t>::min() - other)) {
        throw std::underflow_error("Underflow in addition operation.");
      }
      return SafeInt_t<Size>(this->value_ + other);
    }
    inline SafeInt_t<Size> operator+(const SafeInt_t<Size>& other) const {
      if ((other.get() > 0) && (this->value_ > std::numeric_limits<int_t>::max() - other.get())) {
        throw std::overflow_error("Overflow in addition operation.");
      }
      if ((other.get() < 0) && (this->value_ < std::numeric_limits<int_t>::min() - other.get())) {
        throw std::underflow_error("Underflow in addition operation.");
      }
      return SafeInt_t<Size>(this->value_ + other.get());
    }
    ///@}

    ///@{
    /**
     * Subtraction operator.
     * @param other The integer to subtract.
     * @throw std::overflow_error if an overflow happens.
     * @throw std::underflow_error if an underflow happens.
     * @return A new SafeInt_t with the result of the subtraction.
     */
    inline SafeInt_t<Size> operator-(const int_t& other) const {
      if ((other < 0) && (this->value_ > std::numeric_limits<int_t>::max() + other)) {
        throw std::overflow_error("Overflow in subtraction operation.");
      }
      if ((other > 0) && (this->value_ < std::numeric_limits<int_t>::min() + other)) {
        throw std::underflow_error("Underflow in subtraction operation.");
      }
      return SafeInt_t<Size>(this->value_ - other);
    }
    inline SafeInt_t<Size> operator-(const SafeInt_t<Size>& other) const {
      if ((other.get() < 0) && (this->value_ > std::numeric_limits<int_t>::max() + other.get())) {
        throw std::overflow_error("Overflow in subtraction operation.");
      }
      if ((other.get() > 0) && (this->value_ < std::numeric_limits<int_t>::min() + other.get())) {
        throw std::underflow_error("Underflow in subtraction operation.");
      }
      return SafeInt_t<Size>(this->value_ - other.get());
    }
    ///@}

    ///@{
    /**
     * Multiplication operator.
     * @param other The integer to multiply.
     * @throw std::overflow_error if an overflow happens.
     * @throw std::underflow_error if an underflow happens.
     * @throw std::domain_error if multiplying by 0.
     * @return A new SafeInt_t with the result of the multiplication.
     */
    inline SafeInt_t<Size> operator*(const int_t& other) const {
      if (this->value_ == 0 || other == 0) {
        throw std::domain_error("Multiplication by zero.");
      }
      if (this->value_ > std::numeric_limits<int_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication operation.");
      }
      if (this->value_ < std::numeric_limits<int_t>::min() / other) {
        throw std::underflow_error("Underflow in multiplication operation.");
      }
      return SafeInt_t<Size>(this->value_ * other);
    }
    inline SafeInt_t<Size> operator*(const SafeInt_t<Size>& other) const {
      if (this->value_ == 0 || other.get() == 0) {
        throw std::domain_error("Multiplication by zero.");
      }
      if (this->value_ > std::numeric_limits<int_t>::max() / other.get()) {
        throw std::overflow_error("Overflow in multiplication operation.");
      }
      if (this->value_ < std::numeric_limits<int_t>::min() / other.get()) {
        throw std::underflow_error("Underflow in multiplication operation.");
      }
      return SafeInt_t<Size>(this->value_ * other.get());
    }
    ///@}

    ///@{
    /**
     * Division operator.
     * @param other The integer to divide.
     * @throw std::domain_error if the other value is zero.
     * @throw std::overflow_error if division results in overflow.
     * @return A new SafeInt_t with the result of the division.
     */
    inline SafeInt_t<Size> operator/(const int_t& other) const {
      if (other == 0) throw std::domain_error("Division by zero");
      // Edge case - dividing the smallest negative number by -1 causes overflow
      if (this->value_ == std::numeric_limits<int_t>::min() && other == -1) {
        throw std::overflow_error("Overflow in division operation.");
      }
      return SafeInt_t<Size>(this->value_ / other);
    }
    inline SafeInt_t<Size> operator/(const SafeInt_t<Size>& other) const {
      if (other.get() == 0) throw std::domain_error("Division by zero");
      // Edge case - dividing the smallest negative number by -1 causes overflow
      if (this->value_ == std::numeric_limits<int_t>::min() && other.get() == -1) {
        throw std::overflow_error("Overflow in division operation.");
      }
      return SafeInt_t<Size>(this->value_ / other.get());
    }
    ///@}

    ///@{
    /**
     * Modulus operator.
     * @param other The integer to take the modulus of.
     * @throw std::domain_error if the other value is zero.
     * @return A new SafeInt_t with the result of the modulus.
     */
    inline SafeInt_t<Size> operator%(const int_t& other) const {
      if (other == 0) throw std::domain_error("Modulus by zero");
      return SafeInt_t<Size>(this->value_ % other);
    }
    inline SafeInt_t<Size> operator%(const SafeInt_t<Size>& other) const {
      if (other.get() == 0) throw std::domain_error("Modulus by zero");
      return SafeInt_t<Size>(this->value_ % other.get());
    }
    ///@}

    ///@{
    /**
     * Bitwise AND operator.
     * @param other The integer to apply AND.
     * @return A new SafeInt_t with the result of the AND.
     */
    inline SafeInt_t<Size> operator&(const int_t& other) const {
      return SafeInt_t<Size>(this->value_ & other);
    }
    inline SafeInt_t<Size> operator&(const SafeInt_t<Size>& other) const {
      return SafeInt_t<Size>(this->value_ & other.get());
    }
    ///@}

    ///@{
    /**
     * Bitwise OR operator.
     * @param other The integer to apply OR.
     * @return A new SafeInt_t with the result of the OR.
     */
    inline SafeInt_t<Size> operator|(const int_t& other) const {
      return SafeInt_t<Size>(this->value_ | other);
    }
    inline SafeInt_t<Size> operator|(const SafeInt_t<Size>& other) const {
      return SafeInt_t<Size>(this->value_ | other.get());
    }
    ///@}

    ///@{
    /**
     * Bitwise XOR operator.
     * @param other The integer to apply XOR.
     * @return A new SafeInt_t with the result of the XOR.
     */
    inline SafeInt_t<Size> operator^(const int_t& other) const {
      return SafeInt_t<Size>(this->value_ ^ other);
    }
    inline SafeInt_t<Size> operator^(const SafeInt_t<Size>& other) const {
      return SafeInt_t<Size>(this->value_ ^ other.get());
    }
    ///@}

    // NOTE: Boost types (anything that's not 8, 16, 32 or 64) do not support
    // bit shifting with their own types (e.g. `i >> int256_t(2)`).
    // Because of that, uint8_t is forcibly used instead for all types, given
    // anything bigger than `i >> 31` yields a compiler warning (= "undefined behaviour").

    /**
     * Left shift operator.
     * @param other The integer indicating the number of positions to shift.
     * @return A new SafeInt_t with the result of the shift.
     */
    inline SafeInt_t<Size> operator<<(const uint8_t& other) const {
      return SafeInt_t<Size>(this->value_ << other);
    }

    /**
     * Right shift operator.
     * @param other The integer indicating the number of positions to shift.
     * @return A new SafeInt_t with the result of the shift.
     */
    inline SafeInt_t<Size> operator>>(const uint8_t& other) const {
      return SafeInt_t<Size>(this->value_ >> other);
    }

    /**
     * Boolean operator
     * @return `true` if the value is non-zero, `false` otherwise.
     */
    inline explicit operator bool() const { return bool(this->value_); }

    ///@{
    /**
     * Equality operator.
     * @param other The integer to compare.
     * @return `true` if the values are equal, `false` otherwise.
     */
    inline bool operator==(const int_t& other) const { return (this->value_ == other); }
    inline bool operator==(const SafeInt_t<Size>& other) const { return (this->value_ == other.get()); }
    ///@}

    ///@{
    /**
     * Inequality operator.
     * @param other The integer to compare.
     * @return `true` if the values are not equal, `false` otherwise.
     */
    inline bool operator!=(const int_t& other) const { return (this->value_ != other); }
    inline bool operator!=(const SafeInt_t<Size>& other) const { return (this->value_ != other.get()); }
    ///@}

    ///@{
    /**
     * Less than operator.
     * @param other The integer to compare.
     * @return `true` if the value is less than the other value, `false` otherwise.
     */
    inline bool operator<(const int_t& other) const { return (this->value_ < other); }
    inline bool operator<(const SafeInt_t<Size>& other) const { return (this->value_ < other.get()); }
    ///@}

    ///@{
    /**
     * Less than or equal to operator.
     * @param other The integer to compare.
     * @return `true` if the value is less than or equal to the other value, `false` otherwise.
     */
    inline bool operator<=(const int_t& other) const { return (this->value_ <= other); }
    inline bool operator<=(const SafeInt_t<Size>& other) const { return (this->value_ <= other.get()); }
    ///@}

    ///@{
    /**
     * Greater than operator.
     * @param other The integer to compare.
     * @return `true` if the value is greater than the other value, `false` otherwise.
     */
    inline bool operator>(const int_t& other) const { return (this->value_ > other); }
    inline bool operator>(const SafeInt_t<Size>& other) const { return (this->value_ > other.get()); }
    ///@}

    ///@{
    /**
     * Greater than or equal to operator.
     * @param other The integer to compare.
     * @return `true` if the value is greater than or equal to the other value, `false` otherwise.
     */
    inline bool operator>=(const int_t& other) const { return (this->value_ >= other); }
    inline bool operator>=(const SafeInt_t<Size>& other) const { return (this->value_ >= other.get()); }
    ///@}

    ///@{
    /**
     * Assignment operator.
     * @param other The integer to assign.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator=(const int_t& other) {
      markAsUsed(); this->value_ = other; return *this;
    }
    inline SafeInt_t<Size>& operator=(const SafeInt_t<Size>& other) {
      markAsUsed(); this->value_ = other.get(); return *this;
    }
    ///@}

    ///@{
    /**
     * Addition assignment operator.
     * @param other The integer to add.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator+=(const int_t& other) {
      if ((other > 0) && (this->value_ > std::numeric_limits<int_t>::max() - other)) {
        throw std::overflow_error("Overflow in addition assignment operation.");
      }
      if ((other < 0) && (this->value_ < std::numeric_limits<int_t>::min() - other)) {
        throw std::underflow_error("Underflow in addition assignment operation.");
      }
      markAsUsed(); this->value_ += other; return *this;
    }
    inline SafeInt_t<Size>& operator+=(const SafeInt_t<Size>& other) {
      if ((other.get() > 0) && (this->value_ > std::numeric_limits<int_t>::max() - other.get())) {
        throw std::overflow_error("Overflow in addition assignment operation.");
      }
      if ((other.get() < 0) && (this->value_ < std::numeric_limits<int_t>::min() - other.get())) {
        throw std::underflow_error("Underflow in addition assignment operation.");
      }
      markAsUsed(); this->value_ += other.get(); return *this;
    }
    ///@}

    ///@{
    /**
     * Subtraction assignment operator.
     * @param other The integer to subtract.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator-=(const int_t& other) {
      if ((other < 0) && (this->value_ > std::numeric_limits<int_t>::max() + other)) {
        throw std::overflow_error("Overflow in subtraction assignment operation.");
      }
      if ((other > 0) && (this->value_ < std::numeric_limits<int_t>::min() + other)) {
        throw std::underflow_error("Underflow in subtraction assignment operation.");
      }
      markAsUsed(); this->value_ -= other; return *this;
    }
    inline SafeInt_t<Size>& operator-=(const SafeInt_t<Size>& other) {
      if ((other.get() < 0) && (this->value_ > std::numeric_limits<int_t>::max() + other.get())) {
        throw std::overflow_error("Overflow in subtraction assignment operation.");
      }
      if ((other.get() > 0) && (this->value_ < std::numeric_limits<int_t>::min() + other.get())) {
        throw std::underflow_error("Underflow in subtraction assignment operation.");
      }
      markAsUsed(); this->value_ -= other.get(); return *this;
    }
    ///@}

    ///@{
    /**
     * Multiplication assignment operator.
     * @param other The integer to multiply.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator*=(const int_t& other) {
      if (this->value_ == 0 || other == 0) {
        throw std::domain_error("Multiplication by zero.");
      }
      if (this->value_ > std::numeric_limits<int_t>::max() / other) {
        throw std::overflow_error("Overflow in multiplication assignment operation.");
      }
      if (this->value_ < std::numeric_limits<int_t>::min() / other) {
        throw std::underflow_error("Underflow in multiplication assignment operation.");
      }
      markAsUsed(); this->value_ *= other; return *this;
    }
    inline SafeInt_t<Size>& operator*=(const SafeInt_t<Size>& other) {
      if (this->value_ == 0 || other.get() == 0) {
        throw std::domain_error("Multiplication by zero.");
      }
      if (this->value_ > std::numeric_limits<int_t>::max() / other.get()) {
        throw std::overflow_error("Overflow in multiplication assignment operation.");
      }
      if (this->value_ < std::numeric_limits<int_t>::min() / other.get()) {
        throw std::underflow_error("Underflow in multiplication assignment operation.");
      }
      markAsUsed(); this->value_ *= other.get(); return *this;
    }
    ///@}

    ///@{
    /**
     * Division assignment operator.
     * @param other The integer to divide.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator/=(const int_t& other) {
      if (other == 0) throw std::domain_error("Division assignment by zero.");
      // Handling the edge case where dividing the smallest negative number by -1 causes overflow
      if (this->value_ == std::numeric_limits<int_t>::min() && other == -1) {
        throw std::overflow_error("Overflow in division assignment operation.");
      }
      markAsUsed(); this->value_ /= other; return *this;
    }
    inline SafeInt_t<Size>& operator/=(const SafeInt_t<Size>& other) {
      if (other.get() == 0) throw std::domain_error("Division assignment by zero.");
      // Handling the edge case where dividing the smallest negative number by -1 causes overflow
      if (this->value_ == std::numeric_limits<int_t>::min() && other.get() == -1) {
        throw std::overflow_error("Overflow in division assignment operation.");
      }
      markAsUsed(); this->value_ /= other.get(); return *this;
    }
    ///@}

    ///@{
    /**
     * Modulus assignment operator.
     * @param other The SafeInt_t to take the modulus by.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator%=(const int_t& other) {
      if (other == 0) throw std::domain_error("Modulus assignment by zero.");
      markAsUsed(); this->value_ %= other; return *this;
    }
    inline SafeInt_t<Size>& operator%=(const SafeInt_t<Size>& other) {
      if (other.get() == 0) throw std::domain_error("Modulus assignment by zero.");
      markAsUsed(); this->value_ %= other.get(); return *this;
    }
    ///@}

    ///@{
    /**
     * Bitwise AND assignment operator.
     * @param other The integer to apply AND.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator&=(const int_t& other) {
      markAsUsed(); this->value_ &= other; return *this;
    }
    inline SafeInt_t<Size>& operator&=(const SafeInt_t<Size>& other) {
      markAsUsed(); this->value_ &= other.get(); return *this;
    }
    ///@}

    ///@{
    /**
     * Bitwise OR assignment operator.
     * @param other The integer to apply OR.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator|=(const int_t& other) {
      markAsUsed(); this->value_ |= other; return *this;
    }
    inline SafeInt_t<Size>& operator|=(const SafeInt_t<Size>& other) {
      markAsUsed(); this->value_ |= other.get(); return *this;
    }
    ///@}

    ///@{
    /**
     * Bitwise XOR assignment operator.
     * @param other The integer to apply XOR.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator^=(const int_t& other) {
      markAsUsed(); this->value_ ^= other; return *this;
    }
    inline SafeInt_t<Size>& operator^=(const SafeInt_t<Size>& other) {
      markAsUsed(); this->value_ ^= other.get(); return *this;
    }
    ///@}

    /**
     * Left shift assignment operator.
     * @param other The integer indicating the number of positions to shift.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator<<=(const uint8_t& other) {
      markAsUsed(); this->value_ <<= other; return *this;
    }

    /**
     * Right shift assignment operator.
     * @param other The integer indicating the number of positions to shift.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator>>=(const uint8_t& other) {
      markAsUsed(); this->value_ >>= other; return *this;
    }

    /**
     * Prefix increment operator.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator++() {
      if (this->value_ == std::numeric_limits<int_t>::max()) {
        throw std::overflow_error("Overflow in prefix increment operation.");
      }
      markAsUsed(); ++(this->value_); return *this;
    }

    /**
     * Postfix increment operator.
     * @return A new SafeInt_t with the value of this SafeInt_t before the increment.
     */
    inline SafeInt_t<Size> operator++(int) {
      if (this->value_ == std::numeric_limits<int_t>::max()) {
        throw std::overflow_error("Overflow in postfix increment operation.");
      }
      markAsUsed(); SafeInt_t<Size> temp(this->value_); ++(this->value_); return temp;
    }

    /**
     * Prefix decrement operator.
     * @return A reference to this SafeInt_t.
     */
    inline SafeInt_t<Size>& operator--() {
      if (this->value_ == std::numeric_limits<int_t>::min()) {
        throw std::underflow_error("Underflow in prefix decrement operation.");
      }
      markAsUsed(); --(this->value_); return *this;
    }

    /**
     * Postfix decrement operator.
     * @return A new SafeInt_t with the value of this SafeInt_t before the decrement.
     */
    inline SafeInt_t<Size> operator--(int) {
      if (this->value_ == std::numeric_limits<int_t>::min()) {
        throw std::underflow_error("Underflow in postfix decrement operation.");
      }
      markAsUsed(); SafeInt_t<Size> temp(this->value_); --(this->value_); return temp;
    }

    /// Commit the value.
    inline void commit() override { this->copy_ = this->value_; this->registered_ = false; }

    /// Revert the value.
    inline void revert() override {
      this->value_ = this->copy_;
      this->registered_ = false;
    }
};

#endif // SAFEINT_T_H
