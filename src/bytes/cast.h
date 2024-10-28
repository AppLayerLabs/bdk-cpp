#ifndef BDK_BYTES_CAST_H
#define BDK_BYTES_CAST_H

#include "range.h"

namespace bytes {

/**
 * Casts a range of bytes from one type to another.
 * 
 * @param src the input bytes to be cast
 * @param dest the destiny bytes container
 * @return the destiny bytes container
 * @throws invalid argument exception on size incompatibility
 */
template<Range R>
constexpr R cast(const Range auto& src, R dest = R()) {
  if (std::ranges::size(src) != std::ranges::size(dest)) {
    throw std::invalid_argument("incompatible sizes for casting");
  }

  std::ranges::copy(src, std::ranges::begin(dest));

  return dest;
}

} // namespace bytes

#endif // BDK_BYTES_CAST_H
