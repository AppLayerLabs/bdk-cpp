#include "utils.h"

std::mutex log_lock;
std::mutex debug_mutex;

void Utils::logToFile(std::string str) {
  log_lock.lock();
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
  log.close();
  log_lock.unlock();
}

void Utils::LogPrint(const std::string &prefix, std::string function, std::string data) {
  debug_mutex.lock();
  std::ofstream log("debug.txt", std::ios::app);
  log << prefix << function << " - " << data << std::endl;
  log.close();
  debug_mutex.unlock();
}

Hash Utils::sha3(const std::string_view &input) {
  std::string ret;
  ethash_hash256 h = ethash_keccak256(reinterpret_cast<const unsigned char*>(&input[0]), input.size());
  for(int i = 0; i < 32; i++) {
    ret.push_back(h.bytes[i]);
  }
  Hash retH(std::move(ret));
  return retH;
}

std::string Utils::uint256ToBytes(const uint256_t &i) {
  std::string ret(32, 0x00);
  std::string tmp;
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (uint16_t i = 0; i < tmp.size(); ++i) {
    ret[31-i] = tmp[tmp.size()-i-1];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  return ret;
}

std::string Utils::uint160ToBytes(const uint160_t &i) {
  std::string ret(20, 0x00);
  std::string tmp;
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (uint16_t i = 0; i < tmp.size(); ++i) {
    ret[19-i] = tmp[tmp.size()-i-1];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  return ret;
}

std::string Utils::uint64ToBytes(const uint64_t &i) {
  std::string ret(8, 0x00);
  std::memcpy(&ret[0], &i, 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

std::string Utils::uint32ToBytes(const uint32_t &i) {
  std::string ret(4, 0x00);
  std::memcpy(&ret[0], &i, 4);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

std::string Utils::uint16ToBytes(const uint16_t &i) {
  std::string ret(2, 0x00);
  std::memcpy(&ret[0], &i, 2);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

std::string Utils::uint8ToBytes(const uint8_t &i) {
  std::string ret(1, 0x00);
  std::memcpy(&ret[0], &i, 1);
  return ret;
}

uint256_t Utils::bytesToUint256(const std::string_view &bytes) {
  if (bytes.size() != 32) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Invalid bytes size - expected 32, got ") + std::to_string(bytes.size())
    );
  }
  uint256_t ret;
  boost::multiprecision::import_bits(ret, bytes.begin(), bytes.end(), 8);
  return ret;
}

uint160_t Utils::bytesToUint160(const std::string_view &bytes) {
  if (bytes.size() != 20) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Invalid bytes size - expected 20, got ") + std::to_string(bytes.size())
    );
  }
  uint160_t ret;
  boost::multiprecision::import_bits(ret, bytes.begin(), bytes.end(), 8);
  return ret;
}

uint64_t Utils::bytesToUint64(const std::string_view &bytes) {
  if (bytes.size() != 8) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Invalid bytes size - expected 8, got ") + std::to_string(bytes.size())
    );
  }
  uint64_t ret = 0;
  std::memcpy(&ret, bytes.data(), 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(ret);
  #else
    return ret;
  #endif
}

uint32_t Utils::bytesToUint32(const std::string_view &bytes) {
  if (bytes.size() != 4) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Invalid bytes size - expected 4, got ") + std::to_string(bytes.size())
    );
  }
  uint32_t ret = 0;
  std::memcpy(&ret, bytes.data(), 4);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(ret);
  #else
    return ret;
  #endif
}

uint16_t Utils::bytesToUint16(const std::string_view &bytes) {
  if (bytes.size() != 2) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Invalid bytes size - expected 2, got ") + std::to_string(bytes.size())
    );
  }
  uint16_t ret = 0;
  std::memcpy(&ret, bytes.data(), 2);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap16(ret);
  #else
    return ret;
  #endif
}

uint8_t Utils::bytesToUint8(const std::string_view &bytes) {
  if (bytes.size() != 1) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Invalid bytes size - expected 1, got ") + std::to_string(bytes.size())
    );
  }
  uint8_t ret = 0;
  ret = bytes[0];
  return ret;
}

void Utils::patchHex(std::string& str) {
  if (str[0] == '0' && str[1] == 'x') str = str.substr(2);
  for (auto &c : str) if (std::isupper(c)) c = std::tolower(c);
  return;
}

uint256_t Utils::hexToUint(std::string &hex) {
  patchHex(hex);
  return boost::lexical_cast<HexTo<uint256_t>>(hex);
}

std::string Utils::hexToBytes(std::string hex) {
  patchHex(hex);
  std::string ret;
  uint32_t index = 0;

  // If odd hex (e.g. "abc"), parse only one char first ("a")
  // so we don't go out of range later
  if (hex.size() % 2 != 0) {
    int h = fromHexChar(hex[index]);
    if (h != -1) {
      ret += uint8_t(h);
    } else {
      Utils::LogPrint(Log::utils ,__func__, "Invalid Hex");
      throw std::runtime_error(std::string(__func__) + ": " +
        std::string("Invalid hex char: ") + hex[index]
      );
    }
    index++;
  }

  // Parse two by two chars until the end
  while (index < hex.size()) {
    int h = fromHexChar(hex[index]);
    int l = fromHexChar(hex[index+1]);
    if (h != -1 && l != -1) {
      ret += uint8_t(h * 16 + l);
    } else {
      throw std::runtime_error(std::string(__func__) + ": " +
        std::string("One or more invalid hex chars: ") +
        hex[index] + hex[index + 1]
      );
    }
    index += 2;
  }
  return ret;
}

int Utils::fromHexChar(char c) noexcept {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')	return c - 'A' + 10;
  return -1;
}

std::string Utils::bytesToHex(const std::string_view &bytes) { return dev::toHex(bytes); }

bool Utils::verifySignature(uint8_t const &v, uint256_t const &r, uint256_t const &s) {
  // s_max = 0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141
  static const uint256_t s_max("115792089237316195423570985008687907852837564279074904382605163141518161494337");
  static const uint256_t s_zero = 0;
  return (v <= 1 && r > s_zero && s > s_zero && r < s_max && s < s_max);
}

std::string Utils::padLeft(std::string str, unsigned int charAmount, char sign) {
  bool hasPrefix = (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X");
  if (hasPrefix) { str = str.substr(2); }
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + padded + str;
}

std::string Utils::padRight(std::string str, unsigned int charAmount, char sign) {
  bool hasPrefix = (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X");
  if (hasPrefix) { str = str.substr(2); }
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + str + padded;
}

void Utils::toLowercaseAddress(std::string& address) {
  std::transform(address.begin(), address.end(), address.begin(), ::tolower);
}

void Utils::toUppercaseAddress(std::string& address) {
  std::transform(address.begin(), address.end(), address.begin(), ::toupper);
}

void Utils::toChecksumAddress(std::string& address) {
  // Hash requires lowercase address without "0x"
  if (address.substr(0, 2) == "0x" || address.substr(0, 2) == "0X") {
    address = address.substr(2);
  }
  Utils::toLowercaseAddress(address);
  std::string hash = Utils::sha3(address).get();
  hash = Utils::bytesToHex(hash);
  for (int i = 0; i < address.length(); i++) {
    if (!std::isdigit(address[i])) {  // Only check letters (A-F)
      // If character hash is 8-F then make it uppercase
      int nibble = std::stoi(hash.substr(i, 1), nullptr, 16);
      address[i] = (nibble >= 8) ? std::toupper(address[i]) : std::tolower(address[i]);
    }
  }
  address.insert(0, "0x");
}

bool Utils::isAddress(const std::string& address, const bool &fromRPC) {
  if (fromRPC) {
    // Regexes for checking the basic requirements of an address,
    // all lower or all upper case, respectively.
    std::regex addRegex = std::regex("^(0x|0X)?[0-9a-fA-F]{40}$");
    std::regex lowRegex = std::regex("^(0x|0X)?[0-9a-f]{40}$");
    std::regex uppRegex = std::regex("^(0x|0X)?[0-9A-F]{40}$");
    if (!std::regex_match(address, addRegex)) {
      return false;
    } else if (std::regex_match(address, lowRegex) || std::regex_match(address, uppRegex)) {
      return true;
    } else {
      return checkAddressChecksum(address);
    }
  } else {
    return address.size() == 20;
  }
}

bool Utils::checkAddressChecksum(const std::string& address) {
  std::string addCpy = address;
  Utils::toChecksumAddress(addCpy);
  return address == addCpy;
}

json Utils::readConfigFile() {
  if(!std::filesystem::exists("config.json")){
    Utils::LogPrint(Log::utils, __func__, "No config file found, generating default");
    json config;
    config["rpcport"] = 8080;
    std::ofstream configFile("config.json");
    configFile << config.dump(2);
    configFile.close();
  }

  std::ifstream configFile("config.json");
  json config = json::parse(configFile);
  
  return config;
}


uint64_t Utils::splitmix(uint64_t i) {
  // http://xorshift.di.unimi.it/splitmix64.c
  i += 0x9e3779b97f4a7c15;
  i = (i ^ (i >> 30)) * 0xbf58476d1ce4e5b9;
  i = (i ^ (i >> 27)) * 0x94d049bb133111eb;
  return i ^ (i >> 31);
}