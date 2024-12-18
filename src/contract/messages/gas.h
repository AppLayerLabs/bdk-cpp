#ifndef BDK_MESSAGES_GAS_H
#define BDK_MESSAGES_GAS_H

#include "outofgas.h"

namespace messages {

class Gas {
public:
  constexpr explicit Gas(uint64_t value = 0) : value_(value) {}

  constexpr uint64_t value() const { return value_; }

  constexpr void use(uint64_t amount) {
    if (amount > value_) {
      value_ = 0;
      throw OutOfGas();
    }

    value_ -= amount;
  }

  constexpr void refund(uint64_t amount) {
    value_ += amount;
  }

private:
  uint64_t value_;
};

} // namespace messages

#endif // BDK_MESSAGES_GAS_H
