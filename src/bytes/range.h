/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BYTES_RANGE_H
#define BYTES_RANGE_H

#include <ranges>
#include "utils/bytes.h"

namespace bytes {
  /**
   * Concept of a bytes iterator.
   */
  template<typename T>
  concept Iterator = std::input_or_output_iterator<T> && std::same_as<std::iter_value_t<T>, Byte>;

  /**
   * Concept of a bytes contiguous bytes iterator.
   */
  template<typename T>
  concept DataIterator = Iterator<T> && std::contiguous_iterator<T>;

  /**
   * The concept of a range of bytes.
   */
  template<typename T>
  concept Range = std::ranges::range<T> && std::same_as<std::ranges::range_value_t<T>, Byte>;

  /**
   * The concept of a sized and contiguous range of bytes. This one is more interesting because
   * the copying of data can be defined as std::memcpy or std::memmove calls, which are very fast!
   */
  template<typename T>
  concept DataRange = Range<T> && std::ranges::contiguous_range<T> && std::ranges::sized_range<T>;

  /**
   * A data range that is also borrowed, which means that such range can be taken by value and 
   * pointers and iterators of it can be stored/returned without the danger of dangling. An example
   * of a BorrowedDataRange is the std::span<Byte>, because it does not owns the data.
   */
  template<typename T>
  concept BorrowedDataRange = DataRange<T> && std::ranges::borrowed_range<T>;
} // namespace bytes

#endif // BYTES_RANGE_H
