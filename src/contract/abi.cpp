#include "abi.h"

std::string ABIEncoder::encodeFunction(std::string func) {
  std::string hash = "";
  Utils::sha3(func, hash);
  return hash.substr(0, 8);
}

std::string ABIEncoder::encodeUint256(uint256_t num) {
  std::stringstream ss;
  ss << std::hex << num;
  return Utils::padLeft(ss.str(), 64);
}

std::string ABIEncoder::encodeAddress(Address add) {
  std::string addStr = add.get();
  Utils::patchHex(addStr);
  return Utils::padLeft(addStr, 64);
}

std::string ABIEncoder::encodeBool(bool b) {
  return Utils::padLeft(((b) ? "1" : "0"), 64);
}

std::string ABIEncoder::encodeBytes(std::string bytes) {
  std::string strip, off, len, data = "";
  int pad = 0;
  std::stringstream offSS, lenSS;
  strip = bytes;
  Utils::patchHex(strip);
  offSS << std::hex << 32;
  lenSS << std::hex << (strip.length() / 2);
  off = Utils::padLeft(offSS.str(), 64);
  len = Utils::padLeft(lenSS.str(), 64);
  do { pad += 64; } while (pad < strip.length());
  data = Utils::padRight(strip, pad);
  return off + len + data;
}

std::string ABIEncoder::encodeUint256Arr(std::vector<uint256_t> numV) {
  std::string arrOff, arrLen, arrData = "";
  std::stringstream offSS, lenSS;
  offSS << std::hex << 32;
  lenSS << std::hex << numV.size();
  arrOff = Utils::padLeft(offSS.str(), 64);
  arrLen = Utils::padLeft(lenSS.str(), 64);
  for (uint256_t num : numV) { arrData += encodeUint256(num); }
  return arrOff + arrLen + arrData;
}

std::string ABIEncoder::encodeAddressArr(std::vector<Address> addV) {
  std::string arrOff, arrLen, arrData = "";
  std::stringstream offSS, lenSS;
  offSS << std::hex << 32;
  lenSS << std::hex << addV.size();
  arrOff = Utils::padLeft(offSS.str(), 64);
  arrLen = Utils::padLeft(lenSS.str(), 64);
  for (Address add : addV) { arrData += encodeAddress(add); }
  return arrOff + arrLen + arrData;
}

std::string ABIEncoder::encodeBoolArr(std::vector<bool> bV) {
  std::string arrOff, arrLen, arrData = "";
  std::stringstream offSS, lenSS;
  offSS << std::hex << 32;
  lenSS << std::hex << bV.size();
  arrOff = Utils::padLeft(offSS.str(), 64);
  arrLen = Utils::padLeft(lenSS.str(), 64);
  for (bool b : bV) { arrData += encodeBool(b); }
  return arrOff + arrLen + arrData;
}

std::string ABIEncoder::encodeBytesArr(std::vector<std::string> bytesV) {
  std::string arrOff, arrLen = "";
  std::vector<std::string> bytesStrip, bytesOff, bytesLen, bytesData = {};
  std::stringstream offSS, lenSS;
  offSS << std::hex << 32;
  lenSS << std::hex << bytesV.size();
  arrOff = Utils::padLeft(offSS.str(), 64);
  arrLen = Utils::padLeft(lenSS.str(), 64);
  int pads = 0;
  for (int i = 0; i < bytesV.size(); i++) {
    std::string bS, bO, bL, bD = "";
    std::stringstream bOss, bLss;
    int p = 0;
    bS = bytesV[i];
    Utils::patchHex(bS);
    if (bS.length() % 2 != 0) { bS.insert(0, "0"); } // Complete odd bytes ("aaa" = "0aaa")
    bLss << std::hex << (bS.length() / 2); // Get length first so we can get the right offset
    bOss << std::hex << (32 * bytesV.size()) + (32 * i) + (32 * pads);  // (offsets) + (lengths) + (datas)
    bL = bLss.str();
    bO = bOss.str();
    do { p += 64; } while (p < bS.length());
    pads += (p / 64);
    bD = Utils::padRight(bS, p);
    bytesStrip.push_back(Utils::padLeft(bS, 64));
    bytesOff.push_back(Utils::padLeft(bO, 64));
    bytesLen.push_back(Utils::padLeft(bL, 64));
    bytesData.push_back(Utils::padRight(bD, 64));
  }
  std::string ret = arrOff + arrLen;
  for (std::string off : bytesOff) { ret += off; }
  for (int i = 0; i < bytesV.size(); i++) {
    ret += bytesLen[i] + bytesData[i];
  }
  return ret;
}

ABIEncoder::ABIEncoder(std::vector<std::variant<
  uint256_t, std::vector<uint256_t>, Address, std::vector<Address>,
  bool, std::vector<bool>, std::string, std::vector<std::string>
>> data, std::string func) {
  // Handle function ID first if it exists.
  // We have to check the existence of "()", every type inside it, *and* if
  // the type positions on both header and data vector are the same
  // (e.g. arg 0 on header is a string, arg 0 on data vector has to be a string too).
  this->data = "0x";
  if (!func.empty()) {
    if (func.find("(") == std::string::npos || func.find(")") == std::string::npos) {
      throw std::runtime_error("Invalid function header");
    }
    std::string funcTmp = func;
    funcTmp.erase(0, func.find("(") + 1);
    funcTmp.replace(func.find(")"), 1, ",");
    int pos, posct = 0;
    while ((pos = funcTmp.find(",")) != std::string::npos) {
      std::string funcType = funcTmp.substr(0, pos);
      if (
        funcType != "uint256" && funcType != "address" &&
        funcType != "bool" && funcType != "bytes" &&
        funcType != "string" && funcType != "uint256[]" &&
        funcType != "address[]" && funcType != "bool[]" &&
        funcType != "bytes[]" && funcType != "string[]"
      ) {
        throw std::runtime_error("Invalid function header type");
      }
      if (
        (funcType == "uint256" && !std::holds_alternative<uint256_t>(data[posct])) ||
        (funcType == "address" && !std::holds_alternative<Address>(data[posct])) ||
        (funcType == "bool" && !std::holds_alternative<bool>(data[posct])) ||
        (funcType == "bytes" && !std::holds_alternative<std::string>(data[posct])) ||
        (funcType == "string" && !std::holds_alternative<std::string>(data[posct])) ||
        (funcType == "uint256[]" && !std::holds_alternative<std::vector<uint256_t>>(data[posct])) ||
        (funcType == "address[]" && !std::holds_alternative<std::vector<Address>>(data[posct])) ||
        (funcType == "bool[]" && !std::holds_alternative<std::vector<bool>>(data[posct])) ||
        (funcType == "bytes[]" && !std::holds_alternative<std::vector<std::string>>(data[posct])) ||
        (funcType == "string[]" && !std::holds_alternative<std::vector<std::string>>(data[posct]))
      ) {
        throw std::runtime_error("Header and data types at position " + std::to_string(posct) + " don't match");
      }
      posct++;
      funcTmp.erase(0, pos + 1);
    }
    this->data += encodeFunction(func);
  }

  // Handle each data type and value
  uint64_t nextOffset = 32 * data.size();
  std::string arrToAppend = "";
  for (auto arg : data) {
    if (std::holds_alternative<uint256_t>(arg)) {
      this->data += encodeUint256(std::get<uint256_t>(arg));
    } else if (std::holds_alternative<Address>(arg)) {
      this->data += encodeAddress(std::get<Address>(arg));
    } else if (std::holds_alternative<bool>(arg)) {
      this->data += encodeBool(std::get<bool>(arg));
    } else if (std::holds_alternative<std::string>(arg)) {
      std::stringstream offSS;
      offSS << std::hex << nextOffset;
      this->data += Utils::padLeft(offSS.str(), 64);
      std::string packed = encodeBytes(std::get<std::string>(arg));
      nextOffset += 32 * (packed.length() / 64); // Offset in bytes, packed in chars
      arrToAppend += packed;
    } else if (std::holds_alternative<std::vector<uint256_t>>(arg)) {
      std::vector<uint256_t> argData = std::get<std::vector<uint256_t>>(arg);
      std::stringstream offSS;
      offSS << std::hex << nextOffset;
      this->data += Utils::padLeft(offSS.str(), 64);
      nextOffset += 64 * argData.size();
      arrToAppend += encodeUint256Arr(argData).substr(64);
    } else if (std::holds_alternative<std::vector<Address>>(arg)) {
      std::vector<Address> argData = std::get<std::vector<Address>>(arg);
      std::stringstream offSS;
      offSS << std::hex << nextOffset;
      this->data += Utils::padLeft(offSS.str(), 64);
      nextOffset += 64 * argData.size();
      arrToAppend += encodeAddressArr(argData).substr(64);
    } else if (std::holds_alternative<std::vector<bool>>(arg)) {
      std::vector<bool> argData = std::get<std::vector<bool>>(arg);
      std::stringstream offSS;
      offSS << std::hex << nextOffset;
      this->data += Utils::padLeft(offSS.str(), 64);
      nextOffset += 64 * argData.size();
      arrToAppend += encodeBoolArr(argData).substr(64);
    } else if (std::holds_alternative<std::vector<std::string>>(arg)) {
      std::stringstream offSS;
      offSS << std::hex << nextOffset;
      this->data += Utils::padLeft(offSS.str(), 64);
      std::string packed = encodeBytesArr(std::get<std::vector<std::string>>(arg)).substr(64);
      nextOffset += 32 * (packed.length() / 64); // Offset in bytes, packed in chars
      arrToAppend += packed;
    }
  }
  this->data += arrToAppend;
}

uint256_t ABIDecoder::decodeUint256(const std::string &data, const uint64_t &start) {
  if (start + 32 > data.size()) {
    throw std::runtime_error("Data too short");
  }
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint256_t ret = Utils::bytesToUint256(tmp);
  return ret;
}

Address ABIDecoder::decodeAddress(const std::string &data, const uint64_t &start) {
  if (start + 32 > data.size()) {
    throw std::runtime_error("Data too short");
  }
  std::string tmp;
  std::copy(data.begin() + start + 12, data.begin() + start + 32, std::back_inserter(tmp)); // Skip first 12 bytes
  Address ret(tmp, false);
  return ret;
}

bool ABIDecoder::decodeBool(const std::string &data, const uint64_t &start) {
  if (start + 32 > data.size()) {
    throw std::runtime_error("Data too short");
  }
  bool ret = (data[start + 31] == 0x00) ? false : true; // Bool value is at the very end
  return ret;
}

std::string ABIDecoder::decodeBytes(const std::string &data, const uint64_t &start) {
  // Get bytes offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t bytesStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get bytes length
  tmp.clear();
  std::copy(data.begin() + bytesStart, data.begin() + bytesStart + 32, std::back_inserter(tmp));
  uint64_t bytesLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Size sanity check
  if (start + 32 + 32 + bytesLength > data.size()) {
    throw std::runtime_error("Data too short");
  }

  // Get bytes data
  tmp.clear();
  std::copy(data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength, std::back_inserter(tmp));
  return tmp;
}

std::vector<uint256_t> ABIDecoder::decodeUint256Arr(const std::string &data, const uint64_t &start) {
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array length
  tmp.clear();
  std::copy(data.begin() + arrayStart, data.begin() + arrayStart + 32, std::back_inserter(tmp));
  uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) {
    throw std::runtime_error("Data too short");
  }

  // Get array data
  std::vector<uint256_t> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) {
    tmp.clear();
    std::copy(data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32, std::back_inserter(tmp));
    uint256_t value = Utils::bytesToUint256(tmp);
    tmpArr.emplace_back(value);
  }

  return tmpArr;
}

std::vector<Address> ABIDecoder::decodeAddressArr(const std::string &data, const uint64_t &start) {
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array length
  tmp.clear();
  std::copy(data.begin() + arrayStart, data.begin() + arrayStart + 32, std::back_inserter(tmp));
  uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) {
    throw std::runtime_error("Data too short");
  }

  // Get array data
  std::vector<Address> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) {
    tmp.clear();
    // Don't forget to skip the first 12 bytes of an address!
    std::copy(data.begin() + arrayStart + 32 + (i * 32) + 12, data.begin() + arrayStart + 32 + (i * 32) + 32, std::back_inserter(tmp));
    Address address(tmp, false);
    tmpArr.emplace_back(address);
  }

  return tmpArr;
}

std::vector<bool> ABIDecoder::decodeBoolArr(const std::string &data, const uint64_t &start) {
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array length
  tmp.clear();
  std::copy(data.begin() + arrayStart, data.begin() + arrayStart + 32, std::back_inserter(tmp));
  uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Size sanity check
  if (arrayStart + 32 + (arrayLength * 32) > data.size()) {
    throw std::runtime_error("Data too short");
  }

  // Get array data
  std::vector<bool> tmpArr;
  for (uint64_t i = 0; i < arrayLength; i++) {
    bool value = (data[arrayStart + 32 + (i * 32) + 31] == 0x00) ? false : true;
    tmpArr.emplace_back(value);
  }

  return tmpArr;
}

std::vector<std::string> ABIDecoder::decodeBytesArr(const std::string &data, const uint64_t &start) {
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
    std::copy(data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32, std::back_inserter(tmp));
    uint64_t bytesStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp)) + arrayStart + 32;

    // Get bytes length
    tmp.clear();
    std::copy(data.begin() + bytesStart, data.begin() + bytesStart + 32, std::back_inserter(tmp));
    uint64_t bytesLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

    // Individual size sanity check
    if (bytesStart + 32 + bytesLength > data.size()) {
      throw std::runtime_error("Data too short");
    }

    // Get bytes data
    tmp.clear();
    std::copy(data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength, std::back_inserter(tmp));
    tmpVec.emplace_back(tmp);
  }

  return tmpVec;
}

ABIDecoder::ABIDecoder(std::vector<ABITypes> const &types, std::string const &abiData) {
  uint64_t argsIndex = 0;
  uint64_t dataIndex = 0;
  while (argsIndex < types.size()) {
    if (types[argsIndex] == ABITypes::uint256) {
      this->data.emplace_back(decodeUint256(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::uint256Arr) {
      this->data.emplace_back(decodeUint256Arr(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::address) {
      this->data.emplace_back(decodeAddress(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::addressArr) {
      this->data.emplace_back(decodeAddressArr(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::boolean) {
      this->data.emplace_back(decodeBool(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::booleanArr) {
      this->data.emplace_back(decodeBoolArr(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::string || types[argsIndex] == ABITypes::bytes) {
      this->data.emplace_back(decodeBytes(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::stringArr || types[argsIndex] == ABITypes::bytesArr) {
      this->data.emplace_back(decodeBytesArr(abiData, dataIndex));
    }
    dataIndex += 32;
    ++argsIndex;
  }
}

