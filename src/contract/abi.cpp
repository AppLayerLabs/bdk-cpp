/*
Copyright (c) [2023] [Sparq Network]

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

Bytes ABI::Encoder::encode(const Address& add) { return Utils::padLeftBytes(add.get(), 32); }

Bytes ABI::Encoder::encode(const bool& b) { return Utils::padLeftBytes((b ? Bytes{0x01} : Bytes{0x00}), 32); }

Bytes ABI::Encoder::encode(const Bytes &bytes) {
  int pad = 0;
  do { pad += 32; } while (pad < bytes.size());
  Bytes len = Utils::padLeftBytes(Utils::uintToBytes(bytes.size()), 32);
  Bytes data = Utils::padRightBytes(bytes, pad);
  len.reserve(len.size() + data.size());
  len.insert(len.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
  return len;
}

Bytes ABI::Encoder::encode(const std::string& str) {
  BytesArrView bytes = Utils::create_view_span(str);
  int pad = 0;
  do { pad += 32; } while (pad < bytes.size());
  Bytes len = Utils::padLeftBytes(Utils::uintToBytes(bytes.size()), 32);
  Bytes data = Utils::padRightBytes(bytes, pad);
  len.reserve(len.size() + data.size());
  len.insert(len.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
  return len;
}

Bytes ABI::EventEncoder::encodeUint(const uint256_t& num) {
  return Encoder::encodeUint(num);
}

Bytes ABI::EventEncoder::encodeInt(const int256_t& num) {
  return Encoder::encodeInt(num);
}

Bytes ABI::EventEncoder::encode(const Address& add) {
  return Encoder::encode(add);
}

Bytes ABI::EventEncoder::encode(const bool& b) {
  return Encoder::encode(b);
}

Bytes ABI::EventEncoder::encode(const Bytes& bytes) {
  /// Almost the same as ABI::Encoder::encode, but without the padding.
  int pad = 0;
  do { pad += 32; } while (pad < bytes.size());
  return Utils::padRightBytes(bytes, pad);
}

Bytes ABI::EventEncoder::encode(const std::string& str) {
  BytesArrView bytes = Utils::create_view_span(str);
  int pad = 0;
  do { pad += 32; } while (pad < bytes.size());
  return Utils::padRightBytes(bytes, pad);
}

uint256_t ABI::Decoder::decodeUint(const BytesArrView &bytes, uint64_t &index) {
  if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for uint256");
  uint256_t result = Utils::bytesToUint256(bytes.subspan(index, 32));
  index += 32;
  return result;
}

int256_t ABI::Decoder::decodeInt(const BytesArrView& bytes, uint64_t& index) {
  if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for int256");
  int256_t result = Utils::bytesToInt256(bytes.subspan(index, 32));
  index += 32;
  return result;
}

