#include "utils.h"

std::mutex log_lock;
std::mutex debug_mutex;
std::mutex cout_mutex;

std::atomic<bool> Utils::logToCout = false;

void fail(std::string_view cl, std::string_view func, boost::beast::error_code ec, const char* what) {
  Utils::logToDebug(cl, func, std::string("HTTP Fail ") + what + " : " + ec.message());
}

void Utils::logToFile(std::string_view str) {
  // Lock to prevent multiple memory writes
  std::lock_guard lock(log_lock);
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
  log.close();
}

void Utils::logToDebug(std::string_view pfx, std::string_view func, std::string_view data) {
  std::lock_guard lock(debug_mutex);
  std::ofstream log("debug.txt", std::ios::app);
  log << pfx << "::" << func << " - " << data << std::endl;
  log.close();
}

void Utils::safePrint(std::string_view str) {
  if (!Utils::logToCout) return; // Never print if we are in a test
  std::lock_guard lock(cout_mutex);
  std::cout << str << std::endl;
}

Hash Utils::sha3(const BytesArrView input) {
  ethash_hash256 h = ethash_keccak256(input.data(), input.size());
  BytesArr<32> ret;
  std::copy(reinterpret_cast<const Byte*>(h.bytes), reinterpret_cast<const Byte*>(h.bytes + 32), ret.begin());
  return std::move(ret);
}

BytesArr<32> Utils::uint256ToBytes(const uint256_t& i) {
  BytesArr<32> ret;
  Bytes tmp;
  tmp.reserve(32);
  ret.fill(0x00);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[31 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

uint256_t Utils::bytesToUint256(const BytesArrView b) {
  if (b.size() != 32) throw std::runtime_error(std::string(__func__)
    + ": Invalid bytes size - expected 32, got " + std::to_string(b.size())
  );
  uint256_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

BytesArr<16> Utils::uint128ToBytes(const uint128_t &i) {
  BytesArr<16> ret;
  Bytes tmp;
  tmp.reserve(16);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 32 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[15 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

uint128_t Utils::bytesToUint128(const BytesArrView b) {
  if (b.size() != 16) throw std::runtime_error(std::string(__func__)
    + ": Invalid bytes size - expected 16, got " + std::to_string(b.size())
  );
  uint128_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}


BytesArr<14> Utils::uint112ToBytes(const uint112_t &i) {
  BytesArr<14> ret;
  Bytes tmp;
  tmp.reserve(14);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 32 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[13 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

uint112_t Utils::bytesToUint112(const BytesArrView b) {
  if (b.size() != 14) throw std::runtime_error(std::string(__func__)
                                               + ": Invalid bytes size - expected 16, got " + std::to_string(b.size())
    );
  uint112_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}


BytesArr<20> Utils::uint160ToBytes(const uint160_t& i) {
  BytesArr<20> ret;
  Bytes tmp;
  tmp.reserve(20);
  ret.fill(0x00);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[19 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

uint160_t Utils::bytesToUint160(const BytesArrView b) {
  if (b.size() != 20) throw std::runtime_error(std::string(__func__)
    + ": Invalid bytes size - expected 20, got " + std::to_string(b.size())
  );
  uint160_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

BytesArr<8> Utils::uint64ToBytes(const uint64_t& i) {
  BytesArr<8> ret;
  std::memcpy(&ret[0], &i, 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

uint64_t Utils::bytesToUint64(const BytesArrView b) {
  if (b.size() != 8) throw std::runtime_error(std::string(__func__)
    + ": Invalid bytes size - expected 8, got " + std::to_string(b.size())
  );
  uint64_t ret = 0;
  std::memcpy(&ret, b.data(), 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(ret);
  #endif
  return ret;
}

BytesArr<4> Utils::uint32ToBytes(const uint32_t& i) {
  BytesArr<4> ret;
  std::memcpy(&ret[0], &i, 4);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

uint32_t Utils::bytesToUint32(const BytesArrView b) {
  if (b.size() != 4) throw std::runtime_error(std::string(__func__)
    + ": Invalid bytes size - expected 4, got " + std::to_string(b.size())
  );
  uint32_t ret = 0;
  std::memcpy(&ret, b.data(), 4);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(ret);
  #endif
  return ret;
}

BytesArr<2> Utils::uint16ToBytes(const uint16_t& i) {
  BytesArr<2> ret;
  std::memcpy(&ret[0], &i, 2);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

uint16_t Utils::bytesToUint16(const BytesArrView b) {
  if (b.size() != 2) throw std::runtime_error(std::string(__func__)
    + ": Invalid bytes size - expected 2, got " + std::to_string(b.size())
  );
  uint16_t ret = 0;
  std::memcpy(&ret, b.data(), 2);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap16(ret);
  #endif
  return ret;
}

BytesArr<1> Utils::uint8ToBytes(const uint8_t& i) {
  BytesArr<1> ret;
  std::memcpy(&ret[0], &i, 1);
  return ret;
}

uint8_t Utils::bytesToUint8(const BytesArrView b) {
  if (b.size() != 1) throw std::runtime_error(std::string(__func__)
    + ": Invalid bytes size - expected 1, got " + std::to_string(b.size())
  );
  uint8_t ret;
  ret = b[0];
  return ret;
}

Bytes Utils::randBytes(const int& size) {
  Bytes bytes(size, 0x00);
  RAND_bytes((unsigned char*)bytes.data(), size);
  return bytes;
}

std::string Utils::padLeft(std::string str, unsigned int charAmount, char sign) {
  bool hasPrefix = (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X");
  if (hasPrefix) str = str.substr(2);
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + padded + str;
}

std::string Utils::padRight(std::string str, unsigned int charAmount, char sign) {
  bool hasPrefix = (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X");
  if (hasPrefix) str = str.substr(2);
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + str + padded;
}

Bytes Utils::padLeftBytes(const BytesArrView bytes, unsigned int charAmount, uint8_t sign) {
  size_t padding = (charAmount > bytes.size()) ? (charAmount - bytes.size()) : 0;
  Bytes padded = (padding != 0) ? Bytes(padding, sign) : Bytes(0, 0x00);
  padded.reserve(bytes.size() + padded.size());
  padded.insert(padded.end(), bytes.begin(), bytes.end());
  return padded;
}

Bytes Utils::padRightBytes(const BytesArrView bytes, unsigned int charAmount, uint8_t sign) {
  size_t padding = (charAmount > bytes.size()) ? (charAmount - bytes.size()) : 0;
  Bytes padded = (padding != 0) ? Bytes(padding, sign) : Bytes(0, 0x00);
  Bytes ret;
  ret.reserve(bytes.size() + padded.size());
  ret.insert(ret.end(), bytes.begin(), bytes.end());
  ret.insert(ret.end(), padded.begin(), padded.end());
  return ret;
}


json Utils::readConfigFile() {
  if (!std::filesystem::exists("config.json")) {
    Utils::logToDebug(Log::utils, __func__, "No config file found, generating default");
    json config;
    config["rpcport"] = 8080;
    config["p2pport"] = 8081;
    config["seedNodes"] = {
      "127.0.0.1:8086", "127.0.0.1:8087", "127.0.0.1:8088", "127.0.0.1:8089"
    };
    std::ofstream configFile("config.json");
    configFile << config.dump(2);
    configFile.close();
  }
  std::ifstream configFile("config.json");
  json config = json::parse(configFile);
  return config;
}