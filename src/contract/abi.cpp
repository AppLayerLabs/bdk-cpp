/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "abi.h"

void ABI::Encoder::appendVector(Bytes &dest, const Bytes &src) {
  dest.insert(dest.end(), src.begin(), src.end());
}

Functor ABI::Encoder::encodeFunction(const std::string_view func) {
  auto view = Utils::create_view_span(func);
  auto hash = Utils::sha3(view);
  return Functor(Bytes(hash.cbegin(), hash.cbegin() +4));
}

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
    for (int i = 0; i < 32; i++) {
      ret[31 - i] = ~((unsigned char*)&valueToEncode)[i];
    }
    for (int i = 31; i >= 0; i--) {
      if (ret[i] != 0xff) {
        ret[i]++;
        break;
      } else {
        ret[i] = 0x00;
      }
    }
  } else {
    Bytes tempBytes;
    boost::multiprecision::export_bits(valueToEncode, std::back_inserter(tempBytes), 8);
    std::copy(tempBytes.rbegin(), tempBytes.rend(), ret.rbegin());
  }
  return ret;
}

Bytes ABI::Encoder::encode(const Address& add) {
  return Utils::padLeftBytes(add.get(), 32);
}

Bytes ABI::Encoder::encode(const bool& b) {
  return Utils::padLeftBytes((b ? Bytes{0x01} : Bytes{0x00}), 32);
}

Bytes ABI::Encoder::encode(const BytesArrView &bytes) {
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

Bytes ABI::Encoder::encode(const std::vector<uint256_t>& numV) {
  Bytes ret;
  Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(numV.size()), 32));
  ret.reserve(ret.size() + (numV.size() * 32));
  for (uint256_t num : numV) Utils::appendBytes(ret, encodeUint(num));
  return ret;
}

Bytes ABI::Encoder::encode(const std::vector<int256_t>& numV) {
  Bytes ret;
  Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(numV.size()), 32));
  ret.reserve(ret.size() + (numV.size() * 32));
  for (int256_t num : numV) Utils::appendBytes(ret, encodeInt(num));
  return ret;
}

Bytes ABI::Encoder::encode(const std::vector<Address>& addV) {
  Bytes ret;
  Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(addV.size()), 32));
  ret.reserve(ret.size() + (addV.size() * 32));
  for (Address add : addV) Utils::appendBytes(ret, encode(add));
  return ret;
}

Bytes ABI::Encoder::encode(const std::vector<bool>& bV) {
  Bytes ret;
  Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(bV.size()), 32));
  ret.reserve(ret.size() + (bV.size() * 32));
  for (bool b : bV) Utils::appendBytes(ret, encode(b));
  return ret;
}

Bytes ABI::Encoder::encode(const std::vector<Bytes>& bytesV) {
    std::vector<BytesArrView> argDataView;
    argDataView.reserve(bytesV.size());
    std::transform(bytesV.begin(), bytesV.end(), std::back_inserter(argDataView), [](const Bytes& b) { return Utils::create_view_span(b); });
    Bytes ret;
    Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(argDataView.size()), 32));
    std::vector<Bytes> bytesStrip, bytesOff, bytesLen, bytesData = {};
    int pads = 0;
    for (int i = 0; i < argDataView.size(); i++) {
        int p = 0;
        Bytes bS(argDataView[i].begin(), argDataView[i].end());
        Bytes bL = Utils::uintToBytes(bS.size()); // Get length first so we can get the right offset
        Bytes bO = Utils::uintToBytes((32 * argDataView.size()) + (32 * i) + (32 * pads)); // (offsets) + (lengths) + (datas)
        do { p += 32; } while (p < bS.size());
        pads += (p / 32);
        Bytes bD = Utils::padRightBytes(bS, p);
        bytesOff.push_back(Utils::padLeftBytes(bO, 32));
        bytesLen.push_back(Utils::padLeftBytes(bL, 32));
        bytesData.push_back(Utils::padRightBytes(bD, 32));
    }
    for (Bytes off : bytesOff) ret.insert(ret.end(), off.begin(), off.end());
    for (int i = 0; i < argDataView.size(); i++) {
        ret.insert(ret.end(), bytesLen[i].begin(), bytesLen[i].end());
        ret.insert(ret.end(), bytesData[i].begin(), bytesData[i].end());
    }
    return ret;
}

Bytes ABI::Encoder::encode(const std::vector<std::string>& strV) {
    std::vector<BytesArrView> argDataView;
    argDataView.reserve(strV.size());
    std::transform(strV.begin(), strV.end(), std::back_inserter(argDataView), [](const std::string& b) { return Utils::create_view_span(b); });
    Bytes ret;
    Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(argDataView.size()), 32));
    std::vector<Bytes> bytesStrip, bytesOff, bytesLen, bytesData = {};
    int pads = 0;
    for (int i = 0; i < argDataView.size(); i++) {
        int p = 0;
        Bytes bS(argDataView[i].begin(), argDataView[i].end());
        Bytes bL = Utils::uintToBytes(bS.size()); // Get length first so we can get the right offset
        Bytes bO = Utils::uintToBytes((32 * argDataView.size()) + (32 * i) + (32 * pads)); // (offsets) + (lengths) + (datas)
        do { p += 32; } while (p < bS.size());
        pads += (p / 32);
        Bytes bD = Utils::padRightBytes(bS, p);
        bytesOff.push_back(Utils::padLeftBytes(bO, 32));
        bytesLen.push_back(Utils::padLeftBytes(bL, 32));
        bytesData.push_back(Utils::padRightBytes(bD, 32));
    }
    for (Bytes off : bytesOff) ret.insert(ret.end(), off.begin(), off.end());
    for (int i = 0; i < argDataView.size(); i++) {
        ret.insert(ret.end(), bytesLen[i].begin(), bytesLen[i].end());
        ret.insert(ret.end(), bytesData[i].begin(), bytesData[i].end());
    }
    return ret;
}


uint256_t ABI::Decoder::decodeUint256(const BytesArrView data, const uint64_t& start) const {
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for uint256");
  return Utils::bytesToUint256(data.subspan(start, 32));
}

int256_t ABI::Decoder::decodeInt256(const BytesArrView data, const uint64_t& start) const {
    if (start + 32 > data.size()) throw std::runtime_error("Data too short for int256");
    return Utils::bytesToInt256(data.subspan(start, 32));
}

Address ABI::Decoder::decodeAddress(const BytesArrView data, const uint64_t& start) const {
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for address");
  return Address(data.subspan(start + 12, 20));
}

bool ABI::Decoder::decodeBool(const BytesArrView data, const uint64_t& start) const {
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for bool");
  return (data[start + 31] == 0x01);  // Bool value ("00"/"01") is at the very end
}

Bytes ABI::Decoder::decodeBytes(const BytesArrView data, const uint64_t& start) const {
  // Get bytes offset
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for bytes");
  Bytes tmp(data.begin() + start, data.begin() + start + 32);
  uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp);

  // Get bytes length
  tmp.clear();
  if (bytesStart + 32 > data.size()) throw std::runtime_error("Data too short for bytes");
  tmp.insert(tmp.end(), data.begin() + bytesStart, data.begin() + bytesStart + 32);
  uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

  // Size sanity check
  if (bytesStart + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short for bytes");

  // Get bytes data
  tmp.clear();
  tmp.insert(tmp.end(), data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength);
  return tmp;
}

std::string ABI::Decoder::decodeString(const BytesArrView data, const uint64_t &start) const {
  // Get bytes offset
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for string");
  std::string tmp(data.begin() + start, data.begin() + start + 32);
  uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp);

  // Get bytes length
  tmp.clear();
  if (bytesStart + 32 > data.size()) throw std::runtime_error("Data too short for string");
  tmp.insert(tmp.end(), data.begin() + bytesStart, data.begin() + bytesStart + 32);
  uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

  // Size sanity check
  if (bytesStart + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short for string");

  // Get bytes data
  tmp.clear();
  tmp.insert(tmp.end(), data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength);
  return tmp;
}


std::vector<uint256_t> ABI::Decoder::decodeUint256Arr(const BytesArrView data, const uint64_t& start) const {
  // Get array offset
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for uint256[]");
  Bytes tmp(data.begin() + start, data.begin() + start + 32);
  uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

  // Get array length
  tmp.clear();

  if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for uint256[]");
  tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
  uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short for uint256[]");

  // Get array data
  std::vector<uint256_t> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) {
    tmp.clear();
    tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32);
    uint256_t value = Utils::bytesToUint256(tmp);
    tmpArr.emplace_back(value);
  }

  return tmpArr;
}

std::vector<int256_t> ABI::Decoder::decodeInt256Arr(const BytesArrView data, const uint64_t& start) const {
  // Get array offset
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for int256[]");
  Bytes tmp(data.begin() + start, data.begin() + start + 32);
  uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

  // Get array length
  tmp.clear();

  if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for int256[]");
  tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
  uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short for int256[]");

  // Get array data
  std::vector<int256_t> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) {
    tmp.clear();
    tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32);
    int256_t value = Utils::bytesToInt256(tmp); // Change this line to use bytesToInt256
    tmpArr.emplace_back(value);
  }

  return tmpArr;
}

std::vector<Address> ABI::Decoder::decodeAddressArr(const BytesArrView data, const uint64_t& start) const {
  // Get array offset
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for address[]");
  Bytes tmp(data.begin() + start, data.begin() + start + 32);
  uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

  // Get array length
  tmp.clear();
  if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for address[]");
  tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
  uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short for address[]");

  // Get array data
  std::vector<Address> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) {
    tmp.clear();
    // Don't forget to skip the first 12 bytes of an address!
    tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32) + 12, data.begin() + arrayStart + 32 + (i * 32) + 32);
    tmpArr.emplace_back(tmp);
  }

  return tmpArr;
}

std::vector<bool> ABI::Decoder::decodeBoolArr(const BytesArrView data, const uint64_t& start) const {
  // Get array offset
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for bool[]");
  Bytes tmp(data.begin() + start, data.begin() + start + 32);
  uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

  // Get array length
  tmp.clear();
  if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for bool[]");
  tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
  uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short for bool[]");

  // Get array data
  std::vector<bool> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) tmpArr.emplace_back(
    (data[arrayStart + 32 + (i * 32) + 31] == 0x01)
  );
  return tmpArr;
}

std::vector<Bytes> ABI::Decoder::decodeBytesArr(const BytesArrView data, const uint64_t& start) const {
  // Get array offset
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for bytes[]");
  Bytes tmp(data.begin() + start, data.begin() + start + 32);
  uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

  // Get array length
  tmp.clear();
  if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for bytes[]");
  tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
  uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

  std::vector<Bytes> tmpVec;
  for (uint64_t i = 0; i < arrayLength; ++i) {
    // Get bytes offset
    tmp.clear();
    tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32);
    uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp) + arrayStart + 32;

    // Get bytes length
    tmp.clear();
    tmp.insert(tmp.end(), data.begin() + bytesStart, data.begin() + bytesStart + 32);
    uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

    // Individual size sanity check
    if (bytesStart + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short for bytes[]");

    // Get bytes data
    tmp.clear();
    tmp.insert(tmp.end(), data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength);
    tmpVec.emplace_back(tmp);
  }

  return tmpVec;
}

std::vector<std::string> ABI::Decoder::decodeStringArr(const BytesArrView data, const uint64_t &start) const {
  // Get array offset
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for string[]");
  std::string tmp(data.begin() + start, data.begin() + start + 32);
  uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

  // Get array length
  tmp.clear();
  if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for string[]");
  tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
  uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

  std::vector<std::string> tmpVec;
  for (uint64_t i = 0; i < arrayLength; ++i) {
    // Get bytes offset
    tmp.clear();
    tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32);
    uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp) + arrayStart + 32;

    // Get bytes length
    tmp.clear();
    tmp.insert(tmp.end(), data.begin() + bytesStart, data.begin() + bytesStart + 32);
    uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

    // Individual size sanity check
    if (bytesStart + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short for string[]");

    // Get bytes data
    tmp.clear();
    tmp.insert(tmp.end(), data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength);
    tmpVec.emplace_back(tmp);
  }

  return tmpVec;
}

ABI::Decoder::Decoder(const std::vector<ABI::Types>& types, const BytesArrView bytes) {
  uint64_t argIdx = 0;
  uint64_t dataIdx = 0;
  while (argIdx < types.size()) {
    if (types[argIdx] == ABI::Types::uint8 || types[argIdx] == ABI::Types::uint16 ||
        types[argIdx] == ABI::Types::uint24 || types[argIdx] == ABI::Types::uint32 ||
        types[argIdx] == ABI::Types::uint40 || types[argIdx] == ABI::Types::uint48 ||
        types[argIdx] == ABI::Types::uint56 || types[argIdx] == ABI::Types::uint64 ||
        types[argIdx] == ABI::Types::uint72 || types[argIdx] == ABI::Types::uint80 ||
        types[argIdx] == ABI::Types::uint88 || types[argIdx] == ABI::Types::uint96 ||
        types[argIdx] == ABI::Types::uint104 || types[argIdx] == ABI::Types::uint112 ||
        types[argIdx] == ABI::Types::uint120 || types[argIdx] == ABI::Types::uint128 ||
        types[argIdx] == ABI::Types::uint136 || types[argIdx] == ABI::Types::uint144 ||
        types[argIdx] == ABI::Types::uint152 || types[argIdx] == ABI::Types::uint160 ||
        types[argIdx] == ABI::Types::uint168 || types[argIdx] == ABI::Types::uint176 ||
        types[argIdx] == ABI::Types::uint184 || types[argIdx] == ABI::Types::uint192 ||
        types[argIdx] == ABI::Types::uint200 || types[argIdx] == ABI::Types::uint208 ||
        types[argIdx] == ABI::Types::uint216 || types[argIdx] == ABI::Types::uint224 ||
        types[argIdx] == ABI::Types::uint232 || types[argIdx] == ABI::Types::uint240 ||
        types[argIdx] == ABI::Types::uint248 || types[argIdx] == ABI::Types::uint256) {
      this->data_.emplace_back(decodeUint256(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::int8 || types[argIdx] == ABI::Types::int16 ||
               types[argIdx] == ABI::Types::int24 || types[argIdx] == ABI::Types::int32 ||
               types[argIdx] == ABI::Types::int40 || types[argIdx] == ABI::Types::int48 ||
               types[argIdx] == ABI::Types::int56 || types[argIdx] == ABI::Types::int64 ||
               types[argIdx] == ABI::Types::int72 || types[argIdx] == ABI::Types::int80 ||
               types[argIdx] == ABI::Types::int88 || types[argIdx] == ABI::Types::int96 ||
               types[argIdx] == ABI::Types::int104 || types[argIdx] == ABI::Types::int112 ||
               types[argIdx] == ABI::Types::int120 || types[argIdx] == ABI::Types::int128 ||
               types[argIdx] == ABI::Types::int136 || types[argIdx] == ABI::Types::int144 ||
               types[argIdx] == ABI::Types::int152 || types[argIdx] == ABI::Types::int160 ||
               types[argIdx] == ABI::Types::int168 || types[argIdx] == ABI::Types::int176 ||
               types[argIdx] == ABI::Types::int184 || types[argIdx] == ABI::Types::int192 ||
               types[argIdx] == ABI::Types::int200 || types[argIdx] == ABI::Types::int208 ||
               types[argIdx] == ABI::Types::int216 || types[argIdx] == ABI::Types::int224 ||
               types[argIdx] == ABI::Types::int232 || types[argIdx] == ABI::Types::int240 ||
               types[argIdx] == ABI::Types::int248 || types[argIdx] == ABI::Types::int256) {
      this->data_.emplace_back(decodeInt256(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::uint8Arr || types[argIdx] == ABI::Types::uint16Arr ||
               types[argIdx] == ABI::Types::uint24Arr || types[argIdx] == ABI::Types::uint32Arr ||
               types[argIdx] == ABI::Types::uint40Arr || types[argIdx] == ABI::Types::uint48Arr ||
               types[argIdx] == ABI::Types::uint56Arr || types[argIdx] == ABI::Types::uint64Arr ||
               types[argIdx] == ABI::Types::uint72Arr || types[argIdx] == ABI::Types::uint80Arr ||
               types[argIdx] == ABI::Types::uint88Arr || types[argIdx] == ABI::Types::uint96Arr ||
               types[argIdx] == ABI::Types::uint104Arr || types[argIdx] == ABI::Types::uint112Arr ||
               types[argIdx] == ABI::Types::uint120Arr || types[argIdx] == ABI::Types::uint128Arr ||
               types[argIdx] == ABI::Types::uint136Arr || types[argIdx] == ABI::Types::uint144Arr ||
               types[argIdx] == ABI::Types::uint152Arr || types[argIdx] == ABI::Types::uint160Arr ||
               types[argIdx] == ABI::Types::uint168Arr || types[argIdx] == ABI::Types::uint176Arr ||
               types[argIdx] == ABI::Types::uint184Arr || types[argIdx] == ABI::Types::uint192Arr ||
               types[argIdx] == ABI::Types::uint200Arr || types[argIdx] == ABI::Types::uint208Arr ||
               types[argIdx] == ABI::Types::uint216Arr || types[argIdx] == ABI::Types::uint224Arr ||
               types[argIdx] == ABI::Types::uint232Arr || types[argIdx] == ABI::Types::uint240Arr ||
               types[argIdx] == ABI::Types::uint248Arr || types[argIdx] == ABI::Types::uint256Arr) {
      this->data_.emplace_back(decodeUint256Arr(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::int8Arr || types[argIdx] == ABI::Types::int16Arr ||
               types[argIdx] == ABI::Types::int24Arr || types[argIdx] == ABI::Types::int32Arr ||
               types[argIdx] == ABI::Types::int40Arr || types[argIdx] == ABI::Types::int48Arr ||
               types[argIdx] == ABI::Types::int56Arr || types[argIdx] == ABI::Types::int64Arr ||
               types[argIdx] == ABI::Types::int72Arr || types[argIdx] == ABI::Types::int80Arr ||
               types[argIdx] == ABI::Types::int88Arr || types[argIdx] == ABI::Types::int96Arr ||
               types[argIdx] == ABI::Types::int104Arr || types[argIdx] == ABI::Types::int112Arr ||
               types[argIdx] == ABI::Types::int120Arr || types[argIdx] == ABI::Types::int128Arr ||
               types[argIdx] == ABI::Types::int136Arr || types[argIdx] == ABI::Types::int144Arr ||
               types[argIdx] == ABI::Types::int152Arr || types[argIdx] == ABI::Types::int160Arr ||
               types[argIdx] == ABI::Types::int168Arr || types[argIdx] == ABI::Types::int176Arr ||
               types[argIdx] == ABI::Types::int184Arr || types[argIdx] == ABI::Types::int192Arr ||
               types[argIdx] == ABI::Types::int200Arr || types[argIdx] == ABI::Types::int208Arr ||
               types[argIdx] == ABI::Types::int216Arr || types[argIdx] == ABI::Types::int224Arr ||
               types[argIdx] == ABI::Types::int232Arr || types[argIdx] == ABI::Types::int240Arr ||
               types[argIdx] == ABI::Types::int248Arr || types[argIdx] == ABI::Types::int256Arr) {
      this->data_.emplace_back(decodeInt256Arr(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::address) {
      this->data_.emplace_back(decodeAddress(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::addressArr) {
      this->data_.emplace_back(decodeAddressArr(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::boolean) {
      this->data_.emplace_back(decodeBool(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::booleanArr) {
      this->data_.emplace_back(decodeBoolArr(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::bytes) {
      this->data_.emplace_back(decodeBytes(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::string) {
      this->data_.emplace_back(decodeString(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::bytesArr) {
      this->data_.emplace_back(decodeBytesArr(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::stringArr) {
      this->data_.emplace_back(decodeStringArr(bytes, dataIdx));
    } else {
      throw std::runtime_error("Unknown type to decode");
    }
    dataIdx += 32;
    argIdx++;
  }
}

