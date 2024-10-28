#ifndef BDK_UTILS_VIEW_H
#define BDK_UTILS_VIEW_H

#include <vector>
#include <span>
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
  using std::span<const Byte>::span;
};

#endif // BDK_UTILS_VIEW_H
