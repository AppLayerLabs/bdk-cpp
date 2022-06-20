#include "utils.h"


std::mutex log_lock;

void Utils::logToFile(std::string str) {
  log_lock.lock();
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
  log.close();
  log_lock.unlock();
}

std::vector<uint8_t> Utils::uint256ToBytes(const uint256_t &i) {
  std::vector<uint8_t> ret(32, 0x00);
  std::vector<uint8_t> tmp;
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);  

  // Replace the bytes from tmp into ret, to make it 32 bytes in size.
  for (uint16_t i = 0; i < tmp.size(); ++i) {
    ret[31-i] = tmp[31-i];
  }

  return ret;
}

std::vector<uint8_t> Utils::uint64ToBytes(const uint64_t &i) {
  std::vector<uint8_t> ret(8, 0x00);
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

std::vector<uint8_t> Utils::uint32ToBytes(const uint32_t &i) {
  std::vector<uint8_t> ret(4, 0x00);
  ret[0] = i >> 24;
  ret[1] = i >> 16;
  ret[2] = i >> 8;
  ret[3] = i;
  return ret;
}


uint256_t Utils::bytesToUint256(const std::vector<uint8_t> &bytes) {
  if (bytes.size() != 32) {
    throw; // Invalid size.
  }
  uint256_t ret;
  boost::multiprecision::import_bits(ret, bytes.begin(), bytes.end(), 8);
  return ret;
}

uint64_t Utils::bytesToUint64(const std::vector<uint8_t> &bytes) {
  if (bytes.size() != 8) {
    throw; // Invalid size;
  }
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

uint32_t Utils::bytesToUint32(const std::vector<uint8_t> &bytes) {
  if (bytes.size() != 4) {
    throw; // Invalid size
  }
  uint32_t ret;
  ret |= bytes[0] << 24;
  ret |= bytes[1] << 16;
  ret |= bytes[2] << 8;
  ret |= bytes[3];
  return ret;
}