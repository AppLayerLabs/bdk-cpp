#ifndef UTILS_H
#define UTILS_H

#include <fstream>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/lexical_cast.hpp>
#include <web3cpp/devcore/CommonData.h>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;
using uint256_t = boost::multiprecision::uint256_t;
using bytes = std::vector<uint8_t>;

template <typename ElemT>
struct HexTo {
  ElemT value;
  operator ElemT() const { return value; }
  friend std::istream& operator>>(std::istream& in, HexTo& out) {
    in >> std::hex >> out.value;
    return in;
  }
};

namespace Log {
  const std::string subnet = "Subnet::";
  const std::string chainHead = "ChainHead::";
  const std::string block = "Block::";
  const std::string db = "DBService::";
  const std::string state = "State::";
  const std::string grpcServer = "VMServiceImplementation::";
  const std::string grpcClient = "VMCommClient::";
  const std::string utils = "Utils::";
  const std::string httpServer = "HTTPServer::";
};

namespace Utils {
  void logToFile(std::string str);
  void LogPrint(std::string prefix, std::string function, std::string data);
  std::string uint256ToBytes(const uint256_t &i);
  std::string uint64ToBytes(const uint64_t &i);
  std::string uint32ToBytes(const uint32_t &i);
  uint256_t bytesToUint256(const std::string &bytes);
  uint64_t bytesToUint64(const std::string &bytes);
  uint32_t bytesToUint32(const std::string &bytes);
  // Simple function to remove "0x" and lowercase everything from a hex string.
  void patchHex(std::string& str);
  // Simple uint > hex, does not handle paddings or 0x prefix.
  template <typename T> std::string uintToHex(T i) {
    std::stringstream ss;
    std::string ret;
    ss << std::hex << i;
    ret = ss.str();
    for (auto &c : ret) {
      if (std::isupper(c))
        c = std::tolower(c);
    }
    return ret;
  }
  // Simple hex > uint, return as uint256_t
  uint256_t hexToUint(std::string &hex);
  // Hex <-> Bytes (using string containers)
  std::string hexToBytes(std::string hex);
  std::string bytesToHex(std::string bytes);
}

class Address {
  private:
    // Stored in bytes.
    std::string innerAddress;
  public:
    // RPC Requests address are in hex format
    Address(std::string address, bool fromRPC = true) {
      if (fromRPC) {
        Utils::patchHex(address);
        innerAddress = Utils::hexToBytes(innerAddress);
      } else {
        innerAddress = address;
      }
    }

    std::string get() const { return innerAddress; };
    std::string hex() const { return Utils::bytesToHex(innerAddress); }

    bool operator==(const Address& rAddress) const {
      return bool(innerAddress == rAddress.innerAddress);
    }
};


template <>
struct std::hash<Address> {
  size_t operator() (const Address& address) const {
    return std::hash<std::string>()(address.get());
  }
};
#endif // UTILS_H