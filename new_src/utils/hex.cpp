#include "hex.h"
#include "utils.h"

Hex::Hex(std::string&& value, bool strict) : hex(std::move(value)), strict(strict) {
  isHexValid();
  if (strict) {
    if (this->hex[0] != '0' && (this->hex[1] != 'x' || this->hex[1] != 'X')) {
      this->hex.insert(0, "0x");
    }
  } else {
    if (this->hex[0] == '0' && (this->hex[1] == 'x' || this->hex[1] == 'X')) {
      this->hex.erase(0, 2);
    }
  }
  for (auto& c : this->hex) if (std::isupper(c)) c = std::tolower(c);
}

Hex::Hex(const std::string_view& value, bool strict) : strict(strict) {
  isHexValid(value);
  std::string ret;
  uint64_t i = (strict && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) ? 2 : 0;
  for (; i < value.size(); i++) {
    ret += (std::isupper(value[i])) ? std::tolower(value[i]) : value[i];
  }
  this->hex = ret;
}

bool Hex::isHexValid(const std::string_view& v) const {
  std::string hex = (v.empty()) ? this->hex : v.data();
  if (strict && hex.substr(0, 2) != "0x" && hex.substr(0, 2) != "0X") {
    throw std::runtime_error(std::string("Error at --> Hex::isHexValid():\r\n")
      + "Strict mode requires prefix \"0x\" in \r\n\"" + hex + "\""
    );
  }

  size_t npos = hex.find_first_not_of(filter);
  if (npos != std::string::npos) {
    throw std::runtime_error(std::string("Error at --> Hex::isHexValid():\r\n")
      + "Invalid character \"" + hex.at(npos)
      + "\", at position: " + std::to_string(npos) + "\r\n"
      + "Ref (Hex::hex: \"" + hex + "\")."
    );
  }

  return true;
}

Hex Hex::fromBytes(std::string_view bytes, bool strict) {
  auto it = bytes.begin();
  auto end = bytes.end();
  static const char* digits = "0123456789abcdef";
  size_t off = 0;
  std::string hex(std::distance(it, end) * 2, '0');
  for (; it != end; it++) {
    hex[off++] = digits[(*it >> 4) & 0x0f];
    hex[off++] = digits[*it & 0x0f];
  }
  return Hex(std::move(hex), strict);
}

Hex Hex::fromUTF8(std::string_view bytes, bool strict) {
  std::stringstream ss;
  for (int i = 0; i < bytes.length(); i++) {
    // You need two casts in order to properly cast char to uint
    ss << std::hex << std::setfill('0') << std::setw(2)
      << static_cast<uint>(static_cast<uint8_t>(bytes[i]));
  }
  return Hex(std::move(ss.str()), strict);
}

std::string Hex::bytes() const {
  std::string ret;

  // Parse two by two chars until the end
  uint32_t i = (this->hex.size() % 2 != 0); // Odd offset ("0xaaa")
  for (; i < this->hex.size(); i += 2) {
    int h = Utils::hexCharToInt(this->hex[i]);
    int l = Utils::hexCharToInt(this->hex[i + 1]);
    if (h != -1 && l != -1) {
      ret += (char) uint8_t(h * 16 + l);
    } else {
      throw std::runtime_error(std::string(__func__) + ": " +
        std::string("One or more invalid hex chars: ") + this->hex[i] + this->hex[i + 1]
      );
    }
  }

  return ret;
}

