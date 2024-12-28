/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "uintconv.h"

#include "dynamicexception.h"

// ==========================================================================
// UINT TO BYTES
// ==========================================================================

BytesArr<32> UintConv::uint256ToBytes(const uint256_t& i) {
  BytesArr<32> ret = {0};
  Bytes tmp;
  tmp.reserve(32);
  ret.fill(0x00);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[31 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<31> UintConv::uint248ToBytes(const uint248_t &i) {
  BytesArr<31> ret = {0};
  Bytes tmp;
  tmp.reserve(31);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it the right size. Applies to all similar functions.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[30 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<30> UintConv::uint240ToBytes(const uint240_t &i) {
  BytesArr<30> ret = {0};
  Bytes tmp;
  tmp.reserve(30);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[29 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<29> UintConv::uint232ToBytes(const uint232_t &i) {
  BytesArr<29> ret = {0};
  Bytes tmp;
  tmp.reserve(29);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[28 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<28> UintConv::uint224ToBytes(const uint224_t &i) {
  BytesArr<28> ret = {0};
  Bytes tmp;
  tmp.reserve(28);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[27 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<27> UintConv::uint216ToBytes(const uint216_t &i) {
  BytesArr<27> ret = {0};
  Bytes tmp;
  tmp.reserve(27);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[26 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<26> UintConv::uint208ToBytes(const uint208_t &i) {
  BytesArr<26> ret = {0};
  Bytes tmp;
  tmp.reserve(26);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[25 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<25> UintConv::uint200ToBytes(const uint200_t &i) {
  BytesArr<25> ret = {0};
  Bytes tmp;
  tmp.reserve(25);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[24 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<24> UintConv::uint192ToBytes(const uint192_t &i) {
  BytesArr<24> ret = {0};
  Bytes tmp;
  tmp.reserve(24);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[23 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<23> UintConv::uint184ToBytes(const uint184_t &i) {
  BytesArr<23> ret = {0};
  Bytes tmp;
  tmp.reserve(23);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[22 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<22> UintConv::uint176ToBytes(const uint176_t &i) {
  BytesArr<22> ret = {0};
  Bytes tmp;
  tmp.reserve(22);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[21 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<21> UintConv::uint168ToBytes(const uint168_t &i) {
  BytesArr<21> ret = {0};
  Bytes tmp;
  tmp.reserve(21);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[20 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<20> UintConv::uint160ToBytes(const uint160_t &i) {
  BytesArr<20> ret = {0};
  Bytes tmp;
  tmp.reserve(20);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[19 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<19> UintConv::uint152ToBytes(const uint152_t &i) {
  BytesArr<19> ret = {0};
  Bytes tmp;
  tmp.reserve(19);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[18 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<18> UintConv::uint144ToBytes(const uint144_t &i) {
  BytesArr<18> ret = {0};
  Bytes tmp;
  tmp.reserve(18);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[17 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<17> UintConv::uint136ToBytes(const uint136_t &i) {
  BytesArr<17> ret = {0};
  Bytes tmp;
  tmp.reserve(17);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[16 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<16> UintConv::uint128ToBytes(const uint128_t &i) {
  BytesArr<16> ret = {0};
  Bytes tmp;
  tmp.reserve(16);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[15 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<15> UintConv::uint120ToBytes(const uint120_t &i) {
  BytesArr<15> ret = {0};
  Bytes tmp;
  tmp.reserve(15);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[14 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<14> UintConv::uint112ToBytes(const uint112_t &i) {
  BytesArr<14> ret = {0};
  Bytes tmp;
  tmp.reserve(14);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[13 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<13> UintConv::uint104ToBytes(const uint104_t &i) {
  BytesArr<13> ret = {0};
  Bytes tmp;
  tmp.reserve(13);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[12 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<12> UintConv::uint96ToBytes(const uint96_t &i) {
  BytesArr<12> ret = {0};
  Bytes tmp;
  tmp.reserve(12);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[11 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<11> UintConv::uint88ToBytes(const uint88_t &i) {
  BytesArr<11> ret = {0};
  Bytes tmp;
  tmp.reserve(11);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[10 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<10> UintConv::uint80ToBytes(const uint80_t &i) {
  BytesArr<10> ret = {0};
  Bytes tmp;
  tmp.reserve(10);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[9 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<9> UintConv::uint72ToBytes(const uint72_t &i) {
  BytesArr<9> ret = {0};
  Bytes tmp;
  tmp.reserve(9);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[8 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<7> UintConv::uint56ToBytes(const uint56_t &i) {
  BytesArr<7> ret = {0};
  Bytes tmp;
  tmp.reserve(7);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[6 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<6> UintConv::uint48ToBytes(const uint48_t &i) {
  BytesArr<6> ret = {0};
  Bytes tmp;
  tmp.reserve(6);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[5 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<5> UintConv::uint40ToBytes(const uint40_t &i) {
  BytesArr<5> ret = {0};
  Bytes tmp;
  tmp.reserve(5);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[4 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<3> UintConv::uint24ToBytes(const uint24_t &i) {
  BytesArr<3> ret = {0};
  Bytes tmp;
  tmp.reserve(3);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[2 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<8> UintConv::uint64ToBytes(const uint64_t& i) {
  BytesArr<8> ret = {0};
  std::memcpy(&ret[0], &i, 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

BytesArr<4> UintConv::uint32ToBytes(const uint32_t& i) {
  BytesArr<4> ret = {0};
  std::memcpy(&ret[0], &i, 4);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

BytesArr<2> UintConv::uint16ToBytes(const uint16_t& i) {
  BytesArr<2> ret = {0};
  std::memcpy(&ret[0], &i, 2);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

BytesArr<1> UintConv::uint8ToBytes(const uint8_t& i) {
  BytesArr<1> ret = {0};
  std::memcpy(&ret[0], &i, 1);
  return ret;
}

// ==========================================================================
// BYTES TO UINT
// ==========================================================================

uint256_t UintConv::bytesToUint256(const bytes::View b) {
  if (b.size() != 32) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 32, got " + std::to_string(b.size())
  );
  uint256_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint248_t UintConv::bytesToUint248(const bytes::View b) {
  if (b.size() != 31) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 31, got " + std::to_string(b.size())
  );
  uint248_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint240_t UintConv::bytesToUint240(const bytes::View b) {
  if (b.size() != 30) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 30, got " + std::to_string(b.size())
  );
  uint240_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint232_t UintConv::bytesToUint232(const bytes::View b) {
  if (b.size() != 29) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 29, got " + std::to_string(b.size())
  );
  uint232_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint224_t UintConv::bytesToUint224(const bytes::View b) {
  if (b.size() != 28) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 28, got " + std::to_string(b.size())
  );
  uint224_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint216_t UintConv::bytesToUint216(const bytes::View b) {
  if (b.size() != 27) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 27, got " + std::to_string(b.size())
  );
  uint216_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint208_t UintConv::bytesToUint208(const bytes::View b) {
  if (b.size() != 26) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 26, got " + std::to_string(b.size())
  );
  uint208_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint200_t UintConv::bytesToUint200(const bytes::View b) {
  if (b.size() != 25) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 25, got " + std::to_string(b.size())
  );
  uint200_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint192_t UintConv::bytesToUint192(const bytes::View b) {
  if (b.size() != 24) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 24, got " + std::to_string(b.size())
  );
  uint192_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint184_t UintConv::bytesToUint184(const bytes::View b) {
  if (b.size() != 23) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 23, got " + std::to_string(b.size())
  );
  uint184_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint176_t UintConv::bytesToUint176(const bytes::View b) {
  if (b.size() != 22) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 22, got " + std::to_string(b.size())
  );
  uint176_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint168_t UintConv::bytesToUint168(const bytes::View b) {
  if (b.size() != 21) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 21, got " + std::to_string(b.size())
  );
  uint168_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint160_t UintConv::bytesToUint160(const bytes::View b) {
  if (b.size() != 20) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 20, got " + std::to_string(b.size())
  );
  uint160_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint152_t UintConv::bytesToUint152(const bytes::View b) {
  if (b.size() != 19) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 19, got " + std::to_string(b.size())
  );
  uint152_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint144_t UintConv::bytesToUint144(const bytes::View b) {
  if (b.size() != 18) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 18, got " + std::to_string(b.size())
  );
  uint144_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint136_t UintConv::bytesToUint136(const bytes::View b) {
  if (b.size() != 17) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 17, got " + std::to_string(b.size())
  );
  uint136_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint128_t UintConv::bytesToUint128(const bytes::View b) {
  if (b.size() != 16) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 16, got " + std::to_string(b.size())
  );
  uint128_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint120_t UintConv::bytesToUint120(const bytes::View b) {
  if (b.size() != 15) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 15, got " + std::to_string(b.size())
  );
  uint120_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint112_t UintConv::bytesToUint112(const bytes::View b) {
  if (b.size() != 14) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 16, got " + std::to_string(b.size())
  );
  uint112_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint104_t UintConv::bytesToUint104(const bytes::View b) {
  if (b.size() != 13) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 13, got " + std::to_string(b.size())
  );
  uint104_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint96_t UintConv::bytesToUint96(const bytes::View b) {
  if (b.size() != 12) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 12, got " + std::to_string(b.size())
  );
  uint96_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint88_t UintConv::bytesToUint88(const bytes::View b) {
  if (b.size() != 11) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 11, got " + std::to_string(b.size())
  );
  uint88_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint80_t UintConv::bytesToUint80(const bytes::View b) {
  if (b.size() != 10) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 10, got " + std::to_string(b.size())
  );
  uint80_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint72_t UintConv::bytesToUint72(const bytes::View b) {
  if (b.size() != 9) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 9, got " + std::to_string(b.size())
  );
  uint72_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint56_t UintConv::bytesToUint56(const bytes::View b) {
  if (b.size() != 7) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 7, got " + std::to_string(b.size())
  );
  uint56_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint48_t UintConv::bytesToUint48(const bytes::View b) {
  if (b.size() != 6) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 6, got " + std::to_string(b.size())
  );
  uint48_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint40_t UintConv::bytesToUint40(const bytes::View b) {
  if (b.size() != 5) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 5, got " + std::to_string(b.size())
  );
  uint40_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint24_t UintConv::bytesToUint24(const bytes::View b) {
  if (b.size() != 3) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 3, got " + std::to_string(b.size())
  );
  uint24_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint64_t UintConv::bytesToUint64(const bytes::View b) {
  if (b.size() != 8) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 8, got " + std::to_string(b.size())
  );
  uint64_t ret = 0;
  std::memcpy(&ret, b.data(), 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(ret);
  #endif
  return ret;
}

uint32_t UintConv::bytesToUint32(const bytes::View b) {
  if (b.size() != 4) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 4, got " + std::to_string(b.size())
  );
  uint32_t ret = 0;
  std::memcpy(&ret, b.data(), 4);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(ret);
  #endif
  return ret;
}

uint16_t UintConv::bytesToUint16(const bytes::View b) {
  if (b.size() != 2) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 2, got " + std::to_string(b.size())
  );
  uint16_t ret = 0;
  std::memcpy(&ret, b.data(), 2);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap16(ret);
  #endif
  return ret;
}

uint8_t UintConv::bytesToUint8(const bytes::View b) {
  if (b.size() != 1) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 1, got " + std::to_string(b.size())
  );
  uint8_t ret;
  ret = b[0];
  return ret;
}

