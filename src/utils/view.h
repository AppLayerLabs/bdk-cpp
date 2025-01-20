#ifndef BDK_UTILS_VIEW_H
#define BDK_UTILS_VIEW_H

#include <vector>
#include <span>
#include "bytes/range.h"
#include "bytes.h"

/**
 * Base class template for defining a view of a type.
 * Specific view types must specialize it.
 */
template<typename T>
struct View;

/**
 * Most generic purpose view specialization.
 * Behaves just like a span of constant bytes.
 */
template<>
struct View<Bytes> : std::span<const Byte> {

  /**
   * Constructs an empty view of size 0
   */
  constexpr View() = default;

  /**
   * Constructs a view from a given data range.
   * The range must also follow the constraints for initializing a span.
   * 
   * @param range the contiguous and sized range of bytes
   */
  template<bytes::DataRange R>
  constexpr View(R&& range) : span(std::forward<R>(range)) {}

  /**
   * Constructs a view from an data iterator and a size.
   * 
   * @param it the contiguous iterator of the range beginning
   * @param size the number of bytes to be viewed
   */
  template<bytes::DataIterator I>
  constexpr View(I it, size_t size) : span(it, size) {}

  /**
   * Constructs a view from a iterator-sentinel pair.
   * 
   * @param begin the contiguous iterator of the range beginning
   * @param end the sentinel iterator of the range end
   */
  template<bytes::DataIterator I, std::sized_sentinel_for<I> S>
  constexpr View(I begin, S end) : span(begin, end) {}

  constexpr auto cbegin() const { return begin(); }

  constexpr auto cend() const { return end(); }

  explicit operator Bytes() const {
    return Bytes(begin(), end());
  }
};

/**
 * Partial initialization for enabling all view types as borrowed.
 * All view types are borrowed since they don't own the data.
 */
template<typename T>
constexpr bool std::ranges::enable_borrowed_range<View<T>> = true;

#endif // BDK_UTILS_VIEW_H
