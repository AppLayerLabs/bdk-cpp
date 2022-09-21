#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <fstream>
#include <regex>
#include <string_view>

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "../libs/devcore/CommonData.h"
#include "../libs/devcore/FixedHash.h"
#include "../libs/json.hpp"
#include "../libs/keccak.hpp"
#include <ethash/keccak.h>

using json = nlohmann::ordered_json;
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;
static const uint256_t c_secp256k1n("115792089237316195423570985008687907852837564279074904382605163141518161494337");

template <typename ElemT> struct HexTo {
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
  const std::string chainTip = "ChainTip::";
  const std::string block = "Block::";
  const std::string db = "DBService::";
  const std::string state = "State::";
  const std::string grpcServer = "VMServiceImplementation::";
  const std::string grpcClient = "VMCommClient::";
  const std::string utils = "Utils::";
  const std::string httpServer = "HTTPServer::";
  const std::string blockManager = "BlockManager::";
};

namespace MessagePrefix {
  const char tx = 0x01;
  const char batchedTx = 0x02;
}

enum BlockStatus { Unknown, Processing, Rejected, Accepted };

template <unsigned N>
class StringContainer {
  private:
    std::string _data;
  
  public:
    // TODO: when constructing the StringContainer, if the input string doesn't match the size, it will throw
    // Add error handling for these throws, specially over places where the user can input data (such as subnetRPC.cpp).
    enum { size = N };
    StringContainer() { _data.resize(N, 0x00); }
    StringContainer(const std::string_view& data) { if (data.size() != N) { throw std::runtime_error("Invalid StringContainer input size"); } _data = data; };
    StringContainer(std::string&& data) { if (data.size() != N) { throw std::runtime_error("Invalid StringContainer input size"); } _data = std::move(data); };
    StringContainer(const StringContainer& other) { if (other.size != N) { throw std::runtime_error("Invalid StringContainer input size");} _data = other._data; };
    StringContainer(StringContainer&& other) { if (other.size != N) { throw std::runtime_error("Invalid StringContainer input size");} _data = std::move(other._data); };

    inline const std::string& get() const { return _data; }
    inline const std::string_view& get_view() const { return std::string_view(_data, N); }
    inline const bool empty() const { return _data.empty(); }
    bool operator==(const StringContainer& other) const { return (this->_data == other._data);}
    bool operator!=(const StringContainer& other) const { return (this->_data != other._data); }
    bool operator<(const StringContainer& other) const { return (this->_data < other._data); }
    bool operator>=(const StringContainer& other) const { return (this->_data >= other._data); }
    bool operator<=(const StringContainer& other) const { return (this->_data <= other._data); }
    bool operator>(const StringContainer& other) const { return (this->_data > other._data); }

    char& operator[](const size_t& pos) {
      return _data[pos];
    }

    const char& operator[](const size_t& pos) const {
      return _data[pos];
    }

    const std::string::const_iterator begin() const { return _data.cbegin(); }
    const std::string::const_iterator end() const { return _data.cend(); }

    /// @returns a constant byte pointer to the object's data.
    const char* data() const { return _data.data(); }

    StringContainer& operator=(StringContainer const &_c)
    {
        if (&_c == this)
            return *this;
        this->_data = _c.data();
        return *this;
    }

    StringContainer& operator=(StringContainer &&_c)
    {
        if (&_c == this)
            return *this;
        this->_data = std::move(_c.data());
        return *this;
    }

};

using Hash = StringContainer<32>;

namespace Utils {
  void logToFile(std::string str);
  void LogPrint(const std::string &prefix, std::string function, std::string data);
  Hash sha3(const std::string_view &input);
  std::string uint256ToBytes(const uint256_t &i);
  std::string uint160ToBytes(const uint160_t &i);
  std::string uint64ToBytes(const uint64_t &i);
  std::string uint32ToBytes(const uint32_t &i);
  std::string uint16ToBytes(const uint16_t &i);
  std::string uint8ToBytes(const uint8_t &i);
  uint256_t bytesToUint256(const std::string_view &bytes);
  uint160_t bytesToUint160(const std::string_view &bytes);
  uint64_t bytesToUint64(const std::string_view &bytes);
  uint32_t bytesToUint32(const std::string_view &bytes);
  uint16_t bytesToUint16(const std::string_view &bytes);
  uint8_t bytesToUint8(const std::string_view &bytes);
  int fromHexChar(char c) noexcept;
  void patchHex(std::string& str);
  template <typename T> std::string uintToHex(const T &i) {
    std::stringstream ss;
    std::string ret;
    ss << std::hex << i;
    ret = ss.str();
    for (auto &c : ret) if (std::isupper(c)) c = std::tolower(c);
    return ret;
  }
  uint256_t hexToUint(std::string &hex);
  std::string hexToBytes(std::string hex);
  std::string bytesToHex(const std::string_view& bytes);
  bool verifySignature(uint8_t const &v, uint256_t const &r, uint256_t const &s);
  std::string padLeft(std::string str, unsigned int charAmount, char sign = '0');
  std::string padRight(std::string str, unsigned int charAmount, char sign = '0');
  template <class T, class _In> inline T fromBigEndian(_In const& _bytes) {
    T ret = (T)0;
    for (auto i: _bytes) {
      ret = (T)((ret << 8) | (byte)(typename std::make_unsigned<decltype(i)>::type)i);
    }
    return ret;
  }
  void toLowercaseAddress(std::string& address);
  void toUppercaseAddress(std::string& address);
  void toChecksumAddress(std::string& address);
  bool isAddress(const std::string& address, bool fromRPC);
  bool checkAddressChecksum(const std::string& address);
};

struct Account {
  uint256_t balance = 0;
  uint32_t nonce = 0;
};

class Address {
  private:
    std::string innerAddress;

  public:
    Address() {}

    // C++ can only differ std::string&& and const std::string& on function overloading.
    Address(const std::string& address, bool fromRPC) {
      if (!Utils::isAddress(address, fromRPC)) {
        throw std::runtime_error(address + " is not a valid address");
      }
      if (fromRPC) {
        innerAddress = address;
        Utils::patchHex(innerAddress);
        innerAddress = Utils::hexToBytes(innerAddress);
      } else {
        innerAddress = address;
      }
    }

    // Move from iterators. used in Tx::Base string move constructor
    template<class It> Address(const It&& _begin,const It&& _end) {
      std::move(_begin, _end, std::back_inserter(innerAddress));
    }

    // Copy constructor.
    Address(const Address& other) {
      this->innerAddress = other.innerAddress;
    }

    // Move constructor.
    Address(Address&& other) noexcept :
      innerAddress(std::move(other.innerAddress)) {}

    // Move string constructor.
    Address(std::string&& address, bool fromRPC) {
      if (!Utils::isAddress(address, fromRPC)) {
        throw std::runtime_error(address + " is not a valid address");
      }
      if (fromRPC) {
        Utils::patchHex(address);
        innerAddress = std::move(Utils::hexToBytes(address));
      } else {
        innerAddress = std::move(address);
      }
    }

    // Destructor.
    ~Address() { this->innerAddress = ""; }

    const std::string& get() const { return innerAddress; };
    const std::string hex() const { return Utils::bytesToHex(innerAddress); }
    dev::h160 toHash() const {
      return dev::h160(innerAddress, dev::FixedHash<20>::ConstructFromStringType::FromBinary);
    }
    void operator=(const std::string_view& address) { this->innerAddress = address; }
    void operator=(const std::string& address) { this->innerAddress = address; }
    void operator=(const Address& address) { this->innerAddress = address.innerAddress; }
    void operator=(const dev::h160 &address) { this->innerAddress = address.byteStr(); }
    void operator=(const uint160_t &address) { this->innerAddress = Utils::uint160ToBytes(address); }
    Address& operator=(Address&& other) {
      this->innerAddress = std::move(other.innerAddress);
      return *this;
    }
    bool operator==(const Address& rAddress) const { return bool(innerAddress == rAddress.innerAddress); }
    bool operator!=(const Address& rAddress) const { return bool(innerAddress != rAddress.innerAddress); }
};

template <> struct std::hash<Address> {
  size_t operator() (const Address& address) const {
    return std::hash<std::string>()(address.get());
  }
};

// std::unordered_map uses uint64_t hashes, if we keep the hash the same accross multiple nodes, a issue appears
// hash collisions are possible by having many Accounts and them being distributed in a way that they have the same hash accross all nodes.
// this is a workaround for that issue, it's not perfect because it is still uint64_t but it's better than nothing now that nodes keeps different hashes.
struct SafeHash {
  static uint64_t splitmix(uint64_t x) {
    // http://xorshift.di.unimi.it/splitmix64.c
    x += 0x9e3779b97f4a7c15;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return x ^ (x >> 31);
  }

  size_t operator()(uint64_t x) const {
    static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
    return splitmix(x + FIXED_RANDOM);
  }

  size_t operator()(const Address& address) const {
    static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::string>()(address.get()) + FIXED_RANDOM);
  }

  size_t operator()(const std::string& str) const {
    static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::string>()(str) + FIXED_RANDOM);
  }

  template<typename T>
  size_t operator()(const std::shared_ptr<T> &ptr) const {
    static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::shared_ptr<T>>()(ptr) + FIXED_RANDOM);
  }

  template<unsigned N>
  size_t operator()(const StringContainer<N> &strContainer) const {
    static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
    return splitmix(std::hash<std::string>()(strContainer.get()) + FIXED_RANDOM);
  }
};

#endif // UTILS_H
