/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "strings.h"
#include "utils.h"

Hash::Hash(const uint256_t& data) : FixedBytes<32>(Utils::uint256ToBytes(data)) {};

Hash::Hash(const std::string_view sv) {
  if (sv.size() != 32) throw std::invalid_argument("Hash must be 32 bytes long.");
  std::ranges::copy(sv, this->begin());
}

Hash::Hash(const evmc::bytes32& data) {
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy(data.bytes, data.bytes + 32, this->begin());
}

uint256_t Hash::toUint256() const { return Utils::bytesToUint256(*this); }

evmc::bytes32 Hash::toEvmcBytes32() const {
  evmc::bytes32 bytes;
  std::memcpy(bytes.bytes, this->data(), 32);
  return bytes;
}

uint256_t Signature::r() const { return Utils::bytesToUint256(this->view(0, 32)); }

uint256_t Signature::s() const { return Utils::bytesToUint256(this->view(32, 32)); }

uint8_t Signature::v() const { return Utils::bytesToUint8(this->view(64, 1)); }

Address::Address(const std::string_view add, bool inBytes) {
  if (inBytes) {
    if (add.size() != 20) throw std::invalid_argument("Address must be 20 bytes long.");
    std::ranges::copy(add, this->begin());
  } else {
    if (!Address::isValid(add, false)) throw std::invalid_argument("Invalid Hex address.");
    auto bytes = Hex::toBytes(add);
    std::ranges::copy(bytes, this->begin());
  }
}

Address::Address(const evmc::address& data) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy(data.bytes, data.bytes + 20, this->begin());
}

evmc::address Address::toEvmcAddress() const {
  evmc::address addr;
  std::ranges::copy(*this, addr.bytes);
  return addr;
}

Address::Address(const evmc_address &data) {
  // Same as evmc::address
  std::copy(data.bytes, data.bytes + 20, this->begin());
}

Hex Address::toChksum() const {
  // Hash requires lowercase address without "0x"
  std::string str = Hex::fromBytes(*this, false).get();
  Hex hash = Utils::sha3(Utils::create_view_span(str)).hex();
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
  return (add == std::string_view(myAdd.toChksum()));
}

StorageKey::StorageKey(const evmc::address& addr, const evmc::bytes32& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.bytes, 20, this->begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.bytes,  32, this->begin() + 20);
}

StorageKey::StorageKey(const evmc_address& addr, const evmc_bytes32& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.bytes, 20, this->begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.bytes,  32, this->begin() + 20);
}

StorageKey::StorageKey(const evmc_address& addr, const evmc::bytes32& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.bytes, 20, this->begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.bytes,  32, this->begin() + 20);
}

StorageKey::StorageKey(const evmc::address& addr, const evmc_bytes32& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.bytes, 20, this->begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.bytes,  32, this->begin() + 20);
}

StorageKey::StorageKey(const Address& addr, const Hash& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.cbegin(), 20, this->begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.cbegin(),  32, this->begin() + 20);
}

