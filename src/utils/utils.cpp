#include "utils.h"

std::mutex log_lock;
std::mutex debug_mutex;
std::mutex cout_mutex;

std::atomic<bool> Utils::logToCout = false;

void fail(const std::string& cl, std::string&& func, boost::beast::error_code ec, const char* what) {
  Logger::logToDebug(LogType::ERROR, cl, std::move(func), std::string("HTTP Fail ") + what + " : " + ec.message());
}

void Utils::logToFile(std::string_view str) {
  // Lock to prevent multiple memory writes
  std::lock_guard lock(log_lock);
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
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

BytesArr<31> Utils::uint248ToBytes(const uint248_t &i) {
  BytesArr<31> ret;
  Bytes tmp;
  tmp.reserve(31);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 32 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[30 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<30> Utils::uint240ToBytes(const uint240_t &i) {
  BytesArr<30> ret;
  Bytes tmp;
  tmp.reserve(30);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 32 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[29 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<29> Utils::uint232ToBytes(const uint232_t &i) {
  BytesArr<29> ret;
  Bytes tmp;
  tmp.reserve(29);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 32 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[28 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<28> Utils::uint224ToBytes(const uint224_t &i) {
  BytesArr<28> ret;
  Bytes tmp;
  tmp.reserve(28);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 32 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[27 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<27> Utils::uint216ToBytes(const uint216_t &i) {
  BytesArr<27> ret;
  Bytes tmp;
  tmp.reserve(27);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 32 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[26 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<26> Utils::uint208ToBytes(const uint208_t &i) {
  BytesArr<26> ret;
  Bytes tmp;
  tmp.reserve(26);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 26 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[25 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<25> Utils::uint200ToBytes(const uint200_t &i) {
  BytesArr<25> ret;
  Bytes tmp;
  tmp.reserve(25);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 25 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[24 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<24> Utils::uint192ToBytes(const uint192_t &i) {
  BytesArr<24> ret;
  Bytes tmp;
  tmp.reserve(24);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 24 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[23 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<23> Utils::uint184ToBytes(const uint184_t &i) {
  BytesArr<23> ret;
  Bytes tmp;
  tmp.reserve(23);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 23 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[22 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<22> Utils::uint176ToBytes(const uint176_t &i) {
  BytesArr<22> ret;
  Bytes tmp;
  tmp.reserve(22);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 22 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[21 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<21> Utils::uint168ToBytes(const uint168_t &i) {
  BytesArr<21> ret;
  Bytes tmp;
  tmp.reserve(21);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 21 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[20 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<20> Utils::uint160ToBytes(const uint160_t &i) {
  BytesArr<20> ret;
  Bytes tmp;
  tmp.reserve(20);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 20 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[19 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<19> Utils::uint152ToBytes(const uint152_t &i) {
  BytesArr<19> ret;
  Bytes tmp;
  tmp.reserve(19);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 19 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[18 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<18> Utils::uint144ToBytes(const uint144_t &i) {
  BytesArr<18> ret;
  Bytes tmp;
  tmp.reserve(18);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 18 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[17 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<17> Utils::uint136ToBytes(const uint136_t &i) {
  BytesArr<17> ret;
  Bytes tmp;
  tmp.reserve(17);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 17 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[16 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<16> Utils::uint128ToBytes(const uint128_t &i) {
  BytesArr<16> ret;
  Bytes tmp;
  tmp.reserve(16);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 16 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[15 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<15> Utils::uint120ToBytes(const uint120_t &i) {
  BytesArr<15> ret;
  Bytes tmp;
  tmp.reserve(15);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 15 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[14 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<14> Utils::uint112ToBytes(const uint112_t &i) {
  BytesArr<14> ret;
  Bytes tmp;
  tmp.reserve(14);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 14 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[13 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<13> Utils::uint104ToBytes(const uint104_t &i) {
  BytesArr<13> ret;
  Bytes tmp;
  tmp.reserve(13);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 13 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[12 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<12> Utils::uint96ToBytes(const uint96_t &i) {
  BytesArr<12> ret;
  Bytes tmp;
  tmp.reserve(12);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 12 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[11 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<11> Utils::uint88ToBytes(const uint88_t &i) {
  BytesArr<11> ret;
  Bytes tmp;
  tmp.reserve(11);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 11 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[10 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<10> Utils::uint80ToBytes(const uint80_t &i) {
  BytesArr<10> ret;
  Bytes tmp;
  tmp.reserve(10);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 10 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[9 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<9> Utils::uint72ToBytes(const uint72_t &i) {
  BytesArr<9> ret;
  Bytes tmp;
  tmp.reserve(9);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 9 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[8 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<7> Utils::uint56ToBytes(const uint56_t &i) {
  BytesArr<7> ret;
  Bytes tmp;
  tmp.reserve(7);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 7 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[6 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<6> Utils::uint48ToBytes(const uint48_t &i) {
  BytesArr<6> ret;
  Bytes tmp;
  tmp.reserve(6);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 6 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[5 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<5> Utils::uint40ToBytes(const uint40_t &i) {
  BytesArr<5> ret;
  Bytes tmp;
  tmp.reserve(5);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 5 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[4 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<3> Utils::uint24ToBytes(const uint24_t &i) {
  BytesArr<3> ret;
  Bytes tmp;
  tmp.reserve(3);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it 3 bytes in size.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[2 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
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

uint128_t Utils::bytesToUint128(const BytesArrView b) {
  if (b.size() != 16) throw std::runtime_error(std::string(__func__)
    + ": Invalid bytes size - expected 16, got " + std::to_string(b.size())
  );
  uint128_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
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
    Logger::logToDebug(LogType::INFO, Log::utils, __func__, "No config file found, generating default");
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