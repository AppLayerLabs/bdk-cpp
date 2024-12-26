#ifndef BDK_UTILS_FINALLY_H
#define BDK_UTILS_FINALLY_H

#include <utility>
#include <concepts>

template<std::invocable F>
class Finally {
public:
  explicit constexpr Finally(F func) : func_(std::move(func)) {}
  constexpr ~Finally() { std::invoke(func_); }
private:
  F func_;
};

#endif // BDK_UTILS_FINALLY_H
