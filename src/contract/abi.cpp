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

