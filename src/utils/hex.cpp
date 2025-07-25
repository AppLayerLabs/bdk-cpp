/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "hex.h"

#include "dynamicexception.h"

Hex::Hex(const std::string_view value, bool strict) : strict_(strict) {
  std::string ret(value);
  if (strict) {
    if (ret[0] != '0' && (ret[1] != 'x' || ret[1] != 'X')) ret.insert(0, "0x");
  } else {
    if (ret[0] == '0' && (ret[1] == 'x' || ret[1] == 'X')) ret.erase(0, 2);
  }
  if (ret[1] == 'X') ret[1] = 'x'; // Always fix "0X" back to "0x"
  if (!Hex::isValid(ret, strict)) throw DynamicException("Invalid Hex string at constructor");
  this->hex_ = std::move(ret);
}

Hex::Hex(std::string&& value, bool strict) : hex_(std::move(value)), strict_(strict) {
  if (strict) {
    if (this->hex_[0] != '0' && (this->hex_[1] != 'x' || this->hex_[1] != 'X')) {
      this->hex_.insert(0, "0x");
    }
  } else {
    if (this->hex_[0] == '0' && (this->hex_[1] == 'x' || this->hex_[1] == 'X')) {
      this->hex_.erase(0, 2);
    }
  }
  if (this->hex_[1] == 'X') this->hex_[1] = 'x'; // Always fix "0X" back to "0x"
  if (!Hex::isValid(this->hex_, strict)) throw DynamicException("Invalid Hex string at constructor");
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

Hex Hex::fromBytes(View<Bytes> bytes, bool strict) {
  auto beg = bytes.begin();
  auto end = bytes.end();
  static const char* digits = "0123456789abcdef";
  size_t off = strict ? 2 : 0;
  std::string hex(std::distance(beg, end) * 2 + off, '0');
  hex.replace(0, 2, "0x");
  for (; beg != end; beg++) {
    hex[off] = digits[(*beg >> 4) & 0x0f];
    off++;
    hex[off] = digits[*beg & 0x0f];
    off++;
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
  std::string retHex = this->hex_;
  if (retHex[1] == 'X') retHex[1] = 'x';  // Always fix "0X" back to "0x"
  if (retHex[0] != '0' && retHex[1] != 'x') retHex.insert(0, "0x");
  if (retHex == "0x") { retHex ="0x0"; return retHex; }
  // Check for leading zeroes!
  size_t i = 2;
  while (retHex[i] == '0') retHex.erase(i, 1);
  return retHex;
};

Bytes Hex::toBytes(const std::string_view hex) {
  Bytes ret;
  std::string hexStr(hex);
  if (hexStr[1] == 'X') hexStr[1] = 'x'; // Always fix "0X" back to "0x"
  size_t i = (hexStr.starts_with("0x")) ? 2 : 0;
  const static std::string_view filter("0123456789abcdefABCDEF");
  if (auto pos = hexStr.find_first_not_of(filter, i); pos != std::string::npos) {
    throw DynamicException(std::string(__func__) + ": Invalid hex string: "
      + hexStr + " filter: " + std::string(filter) + " at pos: " + std::to_string(pos));
  }
  if (hexStr.size() % 2) {
    int h = Hex::toInt(hexStr[i]);
    i++;
    ret.emplace_back(uint8_t(h));
  }
  for (; i < hexStr.size(); i += 2) {
    int h = Hex::toInt(hexStr[i]);
    int l = Hex::toInt(hexStr[i + 1]);
    ret.emplace_back(uint8_t(h * 16 + l));
  }
  return ret;
}

Bytes Hex::bytes() const {
  // Parse two by two chars until the end
  Bytes ret;
  uint32_t i = (this->strict_) ? 2 : 0; // Strict offset ("0x")
  if (this->hex_.size() % 2) {
    int h = Hex::toInt(this->hex_[i]);
    i++;
    ret.emplace_back(uint8_t(h));
  }
  for (; i < this->hex_.size(); i += 2) {
    int h = Hex::toInt(this->hex_[i]);
    int l = Hex::toInt(this->hex_[i + 1]);
    ret.emplace_back(uint8_t(h * 16 + l));
  }
  return ret;
}

Hex& Hex::operator+=(const std::string& hex) {
  bool strict = (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'));
  if (Hex::isValid(hex, strict)) {
    this->hex_ += strict ? hex.substr(2) : hex;
    if (this->hex_[1] == 'X') this->hex_[1] = 'x';  // Always fix "0X" back to "0x"
  } else {
    throw DynamicException("Invalid Hex concat operation");
  }
  return *this;
}

Hex& Hex::operator+=(const Hex& other) {
  this->hex_ += (other.strict_) ? other.hex_.substr(2) : other.hex_;
  return *this;
}

