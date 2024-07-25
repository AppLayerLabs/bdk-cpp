#ifndef BYTES_JOIN_H
#define BYTES_JOIN_H

#include "view.h"
#include "range.h"

#include "utils/dynamicexception.h"

namespace bytes {

namespace detail {

std::size_t joinedSize(const SizedInitializer auto& arg) {
  return arg.size();
}

std::size_t joinedSize(const DataRange auto& arg) {
  return std::ranges::size(arg);
}

std::size_t joinedSize(const auto& arg, const auto&... args) {
  return joinedSize(arg) + joinedSize(args...);
}

Byte *joinImpl(Byte *dest, const SizedInitializer auto& init) {
  init.to(dest);
  return dest + init.size();
}

Byte *joinImpl(Byte *dest, const DataRange auto& range) {
  std::memcpy(dest, std::ranges::data(range), std::ranges::size(range));
  return dest + std::ranges::size(range);
}

Byte *joinImpl(Byte *dest, const auto& arg, const auto&... args) {
  return joinImpl(joinImpl(dest, arg), args...);
}

} // namespace detail

template<typename... Ts>
SizedInitializer auto join(Ts&&... args) {
  return makeInitializer(detail::joinedSize(args...), [args_ = std::tuple<Ts...>(std::forward<Ts>(args)...)]
  (Byte *dest) {
    std::apply(detail::joinImpl<Ts...>, std::tuple_cat(std::make_tuple(dest), args_));
  });
}

} // namespace bytes

#endif // BYTES_JOIN_H
