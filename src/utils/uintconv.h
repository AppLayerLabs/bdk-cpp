/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef UINTCONV_H
#define UINTCONV_H

#include <boost/multiprecision/cpp_int.hpp>

#include "../bytes/view.h"

using Byte = uint8_t; ///< Typedef for Byte.
using Bytes = std::vector<Byte>; ///< Typedef for Bytes.
template <std::size_t N> using BytesArr = std::array<Byte, N>; ///< Typedef for BytesArr.

///@{
/** Aliases for uint_t. 8/16/32/64 are builtin from std. */
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint248_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<248, 248, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint240_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<240, 240, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint232_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<232, 232, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint224_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<224, 224, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint216_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<216, 216, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint208_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<208, 208, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint200_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<200, 200, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint192_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<192, 192, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint184_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<184, 184, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint176_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<176, 176, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint168_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<168, 168, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint152_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<152, 152, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint144_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<144, 144, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint136_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<136, 136, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint128_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint120_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<120, 120, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint112_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<112, 112, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint104_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<104, 104, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint96_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<96, 96, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint88_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<88, 88, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint80_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<80, 80, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint72_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<72, 72, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint56_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<56, 56, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint48_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<48, 48, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint40_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<40, 40, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint24_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<24, 24, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
///@}

/// Namespace for unsigned integer conversion functions.
namespace UintConv {
  ///@{
  /**
   * Convert a given integer to a bytes string. Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted integer as a bytes string.
   */
  BytesArr<32> uint256ToBytes(const uint256_t& i);
  BytesArr<31> uint248ToBytes(const uint248_t& i);
  BytesArr<30> uint240ToBytes(const uint240_t& i);
  BytesArr<29> uint232ToBytes(const uint232_t& i);
  BytesArr<28> uint224ToBytes(const uint224_t& i);
  BytesArr<27> uint216ToBytes(const uint216_t& i);
  BytesArr<26> uint208ToBytes(const uint208_t& i);
  BytesArr<25> uint200ToBytes(const uint200_t& i);
  BytesArr<24> uint192ToBytes(const uint192_t& i);
  BytesArr<23> uint184ToBytes(const uint184_t& i);
  BytesArr<22> uint176ToBytes(const uint176_t& i);
  BytesArr<21> uint168ToBytes(const uint168_t& i);
  BytesArr<20> uint160ToBytes(const uint160_t& i);
  BytesArr<19> uint152ToBytes(const uint152_t& i);
  BytesArr<18> uint144ToBytes(const uint144_t& i);
  BytesArr<17> uint136ToBytes(const uint136_t& i);
  BytesArr<16> uint128ToBytes(const uint128_t& i);
  BytesArr<15> uint120ToBytes(const uint120_t& i);
  BytesArr<14> uint112ToBytes(const uint112_t& i);
  BytesArr<13> uint104ToBytes(const uint104_t& i);
  BytesArr<12> uint96ToBytes(const uint96_t& i);
  BytesArr<11> uint88ToBytes(const uint88_t& i);
  BytesArr<10> uint80ToBytes(const uint80_t& i);
  BytesArr<9> uint72ToBytes(const uint72_t& i);
  BytesArr<7> uint56ToBytes(const uint56_t& i);
  BytesArr<6> uint48ToBytes(const uint48_t& i);
  BytesArr<5> uint40ToBytes(const uint40_t& i);
  BytesArr<3> uint24ToBytes(const uint24_t& i);

  BytesArr<8> uint64ToBytes(const uint64_t& i);
  BytesArr<4> uint32ToBytes(const uint32_t& i);
  BytesArr<2> uint16ToBytes(const uint16_t& i);
  BytesArr<1> uint8ToBytes(const uint8_t& i);
  ///@}

  ///@{
  /**
   * Convert a given bytes string to an integer.
   * @param b The bytes string to convert.
   * @return The converted integer.
   * @throw DynamicException if string size is invalid.
   */
  uint256_t bytesToUint256(const bytes::View b);
  uint248_t bytesToUint248(const bytes::View b);
  uint240_t bytesToUint240(const bytes::View b);
  uint232_t bytesToUint232(const bytes::View b);
  uint224_t bytesToUint224(const bytes::View b);
  uint216_t bytesToUint216(const bytes::View b);
  uint208_t bytesToUint208(const bytes::View b);
  uint200_t bytesToUint200(const bytes::View b);
  uint192_t bytesToUint192(const bytes::View b);
  uint184_t bytesToUint184(const bytes::View b);
  uint176_t bytesToUint176(const bytes::View b);
  uint168_t bytesToUint168(const bytes::View b);
  uint160_t bytesToUint160(const bytes::View b);
  uint152_t bytesToUint152(const bytes::View b);
  uint144_t bytesToUint144(const bytes::View b);
  uint136_t bytesToUint136(const bytes::View b);
  uint128_t bytesToUint128(const bytes::View b);
  uint120_t bytesToUint120(const bytes::View b);
  uint112_t bytesToUint112(const bytes::View b);
  uint104_t bytesToUint104(const bytes::View b);
  uint96_t bytesToUint96(const bytes::View b);
  uint88_t bytesToUint88(const bytes::View b);
  uint80_t bytesToUint80(const bytes::View b);
  uint72_t bytesToUint72(const bytes::View b);
  uint56_t bytesToUint56(const bytes::View b);
  uint48_t bytesToUint48(const bytes::View b);
  uint40_t bytesToUint40(const bytes::View b);
  uint24_t bytesToUint24(const bytes::View b);

  uint64_t bytesToUint64(const bytes::View b);
  uint32_t bytesToUint32(const bytes::View b);
  uint16_t bytesToUint16(const bytes::View b);
  uint8_t bytesToUint8(const bytes::View b);
  ///@}
};

#endif // UINTCONV_H
