
#pragma once

#include "../traits/method.h"
#include "../contract.h"
#include "../../utils/strings.h"

namespace cpp {

template<typename M, typename... Args>
struct PackagedMethod {
  using ClassType = traits::Method<M>::ClassType;
  using ReturnType = traits::Method<M>::ReturnType;
  static constexpr bool IsView = traits::Method<M>::IsView;

  explicit PackagedMethod(M func, Args&&... args)
    : func(std::move(func)), args(std::forward<Args>(args)...) {}

  M func;
  std::tuple<Args...> args;
};

template<typename M, typename... Ts>
explicit PackagedMethod(M, Ts&&...) -> PackagedMethod<M, Ts...>;

template<typename Method>
struct Message {
  Address from;
  Address to;
  uint256_t value;
  const BaseContract *caller;
  Method method;
};

} // namespace cpp
