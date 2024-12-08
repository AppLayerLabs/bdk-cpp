/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "intconv.h"

#include "dynamicexception.h"

// ==========================================================================
// INT TO BYTES
// ==========================================================================

BytesArr<32> IntConv::int256ToBytes(const int256_t& i) {
  BytesArr<32> ret;
  if (i < 0) {
    int256_t absValue = -i;
    Bytes tempBytes;
    boost::multiprecision::export_bits(absValue, std::back_inserter(tempBytes), 8);
    for (int j = 0; j < 32; j++) {
      if (j < tempBytes.size()) {
        ret[31 - j] = ~tempBytes[tempBytes.size() - j - 1];
      } else {
        ret[31 - j] = 0xFF;
      }
    }
    for (int j = 31; j >= 0; j--) {
      if (ret[j] != 0xFF) {
        ret[j]++;
        break;
      } else {
        ret[j] = 0x00;
      }
    }
  } else {
    Bytes tempBytes;
    boost::multiprecision::export_bits(i, std::back_inserter(tempBytes), 8);
    std::copy(tempBytes.rbegin(), tempBytes.rend(), ret.rbegin());
  }
  return ret;
}

BytesArr<17> IntConv::int136ToBytes(const int136_t &i) {
  BytesArr<17> ret;
  Bytes tmp;
  tmp.reserve(17);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (signed ii = 0; ii < tmp.size(); ii++) ret[16 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<8> IntConv::int64ToBytes(const int64_t& i) {
  BytesArr<8> ret;
  std::memcpy(&ret[0], &i, 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

// ==========================================================================
// BYTES TO INT
// ==========================================================================

int256_t IntConv::bytesToInt256(const bytes::View b) {
  if (b.size() != 32) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 32, got " + std::to_string(b.size())
  );
  uint256_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);

  // Check the MSB to determine if the value is negative
  // Manually compute two's complement in reverse, since boost::multiprecision::cpp_int doesn't support it
  // Subtract one from the byte array
  if (b[0] & 0x80) {
    int borrow = 1;
    for (int i = 31; i >= 0 && borrow; i--) {
      borrow = (b[i] == 0);
      ret -= (uint256_t(1) << (8 * (31 - i)));
    }
    ret = ~ret;
    return -ret.convert_to<int256_t>();
  } else {
    return ret.convert_to<int256_t>();
  }
}

int136_t IntConv::bytesToInt136(const bytes::View b) {
  if (b.size() != 18) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 18, got " + std::to_string(b.size())
  );
  int136_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

int64_t IntConv::bytesToInt64(const bytes::View b) {
  if (b.size() != 8) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 8, got " + std::to_string(b.size())
  );
  int64_t ret = 0;
  std::memcpy(&ret, b.data(), 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(ret);
  #endif
  return ret;
}

