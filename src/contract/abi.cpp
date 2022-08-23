#include "abi.h"

// TODO: Proper size checking and error handling.
ABIDecoder::ABIDecoder(std::vector<ABITypes> const &types, std::string const &abiData) {

  uint64_t argsIndex = 0;
  uint64_t dataIndex = 0;
  while (argsIndex < types.size()) {
    // Check data type.
    if (types[argsIndex] == ABITypes::uint256) {
      if (dataIndex + 32 > abiData.size()) {
        throw std::runtime_error("Data too short");
      }
      std::string tmp;
      std::copy(abiData.begin() + dataIndex, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      dataIndex += 32;
      uint256_t value = Utils::bytesToUint256(tmp);
      this->data.emplace_back(value);
    } else if (types[argsIndex] == ABITypes::uint256Arr) {
      // Get array start
      std::string tmp;
      std::copy(abiData.begin() + dataIndex, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      // TODO: do not rely on boost::lexical_cast?
      uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));
      // Get array lenght
      tmp.clear();
      std::copy(abiData.begin() + arrayStart, abiData.begin() + arrayStart + 32, std::back_inserter(tmp));
      uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Size sanity check.
      if (arrayStart + 32 + (arrayLength*32) > abiData.size()) {
        throw std::runtime_error("Data too short");
      }

      // Get array data.
      std::vector<uint256_t> tmpArr;
      for (uint64_t i = 0; i < arrayLength; i++) {
        tmp.clear();
        std::copy(abiData.begin() + arrayStart + 32 + (i*32), abiData.begin() + arrayStart + 32 + (i*32) + 32, std::back_inserter(tmp));
        uint256_t value = Utils::bytesToUint256(tmp);
        tmpArr.emplace_back(value);
      }
      this->data.emplace_back(tmpArr);
      dataIndex = dataIndex + 32;
    } else if (types[argsIndex] == ABITypes::address) {
      if (dataIndex + 32 > abiData.size()) {
        throw std::runtime_error("Data too short");
      }
      
      std::string tmp;
      // Skip first 12 bytes.
      std::copy(abiData.begin() + dataIndex + 12, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      Address address(tmp, false);
      this->data.emplace_back(address);
      dataIndex = dataIndex + 32;
    } else if (types[argsIndex] == ABITypes::addressArr) {
      // Get array start
      std::string tmp;
      std::copy(abiData.begin() + dataIndex, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Get array lenght
      tmp.clear();
      std::copy(abiData.begin() + arrayStart, abiData.begin() + arrayStart + 32, std::back_inserter(tmp));
      uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Size sanity check.
      if (arrayStart + 32 + (arrayLength*32) > abiData.size()) {
        throw std::runtime_error("Data too short");
      }

      // Get array data.
      std::vector<Address> tmpArr;
      for (uint64_t i = 0; i < arrayLength; i++) {
        tmp.clear();
        // Don't forget to skip the first 12 bytes of an address!
        std::copy(abiData.begin() + arrayStart + 32 + (i*32) + 12, abiData.begin() + arrayStart + 32 + (i*32) + 32, std::back_inserter(tmp));
        Address address(tmp, false);
        tmpArr.emplace_back(address);
      }
      this->data.emplace_back(tmpArr);
      dataIndex = dataIndex + 32;
    } else if (types[argsIndex] == ABITypes::boolean) {
      // Get boolean value from hex, located at end of 
      bool value = (abiData[dataIndex+31] == 0x00) ? false : true;
      this->data.emplace_back(value);
      dataIndex += dataIndex + 32;
    } else if (types[argsIndex] == ABITypes::booleanArr) {
      // Get array start.
      std::string tmp;
      std::copy(abiData.begin() + dataIndex, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Get array lenght.
      tmp.clear();
      std::copy(abiData.begin() + arrayStart, abiData.begin() + arrayStart + 32, std::back_inserter(tmp));
      uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Size sanity check.
      if (arrayStart + 32 + (arrayLength*32) > abiData.size()) {
        throw std::runtime_error("Data too short");
      }

      // Get array data.
      std::vector<bool> tmpArr;
      for (uint64_t i = 0; i < arrayLength; i++) {
        bool value = (abiData[arrayStart + 32 + (i*32) + 31] == 0x00) ? false : true;
        tmpArr.emplace_back(value);
      }
      this->data.emplace_back(tmpArr);
      dataIndex = dataIndex + 32;
    } else if (types[argsIndex] == ABITypes::bytes) {
      // Get bytes start in abi.
      std::string tmp;
      std::copy(abiData.begin() + dataIndex, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      uint64_t bytesStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Get bytes length.
      tmp.clear();
      std::copy(abiData.begin() + bytesStart, abiData.begin() + bytesStart + 32, std::back_inserter(tmp));
      uint64_t bytesLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Get bytes...
      tmp.clear();
      std::copy(abiData.begin() + bytesStart + 32, abiData.begin() + bytesStart + 32 + bytesLength, std::back_inserter(tmp));
      this->data.emplace_back(tmp);
      dataIndex = dataIndex + 32;
    } else if (types[argsIndex] == ABITypes::bytesArr) {
      // Get array start in the ABI.
      std::string tmp;
      std::copy(abiData.begin() + dataIndex, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Get array length.
      tmp.clear();
      std::copy(abiData.begin() + arrayStart, abiData.begin() + arrayStart + 32, std::back_inserter(tmp));
      uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      std::vector<std::string> tmpVec;
      for (uint64_t i = 0; i < arrayLength; ++i) {
        // Read byte location on ABI.
        tmp.clear();
        std::copy(abiData.begin() + arrayStart + 32 +(i*32), abiData.begin() + arrayStart + 32 + (i*32) + 32, std::back_inserter(tmp));
        uint64_t byteStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp)) + arrayStart + 32;

        // Read byte length.
        tmp.clear();
        std::copy(abiData.begin() + byteStart, abiData.begin() + byteStart + 32, std::back_inserter(tmp));
        uint64_t byteLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

        // Read byte data.
        tmp.clear();
        std::copy(abiData.begin() + byteStart + 32, abiData.begin() + byteStart + 32 + byteLength, std::back_inserter(tmp));
        tmpVec.emplace_back(tmp);
      }
      this->data.emplace_back(tmpVec);
      dataIndex = dataIndex + 32;
    } else if (types[argsIndex] == ABITypes::string) {
      // Strings are very similar to bytes. but instead instead using string to store bytes, we use string to store a normal string.
      // Get string start in abi.
      std::string tmp;
      std::copy(abiData.begin() + dataIndex, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      uint64_t stringStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Get string size.
      tmp.clear();
      std::copy(abiData.begin() + stringStart, abiData.begin() + stringStart + 32, std::back_inserter(tmp));
      uint64_t stringLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Get string data.
      tmp.clear();
      std::copy(abiData.begin() + stringStart + 32, abiData.begin() + stringStart + 32 + stringLength, std::back_inserter(tmp));
      this->data.emplace_back(tmp);
    } else if (types[argsIndex] == ABITypes::stringArr) {
      // Same thing as bytes...
      // Get string start in ABI.
      std::string tmp;
      std::copy(abiData.begin() + dataIndex, abiData.begin() + dataIndex + 32, std::back_inserter(tmp));
      uint64_t arrayStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // Get string length.
      tmp.clear();
      std::copy(abiData.begin() + arrayStart, abiData.begin() + arrayStart + 32, std::back_inserter(tmp));
      uint64_t arrayLength = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

      // get string data.
      std::vector<std::string> tmpVec;
      for (uint64_t i = 0; i < arrayLength; ++i) {
        // Read byte location on ABI.
        tmp.clear();
        std::copy(abiData.begin() + arrayStart + 32 +(i*32), abiData.begin() + arrayStart + 32 + (i*32) + 32, std::back_inserter(tmp));
        uint64_t stringStart = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp)) + arrayStart + 32;

        // Read byte length.
        tmp.clear();
        std::copy(abiData.begin() + stringStart, abiData.begin() + stringStart + 32, std::back_inserter(tmp));
        uint64_t stringLenght = boost::lexical_cast<uint64_t>(Utils::bytesToUint256(tmp));

        // Read byte data.
        tmp.clear();
        std::copy(abiData.begin() + stringStart + 32, abiData.begin() + stringStart + 32 + stringLenght, std::back_inserter(tmp));

        tmpVec.emplace_back(tmp);
      }

      this->data.emplace_back(tmpVec);
      dataIndex = dataIndex + 32;
    }

    ++argsIndex;
  }
  return;
}

