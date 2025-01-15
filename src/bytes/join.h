/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BYTES_JOIN_H
#define BYTES_JOIN_H

#include "initializer.h" // view.h -> range.h

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

Byte* joinImpl(Byte *dest, const SizedInitializer auto& init) {
  init.to(dest);
  return dest + init.size();
}

Byte* joinImpl(Byte *dest, const DataRange auto& range) {
  std::memcpy(dest, std::ranges::data(range), std::ranges::size(range));
  return dest + std::ranges::size(range);
}

Byte* joinImpl(Byte *dest, const auto& arg, const auto&... args) {
  return joinImpl(joinImpl(dest, arg), args...);
}

} // namespace detail

  /**
   * Join several raw byte strings into one.
   * @tparam Ts The raw byte strings' type.
   * @param args One or more raw byte strings.
   * @return A fixed-size initializer with the result of the concatenation.
   */
  template<typename... Ts> SizedInitializer auto join(Ts&&... args) {
    const size_t size = detail::joinedSize(args...);

  auto func = [args_ = std::tuple<Ts...>(std::forward<Ts>(args)...)] (Byte *dest) {
    std::apply(detail::joinImpl<Ts...>, std::tuple_cat(std::make_tuple(dest), std::tuple<const Ts&...>(args_)));
  };

  return makeInitializer(size, std::move(func));
}

} // namespace bytes

#endif // BYTES_JOIN_H
