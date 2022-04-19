#include "utils.h"

std::mutex log_lock;

dev::bytes Utils::u256toBytes(dev::u256 value) {
    dev::bytes ret(sizeof(value));
    unsigned char bytes[sizeof(value)];


    std::memcpy(&bytes, &value, sizeof(value)); 

    for(uint64_t i = 0; i < sizeof(value); ++i) {
        ret[i] = bytes[i];
    }
    
    return ret;
};

void Utils::logToFile(std::string str) {
  log_lock.lock();
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
  log.close();
  log_lock.unlock();
}

dev::u256 Utils::bytesTou256(dev::bytes value) {
    dev::u256 ret;
    unsigned char bytes[sizeof(value)];

    for (uint64_t i = 0; i < sizeof(value); ++i) {
        bytes[i] = value[i];
    }

    std::memcpy(&ret, &bytes, sizeof(bytes));
    return ret;
}

std::string Utils::secondsToGoTimeStamp(uint64_t seconds) {
    std::string ret;
    ret.resize(15);
    // First byte is version. 0x01
    ret[0] = 0x01;
    // Next 8 bytes is timestamp in seconds.   
    seconds += 62135596800; // Sum year 1 to 1970, stupid Go lol
    uint8_t timestamp[8];
    std::memcpy(&timestamp, &seconds, sizeof(seconds));
    ret[1] = timestamp[7];
    ret[2] = timestamp[6];
    ret[3] = timestamp[5];
    ret[4] = timestamp[4];
    ret[5] = timestamp[3];
    ret[6] = timestamp[2];
    ret[7] = timestamp[1];
    ret[8] = timestamp[0];
    // Remaining timestamp is nanoseconds, we ignore it for ease of implementation. I only have 18 hours to "finish" this
    ret[9] = 0x00;
    ret[10] = 0x00;
    ret[11] = 0x00;
    ret[12] = 0x00;
    ret[13] = 0x00;
    ret[14] = 0x00;

    return ret;
}

std::string Utils::hashToBytes(std::string hash) {
    std::string ret;
    ret.resize(32);
    uint64_t index = 0;

    for (int i = 0; i < 32; ++i) {
        // fkin C++ uint16_t and them get the first byte, completely pepega
        std::stringstream strm;
        std::string byteStr = std::string("") + hash[index] + hash[index+1];
        uint16_t byteInt;
        strm << std::hex << byteStr;
        strm >> byteInt;
        uint8_t byte = byteInt >> 0;
        ret[i] = byte;
        index += 2;
    }
    return ret;
}

std::string Utils::uintToHex(std::string input, bool isPadded) {
  // Padding is 32 bytes
  std::string padding = "0000000000000000000000000000000000000000000000000000000000000000";
  std::stringstream ss;
  std::string valueHex;
  dev::u256 value;

  // Convert value to Hex and lower case all letters
  value = boost::lexical_cast<dev::u256>(input);
  ss << std::hex << value;
  valueHex = ss.str();
  for (auto& c : valueHex) {
    if (std::isupper(c)) {
      c = std::tolower(c);
    }
  }

  if (!isPadded) { return valueHex; }

  // Insert value into padding from right to left
  for (size_t i = (valueHex.size() - 1), x = (padding.size() - 1),
    counter = 0; counter < valueHex.size(); --i, --x, ++counter) {
    padding[x] = valueHex[i];
  }
  return padding;
}

std::string Utils::bytesToHex(std::string input, bool isUint) {
  std::string ret;
  if (!isUint) {
    ret += uintToHex(boost::lexical_cast<std::string>(input.size()));
  }
  std::stringstream ss;
  for (auto c : input) {
    ss << std::hex << int(c);
  }
  ret += ss.str();
  // Bytes are left padded
  while ((ret.size() % 64) != 0) {
    ret += "0";
  }
  return ret;
}

std::string Utils::uintFromHex(std::string hex) {
  std::string ret;
  if (hex.substr(0, 2) == "0x") { hex = hex.substr(2); } // Remove the "0x"
  unsigned int number = boost::lexical_cast<HexTo<unsigned int>>(hex);
  ret = boost::lexical_cast<std::string>(number);
  return ret;
}

std::vector<std::string> Utils::parseHex(std::string hexStr, std::vector<std::string> types) {
  std::vector<std::string> ret;
  try {

    // Get rid of the "0x" before converting and lowercase all letters
    hexStr = (hexStr.substr(0, 2) == "0x") ? hexStr.substr(2) : hexStr;

    // Parse each type and erase it from the hex string until it is empty
    for (std::string type : types) {
      if (type == "uint" || type == "bool") {
        // All uints are 32 bytes and each hex char is half a byte, so 32 bytes = 64 chars.
        dev::u256 value = boost::lexical_cast<HexTo<dev::u256>>(hexStr.substr(0, 64));
        ret.push_back(boost::lexical_cast<std::string>(value));
      } else if (type == "bool") {
        // Bools are treated as uints, so the same logic applies, but returning a proper bool.
        bool value = boost::lexical_cast<HexTo<bool>>(hexStr.substr(0, 64));
        ret.push_back(boost::lexical_cast<std::string>(value));
      } else if (type == "address") {
        // Addresses are always 20 bytes (40 chars) but are treated as uints, so we
        // take all 64 chars, get rid of the first 24 chars and add "0x" at the start
        std::string value = hexStr.substr(0, 64);
        value.erase(0, 24);
        ret.push_back("0x" + value);
      }
      hexStr.erase(0, 64);
    }

  } catch (std::exception &e) {
    Utils::logToFile(std::string("parseHex error: ") + e.what() + " value: " + hexStr);
  }

  return ret;
}