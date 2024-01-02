/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef ABI_H
#define ABI_H

#include <string>
#include <any>

#include "../utils/hex.h"
#include "../libs/json.hpp"
#include "../utils/utils.h"

/// Namespace for Solidity ABI-related operations.
namespace ABI {
  /// Struct for the contract ABI object.
  struct MethodDescription {
    std::string name; ///< Name of the method.
    std::vector<std::pair<std::string,std::string>> inputs; ///< Vector of pairs of input names and types. Types encoded with ABI::FunctorEncoder::listArgumentTypesV,
                                                            ///< if the arg name is missing it will be replaced with an empty string.
                                                            ///< Tuples are encoded as (type1,type2,...,typeN), runtime splitting is required.
    std::vector<std::string> outputs; ///< Vector of output types (there is no naming). Types encoded with ABI::FunctorEncoder::listArgumentTypesV.
    FunctionTypes stateMutability; ///< State mutability of the method.
    std::string type; ///< Type of the method.
  };

  /// Common functions used by both encoder and decoder.
  /// Forward declarations.
  template<typename T> struct isTupleOfDynamicTypes;
  /// Forward declaration for std::tuple<Ts...>
  template<typename... Ts> struct isTupleOfDynamicTypes<std::tuple<Ts...>>;
  /// Forward declaration for std::vector<T>
  template<typename T> struct isTupleOfDynamicTypes<std::vector<T>>;
      // Type trait to check if T is a std::vector
  template <typename T>
  struct isVector : std::false_type {};

  template <typename... Args>
  struct isVector<std::vector<Args...>> : std::true_type {};

  /// Helper variable template for is_vector
  template <typename T>
  inline constexpr bool isVectorV = isVector<T>::value;

  /// vectorElementType trait to get the element type of a vector
  template <typename T>
  struct vectorElementType {};

  /// Getter for the element type of a vector
  template <typename... Args>
  struct vectorElementType<std::vector<Args...>> {
    /// The element type of the vector
    using type = typename std::vector<Args...>::value_type;
  };

  /// Helper alias template for vector_element_type
  template <typename T>
  using vectorElementTypeT = typename vectorElementType<T>::type;

  // Helper to check if a type is a std::tuple
  template<typename T>
  struct isTuple : std::false_type {};

  template<typename... Ts>
  struct isTuple<std::tuple<Ts...>> : std::true_type {};

  /**
   * Check if a type is dynamic.
   * @tparam T Any supported ABI type.
   * @return `true` if type is dymanic, `false` otherwise.
   */
  template<typename T> constexpr bool isDynamic() {
    if constexpr (
      std::is_same_v<T, Bytes> ||
      std::is_same_v<T, BytesArrView> ||
      std::is_same_v<T, std::string> || false
    ) return true;
    if constexpr (isVectorV<T>) return true;
    if constexpr (isTupleOfDynamicTypes<T>::value) return true;
    return false;
  }

  /**
   * Check if a type is a tuple of dynamic types.
   * Default declaration for non tuple types.
   * @tparam T Any type.
   */
  template<typename T> struct isTupleOfDynamicTypes {
    /// Default value is false.
    static constexpr bool value = false;
  };

  /**
   * Check if a type is a tuple of dynamic types.
   * Default declaration for std::tuple<Ts...>.
   * @tparam Ts Any list of types.
   */
  template<typename... Ts> struct isTupleOfDynamicTypes<std::tuple<Ts...>> {
    /// For every type in Ts, check if it is dynamic. if it is, return true.
    static constexpr bool value = (... || isDynamic<Ts>());
  };

  /**
   * Check if a std::vector contain a tuple of dynamic types.
   * @tparam T Any type.
   */
  template<typename T> struct isTupleOfDynamicTypes<std::vector<T>> {
    /// For every type in T, check if it is dynamic. if it is, return true.
    static constexpr bool value = isTupleOfDynamicTypes<T>::value;
  };

  /// Calculates the total nextOffset of a given tuple type.
  template<typename T>
  constexpr uint64_t calculateOffsetForType() {
    if constexpr (isDynamic<T>()) {
      return 32;
    }
    if constexpr (isTuple<T>::value) {
      return 32 * std::tuple_size<T>::value;
    } else {
      return 32;
    }
  }

  /**
   * Calculate the total ABI offset start for dynamic types for a list of types.
   * @tparam Ts The types to calculate the offset for.
   * @return The total offset.
   */
  template <typename... Ts>
  constexpr uint64_t calculateTotalOffset() {
    return (calculateOffsetForType<Ts>() + ...);
  }

  /// Namespace for Functor encoding.
  namespace FunctorEncoder {
    // General template for type to string conversion
    template<typename T>
    /**
     * @cond
     * Default TypeName implementation
     */
    struct TypeName {
      static std::string get()
      {
        static_assert(std::is_same_v<T, void>, "TypeName specialization for this type is not defined");
        return "";
      }
    };

    /// Specialization for all numeric types
    template<> struct TypeName<uint8_t> { static std::string get() { return "uint8"; }};
    template<> struct TypeName<uint16_t> { static std::string get() { return "uint16"; }};
    template<> struct TypeName<uint24_t> { static std::string get() { return "uint24"; }};
    template<> struct TypeName<uint32_t> { static std::string get() { return "uint32"; }};
    template<> struct TypeName<uint40_t> { static std::string get() { return "uint40"; }};
    template<> struct TypeName<uint48_t> { static std::string get() { return "uint48"; }};
    template<> struct TypeName<uint56_t> { static std::string get() { return "uint56"; }};
    template<> struct TypeName<uint64_t> { static std::string get() { return "uint64"; }};
    template<> struct TypeName<uint72_t> { static std::string get() { return "uint72"; }};
    template<> struct TypeName<uint80_t> { static std::string get() { return "uint80"; }};
    template<> struct TypeName<uint88_t> { static std::string get() { return "uint88"; }};
    template<> struct TypeName<uint96_t> { static std::string get() { return "uint96"; }};
    template<> struct TypeName<uint104_t> { static std::string get() { return "uint104"; }};
    template<> struct TypeName<uint112_t> { static std::string get() { return "uint112"; }};
    template<> struct TypeName<uint120_t> { static std::string get() { return "uint120"; }};
    template<> struct TypeName<uint128_t> { static std::string get() { return "uint128"; }};
    template<> struct TypeName<uint136_t> { static std::string get() { return "uint136"; }};
    template<> struct TypeName<uint144_t> { static std::string get() { return "uint144"; }};
    template<> struct TypeName<uint152_t> { static std::string get() { return "uint152"; }};
    template<> struct TypeName<uint160_t> { static std::string get() { return "uint160"; }};
    template<> struct TypeName<uint168_t> { static std::string get() { return "uint168"; }};
    template<> struct TypeName<uint176_t> { static std::string get() { return "uint176"; }};
    template<> struct TypeName<uint184_t> { static std::string get() { return "uint184"; }};
    template<> struct TypeName<uint192_t> { static std::string get() { return "uint192"; }};
    template<> struct TypeName<uint200_t> { static std::string get() { return "uint200"; }};
    template<> struct TypeName<uint208_t> { static std::string get() { return "uint208"; }};
    template<> struct TypeName<uint216_t> { static std::string get() { return "uint216"; }};
    template<> struct TypeName<uint224_t> { static std::string get() { return "uint224"; }};
    template<> struct TypeName<uint232_t> { static std::string get() { return "uint232"; }};
    template<> struct TypeName<uint240_t> { static std::string get() { return "uint240"; }};
    template<> struct TypeName<uint248_t> { static std::string get() { return "uint248"; }};
    template<> struct TypeName<uint256_t> { static std::string get() { return "uint256"; }};
    template<> struct TypeName<int8_t> { static std::string get() { return "int8"; }};
    template<> struct TypeName<int16_t> { static std::string get() { return "int16"; }};
    template<> struct TypeName<int24_t> { static std::string get() { return "int24"; }};
    template<> struct TypeName<int32_t> { static std::string get() { return "int32"; }};
    template<> struct TypeName<int40_t> { static std::string get() { return "int40"; }};
    template<> struct TypeName<int48_t> { static std::string get() { return "int48"; }};
    template<> struct TypeName<int56_t> { static std::string get() { return "int56"; }};
    template<> struct TypeName<int64_t> { static std::string get() { return "int64"; }};
    template<> struct TypeName<int72_t> { static std::string get() { return "int72"; }};
    template<> struct TypeName<int80_t> { static std::string get() { return "int80"; }};
    template<> struct TypeName<int88_t> { static std::string get() { return "int88"; }};
    template<> struct TypeName<int96_t> { static std::string get() { return "int96"; }};
    template<> struct TypeName<int104_t> { static std::string get() { return "int104"; }};
    template<> struct TypeName<int112_t> { static std::string get() { return "int112"; }};
    template<> struct TypeName<int120_t> { static std::string get() { return "int120"; }};
    template<> struct TypeName<int128_t> { static std::string get() { return "int128"; }};
    template<> struct TypeName<int136_t> { static std::string get() { return "int136"; }};
    template<> struct TypeName<int144_t> { static std::string get() { return "int144"; }};
    template<> struct TypeName<int152_t> { static std::string get() { return "int152"; }};
    template<> struct TypeName<int160_t> { static std::string get() { return "int160"; }};
    template<> struct TypeName<int168_t> { static std::string get() { return "int168"; }};
    template<> struct TypeName<int176_t> { static std::string get() { return "int176"; }};
    template<> struct TypeName<int184_t> { static std::string get() { return "int184"; }};
    template<> struct TypeName<int192_t> { static std::string get() { return "int192"; }};
    template<> struct TypeName<int200_t> { static std::string get() { return "int200"; }};
    template<> struct TypeName<int208_t> { static std::string get() { return "int208"; }};
    template<> struct TypeName<int216_t> { static std::string get() { return "int216"; }};
    template<> struct TypeName<int224_t> { static std::string get() { return "int224"; }};
    template<> struct TypeName<int232_t> { static std::string get() { return "int232"; }};
    template<> struct TypeName<int240_t> { static std::string get() { return "int240"; }};
    template<> struct TypeName<int248_t> { static std::string get() { return "int248"; }};
    template<> struct TypeName<int256_t> { static std::string get() { return "int256"; }};

    /// Specialization for address type
    template<> struct TypeName<Address> { static std::string get() { return "address"; }};
    /// Specialization for bool type
    template<> struct TypeName<bool> { static std::string get() { return "bool"; }};
    /// Specialization for bytes type
    template<> struct TypeName<Bytes> { static std::string get() { return "bytes"; }};
    /// Specialization for bytes type
    template<> struct TypeName<std::string> { static std::string get() { return "string"; }};
    /// @endcond
    // Helper for tuple types
    template <typename Tuple, typename IndexSequence>
    struct TupleTypeNameHelper;
    /**
     * Helper that expand the tuple and call TypeName for each element
     * @tparam Tuple The tuple type
     * @tparam Is The index sequence
     */
    template <typename Tuple, std::size_t... Is>
    struct TupleTypeNameHelper<Tuple, std::index_sequence<Is...>> {
      /// Static function because this is a struct.
      static std::string get() {
        std::string result;
        ((result += TypeName<std::decay_t<std::tuple_element_t<Is, Tuple>>>::get() + ","), ...);
        if (!result.empty()) {
          result.pop_back(); // Remove the last comma
        }
        return result;
      }
    };

    /**
     * TypeName specialization for std::tuple
     * @tparam Args The tuple types
     */
    template<typename... Args>
    struct TypeName<std::tuple<Args...>> {
      /// Static function because this is a struct.
      static std::string get() {
        return "(" + TupleTypeNameHelper<std::tuple<Args...>, std::index_sequence_for<Args...>>::get() + ")";
      }
    };

    /**
     * TypeName specialization for std::vector
     * @tparam T The vector type
     */
    template<typename T>
    struct TypeName<std::vector<T>> {
      /// Static function because this is a struct.
      static std::string get() {
        return TypeName<T>::get() + "[]";
      }
    };

    /**
     * List the argument types in a string, comma separated, uses () for tuples and [] for arrays
     * Example: listArgumentTypes<int, std::string, std::tuple<int, int>, std::vector<std::string>>()
     * Will result in "int,string,(int,int),string[]"
     * @tparam Args The argument types
     * @return The string with the argument types
     */
    template <typename... Args>
    static std::string listArgumentTypes() {
      std::string result;
      ((result += TypeName<std::decay_t<Args>>::get() + ","), ...);
      if (!result.empty()) {
        result.pop_back(); // Remove the last comma
      }
      return result;
    }

    /**
     * List the argument types in a vector of strings, uses () for tuples and [] for arrays
     * Example: listArgumentTypesV<int, std::string, std::tuple<int, int>, std::vector<std::string>>()
     * Will result in {"int","string","(int,int)","string[]"}
     * @tparam Args
     * @return The vector with the argument types
     */
    template<typename... Args>
    static std::vector<std::string> listArgumentTypesV() {
      std::vector<std::string> result;
      ((result.emplace_back(TypeName<std::decay_t<Args>>::get())), ...);
      return result;
    }

    /**
     * Helper function for listArgumentTypesVFromTuple
     * @tparam Tuple The tuple type
     * @tparam I The index sequence
     * @return The vector with the argument types
     */
    template<typename Tuple, std::size_t... I>
    static std::vector<std::string> unpackTupleAndListTypesV(std::index_sequence<I...>) {
      return listArgumentTypesV<std::tuple_element_t<I, Tuple>...>();
    }

    /**
     * Same as listArgumentTypesV, but takes a tuple as template parameter
     * @tparam Tuple The tuple type
     * @return The vector with the argument types
     */
    template<typename Tuple>
    static std::vector<std::string> listArgumentTypesVFromTuple() {
      return unpackTupleAndListTypesV<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    }

    /**
    * Encode a function signature following solidity rules.
    * @tparam Args The argument types.
    * @param funcSignature The function signature (name).
    */
    template <typename... Args>
    static Functor encode(const std::string& funcSignature) {
      std::string fullSignature = funcSignature;
      fullSignature += "(" + listArgumentTypes<Args...>() + ")";
      return Utils::sha3(Utils::create_view_span(fullSignature)).view_const(0, 4);
    }

    /// Generate the functor for a function.
  }


  /// Namespace for ABI-encoding functions.
  namespace Encoder {
    /**
     * Append a Bytes piece to another Bytes piece.
     * @param dest The Bytes piece to append to.
     * @param src The Bytes piece to be appended.
     */
    template <typename T>
    void append(Bytes &dest, const T &src) {
      dest.insert(dest.end(), src.cbegin(), src.cend());
    }

    /**
     * Encode a uint256.
     * @param num The input to encode.
     * @return The encoded input.
     */
    Bytes encodeUint(const uint256_t& num);

    /**
     * Encode an int256.
     * @param num The input to encode.
     * @return The encoded input.
     */
    Bytes encodeInt(const int256_t& num);

    /**
     * Encode an address.
     * @param add The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const Address& add);

    /**
     * Encode a boolean.
     * @param b The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const bool& b);

    /**
     * Encode a raw byte string.
     * @param bytes The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const Bytes& bytes);

    /**
     * Encode an UTF-8 string.
     * @param str The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const std::string& str);

    /**
     * Specialization for encoding any type of uint or int.
     * @tparam T Any supported uint or int.
     * @param num The input to encode.
     * @return The encoded input.
     * @throw std::runtime_error if type is not found.
     */
    template <typename T> Bytes encode(const T& num) {
      if constexpr (
        std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int24_t> ||
        std::is_same_v<T, int32_t> || std::is_same_v<T, int40_t> || std::is_same_v<T, int48_t> ||
        std::is_same_v<T, int56_t> || std::is_same_v<T, int64_t> || std::is_same_v<T, int72_t> ||
        std::is_same_v<T, int80_t> || std::is_same_v<T, int88_t> || std::is_same_v<T, int96_t> ||
        std::is_same_v<T, int104_t> || std::is_same_v<T, int112_t> || std::is_same_v<T, int120_t> ||
        std::is_same_v<T, int128_t> || std::is_same_v<T, int136_t> || std::is_same_v<T, int144_t> ||
        std::is_same_v<T, int152_t> || std::is_same_v<T, int160_t> || std::is_same_v<T, int168_t> ||
        std::is_same_v<T, int176_t> || std::is_same_v<T, int184_t> || std::is_same_v<T, int192_t> ||
        std::is_same_v<T, int200_t> || std::is_same_v<T, int208_t> || std::is_same_v<T, int216_t> ||
        std::is_same_v<T, int224_t> || std::is_same_v<T, int232_t> || std::is_same_v<T, int240_t> ||
        std::is_same_v<T, int248_t> || std::is_same_v<T, int256_t>
      ) {
        return encodeInt(num);
      } else if constexpr (
        std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint24_t> ||
        std::is_same_v<T, uint32_t> || std::is_same_v<T, uint40_t> || std::is_same_v<T, uint48_t> ||
        std::is_same_v<T, uint56_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, uint72_t> ||
        std::is_same_v<T, uint80_t> || std::is_same_v<T, uint88_t> || std::is_same_v<T, uint96_t> ||
        std::is_same_v<T, uint104_t> || std::is_same_v<T, uint112_t> || std::is_same_v<T, uint120_t> ||
        std::is_same_v<T, uint128_t> || std::is_same_v<T, uint136_t> || std::is_same_v<T, uint144_t> ||
        std::is_same_v<T, uint152_t> || std::is_same_v<T, uint160_t> || std::is_same_v<T, uint168_t> ||
        std::is_same_v<T, uint176_t> || std::is_same_v<T, uint184_t> || std::is_same_v<T, uint192_t> ||
        std::is_same_v<T, uint200_t> || std::is_same_v<T, uint208_t> || std::is_same_v<T, uint216_t> ||
        std::is_same_v<T, uint224_t> || std::is_same_v<T, uint232_t> || std::is_same_v<T, uint240_t> ||
        std::is_same_v<T, uint248_t> || std::is_same_v<T, uint256_t>
      ) {
        return encodeUint(num);
      } else throw std::runtime_error("The type " + Utils::getRealTypeName<T>() + " is not supported on encoding");
    }

    /// Forward declaration so encode(std::tuple<Ts...>) be able to see it.
    template<typename T> Bytes encode(const std::vector<T>& v);

    /// Specialization for encoding a tuple. Expand and call back encode<T,Ts...>
    template<typename... Ts> Bytes encode(const std::tuple<Ts...>& t) {
      Bytes result;
      Bytes dynamicBytes;
      uint64_t nextOffset = calculateTotalOffset<Ts...>();

      std::apply([&](const auto&... args) {
        auto encodeItem = [&](auto&& item) {
          using ItemType = std::decay_t<decltype(item)>;
          if (isDynamic<ItemType>()) {
            Bytes packed = encode(item);
            append(result, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
            nextOffset += packed.size();
            dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
          } else {
            append(result, encode(item));
          }
        };
        (encodeItem(args), ...);
      }, t);

      result.insert(result.end(), dynamicBytes.begin(), dynamicBytes.end());
      return result;
    }

    /// Specialization for encoding a vector of type T.
    template<typename T> Bytes encode(const std::vector<T>& v) {
      Bytes result;
      uint64_t nextOffset = 32 * v.size();  // The first 32 bytes are for the length of the dynamic array
      if constexpr (isDynamic<T>())
      {
        /// If the vector is dynamic, we need to account the offsets of each tuple
        Bytes dynamicData;
        Bytes dynamicOffSets;

        // Encode each item within the vector.
        for (const auto& t : v) {
          append(dynamicOffSets, Utils::uint256ToBytes(nextOffset));
          Bytes dynamicBytes = encode(t);  // We're calling the encode function specialized for the T type.
          nextOffset += dynamicBytes.size();
          dynamicData.insert(dynamicData.end(), dynamicBytes.begin(), dynamicBytes.end());
        }

        append(result, Utils::padLeftBytes(Utils::uintToBytes(v.size()), 32));  // Add the array length to the result
        append(result, dynamicOffSets);  // Add the dynamic offsets
        result.insert(result.end(), dynamicData.begin(), dynamicData.end());  // Add the dynamic data
        return result;
      } else {
        append(result, Utils::padLeftBytes(Utils::uintToBytes(v.size()), 32));  // Add the array length to the result
        for (const auto& t : v) {
          append (result, encode(t));
        }
        return result;
      }
    }

    /**
     * The main encode function. Use this one.
     * @tparam T Any supported ABI type (first one).
     * @tparam Ts Any supported ABI type (any other).
     * @param first First type to encode.
     * @param rest The rest of the types to encode, if any.
     * @return The encoded data.
     */
    template<typename T, typename... Ts> Bytes encodeData(const T& first, const Ts&... rest) {
      Bytes result;
      // Based on the ABI spec, use calculateTotalOffset to calculate the nextOffset
      uint64_t nextOffset = calculateTotalOffset<T, Ts...>();
      Bytes dynamicBytes;

      auto encodeItem = [&](auto&& item) {
        using ItemType = std::decay_t<decltype(item)>;
        if constexpr (isDynamic<ItemType>()) {
          Bytes packed = encode(item);
          append(result, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
          nextOffset += packed.size();
          dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
        } else append(result, encode(item));
      };

      encodeItem(first);
      (encodeItem(rest), ...);
      result.insert(result.end(), dynamicBytes.begin(), dynamicBytes.end());
      return result;
    }
  }; // namespace Encoder

  /// Namespace for ABI-decoding functions.
  namespace Decoder {
    /// Struct for a list of decoded types.
    template <typename T, typename... Ts> struct TypeList;

    // TODO: docs
    template<typename T> inline T decode(const BytesArrView& bytes, uint64_t& index);

    /**
     * Decode a uint256.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    uint256_t decodeUint(const BytesArrView& bytes, uint64_t& index);

    /**
     * Decode an int256.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    int256_t decodeInt(const BytesArrView& bytes, uint64_t& index);

    /**
     * Decode a packed std::tuple<Args...> individually
     * This function takes advante of std::tuple_element and template recurssion
     * in order to parse all the items within that given tuple.
     * @tparam TupleLike The std::tuple<Args...> structure
     * @tparam I - the current tuple index
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @param ret The tuple object to return, needs to be a reference and create outside the function due to recursion
     * Doesn't return, use the referenced TupleLike object..
     */
    template<typename TupleLike, size_t I = 0>
    void decodeTuple(const BytesArrView& bytes, uint64_t& index, TupleLike& ret) {
      if constexpr (I < std::tuple_size_v<TupleLike>)
      {
        using SelectedType = typename std::tuple_element<I, TupleLike>::type;
        std::get<I>(ret) = decode<SelectedType>(bytes, index);
        decodeTuple<TupleLike, I + 1>(bytes, index, ret);
      }
    }

    /**
     * Specialization for decoding any type of uint or int.
     * This function is also used by std::tuple<OtherArgs...> and std::vector<std::tuple<OtherArgs...>>
     * Due to incapability of partially specializing the decode function for std::tuple<Args...>
     * @tparam T Any supported uint or int.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if type is not found.
     */
    template <typename T> inline T decode(const BytesArrView& bytes, uint64_t& index) {
      if constexpr (isTuple<T>::value) {
        T ret;
        if constexpr (isTupleOfDynamicTypes<T>::value) {
          if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for tuple of dynamic types");
          Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
          uint64_t offset = Utils::fromBigEndian<uint64_t>(tmp);
          index += 32;
          uint64_t newIndex = 0;
          auto view = bytes.subspan(offset);
          decodeTuple<T>(view, newIndex, ret);
          return ret;
        }
        if (index + 32 * std::tuple_size_v<T> > bytes.size()) throw std::runtime_error("Data too short for tuple");
        decodeTuple<T>(bytes, index, ret);
        return ret;
      }

      if constexpr (isVectorV<T>) {
        using ElementType = vectorElementTypeT<T>;
        std::vector<ElementType> retVector;
        // Get array offset
        if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for vector");
        Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
        uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);
        index += 32;

        // Get array length
        tmp.clear();
        if (arrayStart + 32 > bytes.size()) throw std::runtime_error("Data too short for vector");
        tmp.insert(tmp.end(), bytes.begin() + arrayStart, bytes.begin() + arrayStart + 32);
        uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

        if (arrayStart + 32 > bytes.size()) throw std::runtime_error("Data too short for vector");
        uint64_t newIndex = 0;
        auto view = bytes.subspan(arrayStart + 32);
        for (uint64_t i = 0; i < arrayLength; ++i) {
          // Recursion here
          retVector.emplace_back(decode<ElementType>(view, newIndex));
        }
        return retVector;
      }

      if constexpr (
        std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int24_t> ||
        std::is_same_v<T, int32_t> || std::is_same_v<T, int40_t> || std::is_same_v<T, int48_t> ||
        std::is_same_v<T, int56_t> || std::is_same_v<T, int64_t> || std::is_same_v<T, int72_t> ||
        std::is_same_v<T, int80_t> || std::is_same_v<T, int88_t> || std::is_same_v<T, int96_t> ||
        std::is_same_v<T, int104_t> || std::is_same_v<T, int112_t> || std::is_same_v<T, int120_t> ||
        std::is_same_v<T, int128_t> || std::is_same_v<T, int136_t> || std::is_same_v<T, int144_t> ||
        std::is_same_v<T, int152_t> || std::is_same_v<T, int160_t> || std::is_same_v<T, int168_t> ||
        std::is_same_v<T, int176_t> || std::is_same_v<T, int184_t> || std::is_same_v<T, int192_t> ||
        std::is_same_v<T, int200_t> || std::is_same_v<T, int208_t> || std::is_same_v<T, int216_t> ||
        std::is_same_v<T, int224_t> || std::is_same_v<T, int232_t> || std::is_same_v<T, int240_t> ||
        std::is_same_v<T, int248_t> || std::is_same_v<T, int256_t>
      ) {
        return static_cast<T>(decodeInt(bytes, index));
      } else if constexpr (
        std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint24_t> ||
        std::is_same_v<T, uint32_t> || std::is_same_v<T, uint40_t> || std::is_same_v<T, uint48_t> ||
        std::is_same_v<T, uint56_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, uint72_t> ||
        std::is_same_v<T, uint80_t> || std::is_same_v<T, uint88_t> || std::is_same_v<T, uint96_t> ||
        std::is_same_v<T, uint104_t> || std::is_same_v<T, uint112_t> || std::is_same_v<T, uint120_t> ||
        std::is_same_v<T, uint128_t> || std::is_same_v<T, uint136_t> || std::is_same_v<T, uint144_t> ||
        std::is_same_v<T, uint152_t> || std::is_same_v<T, uint160_t> || std::is_same_v<T, uint168_t> ||
        std::is_same_v<T, uint176_t> || std::is_same_v<T, uint184_t> || std::is_same_v<T, uint192_t> ||
        std::is_same_v<T, uint200_t> || std::is_same_v<T, uint208_t> || std::is_same_v<T, uint216_t> ||
        std::is_same_v<T, uint224_t> || std::is_same_v<T, uint232_t> || std::is_same_v<T, uint240_t> ||
        std::is_same_v<T, uint248_t> || std::is_same_v<T, uint256_t>
      ) {
        return static_cast<T>(decodeUint(bytes, index));
      } else throw std::runtime_error("The type " + Utils::getRealTypeName<T>() + " is not supported on decoding.");
    }

    /**
     * Decode an address.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline Address decode<Address>(const BytesArrView& bytes, uint64_t& index) {
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for address");
      Address result = Address(bytes.subspan(index + 12, 20));
      index += 32;
      return result;
    }

    /**
     * Decode a boolean.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline bool decode<bool>(const BytesArrView& bytes, uint64_t& index) {
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for bool");
      bool result = (bytes[index + 31] == 0x01);
      index += 32;
      return result;
    }

    /**
     * Decode a raw bytes string.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline Bytes decode<Bytes>(const BytesArrView& bytes, uint64_t& index) {
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for bytes");
      Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
      uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp);
      index += 32;

      // Get bytes length
      tmp.clear();
      if (bytesStart + 32 > bytes.size()) throw std::runtime_error("Data too short for bytes");
      tmp.insert(tmp.end(), bytes.begin() + bytesStart, bytes.begin() + bytesStart + 32);
      uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

      // Size sanity check
      if (bytesStart + 32 + bytesLength > bytes.size()) throw std::runtime_error("Data too short for bytes");

      // Get bytes data
      tmp.clear();
      tmp.insert(tmp.end(), bytes.begin() + bytesStart + 32, bytes.begin() + bytesStart + 32 + bytesLength);
      return tmp;
    }

    /**
     * Decode a UTF-8 string.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline std::string decode<std::string>(const BytesArrView& bytes, uint64_t& index) {
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for string 1");
      std::string tmp(bytes.begin() + index, bytes.begin() + index + 32);
      uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp);
      index += 32;  // Move index to next 32 bytes

      // Get bytes length
      tmp.clear();
      if (bytesStart + 32 > bytes.size()) throw std::runtime_error("Data too short for string 2");
      tmp.insert(tmp.end(), bytes.begin() + bytesStart, bytes.begin() + bytesStart + 32);
      uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

      // Size sanity check
      if (bytesStart + 32 + bytesLength > bytes.size()) throw std::runtime_error("Data too short for string 3");

      // Get bytes data
      tmp.clear();
      tmp.insert(tmp.end(), bytes.begin() + bytesStart + 32, bytes.begin() + bytesStart + 32 + bytesLength);
      return tmp;
    }

    /**
     * The TypeList struct is used in order to decay a std::tuple into a list of types.
     * @tparam T The first type
     * @tparam Ts The rest of the types
     */
    template <typename T, typename... Ts> struct TypeList {
      /// The current type
      T head;
      /// Remaining types
      TypeList<Ts...> tail;
      /// Construct TypeList recursively.
      TypeList(const BytesArrView& bytes, uint64_t& index) : head(decode<T>(bytes, index)), tail(bytes, index) {}
    };

    /**
     * Specialization for the last type in the std::tuple.
     * @tparam T The last type
     */
    template <typename T> struct TypeList<T> {
      /// The last type
      T head;
      /// Construct the last type in the TypeList.
      TypeList(const BytesArrView& bytes, uint64_t& index) : head(decode<T>(bytes, index)) {}
    };

    /**
     * Convert a type list to a tuple.
     * @tparam Args Any supported ABI type.
     * @param tl The list of types to convert.
     * @return A tuple with the converted types.
     */
    template <typename... Args>
    inline std::tuple<Args...> toTuple(TypeList<Args...>& tl) {
      return toTupleHelper(tl, std::tuple<>());
    }

    /**
     * Helper function for toTuple.
     * @tparam Accumulated The accumulated types.
     * @tparam T The current type.
     * @tparam Ts The rest of the types.
     * @param tl The list of types to convert.
     * @param acc The accumulated (already converted) types.
     * @return A tuple with the converted types.
     */
    template<typename... Accumulated, typename T, typename... Ts>
    inline auto toTupleHelper(TypeList<T, Ts...>& tl, std::tuple<Accumulated...> acc) {
      return toTupleHelper(tl.tail, std::tuple_cat(acc, std::tuple<T>(tl.head)));
    }

    /**
    * Helper function for toTuple.
    * @tparam Accumulated The accumulated types.
    * @tparam T The current type.
    * @param tl The list of types to convert.
    * @param acc The accumulated (already converted) types.
    */
    template<typename... Accumulated, typename T>
    inline auto toTupleHelper(TypeList<T>& tl, std::tuple<Accumulated...> acc) {
      return std::tuple_cat(acc, std::tuple<T>(tl.head));
    }

    /**
     * The main decode function. Use this one.
     * @tparam Args Any supported ABI type.
     * @param encodedData The full encoded data string to decode.
     * @param index The point on the encoded string to start decoding. Defaults to start of string (0).
     * @return A tuple with the decoded data, or an empty tuple if there's no arguments to decode.
     */
    template<typename... Args>
    inline std::tuple<Args...> decodeData(const BytesArrView& encodedData, uint64_t index = 0) {
      if constexpr (sizeof...(Args) == 0) {
        return std::tuple<>();
      } else {
        /// TODO: there is a "unecessary" copy here (TypeList to tuple), we could use a std::tuple directly?
        auto typeListResult = TypeList<Args...>(encodedData, index);
        return toTuple(typeListResult);
      }
    }
  };  // namespace Decoder
}; // namespace ABI

#endif // ABI_H
