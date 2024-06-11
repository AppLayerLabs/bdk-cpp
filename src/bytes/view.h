
#ifndef BYTES_VIEW_H
#define BYTES_VIEW_H

#include <span>
#include "range.h"

namespace bytes {

/// A view over sized contiguous bytes
using View = std::span<const Byte>;

/// A span (i.e. a non-owning array of bytes that allows modification) over sized contiguous bytes
using Span = std::span<Byte>;

/**
 * Creates a view from the given data range. It needs to be Borrowed
 * because otherwise we would be allowing the creating of a dangling
 * view of bytes.
 * 
 * @param r the target data range to be viewed
 * @return a view object of the bytes
*/
constexpr View view(BorrowedDataRange auto&& r) { return View(r); }

/**
 * Creates a span from the given data range. It needs to be Borrowed
 * because otherwise we would be allowing the creating of a dangling
 * span of bytes.
 * 
 * @param r the target data range to construct the span
 * @return a span object of the bytes
*/
constexpr Span span(BorrowedDataRange auto&& r) { return Span(r); }

/**
 * Overload for creating a bytes view from a char string. It's useful
 * because a std::string_view is a contiguous and sized range, but the
 * element type is a char, not a Byte (a.k.a unsigned char).
 * 
 * @param str the target string
 * @return a bytes view of the string bytes
*/
inline View view(std::string_view str) {
  return View(reinterpret_cast<const Byte*>(str.data()), str.size());
}

/**
 * Overload for creating a bytes span from a given string reference.
 * 
 * @param str the target string
 * @return a bytes span of the string bytes
*/
inline Span span(std::string& str) {
  return Span(reinterpret_cast<Byte*>(str.data()), str.size());
}

/// The concept of a type that can be viewed as a sized range of contiguous bytes.
template<typename T>
concept Viewable = requires(const T& a) {
  { view(a) } -> std::convertible_to<View>;
};

} // namespace bytes

#endif // BYTES_VIEW_H
