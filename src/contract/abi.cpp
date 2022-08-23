#include "abi.h"

uint256_t ABIDecoder::decodeUint256(const std::string &data, uint64_t &start) {
  if (start + 32 > data.size()) {
    throw std::runtime_error("Data too short");
  }
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint256_t ret = Utils::bytesToUint256(tmp);
  start += 32;
  return ret;
}

Address ABIDecoder::decodeAddress(const std::string &data, uint64_t &start) {
  if (start + 32 > data.size()) {
    throw std::runtime_error("Data too short");
  }
  std::string tmp;
  std::copy(data.begin() + start + 12, data.begin() + start + 32, std::back_inserter(tmp)); // Skip first 12 bytes
  Address ret(tmp, false);
  start += 32;
  return ret;
}

bool ABIDecoder::decodeBool(const std::string &data, uint64_t &start) {
  bool ret = (data[start + 31] == 0x00) ? false : true; // Bool value is at the very end
  start += 32;
  return ret;
}

std::string ABIDecoder::decodeBytes(const std::string &data, uint64_t &start) {
  // Get bytes offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t bytesStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get bytes length
  tmp.clear();
  std::copy(data.begin() + bytesStart, data.begin() + bytesStart + 32, std::back_inserter(tmp));
  uint64_t bytesLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get bytes data
  tmp.clear();
  std::copy(data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength, std::back_inserter(tmp));
  start += 32;
  return tmp;
}

std::string ABIDecoder::decodeString(const std::string &data, uint64_t &start) {
  // Strings are very similar to bytes, but instead of using string to store bytes, we use string to store a normal string.
  // Get string offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t stringStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get string size
  tmp.clear();
  std::copy(data.begin() + stringStart, data.begin() + stringStart + 32, std::back_inserter(tmp));
  uint64_t stringLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get string data
  tmp.clear();
  std::copy(data.begin() + stringStart + 32, data.begin() + stringStart + 32 + stringLength, std::back_inserter(tmp));
  start += 32;
  return tmp;
}

std::vector<uint256_t> ABIDecoder::decodeUint256Arr(const std::string &data, uint64_t &start) {
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  // TODO: do not rely on boost::lexical_cast?
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

  start += 32;
  return tmpArr;
}

std::vector<Address> ABIDecoder::decodeAddressArr(const std::string &data, uint64_t &start) {
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

  start += 32;
  return tmpArr;
}

std::vector<bool> ABIDecoder::decodeBoolArr(const std::string &data, uint64_t &start) {
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

  start += 32 + 32 + (32 * tmpArr.size()); // offset + length + data -> TODO: is this right?
  return tmpArr;
}

std::vector<std::string> ABIDecoder::decodeBytesArr(const std::string &data, uint64_t &start) {
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

    // Get bytes data
    tmp.clear();
    std::copy(data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength, std::back_inserter(tmp));
    tmpVec.emplace_back(tmp);
  }

  start += 32;
  return tmpVec;
}

std::vector<std::string> ABIDecoder::decodeStringArr(const std::string &data, uint64_t &start) {
  // Same thing as bytes...
  // Get array offset
  std::string tmp;
  std::copy(data.begin() + start, data.begin() + start + 32, std::back_inserter(tmp));
  uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array length
  tmp.clear();
  std::copy(data.begin() + arrayStart, data.begin() + arrayStart + 32, std::back_inserter(tmp));
  uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

  // Get array data
  std::vector<std::string> tmpVec;
  for (uint64_t i = 0; i < arrayLength; ++i) {
    // Get string offset
    tmp.clear();
    std::copy(data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32, std::back_inserter(tmp));
    uint64_t stringStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp)) + arrayStart + 32;

    // Get string length
    tmp.clear();
    std::copy(data.begin() + stringStart, data.begin() + stringStart + 32, std::back_inserter(tmp));
    uint64_t stringLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

    // Get string data
    tmp.clear();
    std::copy(data.begin() + stringStart + 32, data.begin() + stringStart + 32 + stringLength, std::back_inserter(tmp));
    tmpVec.emplace_back(tmp);
  }

  start += 32;
  return tmpVec;
}

// TODO: Proper size checking and error handling.
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
    } else if (types[argsIndex] == ABITypes::bytes) {
      this->data.emplace_back(decodeBytes(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::bytesArr) {
      this->data.emplace_back(decodeBytesArr(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::string) {
      this->data.emplace_back(decodeString(abiData, dataIndex));
    } else if (types[argsIndex] == ABITypes::stringArr) {
      this->data.emplace_back(decodeStringArr(abiData, dataIndex));
    }
    ++argsIndex;
  }
}

