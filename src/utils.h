#ifndef UTILS_H
#define UTILS_H

#include "../include/web3cpp/devcore/Common.h"

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sstream>
#include <fstream>
#include <chrono>

template <typename ElemT>
struct HexTo {
  ElemT value;
  operator ElemT() const { return value; }
  friend std::istream& operator>>(std::istream& in, HexTo& out) {
    in >> std::hex >> out.value;
    return in;
  }
};

namespace Utils {
    
  // u256 to 48 bytes
  dev::bytes u256toBytes(dev::u256 value);
  // 48 bytes to u256
  dev::u256 bytesTou256(dev::bytes value);
  std::string secondsToGoTimeStamp(uint64_t seconds);
  std::string hashToBytes(std::string hash);
  std::string uintToHex(std::string input, bool isPadded = true);
  std::string uintFromHex(std::string hex);
  std::string bytesToHex(std::string input, bool isUint);
  std::string hexToUtf8(std::string hex, uint32_t size);
  void logToFile(std::string str);
  std::vector<std::string> parseHex(std::string hexStr, std::vector<std::string> types);

}

#endif // UTILS_H
