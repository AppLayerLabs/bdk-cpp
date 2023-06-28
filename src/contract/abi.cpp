#include "abi.h"

Functor ABI::Encoder::encodeFunction(const std::string_view func) const {
  auto view = Utils::create_view_span(func);
  auto hash = Utils::sha3(view);
  return Functor(Bytes(hash.cbegin(), hash.cbegin() +4));
}

Bytes ABI::Encoder::encodeUint256(const uint256_t& num) const {
  Bytes ret(32, 0x00);
  Bytes tmp;
  tmp.reserve(32);
  boost::multiprecision::export_bits(num, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[31 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

Bytes ABI::Encoder::encodeAddress(const Address& add) const {
  return Utils::padLeftBytes(add.get(), 32);
}

Bytes ABI::Encoder::encodeBool(bool b) const {
  return Utils::padLeftBytes((b ? std::vector<uint8_t>{0x01} : std::vector<uint8_t>{0x00}), 32);
}

Bytes ABI::Encoder::encodeBytes(const BytesArrView bytes) const {
  int pad = 0;
  do { pad += 32; } while (pad < bytes.size());
  Bytes len = Utils::padLeftBytes(Utils::uintToBytes(bytes.size()), 32);
  Bytes data = Utils::padRightBytes(bytes, pad);
  len.reserve(len.size() + data.size());
  len.insert(len.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
  return len;
}

Bytes ABI::Encoder::encodeUint256Arr(const std::vector<uint256_t>& numV) const {
  Bytes ret;
  Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(numV.size()), 32));
  ret.reserve(ret.size() + (numV.size() * 32));
  for (uint256_t num : numV) Utils::appendBytes(ret, encodeUint256(num));
  return ret;
}

Bytes ABI::Encoder::encodeAddressArr(const std::vector<Address>& addV) const {
  Bytes ret;
  Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(addV.size()), 32));
  ret.reserve(ret.size() + (addV.size() * 32));
  for (Address add : addV) Utils::appendBytes(ret, encodeAddress(add));
  return ret;
}

Bytes ABI::Encoder::encodeBoolArr(const std::vector<bool>& bV) const {
  Bytes ret;
  Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(bV.size()), 32));
  ret.reserve(ret.size() + (bV.size() * 32));
  for (bool b : bV) Utils::appendBytes(ret, encodeBool(b));
  return ret;
}

Bytes ABI::Encoder::encodeBytesArr(const std::vector<BytesArrView>& bytesV) const {
  Bytes ret;
  Utils::appendBytes(ret, Utils::padLeftBytes(Utils::uintToBytes(bytesV.size()), 32));
  std::vector<Bytes> bytesStrip, bytesOff, bytesLen, bytesData = {};
  int pads = 0;
  for (int i = 0; i < bytesV.size(); i++) {
    int p = 0;
    Bytes bS(bytesV[i].begin(), bytesV[i].end());
    Bytes bL = Utils::uintToBytes(bS.size()); // Get length first so we can get the right offset
    Bytes bO = Utils::uintToBytes((32 * bytesV.size()) + (32 * i) + (32 * pads)); // (offsets) + (lengths) + (datas)
    do { p += 32; } while (p < bS.size());
    pads += (p / 32);
    Bytes bD = Utils::padRightBytes(bS, p);
    bytesOff.push_back(Utils::padLeftBytes(bO, 32));
    bytesLen.push_back(Utils::padLeftBytes(bL, 32));
    bytesData.push_back(Utils::padRightBytes(bD, 32));
  }
  for (Bytes off : bytesOff) ret.insert(ret.end(), off.begin(), off.end());
  for (int i = 0; i < bytesV.size(); i++) {
    ret.insert(ret.end(), bytesLen[i].begin(), bytesLen[i].end());
    ret.insert(ret.end(), bytesData[i].begin(), bytesData[i].end());
  }
  return ret;
}

ABI::Encoder::Encoder(const ABI::Encoder::EncVar& data, const std::string_view func) {
  // Handle function ID first if it exists.
  // We have to check the existence of "()", every type inside it,
  // *and* if type positions on both header and data vector are the same
  // (e.g. arg[0] on header is string, arg[0] on data vector has to be string too).
  if (!func.empty()) {
    // Check if header is formatted accordingly
    bool invalidFunc = (func.find("(") == std::string::npos || func.find(")") == std::string::npos);
    if (invalidFunc) throw std::runtime_error("Invalid function header");

    // Check if function has args and data, throw if args are missing but data is not
    std::string_view funcTmp(func.begin() + func.find("(") + 1, func.end() - 1);
    if (funcTmp.empty() && data.size() != 0) throw std::runtime_error("Invalid function header");

    // Parse the function header types and encode the function header
    int b = 0, e = 0, ct = 0; // Arg start, arg end, and arg count
    e = funcTmp.find_first_of(",)", b);
    while (ct < data.size()) {
      std::string_view funcType = funcTmp.substr(b, e - b);
      if (
        funcType != "uint256" && funcType != "address" &&
        funcType != "bool" && funcType != "bytes" &&
        funcType != "string" && funcType != "uint256[]" &&
        funcType != "address[]" && funcType != "bool[]" &&
        funcType != "bytes[]" && funcType != "string[]"
      ) throw std::runtime_error("Invalid function header type: " + std::string(funcType));
      if (
        (funcType == "uint256" && !std::holds_alternative<uint256_t>(data[ct])) ||
        (funcType == "address" && !std::holds_alternative<Address>(data[ct])) ||
        (funcType == "bool" && !std::holds_alternative<bool>(data[ct])) ||
        (funcType == "bytes" && !std::holds_alternative<Bytes>(data[ct])) ||
        (funcType == "string" && !std::holds_alternative<std::string>(data[ct])) ||
        (funcType == "uint256[]" && !std::holds_alternative<std::vector<uint256_t>>(data[ct])) ||
        (funcType == "address[]" && !std::holds_alternative<std::vector<Address>>(data[ct])) ||
        (funcType == "bool[]" && !std::holds_alternative<std::vector<bool>>(data[ct])) ||
        (funcType == "bytes[]" && !std::holds_alternative<std::vector<Bytes>>(data[ct])) ||
        (funcType == "string[]" && !std::holds_alternative<std::vector<std::string>>(data[ct]))
      ) throw std::runtime_error("Header and data types at position " + std::to_string(ct) + " don't match");
      b = e + 1; ct++; // Skip "," and go to next types
      e = funcTmp.find_first_of(",", b);
    }
    this->functor = encodeFunction(func);
  }

  /**
   * Handle each data type and value, like this:
   * - Split parsing in two: static vars + offsets (appended to this->data_),
   * and dynamic vars (appended to dynamicStr)
   * - Consider each 32-byte space, thus each declared item in the
   * array, as a "line" (for easier understanding)
   * - Count a "global" dynamic offset (nextOffset),
   * starting from (32 bytes * starting number of lines), and
   * adding every (32 bytes * number of lines from current item)
   * for each parsed item
   * - Offsets, lengths and data are all in bytes.
   */
  uint64_t nextOffset = 32 * data.size();
  Bytes dynamicBytes;
  for (auto arg : data) {
    // uint256 (static)
    if (std::holds_alternative<uint256_t>(arg)) {
      Utils::appendBytes(this->data_, encodeUint256(std::get<uint256_t>(arg)));
    // address (static)
    } else if (std::holds_alternative<Address>(arg)) {
      Utils::appendBytes(this->data_, encodeAddress(std::get<Address>(arg)));
    // bool (static)
    } else if (std::holds_alternative<bool>(arg)) {
      Utils::appendBytes(this->data_, encodeBool(std::get<bool>(arg)));
    // string (dynamic)
    } else if (std::holds_alternative<std::string>(arg)) {
      const Bytes &packed = encodeBytes(Utils::create_view_span(std::get<std::string>(arg)));
      Utils::appendBytes(this->data_, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
      nextOffset += 32 * (packed.size() / 32); // Both offset and packed in bytes
      dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
      // bytes (dynamic)
    } else if (std::holds_alternative<Bytes>(arg)) {
      const Bytes &packed = encodeBytes(std::get<Bytes>(arg));
      Utils::appendBytes(this->data_, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
      nextOffset += 32 * (packed.size() / 32); // Both offset and packed in bytes
      dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
    // uint256[] (dynamic)
      } else if (std::holds_alternative<std::vector<uint256_t>>(arg)) {
      const std::vector<uint256_t>& argData = std::get<std::vector<uint256_t>>(arg);
      Utils::appendBytes(this->data_, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
      nextOffset += 32 + (32 * argData.size());  // Length + items, in bytes
      Utils::appendBytes(dynamicBytes, encodeUint256Arr(argData));
    // address[] (dynamic)
    } else if (std::holds_alternative<std::vector<Address>>(arg)) {
      const std::vector<Address>& argData = std::get<std::vector<Address>>(arg);
      Utils::appendBytes(this->data_, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
      nextOffset += 32 + (32 * argData.size());  // Length + items, in bytes
      Utils::appendBytes(dynamicBytes, encodeAddressArr(argData));
    // bool[] (dynamic)
    } else if (std::holds_alternative<std::vector<bool>>(arg)) {
      const std::vector<bool> &argData = std::get<std::vector<bool>>(arg);
      Utils::appendBytes(this->data_, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
      nextOffset += 32 + (32 * argData.size());  // Length + items, in bytes
      Utils::appendBytes(dynamicBytes, encodeBoolArr(argData));
      // bytes[] (dynamic)
    } else if (std::holds_alternative<std::vector<Bytes>>(arg)) {
      const std::vector<Bytes>& argData = std::get<std::vector<Bytes>>(arg);
      std::vector<BytesArrView> argDataView;
      argDataView.reserve(argData.size());
      std::transform(argData.begin(), argData.end(), std::back_inserter(argDataView), [](const Bytes& b) { return Utils::create_view_span(b); });
      Bytes packed = encodeBytesArr(argDataView);
      Utils::appendBytes(this->data_, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
      nextOffset += 32 * (packed.size() / 32); // Both offset and packed in bytes
      dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
      // string[] (dynamic)
    } else if (std::holds_alternative<std::vector<std::string>>(arg)) {
      const std::vector<std::string>& argData = std::get<std::vector<std::string>>(arg);
      std::vector<BytesArrView> argDataView;
      argDataView.reserve(argData.size());
      std::transform(argData.begin(), argData.end(), std::back_inserter(argDataView), [](const std::string& b) { return Utils::create_view_span(b); });
      Bytes packed = encodeBytesArr(argDataView);
      Utils::appendBytes(this->data_, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
      nextOffset += 32 * (packed.size() / 32); // Both offset and packed in bytes
      dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
    }
  }
  this->data_.insert(this->data_.end(), dynamicBytes.begin(), dynamicBytes.end());
}

uint256_t ABI::Decoder::decodeUint256(const BytesArrView data, const uint64_t& start) const {
  if (start + 32 > data.size()) throw std::runtime_error("Data too short for uint256");
  return Utils::bytesToUint256(data.subspan(start, 32));
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
    if (types[argIdx] == ABI::Types::uint256 || types[argIdx] == ABI::Types::uint8 ||
        types[argIdx] == ABI::Types::uint16 || types[argIdx] == ABI::Types::uint32 ||
        types[argIdx] == ABI::Types::uint64) {
      this->data_.emplace_back(decodeUint256(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::uint256Arr) {
      this->data_.emplace_back(decodeUint256Arr(bytes, dataIdx));
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

