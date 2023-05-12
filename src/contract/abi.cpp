#include "abi.h"

std::string ABI::Encoder::encodeFunction(std::string_view func) const {
  return Utils::sha3(func).get().substr(0, 4);
}

std::string ABI::Encoder::encodeUint256(const uint256_t num) const {
  return Hash(num).get();
}

std::string ABI::Encoder::encodeAddress(const Address& add) const {
  return Utils::padLeftBytes(add.get(), 32);
}

std::string ABI::Encoder::encodeBool(bool b) const {
  return Utils::padLeftBytes(((b) ? Hex::toBytes("1") : Hex::toBytes("0")), 32);
}

std::string ABI::Encoder::encodeBytes(std::string_view bytes) const {
  int pad = 0;
  do { pad += 32; } while (pad < bytes.length());
  std::string len = Utils::padLeftBytes(Utils::uintToBytes(bytes.length()), 32);
  std::string data = Utils::padRightBytes(std::string(bytes), pad);
  return len + data;
}

std::string ABI::Encoder::encodeUint256Arr(const std::vector<uint256_t>& numV) const {
  std::string arrOff = Utils::padLeft(Hex::fromUint(32).get(), 64);
  std::string arrLen = Utils::padLeft(Hex::fromUint(numV.size()), 64);
  std::string arrData = "";
  for (uint256_t num : numV) arrData += Hex::fromBytes(encodeUint256(num)).get();
  return Hex::toBytes(arrOff + arrLen + arrData);
}

std::string ABI::Encoder::encodeAddressArr(const std::vector<Address>& addV) const {
  std::string arrOff = Utils::padLeft(Hex::fromUint(32).get(), 64);
  std::string arrLen = Utils::padLeft(Hex::fromUint(addV.size()).get(), 64);
  std::string arrData = "";
  for (Address add : addV) arrData += Hex::fromBytes(encodeAddress(add)).get();
  return Hex::toBytes(arrOff + arrLen + arrData);
}

std::string ABI::Encoder::encodeBoolArr(const std::vector<bool>& bV) const {
  std::string arrOff = Utils::padLeft(Hex::fromUint(32).get(), 64);
  std::string arrLen = Utils::padLeft(Hex::fromUint(bV.size()).get(), 64);
  std::string arrData = "";
  for (bool b : bV) arrData += Hex::fromBytes(encodeBool(b)).get();
  return Hex::toBytes(arrOff + arrLen + arrData);
}

std::string ABI::Encoder::encodeBytesArr(const std::vector<std::string>& bytesV) const {
  std::string arrOff = Utils::padLeft(Hex::fromUint(32).get(), 64);
  std::string arrLen = Utils::padLeft(Hex::fromUint(bytesV.size()).get(), 64);
  std::vector<std::string> bytesStrip, bytesOff, bytesLen, bytesData = {};
  int pads = 0;
  for (int i = 0; i < bytesV.size(); i++) {
    std::string bS, bO, bL, bD = "";
    int p = 0;
    bS = (bytesV[i].substr(0, 2) == "0x" || bytesV[i].substr(0, 2) == "0X")
      ? Hex(bytesV[i]).get() : Hex::fromUTF8(bytesV[i]).get();  // Bytes or string
    Utils::toLower(bS);
    if (bS.substr(0, 2) == "0x") bS.erase(0, 2);
    if (bS.length() % 2 != 0) { bS.insert(0, "0"); } // Complete odd bytes ("aaa" = "0aaa")
    bL = Hex::fromUint(bS.length() / 2).get(); // Get length first so we can get the right offset
    bO = Hex::fromUint((32 * bytesV.size()) + (32 * i) + (32 * pads)).get(); // (offsets) + (lengths) + (datas)
    do { p += 64; } while (p < bS.length());
    pads += (p / 64);
    bD = Utils::padRight(bS, p);
    bytesStrip.push_back(Utils::padLeft(bS, 64));
    bytesOff.push_back(Utils::padLeft(bO, 64));
    bytesLen.push_back(Utils::padLeft(bL, 64));
    bytesData.push_back(Utils::padRight(bD, 64));
  }
  std::string ret = arrOff + arrLen;
  for (std::string off : bytesOff) ret += off;
  for (int i = 0; i < bytesV.size(); i++) ret += bytesLen[i] + bytesData[i];
  return Hex::toBytes(ret);
}

ABI::Encoder::Encoder(const ABI::Encoder::EncVar& data, std::string_view func) {
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
    while (e = funcTmp.find(",", b)) {
      std::string_view funcType = funcTmp.substr(b, e - b);
      if (e == std::string::npos) break; // End of args, stop parsing and move on
      if (
        funcType != "uint256" && funcType != "address" &&
        funcType != "bool" && funcType != "bytes" &&
        funcType != "string" && funcType != "uint256[]" &&
        funcType != "address[]" && funcType != "bool[]" &&
        funcType != "bytes[]" && funcType != "string[]"
      ) throw std::runtime_error("Invalid function header type");
      if (
        (funcType == "uint256" && !std::holds_alternative<uint256_t>(data[ct])) ||
        (funcType == "address" && !std::holds_alternative<Address>(data[ct])) ||
        (funcType == "bool" && !std::holds_alternative<bool>(data[ct])) ||
        (funcType == "bytes" && !std::holds_alternative<std::string>(data[ct])) ||
        (funcType == "string" && !std::holds_alternative<std::string>(data[ct])) ||
        (funcType == "uint256[]" && !std::holds_alternative<std::vector<uint256_t>>(data[ct])) ||
        (funcType == "address[]" && !std::holds_alternative<std::vector<Address>>(data[ct])) ||
        (funcType == "bool[]" && !std::holds_alternative<std::vector<bool>>(data[ct])) ||
        (funcType == "bytes[]" && !std::holds_alternative<std::vector<std::string>>(data[ct])) ||
        (funcType == "string[]" && !std::holds_alternative<std::vector<std::string>>(data[ct]))
      ) throw std::runtime_error("Header and data types at position " + std::to_string(ct) + " don't match");
      b = e + 1; ct++; // Skip "," and go to next type
    }
    this->data += encodeFunction(func);
  }

  /**
   * Handle each data type and value, like this:
   * - Split parsing in two: static vars + offsets (appended to this->data),
   * and dynamic vars (appended to dynamicStr)
   * - Consider each 32-byte/64-char space, thus each declared item in the
   * array, as a "line" (for easier understanding)
   * - Count a "global" dynamic offset (nextOffset),
   * starting from (32 bytes * starting number of lines), and
   * adding every (32 bytes * number of lines from current item)
   * for each parsed item
   */
  uint64_t nextOffset = 32 * data.size();
  std::string dynamicStr = "";
  for (auto arg : data) {
    // uint256 (static)
    if (std::holds_alternative<uint256_t>(arg)) {
      this->data += encodeUint256(std::get<uint256_t>(arg));
    // address (static)
    } else if (std::holds_alternative<Address>(arg)) {
      this->data += encodeAddress(std::get<Address>(arg));
    // bool (static)
    } else if (std::holds_alternative<bool>(arg)) {
      this->data += encodeBool(std::get<bool>(arg));
    // bytes/string (dynamic)
    } else if (std::holds_alternative<std::string>(arg)) {
      std::string packed = encodeBytes(std::get<std::string>(arg));
      this->data += Hex::toBytes(Utils::padLeft(Hex::fromUint(nextOffset).get(), 64));
      nextOffset += 32 * (packed.length() / 32); // Both offset and packed in bytes
      dynamicStr += packed;
    // uint256[] (dynamic)
    } else if (std::holds_alternative<std::vector<uint256_t>>(arg)) {
      std::vector<uint256_t> argData = std::get<std::vector<uint256_t>>(arg);
      this->data += Hex::toBytes(Utils::padLeft(Hex::fromUint(nextOffset).get(), 64));
      nextOffset += 32 + (32 * argData.size());  // Length + items, in bytes
      dynamicStr += encodeUint256Arr(argData).substr(32);
    // address[] (dynamic)
    } else if (std::holds_alternative<std::vector<Address>>(arg)) {
      std::vector<Address> argData = std::get<std::vector<Address>>(arg);
      this->data += Hex::toBytes(Utils::padLeft(Hex::fromUint(nextOffset).get(), 64));
      nextOffset += 32 + (32 * argData.size());  // Length + items, in bytes
      dynamicStr += encodeAddressArr(argData).substr(32);
    // bool[] (dynamic)
    } else if (std::holds_alternative<std::vector<bool>>(arg)) {
      std::vector<bool> argData = std::get<std::vector<bool>>(arg);
      this->data += Hex::toBytes(Utils::padLeft(Hex::fromUint(nextOffset).get(), 64));
      nextOffset += 32 + (32 * argData.size());  // Length + items, in bytes
      dynamicStr += encodeBoolArr(argData).substr(32);
    // bytes[]/string[] (dynamic)
    } else if (std::holds_alternative<std::vector<std::string>>(arg)) {
      std::vector<std::string> argData = std::get<std::vector<std::string>>(arg);
      std::string packed = encodeBytesArr(argData).substr(32);
      this->data += Hex::toBytes(Utils::padLeft(Hex::fromUint(nextOffset).get(), 64));
      nextOffset += 32 * (packed.length() / 32); // Both offset and packed in bytes
      dynamicStr += packed;
    }
  }
  this->data += dynamicStr;
}

uint256_t ABI::Decoder::decodeUint256(const std::string_view data, const uint64_t& start) const {
  if (start + 32 > data.size()) throw std::runtime_error("Data too short");
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  return Utils::bytesToUint256(tmp);
}

Address ABI::Decoder::decodeAddress(const std::string_view data, const uint64_t& start) const {
  if (start + 32 > data.size()) throw std::runtime_error("Data too short");
  std::string tmp;
  std::copy(data.begin() + start + 12, data.begin() + start + 32, std::back_inserter(tmp)); // Skip first 12 bytes
  return Address(tmp, true);
}

bool ABI::Decoder::decodeBool(const std::string_view data, const uint64_t& start) const {
  if (start + 32 > data.size()) throw std::runtime_error("Data too short");
  return (data[start + 31] == 0x01);  // Bool value ("00"/"01") is at the very end
}

std::string ABI::Decoder::decodeBytes(const std::string_view data, const uint64_t& start) const {
  // Get bytes offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t bytesStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get bytes length
  tmp.clear();
  std::copy(data.begin() + bytesStart, data.begin() + bytesStart + 32, std::back_inserter(tmp));
  uint64_t bytesLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Size sanity check
  if (start + 32 + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short");

  // Get bytes data
  tmp.clear();
  std::copy(
    data.begin() + bytesStart + 32,
    data.begin() + bytesStart + 32 + bytesLength,
    std::back_inserter(tmp)
  );
  return tmp;
}

std::vector<uint256_t> ABI::Decoder::decodeUint256Arr(const std::string_view data, const uint64_t& start) const {
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array length
  tmp.clear();
  std::copy(data.begin() + arrayStart, data.begin() + arrayStart + 32, std::back_inserter(tmp));
  uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short");

  // Get array data
  std::vector<uint256_t> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) {
    tmp.clear();
    std::copy(
      data.begin() + arrayStart + 32 + (i * 32),
      data.begin() + arrayStart + 32 + (i * 32) + 32,
      std::back_inserter(tmp)
    );
    uint256_t value = Utils::bytesToUint256(tmp);
    tmpArr.emplace_back(value);
  }

  return tmpArr;
}

std::vector<Address> ABI::Decoder::decodeAddressArr(const std::string_view data, const uint64_t& start) const {
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array length
  tmp.clear();
  std::copy(data.begin() + arrayStart, data.begin() + arrayStart + 32, std::back_inserter(tmp));
  uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short");

  // Get array data
  std::vector<Address> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) {
    tmp.clear();
    // Don't forget to skip the first 12 bytes of an address!
    std::copy(
      data.begin() + arrayStart + 32 + (i * 32) + 12,
      data.begin() + arrayStart + 32 + (i * 32) + 32,
      std::back_inserter(tmp)
    );
    Address address(tmp, true); // tmp is in bytes
    tmpArr.emplace_back(address);
  }

  return tmpArr;
}

std::vector<bool> ABI::Decoder::decodeBoolArr(const std::string_view data, const uint64_t& start) const {
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array length
  tmp.clear();
  std::copy(data.begin() + arrayStart, data.begin() + arrayStart + 32, std::back_inserter(tmp));
  uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short");

  // Get array data
  std::vector<bool> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) tmpArr.emplace_back(
    (data[arrayStart + 32 + (i * 32) + 31] == 0x01)
  );
  return tmpArr;
}

std::vector<std::string> ABI::Decoder::decodeBytesArr(const std::string_view data, const uint64_t& start) const {
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array length
  tmp.clear();
  std::copy(data.begin() + arrayStart, data.begin() + arrayStart + 32, std::back_inserter(tmp));
  uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  std::vector<std::string> tmpVec;
  for (uint64_t i = 0; i < arrayLength; ++i) {
    // Get bytes offset
    tmp.clear();
    std::copy(
      data.begin() + arrayStart + 32 + (i * 32),
      data.begin() + arrayStart + 32 + (i * 32) + 32,
      std::back_inserter(tmp)
    );
    uint64_t bytesStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp)) + arrayStart + 32;

    // Get bytes length
    tmp.clear();
    std::copy(
      data.begin() + bytesStart,
      data.begin() + bytesStart + 32,
      std::back_inserter(tmp)
    );
    uint64_t bytesLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

    // Individual size sanity check
    if (bytesStart + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short");

    // Get bytes data
    tmp.clear();
    std::copy(
      data.begin() + bytesStart + 32,
      data.begin() + bytesStart + 32 + bytesLength,
      std::back_inserter(tmp)
    );
    tmpVec.emplace_back(tmp);
  }

  return tmpVec;
}

ABI::Decoder::Decoder(const std::vector<ABI::Types>& types, const std::string_view bytes) {
  uint64_t argIdx = 0;
  uint64_t dataIdx = 0;
  while (argIdx < types.size()) {
    if (types[argIdx] == ABI::Types::uint256) {
      this->data.emplace_back(decodeUint256(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::uint256Arr) {
      this->data.emplace_back(decodeUint256Arr(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::address) {
      this->data.emplace_back(decodeAddress(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::addressArr) {
      this->data.emplace_back(decodeAddressArr(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::boolean) {
      this->data.emplace_back(decodeBool(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::booleanArr) {
      this->data.emplace_back(decodeBoolArr(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::bytes || types[argIdx] == ABI::Types::string) {
      this->data.emplace_back(decodeBytes(bytes, dataIdx));
    } else if (types[argIdx] == ABI::Types::bytesArr || types[argIdx] == ABI::Types::stringArr) {
      this->data.emplace_back(decodeBytesArr(bytes, dataIdx));
    }
    dataIdx += 32;
    argIdx++;
  }
}

