/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_VARIADICPARSER_H
#define JSONRPC_VARIADICPARSER_H

#include "parser.h"

namespace jsonrpc {

template<typename T, typename... Ts>
struct VariadicParser {
  std::tuple<T, Ts...> operator()(const json& data) const {
    if (data.is_array()) return (*this)(data.begin(), data.end());
    throw Error::insufficientValues();
  }

  template<typename It>
  inline std::tuple<T, Ts...> operator()(It it, It end) const {
    if (it == end) throw Error::insufficientValues();
    return std::tuple_cat(VariadicParser<T>{}(it, end), VariadicParser<Ts...>{}(std::next(it), end));
  }
};

template<typename T>
struct VariadicParser<T> {
  std::tuple<T> operator()(const json& data) const {
    if (data.is_array()) return (*this)(data.begin(), data.end());
    if (data.is_object()) return std::make_tuple(Parser<T>{}(data));
    throw Error::invalidType("object or array", data.type_name());
  }

  template<typename It>
  std::tuple<T> operator()(It it, It end) const {
    if (it == end) throw Error::insufficientValues();
    return std::make_tuple(Parser<T>{}(*it));
  }
};

template<typename T>
struct VariadicParser<std::optional<T>> {
  std::tuple<std::optional<T>> operator()(const json& data) const {
    if (data.is_array()) return (*this)(data.begin(), data.end());
    if (data.is_object()) return std::make_tuple(Parser<T>{}(data));
    throw Error::invalidType("object or array", data.type_name());
  }

  template<typename It>
  std::tuple<std::optional<T>> operator()(It it, It end) const {
    if (it == end) return std::make_tuple(std::nullopt);
    return std::make_tuple(std::make_optional(Parser<T>{}(*it)));
  }
};

template<typename T, typename U>
struct VariadicParser<T, std::optional<U>> {
  std::tuple<T, std::optional<U>> operator()(const json& data) const {
    if (data.is_array()) return (*this)(data.begin(), data.end());
    if (data.is_object()) return std::make_tuple(Parser<T>{}(data), std::optional<U>(std::nullopt));
    throw Error::invalidType("object or array", data.type_name());
  }

  template<typename It>
  std::tuple<T, std::optional<U>> operator()(It it, It end) const {
    if (it == end) throw Error::insufficientValues();
    std::tuple<T, std::optional<U>> res(Parser<T>{}(*it++), std::optional<U>());
    if (it != end) std::get<1>(res) = Parser<U>{}(*it);
    return res;
  }
};

/// @brief parses the json array or object to a tuple of the given types
/// @tparam ...Ts the target types
/// @param data the json array or object
/// @return a tuple containing the parsed json elements
/// @throws Error if the parsing of any element throws an exception
/// @throws Error if the json array does not have sufficient values
/// @note ideally, the number of elements in the json matches the number of template parameters,
///       but the json can contain more elements which will be ignored
/// @note the last template parameter can be a std::optional type which will only be parsed
///       if the json array contain enough elements, otherwise an empty optional will be returned
template<typename... Ts>
inline std::tuple<Ts...> parseAll(const json& data) {
  return VariadicParser<Ts...>{}(data);
}

/// @brief utility function for the common task of parsing the "params" field that
///        contains an array of values to be parsed
template<typename... Ts>
inline std::tuple<Ts...> parseAllParams(const json& target) {
  if (!target.contains("params")) throw DynamicException("\"params\" not available in json");
  return parseAll<Ts...>(target["params"]);
}

} // namespace jsonrpc

#endif // JSONRPC_VARIADICPARSER_H
