#include "safeint.h"

// Explicit instantiations

template class SafeInt_t<8>;
template class SafeInt_t<16>;
template class SafeInt_t<32>;
template class SafeInt_t<64>;

template class SafeInt_t<24>;
template class SafeInt_t<40>;
template class SafeInt_t<48>;
template class SafeInt_t<56>;
template class SafeInt_t<72>;
template class SafeInt_t<80>;
template class SafeInt_t<88>;
template class SafeInt_t<96>;
template class SafeInt_t<104>;
template class SafeInt_t<112>;
template class SafeInt_t<120>;
template class SafeInt_t<128>;
template class SafeInt_t<136>;
template class SafeInt_t<144>;
template class SafeInt_t<152>;
template class SafeInt_t<160>;
template class SafeInt_t<168>;
template class SafeInt_t<176>;
template class SafeInt_t<184>;
template class SafeInt_t<192>;
template class SafeInt_t<200>;
template class SafeInt_t<208>;
template class SafeInt_t<216>;
template class SafeInt_t<224>;
template class SafeInt_t<232>;
template class SafeInt_t<240>;
template class SafeInt_t<248>;
template class SafeInt_t<256>;

// Class impl starts here

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator+(const int_t& other) const {
  if ((other > 0) && (this->value_ > std::numeric_limits<int_t>::max() - other)) {
    throw std::overflow_error("Overflow in addition operation.");
  }
  if ((other < 0) && (this->value_ < std::numeric_limits<int_t>::min() - other)) {
    throw std::underflow_error("Underflow in addition operation.");
  }
  return SafeInt_t<Size>(this->value_ + other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator+(const SafeInt_t<Size>& other) const {
  if ((other.get() > 0) && (this->value_ > std::numeric_limits<int_t>::max() - other.get())) {
    throw std::overflow_error("Overflow in addition operation.");
  }
  if ((other.get() < 0) && (this->value_ < std::numeric_limits<int_t>::min() - other.get())) {
    throw std::underflow_error("Underflow in addition operation.");
  }
  return SafeInt_t<Size>(this->value_ + other.get());
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator-(const int_t& other) const {
  if ((other < 0) && (this->value_ > std::numeric_limits<int_t>::max() + other)) {
    throw std::overflow_error("Overflow in subtraction operation.");
  }
  if ((other > 0) && (this->value_ < std::numeric_limits<int_t>::min() + other)) {
    throw std::underflow_error("Underflow in subtraction operation.");
  }
  return SafeInt_t<Size>(this->value_ - other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator-(const SafeInt_t<Size>& other) const {
  if ((other.get() < 0) && (this->value_ > std::numeric_limits<int_t>::max() + other.get())) {
    throw std::overflow_error("Overflow in subtraction operation.");
  }
  if ((other.get() > 0) && (this->value_ < std::numeric_limits<int_t>::min() + other.get())) {
    throw std::underflow_error("Underflow in subtraction operation.");
  }
  return SafeInt_t<Size>(this->value_ - other.get());
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator*(const int_t& other) const {
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

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator*(const SafeInt_t<Size>& other) const {
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

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator/(const int_t& other) const {
  if (other == 0) throw std::domain_error("Division by zero");
  // Edge case - dividing the smallest negative number by -1 causes overflow
  if (this->value_ == std::numeric_limits<int_t>::min() && other == -1) {
    throw std::overflow_error("Overflow in division operation.");
  }
  return SafeInt_t<Size>(this->value_ / other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator/(const SafeInt_t<Size>& other) const {
  if (other.get() == 0) throw std::domain_error("Division by zero");
  // Edge case - dividing the smallest negative number by -1 causes overflow
  if (this->value_ == std::numeric_limits<int_t>::min() && other.get() == -1) {
    throw std::overflow_error("Overflow in division operation.");
  }
  return SafeInt_t<Size>(this->value_ / other.get());
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator%(const int_t& other) const {
  if (other == 0) throw std::domain_error("Modulus by zero");
  return SafeInt_t<Size>(this->value_ % other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator%(const SafeInt_t<Size>& other) const {
  if (other.get() == 0) throw std::domain_error("Modulus by zero");
  return SafeInt_t<Size>(this->value_ % other.get());
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator&(const int_t& other) const {
  return SafeInt_t<Size>(this->value_ & other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator&(const SafeInt_t<Size>& other) const {
  return SafeInt_t<Size>(this->value_ & other.get());
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator|(const int_t& other) const {
  return SafeInt_t<Size>(this->value_ | other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator|(const SafeInt_t<Size>& other) const {
  return SafeInt_t<Size>(this->value_ | other.get());
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator^(const int_t& other) const {
  return SafeInt_t<Size>(this->value_ ^ other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator^(const SafeInt_t<Size>& other) const {
  return SafeInt_t<Size>(this->value_ ^ other.get());
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator<<(const uint8_t& other) const {
  return SafeInt_t<Size>(this->value_ << other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator>>(const uint8_t& other) const {
  return SafeInt_t<Size>(this->value_ >> other);
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>::operator bool() const { return bool(this->value_); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator==(const int_t& other) const { return (this->value_ == other); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator==(const SafeInt_t<Size>& other) const { return (this->value_ == other.get()); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator!=(const int_t& other) const { return (this->value_ != other); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator!=(const SafeInt_t<Size>& other) const { return (this->value_ != other.get()); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator<(const int_t& other) const { return (this->value_ < other); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator<(const SafeInt_t<Size>& other) const { return (this->value_ < other.get()); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator<=(const int_t& other) const { return (this->value_ <= other); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator<=(const SafeInt_t<Size>& other) const { return (this->value_ <= other.get()); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator>(const int_t& other) const { return (this->value_ > other); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator>(const SafeInt_t<Size>& other) const { return (this->value_ > other.get()); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator>=(const int_t& other) const { return (this->value_ >= other); }

template <int Size> requires IntInterval<Size>
bool SafeInt_t<Size>::operator>=(const SafeInt_t<Size>& other) const { return (this->value_ >= other.get()); }

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator=(const int_t& other) {
  markAsUsed(); this->value_ = other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator=(const SafeInt_t<Size>& other) {
  markAsUsed(); this->value_ = other.get(); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator+=(const int_t& other) {
  if ((other > 0) && (this->value_ > std::numeric_limits<int_t>::max() - other)) {
    throw std::overflow_error("Overflow in addition assignment operation.");
  }
  if ((other < 0) && (this->value_ < std::numeric_limits<int_t>::min() - other)) {
    throw std::underflow_error("Underflow in addition assignment operation.");
  }
  markAsUsed(); this->value_ += other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator+=(const SafeInt_t<Size>& other) {
  if ((other.get() > 0) && (this->value_ > std::numeric_limits<int_t>::max() - other.get())) {
    throw std::overflow_error("Overflow in addition assignment operation.");
  }
  if ((other.get() < 0) && (this->value_ < std::numeric_limits<int_t>::min() - other.get())) {
    throw std::underflow_error("Underflow in addition assignment operation.");
  }
  markAsUsed(); this->value_ += other.get(); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator-=(const int_t& other) {
  if ((other < 0) && (this->value_ > std::numeric_limits<int_t>::max() + other)) {
    throw std::overflow_error("Overflow in subtraction assignment operation.");
  }
  if ((other > 0) && (this->value_ < std::numeric_limits<int_t>::min() + other)) {
    throw std::underflow_error("Underflow in subtraction assignment operation.");
  }
  markAsUsed(); this->value_ -= other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator-=(const SafeInt_t<Size>& other) {
  if ((other.get() < 0) && (this->value_ > std::numeric_limits<int_t>::max() + other.get())) {
    throw std::overflow_error("Overflow in subtraction assignment operation.");
  }
  if ((other.get() > 0) && (this->value_ < std::numeric_limits<int_t>::min() + other.get())) {
    throw std::underflow_error("Underflow in subtraction assignment operation.");
  }
  markAsUsed(); this->value_ -= other.get(); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator*=(const int_t& other) {
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

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator*=(const SafeInt_t<Size>& other) {
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

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator/=(const int_t& other) {
  if (other == 0) throw std::domain_error("Division assignment by zero.");
  // Handling the edge case where dividing the smallest negative number by -1 causes overflow
  if (this->value_ == std::numeric_limits<int_t>::min() && other == -1) {
    throw std::overflow_error("Overflow in division assignment operation.");
  }
  markAsUsed(); this->value_ /= other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator/=(const SafeInt_t<Size>& other) {
  if (other.get() == 0) throw std::domain_error("Division assignment by zero.");
  // Handling the edge case where dividing the smallest negative number by -1 causes overflow
  if (this->value_ == std::numeric_limits<int_t>::min() && other.get() == -1) {
    throw std::overflow_error("Overflow in division assignment operation.");
  }
  markAsUsed(); this->value_ /= other.get(); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator%=(const int_t& other) {
  if (other == 0) throw std::domain_error("Modulus assignment by zero.");
  markAsUsed(); this->value_ %= other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator%=(const SafeInt_t<Size>& other) {
  if (other.get() == 0) throw std::domain_error("Modulus assignment by zero.");
  markAsUsed(); this->value_ %= other.get(); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator&=(const int_t& other) {
  markAsUsed(); this->value_ &= other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator&=(const SafeInt_t<Size>& other) {
  markAsUsed(); this->value_ &= other.get(); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator|=(const int_t& other) {
  markAsUsed(); this->value_ |= other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator|=(const SafeInt_t<Size>& other) {
  markAsUsed(); this->value_ |= other.get(); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator^=(const int_t& other) {
  markAsUsed(); this->value_ ^= other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator^=(const SafeInt_t<Size>& other) {
  markAsUsed(); this->value_ ^= other.get(); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator<<=(const uint8_t& other) {
  markAsUsed(); this->value_ <<= other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator>>=(const uint8_t& other) {
  markAsUsed(); this->value_ >>= other; return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator++() {
  if (this->value_ == std::numeric_limits<int_t>::max()) {
    throw std::overflow_error("Overflow in prefix increment operation.");
  }
  markAsUsed(); ++(this->value_); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator++(int) {
  if (this->value_ == std::numeric_limits<int_t>::max()) {
    throw std::overflow_error("Overflow in postfix increment operation.");
  }
  markAsUsed(); SafeInt_t<Size> temp(this->value_); ++(this->value_); return temp;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size>& SafeInt_t<Size>::operator--() {
  if (this->value_ == std::numeric_limits<int_t>::min()) {
    throw std::underflow_error("Underflow in prefix decrement operation.");
  }
  markAsUsed(); --(this->value_); return *this;
}

template <int Size> requires IntInterval<Size>
SafeInt_t<Size> SafeInt_t<Size>::operator--(int) {
  if (this->value_ == std::numeric_limits<int_t>::min()) {
    throw std::underflow_error("Underflow in postfix decrement operation.");
  }
  markAsUsed(); SafeInt_t<Size> temp(this->value_); --(this->value_); return temp;
}

