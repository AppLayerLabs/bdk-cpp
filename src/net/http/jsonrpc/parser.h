#ifndef JSONRPC_PARSER_H
#define JSONRPC_PARSER_H

#include "error.h"
#include "../../../utils/utils.h"
#include "../../../utils/strings.h"
#include "../../../utils/tx.h"
#include "../../../contract/contract.h"
#include "../../../contract/event.h"

#include <ranges>

namespace jsonrpc {

/// @brief base template class for a json parser
template<typename T> struct Parser { T operator()(const json& target) const = delete; };

/// @brief identity json parser for json types
template<> struct Parser<json> { json operator()(const json& data) const { return data; } };

/// @brief parses a json string in hexadecimal to a Hash type
template<> struct Parser<Hash> { Hash operator()(const json& data) const; };

/// @brief parses a json string in hexadecimal to a Address type
template<> struct Parser<Address> { Address operator()(const json& data) const; };

/// @brief parses a json string in hexadecimal to a Bytes type
template<> struct Parser<Bytes> { Bytes operator()(const json& data) const; };

/// @brief parses a json boolean to a bool type if valid
template<> struct Parser<bool> { bool operator()(const json& data) const; };

/// @brief parses a hexadecimal encoded json string to an unsigned integer
template<> struct Parser<uint64_t> { uint64_t operator()(const json& data) const; };

/// @brief partial specialization to optionally parse a json field
template<typename T>
struct Parser<std::optional<T>> {

  /// @brief parses a json object if not null
  /// @param data the json object to be parsed
  /// @return an optional containing the parsed json, or an empty optional if the json is null
  std::optional<T> operator()(const json& data) const {
    if (data.is_null())
      return std::nullopt;

    return Parser<T>{}(data);
  }
};

/// @brief partial specialization for variants with a single type
template<typename T>
struct Parser<std::variant<T>> {
  std::variant<T> operator()(const json& data) const {
    return Parser<T>{}(data);
  }
};

/// @brief partial specialization for variants with multiple types
template<typename T, typename... Ts>
struct Parser<std::variant<T, Ts...>> {

  /// @brief parses the json data as one of the variant options, trying the left-most type first
  /// @param data the json to be parsed
  /// @return the json parsed as one of the variant types
  std::variant<T, Ts...> operator()(const json& data) const {
    try {
      return Parser<T>{}(data);
    } catch (const Error& ignored) {
      std::variant<T, Ts...> res;

      std::visit([&res] (auto&& a) {
        res = std::forward<decltype(a)>(a);
      }, Parser<std::variant<Ts...>>{}(data));

      return res;
    }
  }
};

/// @brief parses a json object to the given type
/// @tparam T the target parsing type
/// @param data the json object to be parsed
/// @return the parsed object
/// @throws Error if the json can not be parsed to the specified type
template<typename T>
T parse(const json& data) {
  return Parser<T>{}(data);
}

/// @brief parses a json field (by integer position or string key) if it exists and it's not null
/// @tparam T the target parsing type
/// @param data the json object to be parsed
/// @return an empty optional if the key does not exists or the field is null,
///         otherwise an optional containing the parsed result
/// @throws Error if the json can not be parsed to the specified type
///         due to incorrect type or format
template<typename T, typename K>
std::optional<T> parseIfExists(const json& data, const K& key) {
  if (!data.contains(key))
    return std::nullopt;

  return parse<std::optional<T>>(data.at(key));
}

/// @brief parses a json array into a range of the specified type
/// @tparam T the target parsing type for the array elements
/// @param data the json object to be parsed
/// @return an iterable range view with the parsed elements
/// @throws Error if the json can not be parsed to the specified type
///         due to incorrect type or format
template<typename T>
auto parseArray(const json& data) {
  if (!data.is_array())
      throw Error::invalidType("array", data.type_name());

  return data | std::views::transform([] (const json& elem) -> T { return Parser<T>{}(elem); });
}

/// @brief parses a json array field if existent and not null
/// @tparam T the target parsing type for the array elements
/// @param data the json object to be parsed
/// @return an empty optional if the field does not exist or its value is null,
///         otherwise, an optional containing thje iterable range view with the parsed elements
/// @throws Error if the json can not be parsed to the specified type
///         due to incorrect type or format
template<typename T, typename K>
auto parseArrayIfExists(const json& data, const K& key) -> std::optional<std::invoke_result_t<decltype(parseArray<T>), const json&>> {
  if (!data.contains(key))
    return std::nullopt; 

  return parseArray<T>(data);
}

} // namespace jsonrpc

#endif // JSONRPC_PARSER_H
