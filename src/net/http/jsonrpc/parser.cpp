/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "parser.h"

static inline const std::regex hashFormat{"^0x[0-9a-f]{64}$"};
static inline const std::regex addressFormat{"^0x[0-9,a-f,A-F]{40}$"};
static inline const std::regex numberFormat{"^0x([1-9A-Fa-f]+[0-9A-Fa-f]*|0)$"};

namespace jsonrpc {
  Hash Parser<Hash>::operator()(const json& data) const {
    if (!data.is_string()) throw Error::invalidType("string", data.type_name());
    std::string rawData = data.get<std::string>();
    if (!std::regex_match(rawData, hashFormat)) throw Error::invalidFormat(rawData);
    return Hash(Hex::toBytes(rawData));
  }

  Address Parser<Address>::operator()(const json& data) const {
    if (!data.is_string()) throw Error::invalidType("string", data.type_name());
    std::string rawData = data.get<std::string>();
    if (!std::regex_match(rawData, addressFormat)) throw Error::invalidFormat(rawData);
    return Address(Hex::toBytes(rawData));
  }

  Bytes Parser<Bytes>::operator()(const json& data) const {
    if (!data.is_string()) throw Error::invalidType("string", data.type_name());
    std::string rawData = data.get<std::string>();
    if (!Hex::isValid(rawData, true)) throw Error::invalidFormat(rawData);
    return Hex::toBytes(rawData);
  }

  bool Parser<bool>::operator()(const json& data) const {
    if (!data.is_boolean()) throw Error::invalidType("boolean", data.type_name());
    return data.get<bool>();
  }

  float Parser<float>::operator()(const json& data) const {
    if (!data.is_number()) throw Error::invalidType("number", data.type_name());
    return data.get<float>();
  }

  uint64_t Parser<uint64_t>::operator()(const json& data) const {
    if (data.is_number_unsigned()) return data.get<uint64_t>();
    if (!data.is_string()) throw Error::invalidType("string", data.type_name());
    auto value = data.get<std::string>();
    if (std::regex_match(value, numberFormat)) return uint64_t(Hex(value).getUint());
    // Check if the string is a valid number
    if (value.empty()) throw Error::invalidFormat("empty string");
    for (const auto& c : value) if (!std::isdigit(c)) throw Error::invalidFormat(value);
    return std::stoull(value);
  }

  uint256_t Parser<uint256_t>::operator()(const json& data) const {
    if (data.is_number_unsigned())
      return uint256_t(data.get<std::string>());

    if (!data.is_string())
      throw Error::invalidType("string", data.type_name());

    auto value = data.get<std::string>();

    if (!std::regex_match(value, numberFormat))
      throw Error::invalidFormat(value);

    return Hex(value).getUint();
  }
  

  std::string Parser<std::string>::operator()(const json& data) const {
    if (!data.is_string()) throw Error::invalidType("string", data.type_name());
    return data.get<std::string>();
  }
} // namespace jsonrpc

