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

std::string Utils::uint256ToBytes(const uint256_t &i) {
  std::string ret(32, 0x00);
  std::string tmp;
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (uint16_t i = 0; i < tmp.size(); ++i) {
    ret[31-i] = tmp[31-i];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  return ret;
}

std::string Utils::uint64ToBytes(const uint64_t &i) {
  std::string ret(8, 0x00);
  ret[0] = i >> 56;
  ret[1] = i >> 48;
  ret[2] = i >> 40;
  ret[3] = i >> 32;
  ret[4] = i >> 24;
  ret[5] = i >> 16;
  ret[6] = i >> 8;
  ret[7] = i;
  return ret;
}

std::string Utils::uint32ToBytes(const uint32_t &i) {
  std::string ret(4, 0x00);
  ret[0] = i >> 24;
  ret[1] = i >> 16;
  ret[2] = i >> 8;
  ret[3] = i;
  return ret;
}

uint256_t Utils::bytesToUint256(const std::string &bytes) {
  if (bytes.size() != 32) throw; // Invalid size.
  uint256_t ret;
  boost::multiprecision::import_bits(ret, bytes.begin(), bytes.end(), 8);
  return ret;
}

uint64_t Utils::bytesToUint64(const std::string &bytes) {
  if (bytes.size() != 8) throw; // Invalid size;
  uint64_t ret;
  ret |= uint64_t(bytes[0]) << 56;
  ret |= uint64_t(bytes[1]) << 48;
  ret |= uint64_t(bytes[2]) << 40;
  ret |= uint64_t(bytes[3]) << 32;
  ret |= uint64_t(bytes[4]) << 24;
  ret |= uint64_t(bytes[5]) << 16;
  ret |= uint64_t(bytes[6]) << 8;
  ret |= uint64_t(bytes[7]);
  return ret;
}

uint32_t Utils::bytesToUint32(const std::string &bytes) {
  if (bytes.size() != 4) throw; // Invalid size
  uint32_t ret;
  ret |= bytes[0] << 24;
  ret |= bytes[1] << 16;
  ret |= bytes[2] << 8;
  ret |= bytes[3];
  return ret;
}

void Utils::LogPrint(std::string prefix, std::string function, std::string data) {
  debug_mutex.lock();
  std::ofstream log("debug.txt", std::ios::app);
  log << prefix << function << " - " << data << std::endl;
  log.close();
  debug_mutex.unlock();
}

void Utils::patchHex(std::string& str) {
  if (str[0] == '0' && str[1] == 'x') str = str.substr(2);
  for (auto &c : str) {
    if (std::isupper(c)) c = std::tolower(c);
  }
  return;
}

uint256_t Utils::hexToUint(std::string &hex) {
  patchHex(hex);
  return boost::lexical_cast<HexTo<uint256_t>>(hex);
}

std::string Utils::hexToBytes(std::string hex) {
  std::string ret;
  for (uint64_t i = 0; i < hex.size(); i += 2) {
    std::stringstream ss;
    std::string byteStr = std::string("") + hex[i] + hex[i+1];
    uint16_t byteInt;
    ss << std::hex << byteStr;
    ss >> byteInt;
    uint8_t byte = byteInt >> 0;
    ret += byte;
  }
  return ret;
}

std::string Utils::bytesToHex(std::string bytes) {
  return dev::toHex(bytes);
}

