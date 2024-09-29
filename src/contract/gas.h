
#pragma once

struct ExecutionFailure : std::runtime_error {
  explicit ExecutionFailure(std::string_view str) : std::runtime_error(str.data()) {}
};

struct OutOfGas : ExecutionFailure {
  OutOfGas() : ExecutionFailure("out of gas") {}
};

struct ExecutionReverted : ExecutionFailure {
  explicit ExecutionReverted(std::string_view msg) : ExecutionFailure(msg) {}
  explicit ExecutionReverted() : ExecutionReverted("execution reverted") {}
};

namespace kind {

struct Normal{};
struct Static{};
struct Delegate{};

using Any = std::variant<Normal, Static, Delegate>;

inline constexpr Normal NORMAL{};
inline constexpr Static STATIC{};
inline constexpr Delegate DELEGATE{};

};

class Gas {
public:
  constexpr explicit Gas(uint64_t value) : value_(value) {}

  constexpr uint64_t value() const { return value_; }

  constexpr void use(uint64_t amount) {
    if (amount > value_) {
      value_ = 0;
      throw OutOfGas();
    }

    value_ -= amount;
  }

  constexpr void refund(uint64_t amount) {
    value_ += amount; // TODO: check overflow?
  }

// private:
  uint64_t value_;
};