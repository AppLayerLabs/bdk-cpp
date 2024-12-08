#include "safeuint.h"

// Explicit instantiations

template class SafeUint_t<8>;
template class SafeUint_t<16>;
template class SafeUint_t<32>;
template class SafeUint_t<64>;

template class SafeUint_t<24>;
template class SafeUint_t<40>;
template class SafeUint_t<48>;
template class SafeUint_t<56>;
template class SafeUint_t<72>;
template class SafeUint_t<80>;
template class SafeUint_t<88>;
template class SafeUint_t<96>;
template class SafeUint_t<104>;
template class SafeUint_t<112>;
template class SafeUint_t<120>;
template class SafeUint_t<128>;
template class SafeUint_t<136>;
template class SafeUint_t<144>;
template class SafeUint_t<152>;
template class SafeUint_t<160>;
template class SafeUint_t<168>;
template class SafeUint_t<176>;
template class SafeUint_t<184>;
template class SafeUint_t<192>;
template class SafeUint_t<200>;
template class SafeUint_t<208>;
template class SafeUint_t<216>;
template class SafeUint_t<224>;
template class SafeUint_t<232>;
template class SafeUint_t<240>;
template class SafeUint_t<248>;
template class SafeUint_t<256>;

/// Biggest int type for dealing with uint/int operations
using int256_t = boost::multiprecision::number<
  boost::multiprecision::cpp_int_backend<
    256, 256, boost::multiprecision::signed_magnitude,
    boost::multiprecision::cpp_int_check_type::checked, void
  >
>;

// Class impl starts here

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator+(const SafeUint_t<Size>& other) const {
  if (this->value_ > std::numeric_limits<uint_t>::max() - other.value_) {
    throw std::overflow_error("Overflow in addition operation.");
  }
  return SafeUint_t<Size>(this->value_ + other.value_);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator+(const uint_t& other) const {
  if (this->value_ > std::numeric_limits<uint_t>::max() - other) {
    throw std::overflow_error("Overflow in addition operation.");
  }
  return SafeUint_t<Size>(this->value_ + other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator+(const int& other) const {
  // uint/int operations require a conversion to a bigger int type, which will
  // be operated on instead of the original value, then the remainder is checked
  // to see if it fits back into the original. This is expected behaviour.
  // We use int256_t as it is the biggest int type used within the project.
  // Another option is to disable `checked` in the Boost types, but then we lose the
  // intrinsic over/underflow checks and rely on Boost itself to do the conversion correctly.
  // See https://www.boost.org/doc/libs/1_77_0/libs/multiprecision/doc/html/boost_multiprecision/tut/ints/cpp_int.html
  auto tmp = static_cast<int256_t>(this->value_);
  tmp = tmp + other;
  if (tmp < 0) {
    throw std::underflow_error("Underflow in addition operation.");
  } else if (tmp > static_cast<int256_t>(std::numeric_limits<uint_t>::max())) {
    throw std::overflow_error("Overflow in addition operation.");
  }
  return SafeUint_t<Size>(static_cast<uint_t>(tmp));
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator-(const SafeUint_t<Size>& other) const {
  if (this->value_ < other.value_) throw std::underflow_error("Underflow in subtraction operation.");
  return SafeUint_t<Size>(this->value_ - other.value_);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator-(const uint_t& other) const {
  if (this->value_ < other) throw std::underflow_error("Underflow in subtraction operation.");
  return SafeUint_t<Size>(this->value_ - other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator-(const int& other) const {
  // See operator+.
  auto tmp = static_cast<int256_t>(this->value_);
  tmp = tmp - other;
  if (tmp < 0) {
    throw std::underflow_error("Underflow in addition operation.");
  } else if (tmp > static_cast<int256_t>(std::numeric_limits<uint_t>::max())) {
    throw std::overflow_error("Overflow in addition operation.");
  }
  return SafeUint_t<Size>(static_cast<uint_t>(tmp));
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator*(const SafeUint_t<Size>& other) const {
  if (other.value_ == 0 || this->value_ == 0) throw std::domain_error("Multiplication by zero");
  if (this->value_ > std::numeric_limits<uint_t>::max() / other.value_) {
    throw std::overflow_error("Overflow in multiplication operation.");
  }
  return SafeUint_t<Size>(this->value_ * other.value_);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator*(const uint_t& other) const {
  if (other == 0 || this->value_ == 0) throw std::domain_error("Multiplication by zero");
  if (this->value_ > std::numeric_limits<uint_t>::max() / other) {
    throw std::overflow_error("Overflow in multiplication operation.");
  }
  return SafeUint_t<Size>(this->value_ * other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator*(const int& other) const {
  if (other == 0 || this->value_ == 0) throw std::domain_error("Multiplication by zero");
  if (other < 0) {
    throw std::underflow_error("Underflow in multiplication operation.");
  } else {
    if (this->value_ > std::numeric_limits<uint_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication operation.");
    }
  }
  return SafeUint_t<Size>(this->value_ * other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator/(const SafeUint_t<Size>& other) const {
  if (this->value_ == 0 || other.value_ == 0) throw std::domain_error("Division by zero");
  return SafeUint_t<Size>(this->value_ / other.value_);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator/(const uint_t& other) const {
  if (this->value_ == 0 || other == 0) throw std::domain_error("Division by zero");
  return SafeUint_t<Size>(this->value_ / other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator/(const int& other) const {
  if (other == 0) throw std::domain_error("Division by zero");
  // Division by a negative number results in a negative result, which cannot be represented in an unsigned integer.
  if (other < 0) throw std::domain_error("Division by a negative number");
  return SafeUint_t<Size>(this->value_ / other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator%(const SafeUint_t<Size>& other) const {
  if (this->value_ == 0 || other.value_ == 0) throw std::domain_error("Modulus by zero");
  return SafeUint_t<Size>(this->value_ % other.value_);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator%(const uint_t& other) const {
  if (this->value_ == 0 || other == 0) throw std::domain_error("Modulus by zero");
  return SafeUint_t<Size>(this->value_ % other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator%(const int& other) const {
  if (this->value_ == 0 || other == 0) throw std::domain_error("Modulus by zero");
  return SafeUint_t<Size>(this->value_ % static_cast<uint_t>(other));
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator&(const SafeUint_t<Size>& other) const {
  return SafeUint_t<Size>(this->value_ & other.value_);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator&(const uint_t& other) const {
  return SafeUint_t<Size>(this->value_ & other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator&(const int& other) const {
  if (other < 0) throw std::domain_error("Bitwise AND with a negative number");
  return SafeUint_t<Size>(this->value_ & static_cast<uint_t>(other));
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator|(const SafeUint_t<Size>& other) const {
  return SafeUint_t<Size>(this->value_ | other.value_);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator|(const uint_t& other) const {
  return SafeUint_t<Size>(this->value_ | other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator|(const int& other) const {
  if (other < 0) throw std::domain_error("Bitwise OR with a negative number");
  return SafeUint_t<Size>(this->value_ | static_cast<uint_t>(other));
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator^(const SafeUint_t<Size>& other) const {
  return SafeUint_t<Size>(this->value_ ^ other.value_);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator^(const uint_t& other) const {
  return SafeUint_t<Size>(this->value_ ^ other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator^(const int& other) const {
  if (other < 0) throw std::domain_error("Bitwise XOR with a negative number");
  return SafeUint_t<Size>(this->value_ ^ static_cast<uint_t>(other));
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator<<(const uint8_t& other) const {
  return SafeUint_t<Size>(this->value_ << other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator>>(const uint8_t& other) const {
  return SafeUint_t<Size>(this->value_ >> other);
}

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator!() const { return !(this->value_); }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator&&(const SafeUint_t<Size>& other) const { return this->value_ && other.value_; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator&&(const uint_t& other) const { return this->value_ && other; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator||(const SafeUint_t<Size>& other) const { return this->value_ || other.value_; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator||(const uint_t& other) const { return this->value_ || other; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator==(const SafeUint_t<Size>& other) const { return this->value_ == other.value_; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator==(const uint_t& other) const { return this->value_ == other; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator==(const int& other) const {
  if (other < 0) return false;  // Unsigned value will never equal a negative
  return this->value_ == static_cast<uint_t>(other);
}

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator!=(const SafeUint_t<Size>& other) const { return this->value_ != other.value_; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator!=(const uint_t& other) const { return this->value_ != other; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator!=(const int& other) const {
  if (other < 0) return true;  // Unsigned value will always differ from a negative
  return this->value_ != static_cast<uint_t>(other);
}

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator<(const SafeUint_t<Size>& other) const { return this->value_ < other.value_; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator<(const uint_t& other) const { return this->value_ < other; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator<(const int& other) const {
  if (other < 0) return false;  // Unsigned value will never be less than a negative
  return this->value_ < static_cast<uint_t>(other);
}

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator<=(const SafeUint_t<Size>& other) const { return this->value_ <= other.value_; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator<=(const uint_t& other) const { return this->value_ <= other; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator<=(const int& other) const {
  if (other < 0) return false;  // Unsigned value will never be less nor equal than a negative
  return this->value_ <= static_cast<uint_t>(other);
}

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator>(const SafeUint_t<Size>& other) const { return this->value_ > other.value_; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator>(const uint_t& other) const { return this->value_ > other; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator>(const int& other) const {
  if (other < 0) return true;  // Unsigned value will always be more than a negative
  return this->value_ > static_cast<uint_t>(other);
}

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator>=(const SafeUint_t<Size>& other) const { return this->value_ >= other.value_; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator>=(const uint_t& other) const { return this->value_ >= other; }

template <int Size> requires UintInterval<Size>
bool SafeUint_t<Size>::operator>=(const int& other) const {
  if (other < 0) return true;  // Unsigned value will be never equal, but always more than a negative
  return this->value_ >= static_cast<uint_t>(other);
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator=(const SafeUint_t<Size>& other) {
  markAsUsed(); this->value_ = other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator=(const uint_t& other) {
  markAsUsed(); this->value_ = other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator=(const int& other) {
  if (other < 0) throw std::domain_error("Cannot assign negative value to SafeUint_t");
  markAsUsed(); this->value_ = static_cast<uint_t>(other); return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator+=(const SafeUint_t<Size>& other) {
  if (this->value_ > std::numeric_limits<uint_t>::max() - other.value_) {
    throw std::overflow_error("Overflow in addition assignment operation.");
  }
  markAsUsed(); this->value_ += other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator+=(const uint_t& other) {
  if (this->value_ > std::numeric_limits<uint_t>::max() - other) {
    throw std::overflow_error("Overflow in addition assignment operation.");
  }
  markAsUsed(); this->value_ += other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator+=(const int& other) {
  // See operator+.
  auto tmp = static_cast<int256_t>(this->value_);
  tmp += other;
  if (tmp < 0) {
    throw std::underflow_error("Underflow in addition assignment operation.");
  } else if (tmp > static_cast<int256_t>(std::numeric_limits<uint_t>::max())) {
    throw std::overflow_error("Overflow in addition assignment operation.");
  }
  markAsUsed(); this->value_ = static_cast<uint_t>(tmp); return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator-=(const SafeUint_t<Size>& other) {
  if (this->value_ < other.value_) throw std::underflow_error("Underflow in subtraction assignment operation.");
  markAsUsed(); this->value_ -= other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator-=(const uint_t& other) {
  if (this->value_ < other) throw std::underflow_error("Underflow in subtraction assignment operation.");
  markAsUsed(); this->value_ -= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator-=(const int& other) {
  // See operator+.
  auto tmp = static_cast<int256_t>(this->value_);
  tmp -= other;
  if (tmp < 0) {
    throw std::underflow_error("Underflow in addition assignment operation.");
  } else if (tmp > static_cast<int256_t>(std::numeric_limits<uint_t>::max())) {
    throw std::overflow_error("Overflow in addition assignment operation.");
  }
  markAsUsed(); this->value_ = static_cast<uint_t>(tmp); return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator*=(const SafeUint_t<Size>& other) {
  if (other.value_ == 0 || this->value_ == 0) throw std::domain_error("Multiplication assignment by zero");
  if (this->value_ > std::numeric_limits<uint_t>::max() / other.value_) {
    throw std::overflow_error("Overflow in multiplication assignment operation.");
  }
  markAsUsed(); this->value_ *= other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator*=(const uint_t& other) {
  if (other == 0 || this->value_ == 0) throw std::domain_error("Multiplication assignment by zero");
  if (this->value_ > std::numeric_limits<uint_t>::max() / other) {
    throw std::overflow_error("Overflow in multiplication assignment operation.");
  }
  markAsUsed(); this->value_ *= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator*=(const int& other) {
  if (other == 0 || this->value_ == 0) throw std::domain_error("Multiplication assignment by zero");
  if (other < 0) {
    throw std::underflow_error("Underflow in multiplication assignment operation.");
  } else {
    if (this->value_ > std::numeric_limits<uint_t>::max() / other) {
      throw std::overflow_error("Overflow in multiplication assignment operation.");
    }
  }
  markAsUsed(); this->value_ *= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator/=(const SafeUint_t<Size>& other) {
  if (this->value_ == 0 || other.value_ == 0) throw std::domain_error("Division assignment by zero");
  markAsUsed(); this->value_ /= other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator/=(const uint_t& other) {
  if (this->value_ == 0 || other == 0) throw std::domain_error("Division assignment by zero");
  markAsUsed(); this->value_ /= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator/=(const int& other) {
  if (other == 0) throw std::domain_error("Division assignment by zero");
  // Division by a negative number results in a negative result, which cannot be represented in an unsigned integer.
  if (other < 0) throw std::domain_error("Division assignment by a negative number");
  markAsUsed(); this->value_ /= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator%=(const SafeUint_t<Size>& other) {
  if (this->value_ == 0 || other.value_ == 0) throw std::domain_error("Modulus assignment by zero");
  markAsUsed(); this->value_ %= other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator%=(const uint_t& other) {
  if (this->value_ == 0 || other == 0) throw std::domain_error("Modulus assignment by zero");
  markAsUsed(); this->value_ %= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator%=(const int& other) {
  if (this->value_ == 0 || other == 0) throw std::domain_error("Modulus assignment by zero");
  markAsUsed(); this->value_ %= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator&=(const SafeUint_t<Size>& other) {
  markAsUsed(); this->value_ &= other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator&=(const uint_t& other) {
  markAsUsed(); this->value_ &= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator&=(const int& other) {
  if (other < 0) throw std::domain_error("Bitwise AND assignment with a negative value");
  markAsUsed(); this->value_ &= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator|=(const SafeUint_t<Size>& other) {
  markAsUsed(); this->value_ |= other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator|=(const uint_t& other) {
  markAsUsed(); this->value_ |= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator|=(const int& other) {
  if (other < 0) throw std::domain_error("Bitwise OR assignment with a negative value");
  markAsUsed(); this->value_ |= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator^=(const SafeUint_t<Size>& other) {
  markAsUsed(); this->value_ ^= other.value_; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator^=(const uint_t& other) {
  markAsUsed(); this->value_ ^= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator^=(const int& other) {
  if (other < 0) throw std::domain_error("Bitwise XOR assignment with a negative value.");
  markAsUsed(); this->value_ ^= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator<<=(const uint8_t& other) {
  markAsUsed(); this->value_ <<= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator>>=(const uint8_t& other) {
  markAsUsed(); this->value_ >>= other; return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator++() {
  if (this->value_ == std::numeric_limits<uint_t>::max()) {
    throw std::overflow_error("Overflow in prefix increment operation.");
  }
  markAsUsed(); ++(this->value_); return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator++(int) {
  if (this->value_ == std::numeric_limits<uint_t>::max()) {
    throw std::overflow_error("Overflow in postfix increment operation.");
  }
  SafeUint_t<Size> tmp(this->value_);
  markAsUsed(); ++(this->value_); return tmp;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size>& SafeUint_t<Size>::operator--() {
  if (this->value_ == 0) throw std::underflow_error("Underflow in prefix decrement operation.");
  markAsUsed(); --(this->value_); return *this;
}

template <int Size> requires UintInterval<Size>
SafeUint_t<Size> SafeUint_t<Size>::operator--(int) {
  if (this->value_ == 0) throw std::underflow_error("Underflow in postfix decrement operation.");
  SafeUint_t<Size> tmp(this->value_);
  markAsUsed(); --(this->value_); return tmp;
}

