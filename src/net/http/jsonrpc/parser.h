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
  
template<typename T> struct Parser { T operator()(const json& target) const = delete; };

template<> struct Parser<json> { json operator()(const json& data) const { return data; } };

template<> struct Parser<Hash> { Hash operator()(const json& data) const; };

template<> struct Parser<Address> { Address operator()(const json& data) const; };

template<> struct Parser<Bytes> { Bytes operator()(const json& data) const; };

template<> struct Parser<bool> { bool operator()(const json& data) const; };

template<> struct Parser<uint64_t> { uint64_t operator()(const json& data) const; };

template<typename T>
struct Parser<std::variant<T>> {
  std::variant<T> operator()(const json& data) const {
    return Parser<T>{}(data);
  }
};

template<typename T>
struct Parser<std::optional<T>> {
  std::optional<T> operator()(const json& data) const {
    if (data.is_null())
      return std::nullopt;

    return Parser<T>{}(data);
  }
};

template<typename T, typename... Ts>
struct Parser<std::variant<T, Ts...>> {
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

template<typename T>
T parse(const json& data) {
  return Parser<T>{}(data);
}

template<typename T, typename K>
std::optional<T> parseIfExists(const json& target, const K& key) {
  if (!target.contains(key))
    return std::nullopt;

  return parse<std::optional<T>>(target.at(key));
}

template<typename T>
auto parseArray(const json& data) {
  if (!data.is_array())
      throw Error::invalidType("array", data.type_name());

  return data | std::views::transform([] (const json& elem) -> T { return Parser<T>{}(elem); });
}

template<typename T, typename K>
auto parseArrayIfExists(const json& target, const K& key) -> std::optional<std::invoke_result_t<decltype(parseArray<T>), const json&>> {
  if (!target.contains(key))
    return std::nullopt; 

  return parseArray<T>(target);
}

} // namespace jsonrpc

#endif // JSONRPC_PARSER_H
