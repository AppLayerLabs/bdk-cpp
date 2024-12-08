/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_VARIADICPARSER_H
#define JSONRPC_VARIADICPARSER_H

#include "parser.h"

namespace jsonrpc {
  /**
   * Variadic parser for multiple arguments of a given type.
   * @tparam T The argument type.
   * @tparam Ts One or more arguments.
   */
  template<typename T, typename... Ts> struct VariadicParser {
    /**
     * Function call operator for a single JSON object.
     * @param data The JSON data object to be parsed.
     * @return A tuple with the parsing results.
     * @throw Error if values are insufficient.
     */
    std::tuple<T, Ts...> operator()(const json& data) const {
      if (data.is_array()) return (*this)(data.begin(), data.end());
      throw Error::insufficientValues();
    }

    /**
     * Function call operator for a range of JSON objects.
     * @tparam It The iterator type for the objects.
     * @param it An iterator to the start of the range.
     * @param end An iterator to the end of the range.
     * @return A tuple with the parsing results.
     * @throw Error if values are insufficient.
     */
    template<typename It> inline std::tuple<T, Ts...> operator()(It it, It end) const {
      if (it == end) throw Error::insufficientValues();
      return std::tuple_cat(VariadicParser<T>{}(it, end), VariadicParser<Ts...>{}(std::next(it), end));
    }
  };

  /**
   * Variadic parser for a single argument of a given type.
   * @tparam T The argument type.
   */
  template<typename T> struct VariadicParser<T> {
    /**
     * Function call operator for a single JSON object.
     * @param data The JSON data object to be parsed.
     * @return A tuple with the parsing results.
     * @throw Error if value has an invalid type.
     */
    std::tuple<T> operator()(const json& data) const {
      if (data.is_array()) return (*this)(data.begin(), data.end());
      if (data.is_object()) return std::make_tuple(Parser<T>{}(data));
      throw Error::invalidType("object or array", data.type_name());
    }

    /**
     * Function call operator for a range of JSON objects.
     * @tparam It The iterator type for the objects.
     * @param it An iterator to the start of the range.
     * @param end An iterator to the end of the range.
     * @return A tuple with the parsing results.
     * @throw Error if values are insufficient.
     */
    template<typename It> std::tuple<T> operator()(It it, It end) const {
      if (it == end) throw Error::insufficientValues();
      return std::make_tuple(Parser<T>{}(*it));
    }
  };

  /**
   * Variadic parser for a single optional argument of a given type.
   * @tparam T The argument type.
   */
  template<typename T> struct VariadicParser<std::optional<T>> {
    /**
     * Function call operator for a single JSON object.
     * @param data The JSON data object to be parsed.
     * @return A tuple with the parsing results.
     * @throw Error if value has an invalid type.
     */
    std::tuple<std::optional<T>> operator()(const json& data) const {
      if (data.is_array()) return (*this)(data.begin(), data.end());
      if (data.is_object()) return std::make_tuple(Parser<T>{}(data));
      throw Error::invalidType("object or array", data.type_name());
    }

    /**
     * Function call operator for a range of JSON objects.
     * @tparam It The iterator type for the objects.
     * @param it An iterator to the start of the range.
     * @param end An iterator to the end of the range.
     * @return A tuple with the parsing results.
     */
    template<typename It> std::tuple<std::optional<T>> operator()(It it, It end) const {
      if (it == end) return std::make_tuple(std::nullopt);
      return std::make_tuple(std::make_optional(Parser<T>{}(*it)));
    }
  };

  /**
   * Variadic parser for a given type and another optional argument.
   * @tparam T The argument type.
   * @tparam U The other argument.
   */
  template<typename T, typename U> struct VariadicParser<T, std::optional<U>> {
    /**
     * Function call operator for a single JSON object.
     * @param data The JSON data object to be parsed.
     * @return A tuple with the parsing results.
     * @throw Error if value has an invalid type.
     */
    std::tuple<T, std::optional<U>> operator()(const json& data) const {
      if (data.is_array()) return (*this)(data.begin(), data.end());
      if (data.is_object()) return std::make_tuple(Parser<T>{}(data), std::optional<U>(std::nullopt));
      throw Error::invalidType("object or array", data.type_name());
    }

    /**
     * Function call operator for a range of JSON objects.
     * @tparam It The iterator type for the objects.
     * @param it An iterator to the start of the range.
     * @param end An iterator to the end of the range.
     * @return A tuple with the parsing results.
     */
    template<typename It> std::tuple<T, std::optional<U>> operator()(It it, It end) const {
      if (it == end) throw Error::insufficientValues();
      std::tuple<T, std::optional<U>> res(Parser<T>{}(*it++), std::optional<U>());
      if (it != end) std::get<1>(res) = Parser<U>{}(*it);
      return res;
    }
  };

  /**
   * Parse the JSON array or object to a tuple of the given types.
   * @tparam Ts the target types.
   * @param data The JSON array or object.
   * @return A tuple containing the parsed JSON elements.
   * @throw Error if the parsing of any element throws an exception.
   * @throw Error if the JSON array does not have sufficient values.
   * @note Ideally, the number of elements in the json matches the number of template parameters,
   *       but the JSON can contain more elements which will be ignored.
   * @note The last template parameter can be a std::optional type which will only be parsed
   *       if the JSON array contain enough elements, otherwise an empty optional will be returned.
   */
  template<typename... Ts> inline std::tuple<Ts...> parseAll(const json& data) {
    return VariadicParser<Ts...>{}(data);
  }

  /**
   * Utility function for the common task of parsing the "params" field that contains an array of values to be parsed.
   * @param target The JSON object to parse.
   * @return A tuple containing the parsed JSON elements.
   * @throw DynamicException if the "params" field does not exist inside the JSON.
   */
  template<typename... Ts> inline std::tuple<Ts...> parseAllParams(const json& target) {
    if (!target.contains("params")) throw DynamicException("\"params\" not available in json");
    return parseAll<Ts...>(target["params"]);
  }
} // namespace jsonrpc

#endif // JSONRPC_VARIADICPARSER_H
