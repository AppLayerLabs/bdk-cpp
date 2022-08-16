#include "tests.h"
#include "../src/core/utils.h"
#include "../src/libs/devcore/CommonData.h"

void Tests::uint256ToBytes() {
  std::string expected = Utils::hexToBytes("0x00000000000000000000000000000000000000000000000011234d578edbdd73");
  uint256_t number(1234915761283915123);
  std::string actual = Utils::uint256ToBytes(number);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
};

void Tests::uint160ToBytes() {
  std::string expected = Utils::hexToBytes("0x00000000000000000000000011234d578edbdd73");
  uint160_t number(1234915761283915123);
  std::string actual = Utils::uint160ToBytes(number);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
}

void Tests::uint64ToBytes() {
  std::string expected = Utils::hexToBytes("0x000000008edbdd73");
  uint64_t number(2396773747);
  std::string actual = Utils::uint64ToBytes(number);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
}

void Tests::uint32ToBytes() {
  std::string expected = Utils::hexToBytes("0x0000dd73");
  uint32_t number(56691);
  std::string actual = Utils::uint32ToBytes(number);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
}

void Tests::uint8ToBytes() {
  std::string expected = Utils::hexToBytes("0x73");
  uint8_t number(115);
  std::string actual = Utils::uint8ToBytes(number);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
};

void Tests::bytesToUint256() {
  uint256_t expected(1234915761283915123);
  std::string bytes = Utils::hexToBytes("0x00000000000000000000000000000000000000000000000011234d578edbdd73");
  uint256_t actual = Utils::bytesToUint256(bytes);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
}

void Tests::bytesToUint160() {
  uint160_t expected(1234915761283915123);
  std::string bytes = Utils::hexToBytes("0x00000000000000000000000011234d578edbdd73");
  uint160_t actual = Utils::bytesToUint160(bytes);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
}

void Tests::bytesToUint64() {
  uint64_t expected(1234915761283915123);
  std::string bytes = Utils::hexToBytes("0x11234d578edbdd73");
  uint64_t actual = Utils::bytesToUint64(bytes);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
}

void Tests::bytesToUint32() {
  uint32_t expected(19076417);
  std::string bytes = Utils::hexToBytes("0x1231541");
  uint32_t actual = Utils::bytesToUint32(bytes);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
}

void Tests::bytesToUint8() {
  uint32_t expected(115);
  std::string bytes = Utils::hexToBytes("0x73");
  uint8_t actual = Utils::bytesToUint8(bytes);
  assert(expected == actual);
  std::cout << __func__ << " OK" << std::endl;
}

