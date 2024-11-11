#ifndef BDK_UTILS_BYTES_H
#define BDK_UTILS_BYTES_H

#include <array>
#include <vector>

using Byte = uint8_t;

using Bytes = std::vector<Byte>;

template <size_t N>
using BytesArr = std::array<Byte, N>;

#endif // BDK_UTILS_BYTES_H
