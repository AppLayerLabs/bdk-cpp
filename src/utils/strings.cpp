#include "strings.h"
#include "utils.h"

Hash::Hash(uint256_t data) : FixedStr<32>(Utils::uint256ToBytes(data)) {};

const uint256_t Hash::toUint256() const { return Utils::bytesToUint256(data); }

uint256_t Signature::r() const { return Utils::bytesToUint256(this->data.substr(0, 32)); }

uint256_t Signature::s() const { return Utils::bytesToUint256(this->data.substr(32, 32)); }

uint8_t Signature::v() const { return Utils::bytesToUint8(this->data.substr(64, 1)); }

Address::Address(const std::string_view add, bool inBytes) {
  if (inBytes) {
    if (add.size() != 20) throw std::invalid_argument("Address must be 20 bytes long.");
    this->data = add;
  } else {
    if (!Address::isValid(add, false)) throw std::invalid_argument("Invalid Hex address.");
    this->data = std::move(Hex::toBytes(add));
  }
}

Address::Address(std::string&& add, bool inBytes) {
  if (inBytes) {
    if (add.size() != 20) throw std::invalid_argument("Address must be 20 bytes long.");
    this->data = std::move(add);
  } else {
    if (!Address::isValid(add, false)) throw std::invalid_argument("Invalid Hex address.");
    add = Hex::toBytes(add);
    this->data = std::move(add);
  }
}

Hex Address::toChksum() const {
  // Hash requires lowercase address without "0x"
  std::string str = Hex::fromBytes(this->data, false).get();
  Hex hash = Utils::sha3(str).hex();
  for (int i = 0; i < str.length(); i++) {
    if (!std::isdigit(str[i])) {  // Only check letters (A-F)
      // If character hash is 8-F then make it uppercase
      int nibble = std::stoi(hash.substr(i, 1), nullptr, 16);
      str[i] = (nibble >= 8) ? std::toupper(str[i]) : std::tolower(str[i]);
    }
  }
  str.insert(0, "0x");
  return Hex(str, true);
}

bool Address::isValid(const std::string_view add, bool inBytes) {
  if (inBytes) return (add.size() == 20);
  if (add[0] == '0' && (add[1] == 'x' || add[1] == 'X')) {
    return (add.size() == 42 &&
      add.substr(2).find_first_not_of("0123456789abcdefABCDEF") == std::string::npos
    );
  } else {
    return (add.size() == 40 &&
      add.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos
    );
  }
}

bool Address::isChksum(const std::string_view add) {
  Address myAdd(add, false);
  return (add == myAdd.toChksum());
}

