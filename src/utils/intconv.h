/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef INTCONV_H
#define INTCONV_H

#include <boost/multiprecision/cpp_int.hpp>

#include "../bytes/view.h"

using Byte = uint8_t; ///< Typedef for Byte.
using Bytes = std::vector<Byte>; ///< Typedef for Bytes.
template <std::size_t N> using BytesArr = std::array<Byte, N>; ///< Typedef for BytesArr.

// Used internally by bytesToInt256().
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

///@{
/** Aliases for int_t. 8/16/32/64 are builtin from std. */
using int256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int248_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<248, 248, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int240_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<240, 240, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int232_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<232, 232, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int224_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<224, 224, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int216_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<216, 216, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int208_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<208, 208, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int200_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<200, 200, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int192_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<192, 192, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int184_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<184, 184, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int176_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<176, 176, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int168_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<168, 168, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int152_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<152, 152, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int144_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<144, 144, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int136_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<136, 136, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int128_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int120_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<120, 120, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int112_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<112, 112, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int104_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<104, 104, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int96_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<96, 96, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int88_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<88, 88, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int80_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<80, 80, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int72_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<72, 72, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int56_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<56, 56, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int48_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<48, 48, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int40_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<40, 40, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int24_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<24, 24, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
///@}

// TODO: theoretically missing the other intervals

/// Namespace for signed integer conversion functions.
namespace IntConv {
  ///@{
  /**
   * Convert a given integer to a bytes string. Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted integer as a bytes string.
   */
  BytesArr<32> int256ToBytes(const int256_t& i);
  BytesArr<17> int136ToBytes(const int136_t& i);
  BytesArr<8> int64ToBytes(const int64_t& i);
  BytesArr<4> int32ToBytes(const int32_t& i);
  ///@}

  ///@{
  /**
   * Convert a given bytes string to an integer.
   * @param b The bytes string to convert.
   * @return The converted integer.
   * @throw DynamicException if string size is invalid.
   */
  int256_t bytesToInt256(const View<Bytes> b);
  int136_t bytesToInt136(const View<Bytes> b);
  int64_t bytesToInt64(const View<Bytes> b);
  int32_t bytesToInt32(const View<Bytes> b);
  ///@}
};

#endif // INTCONV_H
