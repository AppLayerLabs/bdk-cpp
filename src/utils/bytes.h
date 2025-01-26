#ifndef BDK_UTILS_BYTES_H
#define BDK_UTILS_BYTES_H

#include <cstdint>
#include <vector>

using Byte = uint8_t; ///< Typedef for a byte type.
using Bytes = std::vector<Byte>; ///< Typedef for a dynamically-sized byte container.
template <std::size_t N> using BytesArr = std::array<Byte, N>; ///< Typedef for a fixed-size byte array.

#endif // BDK_UTILS_BYTES_H
