/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "hex.h"

#include "dynamicexception.h"

Hex::Hex(std::string_view value, bool strict) : strict_(strict) {
  std::string s(value);
  normalize(s, strict);
  if (!Hex::isValid(s, strict)) throw DynamicException("Invalid Hex string at constructor");
  hex_ = std::move(s);
}

Hex::Hex(std::string&& value, bool strict) : hex_(std::move(value)), strict_(strict) {
  normalize(hex_, strict_);
  if (!Hex::isValid(hex_, strict_)) throw DynamicException("Invalid Hex string at constructor");
}

bool Hex::isValid(const std::string_view hex, bool strict) {
  int off = 0;
  if (strict) {
    if (!hex.starts_with("0x") && !hex.starts_with("0X")) return false;
    off = 2;
  }
  if (
    const static std::string_view filter("0123456789abcdefABCDEF");
    hex.find_first_not_of(filter, off) != std::string::npos
  ) return false;
  return true;
}

Hex Hex::fromBytes(View<Bytes> bytes, bool strict, bool upper) {
  auto beg = bytes.begin();
  auto end = bytes.end();
  static const char* digits_lower = "0123456789abcdef";
  static const char* digits_upper = "0123456789ABCDEF";
  const char* digits = upper ? digits_upper : digits_lower;
  size_t off = strict ? 2 : 0;
  std::string hex(bytes.size() * 2 + (strict ? 2 : 0), '0');
  std::size_t pos = 0;
  if (strict) {
    hex[0] = '0';
    hex[1] = 'x';
    pos = 2;
  }
  for (auto b : bytes) {
    hex[pos++] = digits[(b >> 4) & 0x0f];
    hex[pos++] = digits[b & 0x0f];
  }
  return Hex(std::move(hex), strict);
}

Hex Hex::fromUTF8(std::string_view str, bool strict) {
  std::stringstream ss;
  if (strict) ss << "0x";
  for (int i = 0; i < str.length(); i++) {
    // We need two casts in order to properly cast char to uint
    ss << std::hex << std::setfill('0') << std::setw(2)
      << static_cast<uint>(static_cast<uint8_t>(str[i]));
  }
  return Hex(ss.str(), strict);
}

std::string Hex::forRPC() const {
  std::string s = hex_;
  const bool hasPrefix = s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X');
  if (!hasPrefix) s.insert(0, "0x");
  if (s.size() >= 2 && s[1] == 'X') s[1] = 'x';
  std::size_t i = 2;
  while (i + 1 < s.size() && s[i] == '0') {
    s.erase(i, 1);
  }
  if (s.size() == 2) s.push_back('0');
  return s;
}

Bytes Hex::toBytes(std::string_view hex) {
  Bytes ret;
  std::string s(hex);

  const bool hasPrefix =
      s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X');

  if (hasPrefix && s[1] == 'X') s[1] = 'x';

  const std::size_t i0 = hasPrefix ? 2 : 0;
  const std::size_t payloadLen = s.size() - i0;

  static constexpr std::string_view filter = "0123456789abcdefABCDEF";
  if (auto pos = s.find_first_not_of(filter, i0); pos != std::string::npos) {
    throw DynamicException(std::string(__func__) + ": Invalid hex string: " +
                           s + " at pos: " + std::to_string(pos));
  }
  std::size_t i = i0;
  if (payloadLen % 2 == 1) {
    ret.emplace_back(static_cast<uint8_t>(Hex::toInt(s[i++])));
  }

  for (; i + 1 < s.size(); i += 2) {
    int h = Hex::toInt(s[i]);
    int l = Hex::toInt(s[i + 1]);
    ret.emplace_back(static_cast<uint8_t>((h << 4) | l));
  }
  return ret;
}


Bytes Hex::bytes() const {
  Bytes ret;

  const std::size_t off = strict_ ? 2 : 0;
  if (hex_.size() < off) return ret;              // or throw, depending on your rules

  const std::size_t n = hex_.size() - off;        // payload length
  std::size_t i = off;

  // if payload length is odd, consume first nibble
  if (n % 2 == 1) {
    ret.emplace_back(static_cast<uint8_t>(Hex::toInt(hex_[i++])));
  }

  for (; i + 1 < hex_.size(); i += 2) {           // ensure i and i+1 are valid
    int h = Hex::toInt(hex_[i]);
    int l = Hex::toInt(hex_[i + 1]);
    ret.emplace_back(static_cast<uint8_t>((h << 4) | l));
  }

  return ret;
}

Hex& Hex::operator+=(const std::string& hex) {
  const bool incomingStrict =
      hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X');

  if (!Hex::isValid(hex, incomingStrict)) {
    throw DynamicException("Invalid Hex concat operation");
  }
  this->hex_ += incomingStrict ? hex.substr(2) : hex;

  if (this->hex_.size() >= 2 && this->hex_[0] == '0' && this->hex_[1] == 'X') {
    this->hex_[1] = 'x';
  }
  return *this;
}

Hex& Hex::operator+=(const Hex& other) {
  this->hex_ += (other.strict_) ? other.hex_.substr(2) : other.hex_;
  return *this;
}

