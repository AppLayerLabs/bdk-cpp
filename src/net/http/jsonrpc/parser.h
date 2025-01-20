/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_PARSER_H
#define JSONRPC_PARSER_H

#include <ranges>

#include "../../../core/storage.h" // utils/safehash.h -> tx.h -> ecdsa.h -> utils.h

#include "error.h"

namespace jsonrpc {
  /**
   * Templated struct for a data parser.
   * @tparam T The data type.
   */
  template<typename T> struct Parser {
    /**
     * Function call operator.
     * @param data The data object to parse.
     * @return The data type resulted from the parsing.
     */
    T operator()(const json& data) const = delete;
  };

  /// Specialization.
  template<> struct Parser<json> {
    json operator()(const json& data) const { return data; }  ///< Function call operator.
  };

  /// Specialization.
  template<> struct Parser<Hash> {
    Hash operator()(const json& data) const;  ///< Function call operator.
  };

  /// Specialization.
  template<> struct Parser<Address> {
    Address operator()(const json& data) const;  ///< Function call operator.
  };

  /// Specialization.
  template<> struct Parser<Bytes> {
    Bytes operator()(const json& data) const;  ///< Function call operator.
  };

  /// Specialization.
  template<> struct Parser<bool> {
    bool operator()(const json& data) const;  ///< Function call operator.
  };

  /// Specialization.
  template<> struct Parser<float> {
    float operator()(const json& data) const;  ///< Function call operator.
  };

  /// Specialization.
  template<> struct Parser<uint64_t> {
    uint64_t operator()(const json& data) const;  ///< Function call operator.
  };

    /// Specialization.
  template<> struct Parser<uint256_t> {
    uint256_t operator()(const json& data) const;  ///< Function call operator.
  };

  /// Partial specialization to optionally parse a json field.
  template<typename T> struct Parser<std::optional<T>> {
    /**
     * Parse a JSON object if not null.
     * @param data The JSON object to be parsed.
     * @return An optional containing the parsed JSON, or an empty optional if the JSON is null.
     */
    std::optional<T> operator()(const json& data) const {
      if (data.is_null()) return std::nullopt;
      return Parser<T>{}(data);
    }
  };

  /// Partial specialization for variants with a single type.
  template<typename T> struct Parser<std::variant<T>> {
    std::variant<T> operator()(const json& data) const { return Parser<T>{}(data); } ///< Function call operator.
  };

  /// Partial specialization for variants with multiple types.
  template<typename T, typename... Ts> struct Parser<std::variant<T, Ts...>> {
    /**
     * Parse the JSON data as one of the variant options, trying the left-most type first.
     * @param data The JSON object to be parsed.
     * @return The JSON parsed as one of the variant types.
     */
    std::variant<T, Ts...> operator()(const json& data) const {
      try {
        return Parser<T>{}(data);
      } catch ([[maybe_unused]] const Error& ignored) {
        std::variant<T, Ts...> res;
        std::visit([&res] (auto&& a) {
          res = std::forward<decltype(a)>(a);
        }, Parser<std::variant<Ts...>>{}(data));
        return res;
      }
    }
  };

  /// Partial specialization for types within containers.
  template<typename T> struct Parser<std::vector<T>> {
    /**
     * Parse the JSON data as one of the containers.
     * @param data The JSON object to be parsed. Can be either an array (e.g.
     *             "[1,2,3]") or an object (e.g. "{{"a", 1}, {"b", 2}, {"c", 3}}").
     *             Object keys are ignored altogether.
     * @return The container with the variant type.
     */
    std::vector<T> operator()(const json& data) const {
      std::vector<T> res;
      if (data.is_array()) {
        res.reserve(data.size());
        std::ranges::transform(data, std::back_inserter(res), [](const json& elem){ return Parser<T>{}(elem); });
      } else if (data.is_object()) {
        res.reserve(data.size());
        for (auto& el : data.items()) res.emplace_back(Parser<T>{}(el.value()));
      } else {
        throw Error::invalidType("array or object", data.type_name());
      }
      return res;
    }
  };

  /**
   * Parse a JSON object to the given type.
   * @tparam T The target parsing type.
   * @param data The JSON object to be parsed.
   * @return The parsed object
   * @throw Error if the JSON can not be parsed to the specified type.
   */
  template<typename T> T parse(const json& data) { return Parser<T>{}(data); }

  /**
   * Parse a JSON field (by integer position or string key) if it exists and it's not null.
   * @tparam T The target parsing type.
   * @param data The JSON object to be parsed.
   * @param key The key to check.
   * @return An empty optional if the key does not exists or the field is null,
   *         otherwise an optional containing the parsed result.
   * @throw Error if the JSON can not be parsed to the specified type due to incorrect type or format.
   */
  template<typename T, typename K> std::optional<T> parseIfExists(const json& data, const K& key) {
    if (!data.contains(key)) return std::nullopt;
    return parse<std::optional<T>>(data.at(key));
  }

  /**
   * Parse a JSON array into a range of the specified type.
   * @tparam T the target parsing type for the array elements.
   * @param data The JSON object to be parsed.
   * @return An iterable range view with the parsed elements.
   * @throw Error if the json can not be parsed to the specified type due to incorrect type or format.
   */
  template<typename T> auto parseArray(const json& data) {
    if (!data.is_array()) throw Error::invalidType("array", data.type_name());
    return data | std::views::transform([] (const json& elem) -> T { return Parser<T>{}(elem); });
  }

  /**
   * Parse a JSON array field if existent and not null.
   * @tparam T The target parsing type for the array elements.
   * @param data The JSON object to be parsed.
   * @param key The key to check.
   * @return An empty optional if the field does not exist or its value is null,
   *         otherwise, an optional containing the iterable range view with the parsed elements.
   * @throw Error if the json can not be parsed to the specified type due to incorrect type or format.
   */
  template<typename T, typename K> auto parseArrayIfExists(const json& data, const K& key)
  -> std::optional<std::invoke_result_t<decltype(parseArray<T>), const json&>> {
    if (!data.contains(key)) return std::nullopt;
    return parseArray<T>(data);
  }

} // namespace jsonrpc

#endif // JSONRPC_PARSER_H
