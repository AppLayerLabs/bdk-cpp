/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "abi.h"

Bytes ABI::Encoder::encodeUint(const uint256_t& num) {
  Bytes ret(32, 0x00);
  Bytes tmp;
  tmp.reserve(32);
  boost::multiprecision::export_bits(num, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[31 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

Bytes ABI::Encoder::encodeInt(const int256_t& num) {
  Bytes ret(32, num < 0 ? 0xff : 0x00);
  int256_t valueToEncode = num;
  if (num < 0) {
    valueToEncode = -num;
    for (int i = 0; i < 32; i++) ret[31 - i] = ~((unsigned char*)&valueToEncode)[i];
    for (int i = 31; i >= 0; i--) { if (ret[i] != 0xff) { ret[i]++; break; } else ret[i] = 0x00; }
  } else {
    Bytes tempBytes;
    boost::multiprecision::export_bits(valueToEncode, std::back_inserter(tempBytes), 8);
    std::copy(tempBytes.rbegin(), tempBytes.rend(), ret.rbegin());
  }
  return ret;
}

uint256_t ABI::Decoder::decodeUint(const bytes::View &bytes, uint64_t &index) {
  if (index + 32 > bytes.size()) throw std::length_error("Data too short for uint256");
  uint256_t result = Utils::bytesToUint256(bytes.subspan(index, 32));
  index += 32;
  return result;
}

int256_t ABI::Decoder::decodeInt(const bytes::View& bytes, uint64_t& index) {
  if (index + 32 > bytes.size()) throw std::length_error("Data too short for int256");
  int256_t result = Utils::bytesToInt256(bytes.subspan(index, 32));
  index += 32;
  return result;
}

