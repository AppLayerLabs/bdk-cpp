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

uint256_t ABI::Decoder::decodeUint(const View<Bytes> &bytes, uint64_t &index) {
  if (index + 32 > bytes.size()) throw std::length_error("Data too short for uint256");
  uint256_t result = UintConv::bytesToUint256(bytes.subspan(index, 32));
  index += 32;
  return result;
}

int256_t ABI::Decoder::decodeInt(const View<Bytes>& bytes, uint64_t& index) {
  if (index + 32 > bytes.size()) throw std::length_error("Data too short for int256");
  int256_t result = IntConv::bytesToInt256(bytes.subspan(index, 32));
  index += 32;
  return result;
}

Bytes ABI::Encoder::encodeError(std::string_view reason) {
  FixedBytes<32> reasonEncoded{};

  const size_t count = std::min(reason.size(), reasonEncoded.size());
  std::copy_n(reason.begin(), count, reasonEncoded.begin());

  const uint256_t size(reason.size());
  const FixedBytes<32> sizeEncoded(Utils::uint256ToBytes(size));

  return Utils::makeBytes(bytes::join(
    Hex::toBytes("0x08c379a0"),
    Hex::toBytes("0x0000000000000000000000000000000000000000000000000000000000000020"),
    sizeEncoded,
    reasonEncoded
  ));
}

std::string ABI::Decoder::decodeError(View<Bytes> data) {
    if (data.size() != 100) {
    throw DynamicException("Encoded revert reason is expected to have exactly 100 bytes");
  }

  const size_t size = Utils::bytesToUint256(data.subspan(36, 32)).convert_to<size_t>();

  std::string res;
  res.reserve(size);
  std::ranges::copy(data.subspan(68, size), std::back_inserter(res));
  return res;
}
