#pragma once

namespace traits {

template<typename T>
struct Method {};

template<typename C, typename R, typename... Ts>
struct Method<R (C::*)(Ts...)> {
  using ReturnType = R;
  using ClassType = C;
  static constexpr bool IsView = false;
};

template<typename C, typename R, typename... Ts>
struct Method<R (C::*)(Ts...) const> {
  using ReturnType = R;
  using ClassType = const C;
  static constexpr bool IsView = true;
};

} // namespace traits
