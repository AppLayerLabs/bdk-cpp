#ifndef STRINGS_H
#define STRINGS_H

#include <string>

#include "utils.h"

template <unsigned N> class FixedStr {
  protected:
    std::string data;
  public:
    inline FixedStr() { this->data.resize(N, 0x00); };
    FixedStr(const std::string_view& data);
    FixedStr(std::string&& data);
    FixedStr(const FixedStr& other);
    FixedStr(FixedStr&& other);
    inline const std::string get() { return this->data; }
    inline const char* raw() { return this->data.data(); }
    inline const std::string hex() { return Utils::bytesToHex(this->data); }
    const std::string_view view(const size_t& size = N, const size_t& offset = 0);
    inline bool empty() { return this->data.empty(); }
    inline size_t size() { return this->data.size(); }
    const std::string::const_iterator begin() const { return this->data.cbegin(); }
    const std::string::const_iterator end() const { return this->data.cend(); }
    inline bool operator==(const FixedStr& other) { return (this->data == other.data); }
    inline bool operator!=(const FixedStr& other) { return (this->data != other.data); }
    inline bool operator<(const FixedStr& other) { return (this->data < other.data); }
    inline bool operator>=(const FixedStr& other) { return (this->data >= other.data); }
    inline bool operator<=(const FixedStr& other) { return (this->data <= other.data); }
    inline bool operator>(const FixedStr& other) { return (this->data > other.data); }
    inline FixedStr& operator=(const FixedStr& str) {
      if (&str != this) this->data = str.data; return *this;
    }
    inline FixedStr& operator=(FixedStr&& str) {
      if (&str != this) this->data = std::move(str.data); return *this;
    }
    inline char& operator[](const size_t pos) { return data[pos]; }
    inline const char& operator[](const size_t pos) { return data[pos]; }
};

class Hash : public FixedStr<32> {
  public:
    using FixedStr<32>::FixedStr; // Parent constructor
    using FixedStr<32>::operator=; // Parent operator=
    Hash(uint256_t data) : FixedStr<32>(Utils::uint256ToBytes(data)) {};
    inline const uint256_t toUint256() { return Utils::bytesToUint256(data); }
    static Hash random();
};

class Signature : public FixedStr<65> {
  public:
    using FixedStr<65>::FixedStr; // Parent constructor
    inline FixedStr<32> r() { return FixedStr<32>(this->data.substr(0, 32)); };
    inline FixedStr<32> s() { return FixedStr<32>(this->data.substr(32, 32)); };
    inline FixedStr<1> v() { return FixedStr<1>(this->data.substr(64, 1)); };
};

class Address : public FixedStr<20> {
  public:
    Address(const std::string& add, bool fromRPC);
    Address(std::string&& add, bool fromRPC);
    inline const dev::h160 toHash() {
      return dev::h160(this->data, dev::FixedHash<20>::ConstructFromStringType::FromBinary);
    }
};

#endif  // STRINGS_H
