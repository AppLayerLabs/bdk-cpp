#ifndef BYTES_INITIALIZER_H
#define BYTES_INITIALIZER_H

#include "view.h"

namespace bytes {

/**
 * The concept of a initializer. It's basically a callable that's capable
 * of receiving a bytes Span (i.e. a sized range of contiguous bytes).
*/
template<typename T>
concept Initializer = requires (T&& a) {
  a(std::declval<Span>());
};

/**
 * An Initializer that has a size() member. It means that the
 * initializer can only fit containers that has capacity to
 * hold size() bytes.
*/
template<typename T>
concept SizedInitializer = Initializer<T> && requires(T&& a) {
  { a.size() } -> std::convertible_to<size_t>;
};

} // namespace bytes

#endif // BYTES_INITIALIZER_H
