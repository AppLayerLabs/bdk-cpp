#ifndef BDK_BYTES_HEX_H
#define BDK_BYTES_HEX_H

#include <string_view>
#include <stdexcept>
#include "range.h"
#include "initializer.h"
#include "utils/hex.h"

namespace bytes {

/**
 * Creates a sized initializer from a hex representation. The initializer will
 * fill the container data with by converting the hexadecimal representation
 * into binary data.
 * @param str the hex string
 * @return the sized initializer
 */
constexpr SizedInitializer auto hex(std::string_view str) {
  if (str.size() >= 2 && str.starts_with("0x")) {
    str = str.substr(2);
  }

  if (str.size() % 2) {
    throw std::invalid_argument("the length of hex string is required to be even number");
  }

  return makeInitializer(str.size() / 2, [str] (Byte* dest) {
    const auto value = [] (char c) -> Byte {
      if (c >= '0' && c <= '9')
        return c - '0';
      else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
      else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
      else
        throw std::invalid_argument("character '" + std::to_string(c) + "' is invalid in hex string");
    };

    auto it = str.begin();

    while (it != str.end()) {
      const Byte l = value(*it++);
      const Byte r = value(*it++);
      *dest++ = (l << 4) | r;
    }
  });
}

} // namespace bytes

#endif // BDK_BYTES_HEX_H
