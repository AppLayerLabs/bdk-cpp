/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STRCONV_H
#define STRCONV_H

#include <cstdint> // uint8_t
#include <string>
#include <vector>

#include "../bytes/view.h"

using Byte = uint8_t; ///< Typedef for Byte.
using Bytes = std::vector<Byte>; ///< Typedef for Bytes.

/// Namespace for general string manipulation and conversion functions.
namespace StrConv {
  /**
   * Convert a C-style raw byte array to a raw bytes string.
   * @param arr The array to convert.
   * @param size The size of the array.
   * @return The converted raw bytes string.
   */
  Bytes cArrayToBytes(const uint8_t* arr, size_t size);

  /**
   * Add padding to the left of a byte vector.
   * @param bytes The vector to pad.
   * @param charAmount The total amount of characters the resulting string should have.
   *                   If this is less than the string's original size,
   *                   the string will remain untouched.
   *                   e.g. `padLeftBytes("aaa", 5)` = "00aaa", `padLeftBytes("aaa", 2)` = "aaa"
   * @param sign (optional) The character to use as padding. Defaults to '0'.
   * @return The padded vector.
   */
  Bytes padLeftBytes(const bytes::View bytes, unsigned int charAmount, uint8_t sign = 0x00);

  /**
   * Add padding to the right of a byte vector.
   * @param bytes The vector to pad.
   * @param charAmount The total amount of characters the resulting string should have.
   *                   If this is less than the string's original size,
   *                   the string will remain untouched.
   *                   e.g. `padLeftBytes("aaa", 5)` = "aaa00", `padLeftBytes("aaa", 2)` = "aaa"
   * @param sign (optional) The character to use as padding. Defaults to '0'.
   * @return The padded vector.
   */
  Bytes padRightBytes(const bytes::View bytes, unsigned int charAmount, uint8_t sign = 0x00);

  /// Overload of padLeftBytes() that works with UTF-8 strings.
  std::string padLeft(std::string str, unsigned int charAmount, char sign = '\x00');

  /// Overload of padRightBytes() that works with UTF-8 strings.
  std::string padRight(std::string str, unsigned int charAmount, char sign = '\x00');

  /**
   * Convert a string to all-lowercase. Conversion is done in-place.
   * @param str The string to convert.
   */
  void toLower(std::string& str);

  /**
   * Convert a string to all-uppercase. Conversion is done in-place.
   * @param str The string to convert.
   */
  void toUpper(std::string& str);
};

#endif  // STRCONV_H
