/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "strings.h"
#include "utils.h"

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

