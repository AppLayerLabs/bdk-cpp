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
  std::copy(sv.begin(), sv.end(), this->data_.begin());
}

Hash::Hash(const evmc::bytes32& data) {
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy(data.bytes, data.bytes + 32, this->data_.begin());
}

uint256_t Hash::toUint256() const { return Utils::bytesToUint256(data_); }

evmc::bytes32 Hash::toEvmcBytes32() const {
  evmc::bytes32 bytes;
  std::memcpy(reinterpret_cast<void*>(bytes.bytes), this->data_.data(), 32);
  return bytes;
}

uint256_t Signature::r() const { return Utils::bytesToUint256(this->view(0, 32)); }

uint256_t Signature::s() const { return Utils::bytesToUint256(this->view(32, 32)); }

uint8_t Signature::v() const { return Utils::bytesToUint8(this->view(64, 1)); }

Address::Address(const std::string_view add, bool inBytes) {
  if (inBytes) {
    if (add.size() != 20) throw std::invalid_argument("Address must be 20 bytes long.");
    std::copy(add.begin(), add.end(), this->data_.begin());
  } else {
    if (!Address::isValid(add, false)) throw std::invalid_argument("Invalid Hex address.");
    auto bytes = Hex::toBytes(add);
    std::copy(bytes.begin(), bytes.end(), this->data_.begin());
  }
}

Address::Address(const evmc::address& data) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy(data.bytes, data.bytes + 20, this->data_.begin());
}

evmc::address Address::toEvmcAddress() const {
  evmc::address addr;
  std::copy(this->data_.begin(), this->data_.end(), addr.bytes);
  return addr;
}

Address::Address(const evmc_address &data) {
  // Same as evmc::address
  std::copy(data.bytes, data.bytes + 20, this->data_.begin());
}

Hex Address::toChksum() const {
  // Hash requires lowercase address without "0x"
  std::string str = Hex::fromBytes(this->data_, false).get();
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
  return (add == myAdd.toChksum());
}

StorageKey::StorageKey(const evmc::address& addr, const evmc::bytes32& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.bytes, 20, this->data_.begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.bytes,  32, this->data_.begin() + 20);
}

StorageKey::StorageKey(const evmc_address& addr, const evmc_bytes32& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.bytes, 20, this->data_.begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.bytes,  32, this->data_.begin() + 20);
}

StorageKey::StorageKey(const evmc_address& addr, const evmc::bytes32& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.bytes, 20, this->data_.begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.bytes,  32, this->data_.begin() + 20);
}

StorageKey::StorageKey(const evmc::address& addr, const evmc_bytes32& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.bytes, 20, this->data_.begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.bytes,  32, this->data_.begin() + 20);
}

StorageKey::StorageKey(const Address& addr, const Hash& slot) {
  // Copy the data from the evmc::address struct to this->data_
  std::copy_n(addr.cbegin(), 20, this->data_.begin());
  // Copy the data from the evmc::bytes32 struct to this->data_
  std::copy_n(slot.cbegin(),  32, this->data_.begin() + 20);
}

