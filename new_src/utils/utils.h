#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string_view>
#include <thread>

#include <boost/asio/ip/address.hpp>
#include <boost/beast/core.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <ethash/keccak.h>
#include <openssl/rand.h>

#include "../libs/devcore/CommonData.h"
#include "../libs/devcore/FixedHash.h"
#include "../libs/json.hpp"

using json = nlohmann::ordered_json;
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;

void fail(std::string cl, std::string func, boost::beast::error_code ec, const char* what);

namespace Log {
  const std::string subnet = "Subnet::";
  const std::string chain = "BlockChain::";
  const std::string mempool = "BlockMempool::";
  const std::string block = "Block::";
  const std::string db = "DB::";
  const std::string state = "State::";
  const std::string grpcServer = "gRPCServer::";
  const std::string grpcClient = "gRPCClient::";
  const std::string utils = "Utils::";
  const std::string httpServer = "HTTPServer::";
  const std::string blockManager = "BlockManager::";
  const std::string ABI = "ABI::";
  const std::string P2PClient = "P2PClient::";
  const std::string P2PServer = "P2PServer::";
  const std::string P2PManager = "P2PManager::";
};

enum BlockStatus { Unknown, Processing, Rejected, Accepted };

enum Networks { Mainnet, Testnet, LocalTestnet };

struct Account { uint256_t balance = 0; uint32_t nonce = 0; };

namespace Utils {
  void logToFile(std::string str);
  void logToDebug(std::string pfx, std::string func, std::string data);
  Hash sha3(const string_view& input);
  std::string uint256ToBytes(const uint256_t& i);
  std::string uint160ToBytes(const uint160_t& i);
  std::string uint64ToBytes(const uint64_t& i);
  std::string uint32ToBytes(const uint32_t& i);
  std::string uint16ToBytes(const uint16_t& i);
  std::string uint8ToBytes(const uint8_t& i);
  std::string randBytes(const size_t& size);
  bool isHex(const std::string_view& input, bool strict = false);
  std::string utf8ToHex(const std::string_view& input);
  std::string bytesToHex(const std::string_view& b);
  uint256_t bytesToUint256(const std::string_view& b);
  uint160_t bytesToUint160(const std::string_view& b);
  uint64_t bytesToUint64(const std::string_view& b);
  uint32_t bytesToUint32(const std::string_view& b);
  uint16_t bytesToUint16(const std::string_view& b);
  uint8_t bytesToUint8(const std::string_view& b);
  int hexCharToInt(char c);
  void patchHex(std::string& str);
  template <typename T> std::string uintToHex(const T& i) {
    std::stringstream ss;
    std::string ret;
    ss << std::hex << i;
    ret = ss.str();
    for (auto &c : ret) if (std::isupper(c)) c = std::tolower(c);
    return ret;
  }
  void stripHexPrefix(std::string& str);
  uint256_t hexToUint(std::string& hex);
  std::string hexToBytes(std::string hex);
  bool verifySig(const uint256_t& r, const uint256_t& s, const uint8_t& v);
  std::string padLeft(std::string str, unsigned int charAmount, char sign = '0');
  std::string padRight(std::string str, unsigned int charAmount, char sign = '0');
  template <class T, class In> T fromBigEndian(const In& bytes) {
    T ret = (T)0;
    for (auto i: bytes) {
      ret = (T)((ret << 8) | (byte)(typename std::make_unsigned<decltype(i)>::type)i);
    }
    return ret;
  }
  void toLower(std::string& str);
  void toUpper(std::string& str);
  void toChksum(std::string& str);
  bool isChksum(const std::string& str);
  bool isAddress(const std::string& add, bool fromRPC);
  json readConfigFile();
};

#endif  // UTILS_H
