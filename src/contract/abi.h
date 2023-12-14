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
  /**
   * Enum for the types of Solidity variables.
   * Equivalency is as follows:
   * - uintN = uintN (Solidity) = uintN_t (C++), where N <= 256 and N % 8 == 0
   * - uintNArr = uintN[] (Solidity) = std::vector<uintN_t> (C++), where N <= 256 and N % 8 == 0
   * - address = address (Solidity) = Address (C++)
   * - addressArr = address[] (Solidity) = std::vector<Address> (C++)
   * - bool = bool (Solidity) = bool (C++)
   * - boolArr = bool[] (Solidity) = vector<bool> (C++)
   * - bytes = bytes (Solidity) = Bytes (C++)
   * - bytesArr = bytes[] (Solidity) = std::vector<Bytes> (C++)
   * - string = string (Solidity) = std::string (C++)
   * - stringArr = string[] (Solidity) = std::vector<std::string> (C++)
   * - tuple = tuple (Soldiity) = std::tuple(C++)
   */
  enum Types {
    uint8, uint8Arr, uint16, uint16Arr, uint24, uint24Arr, uint32, uint32Arr,
    uint40, uint40Arr, uint48, uint48Arr, uint56, uint56Arr, uint64, uint64Arr,
    uint72, uint72Arr, uint80, uint80Arr, uint88, uint88Arr, uint96, uint96Arr,
    uint104, uint104Arr, uint112, uint112Arr, uint120, uint120Arr, uint128, uint128Arr,
    uint136, uint136Arr, uint144, uint144Arr, uint152, uint152Arr, uint160, uint160Arr,
    uint168, uint168Arr, uint176, uint176Arr, uint184, uint184Arr, uint192, uint192Arr,
    uint200, uint200Arr, uint208, uint208Arr, uint216, uint216Arr, uint224, uint224Arr,
    uint232, uint232Arr, uint240, uint240Arr, uint248, uint248Arr, uint256, uint256Arr,

    int8, int8Arr, int16, int16Arr, int24, int24Arr, int32, int32Arr,
    int40, int40Arr, int48, int48Arr, int56, int56Arr, int64, int64Arr,
    int72, int72Arr, int80, int80Arr, int88, int88Arr, int96, int96Arr,
    int104, int104Arr, int112, int112Arr, int120, int120Arr, int128, int128Arr,
    int136, int136Arr, int144, int144Arr, int152, int152Arr, int160, int160Arr,
    int168, int168Arr, int176, int176Arr, int184, int184Arr, int192, int192Arr,
    int200, int200Arr, int208, int208Arr, int216, int216Arr, int224, int224Arr,
    int232, int232Arr, int240, int240Arr, int248, int248Arr, int256, int256Arr,

    address, addressArr, boolean, booleanArr, bytes, bytesArr, string, stringArr, tuple
  };

  /**
   * Enum struct for the types of Solidity functions.
   * @tparam T The type to map to an ABI type.
   */
  template <typename T> struct TypeToEnum;

  /**
  * Helper struct to map a type to an ABI type. Specializations for each type are defined below.
  * Default ABI type is uint256.
  * @tparam T The type to map to an ABI type.
  */
  template <typename T> struct ABIType { static constexpr Types value = Types::uint256; };

  /// Specialization for Address. ABI type is address.
  template <> struct ABIType<Address> { static constexpr Types value = Types::address; };

  /// Specialization for std::vector<Address>. ABI type is address.
  template <> struct ABIType<std::vector<Address>> { static constexpr Types value = Types::addressArr; };

  /// Specialization for bool. ABI type is boolean.
  template <> struct ABIType<bool> { static constexpr Types value = Types::boolean; };

  /// Specialization for std::string. ABI type is string.
  template <> struct ABIType<std::string> { static constexpr Types value = Types::string; };

  /// Specialization for Bytes. ABI type is bytes.
  template <> struct ABIType<Bytes> { static constexpr Types value = Types::bytes; };

  /// Specialization for std::tuple with N elements. ABI type is tuple.
  template <typename... Args> struct ABIType<std::tuple<Args...>> { static constexpr Types value = Types::tuple; };

  /// Specialization for uint8_t. ABI type is uint8.
  template <> struct ABIType<uint8_t> { static constexpr Types value = Types::uint8; };

  /// Specialization for uint16_t. ABI type is uint16.
  template <> struct ABIType<uint16_t> { static constexpr Types value = Types::uint16; };

  /// Specialization for uint24_t. ABI type is uint24.
  template <> struct ABIType<uint24_t> { static constexpr Types value = Types::uint24; };

  /// Specialization for uint32_t. ABI type is uint32.
  template <> struct ABIType<uint32_t> { static constexpr Types value = Types::uint32; };

  /// Specialization for uint40_t. ABI type is uint40.
  template <> struct ABIType<uint40_t> { static constexpr Types value = Types::uint40; };

  /// Specialization for uint48_t. ABI type is uint48.
  template <> struct ABIType<uint48_t> { static constexpr Types value = Types::uint48; };

  /// Specialization for uint56_t. ABI type is uint56.
  template <> struct ABIType<uint56_t> { static constexpr Types value = Types::uint56; };

  /// Specialization for uint64_t. ABI type is uint64.
  template <> struct ABIType<uint64_t> { static constexpr Types value = Types::uint64; };

  /// Specialization for uint72_t. ABI type is uint72.
  template <> struct ABIType<uint72_t> { static constexpr Types value = Types::uint72; };

  /// Specialization for uint80_t. ABI type is uint80.
  template <> struct ABIType<uint80_t> { static constexpr Types value = Types::uint80; };

  /// Specialization for uint88_t. ABI type is uint88.
  template <> struct ABIType<uint88_t> { static constexpr Types value = Types::uint88; };

  /// Specialization for uint96_t. ABI type is uint96.
  template <> struct ABIType<uint96_t> { static constexpr Types value = Types::uint96; };

  /// Specialization for uint104_t. ABI type is uint104.
  template <> struct ABIType<uint104_t> { static constexpr Types value = Types::uint104; };

  /// Specialization for uint112_t. ABI type is uint112.
  template <> struct ABIType<uint112_t> { static constexpr Types value = Types::uint112; };

  /// Specialization for uint120_t. ABI type is uint120.
  template <> struct ABIType<uint120_t> { static constexpr Types value = Types::uint120; };

  /// Specialization for uint128_t. ABI type is uint128.
  template <> struct ABIType<uint128_t> { static constexpr Types value = Types::uint128; };

  /// Specialization for uint136_t. ABI type is uint136.
  template <> struct ABIType<uint136_t> { static constexpr Types value = Types::uint136; };

  /// Specialization for uint144_t. ABI type is uint144.
  template <> struct ABIType<uint144_t> { static constexpr Types value = Types::uint144; };

  /// Specialization for uint152_t. ABI type is uint152.
  template <> struct ABIType<uint152_t> { static constexpr Types value = Types::uint152; };

  /// Specialization for uint160_t. ABI type is uint160.
  template <> struct ABIType<uint160_t> { static constexpr Types value = Types::uint160; };

  /// Specialization for uint168_t. ABI type is uint168.
  template <> struct ABIType<uint168_t> { static constexpr Types value = Types::uint168; };

  /// Specialization for uint176_t. ABI type is uint176.
  template <> struct ABIType<uint176_t> { static constexpr Types value = Types::uint176; };

  /// Specialization for uint184_t. ABI type is uint184.
  template <> struct ABIType<uint184_t> { static constexpr Types value = Types::uint184; };

  /// Specialization for uint192_t. ABI type is uint192.
  template <> struct ABIType<uint192_t> { static constexpr Types value = Types::uint192; };

  /// Specialization for uint200_t. ABI type is uint200.
  template <> struct ABIType<uint200_t> { static constexpr Types value = Types::uint200; };

  /// Specialization for uint208_t. ABI type is uint208.
  template <> struct ABIType<uint208_t> { static constexpr Types value = Types::uint208; };

  /// Specialization for uint216_t. ABI type is uint216.
  template <> struct ABIType<uint216_t> { static constexpr Types value = Types::uint216; };

  /// Specialization for uint224_t. ABI type is uint224.
  template <> struct ABIType<uint224_t> { static constexpr Types value = Types::uint224; };

  /// Specialization for uint232_t. ABI type is uint232.
  template <> struct ABIType<uint232_t> { static constexpr Types value = Types::uint232; };

  /// Specialization for uint240_t. ABI type is uint240.
  template <> struct ABIType<uint240_t> { static constexpr Types value = Types::uint240; };

  /// Specialization for uint248_t. ABI type is uint248.
  template <> struct ABIType<uint248_t> { static constexpr Types value = Types::uint248; };

  /// Specialization for int8_t. ABI type is int8.
  template <> struct ABIType<int8_t> { static constexpr Types value = Types::int8; };

  /// Specialization for int16_t. ABI type is int16.
  template <> struct ABIType<int16_t> { static constexpr Types value = Types::int16; };

  /// Specialization for int24_t. ABI type is int24.
  template <> struct ABIType<int24_t> { static constexpr Types value = Types::int24; };

  /// Specialization for int32_t. ABI type is int32.
  template <> struct ABIType<int32_t> { static constexpr Types value = Types::int32; };

  /// Specialization for int40_t. ABI type is int40.
  template <> struct ABIType<int40_t> { static constexpr Types value = Types::int40; };

  /// Specialization for int48_t. ABI type is int48.
  template <> struct ABIType<int48_t> { static constexpr Types value = Types::int48; };

  /// Specialization for int56_t. ABI type is int56.
  template <> struct ABIType<int56_t> { static constexpr Types value = Types::int56; };

  /// Specialization for int64_t. ABI type is int64.
  template <> struct ABIType<int64_t> { static constexpr Types value = Types::int64; };

  /// Specialization for int72_t. ABI type is int72.
  template <> struct ABIType<int72_t> { static constexpr Types value = Types::int72; };

  /// Specialization for int80_t. ABI type is int80.
  template <> struct ABIType<int80_t> { static constexpr Types value = Types::int80; };

  /// Specialization for int88_t. ABI type is int88.
  template <> struct ABIType<int88_t> { static constexpr Types value = Types::int88; };

  /// Specialization for int96_t. ABI type is int96.
  template <> struct ABIType<int96_t> { static constexpr Types value = Types::int96; };

  /// Specialization for int104_t. ABI type is int104.
  template <> struct ABIType<int104_t> { static constexpr Types value = Types::int104; };

  /// Specialization for int112_t. ABI type is int112.
  template <> struct ABIType<int112_t> { static constexpr Types value = Types::int112; };

  /// Specialization for int120_t. ABI type is int120.
  template <> struct ABIType<int120_t> { static constexpr Types value = Types::int120; };

  /// Specialization for int128_t. ABI type is int128.
  template <> struct ABIType<int128_t> { static constexpr Types value = Types::int128; };

  /// Specialization for int136_t. ABI type is int136.
  template <> struct ABIType<int136_t> { static constexpr Types value = Types::int136; };

  /// Specialization for int144_t. ABI type is int144.
  template <> struct ABIType<int144_t> { static constexpr Types value = Types::int144; };

  /// Specialization for int152_t. ABI type is int152.
  template <> struct ABIType<int152_t> { static constexpr Types value = Types::int152; };

  /// Specialization for int160_t. ABI type is int160.
  template <> struct ABIType<int160_t> { static constexpr Types value = Types::int160; };

  /// Specialization for int168_t. ABI type is int168.
  template <> struct ABIType<int168_t> { static constexpr Types value = Types::int168; };

  /// Specialization for int176_t. ABI type is int176.
  template <> struct ABIType<int176_t> { static constexpr Types value = Types::int176; };

  /// Specialization for int184_t. ABI type is int184.
  template <> struct ABIType<int184_t> { static constexpr Types value = Types::int184; };

  /// Specialization for int192_t. ABI type is int192.
  template <> struct ABIType<int192_t> { static constexpr Types value = Types::int192; };

  /// Specialization for int200_t. ABI type is int200.
  template <> struct ABIType<int200_t> { static constexpr Types value = Types::int200; };

  /// Specialization for int208_t. ABI type is int208.
  template <> struct ABIType<int208_t> { static constexpr Types value = Types::int208; };

  /// Specialization for int216_t. ABI type is int216.
  template <> struct ABIType<int216_t> { static constexpr Types value = Types::int216; };

  /// Specialization for int224_t. ABI type is int224.
  template <> struct ABIType<int224_t> { static constexpr Types value = Types::int224; };

  /// Specialization for int232_t. ABI type is int232.
  template <> struct ABIType<int232_t> { static constexpr Types value = Types::int232; };

  /// Specialization for int240_t. ABI type is int240.
  template <> struct ABIType<int240_t> { static constexpr Types value = Types::int240; };

  /// Specialization for int248_t. ABI type is int248.
  template <> struct ABIType<int248_t> { static constexpr Types value = Types::int248; };

  /// Specialization for int256_t. ABI type is int256.
  template <> struct ABIType<int256_t> { static constexpr Types value = Types::int256; };

  /**
  * Specialization for std::vector<T>. Used for all vector types, including bytesArr and stringArr.
  * ABI type is vector of T.
  * @tparam T The type to map to an ABI type.
  */
  template <typename T> struct ABIType<std::vector<T>> {
    static constexpr Types value = static_cast<Types>(static_cast<int>(ABIType<T>::value) + 1);
  };

  /**
  * Struct to map a type to an ABI type.
  * @tparam T The type to map to an ABI type.
  */
  template <typename T> struct TypeToEnum { static constexpr Types value = ABIType<T>::value; };

  /**
  * Specialization for reference types.
  * @tparam T The type to map to an ABI type.
  */
  template <typename T> struct TypeToEnum<T&> : TypeToEnum<T> {};

  /**
  * Specialization for const reference types
  * @tparam T The type to map to an ABI type.
  */
  template <typename T> struct TypeToEnum<const T&> : TypeToEnum<T> {};

  /**
  * Specialization for vector reference types.
  * @tparam T The type to map to an ABI type.
  */
  template <typename T> struct TypeToEnum<std::vector<T>&> : TypeToEnum<std::vector<T>> {};

  /**
  * Specialization for const vector reference types.
  * @tparam T The type to map to an ABI type.
  */
  template <typename T> struct TypeToEnum<const std::vector<T>&> : TypeToEnum<std::vector<T>> {};

  /**
   * Get the ABI type string for a given ABI type.
   * @param type The ABI type.
   * @return The ABI type string.
   * @throw std::runtime_error if type is not found.
   */
  inline std::string getStringFromABIEnum(Types type) {
    const std::unordered_map<Types, std::string> typeMappings = {
      {Types::uint8, "uint8"}, {Types::uint8Arr, "uint8[]"},
      {Types::uint16, "uint16"}, {Types::uint16Arr, "uint16[]"},
      {Types::uint24, "uint24"}, {Types::uint24Arr, "uint24[]"},
      {Types::uint32, "uint32"}, {Types::uint32Arr, "uint32[]"},
      {Types::uint40, "uint40"}, {Types::uint40Arr, "uint40[]"},
      {Types::uint48, "uint48"}, {Types::uint48Arr, "uint48[]"},
      {Types::uint56, "uint56"}, {Types::uint56Arr, "uint56[]"},
      {Types::uint64, "uint64"}, {Types::uint64Arr, "uint64[]"},
      {Types::uint72, "uint72"}, {Types::uint72Arr, "uint72[]"},
      {Types::uint80, "uint80"}, {Types::uint80Arr, "uint80[]"},
      {Types::uint88, "uint88"}, {Types::uint88Arr, "uint88[]"},
      {Types::uint96, "uint96"}, {Types::uint96Arr, "uint96[]"},
      {Types::uint104, "uint104"}, {Types::uint104Arr, "uint104[]"},
      {Types::uint112, "uint112"}, {Types::uint112Arr, "uint112[]"},
      {Types::uint120, "uint120"}, {Types::uint120Arr, "uint120[]"},
      {Types::uint128, "uint128"}, {Types::uint128Arr, "uint128[]"},
      {Types::uint136, "uint136"}, {Types::uint136Arr, "uint136[]"},
      {Types::uint144, "uint144"}, {Types::uint144Arr, "uint144[]"},
      {Types::uint152, "uint152"}, {Types::uint152Arr, "uint152[]"},
      {Types::uint160, "uint160"}, {Types::uint160Arr, "uint160[]"},
      {Types::uint168, "uint168"}, {Types::uint168Arr, "uint168[]"},
      {Types::uint176, "uint176"}, {Types::uint176Arr, "uint176[]"},
      {Types::uint184, "uint184"}, {Types::uint184Arr, "uint184[]"},
      {Types::uint192, "uint192"}, {Types::uint192Arr, "uint192[]"},
      {Types::uint200, "uint200"}, {Types::uint200Arr, "uint200[]"},
      {Types::uint208, "uint208"}, {Types::uint208Arr, "uint208[]"},
      {Types::uint216, "uint216"}, {Types::uint216Arr, "uint216[]"},
      {Types::uint224, "uint224"}, {Types::uint224Arr, "uint224[]"},
      {Types::uint232, "uint232"}, {Types::uint232Arr, "uint232[]"},
      {Types::uint240, "uint240"}, {Types::uint240Arr, "uint240[]"},
      {Types::uint248, "uint248"}, {Types::uint248Arr, "uint248[]"},
      {Types::uint256, "uint256"}, {Types::uint256Arr, "uint256[]"},
      {Types::int8, "int8"}, {Types::int8Arr, "int8[]"},
      {Types::int16, "int16"}, {Types::int16Arr, "int16[]"},
      {Types::int24, "int24"}, {Types::int24Arr, "int24[]"},
      {Types::int32, "int32"}, {Types::int32Arr, "int32[]"},
      {Types::int40, "int40"}, {Types::int40Arr, "int40[]"},
      {Types::int48, "int48"}, {Types::int48Arr, "int48[]"},
      {Types::int56, "int56"}, {Types::int56Arr, "int56[]"},
      {Types::int64, "int64"}, {Types::int64Arr, "int64[]"},
      {Types::int72, "int72"}, {Types::int72Arr, "int72[]"},
      {Types::int80, "int80"}, {Types::int80Arr, "int80[]"},
      {Types::int88, "int88"}, {Types::int88Arr, "int88[]"},
      {Types::int96, "int96"}, {Types::int96Arr, "int96[]"},
      {Types::int104, "int104"}, {Types::int104Arr, "int104[]"},
      {Types::int112, "int112"}, {Types::int112Arr, "int112[]"},
      {Types::int120, "int120"}, {Types::int120Arr, "int120[]"},
      {Types::int128, "int128"}, {Types::int128Arr, "int128[]"},
      {Types::int136, "int136"}, {Types::int136Arr, "int136[]"},
      {Types::int144, "int144"}, {Types::int144Arr, "int144[]"},
      {Types::int152, "int152"}, {Types::int152Arr, "int152[]"},
      {Types::int160, "int160"}, {Types::int160Arr, "int160[]"},
      {Types::int168, "int168"}, {Types::int168Arr, "int168[]"},
      {Types::int176, "int176"}, {Types::int176Arr, "int176[]"},
      {Types::int184, "int184"}, {Types::int184Arr, "int184[]"},
      {Types::int192, "int192"}, {Types::int192Arr, "int192[]"},
      {Types::int200, "int200"}, {Types::int200Arr, "int200[]"},
      {Types::int208, "int208"}, {Types::int208Arr, "int208[]"},
      {Types::int216, "int216"}, {Types::int216Arr, "int216[]"},
      {Types::int224, "int224"}, {Types::int224Arr, "int224[]"},
      {Types::int232, "int232"}, {Types::int232Arr, "int232[]"},
      {Types::int240, "int240"}, {Types::int240Arr, "int240[]"},
      {Types::int248, "int248"}, {Types::int248Arr, "int248[]"},
      {Types::int256, "int256"}, {Types::int256Arr, "int256[]"},
      {Types::address, "address"}, {Types::addressArr, "address[]"},
      {Types::boolean, "bool"}, {Types::booleanArr, "bool[]"},
      {Types::bytes, "bytes"}, {Types::bytesArr, "bytes[]"},
      {Types::string, "string"}, {Types::stringArr, "string[]"},
      {Types::tuple, "tuple"}
    };

    auto it = typeMappings.find(type);
    if (it == typeMappings.end()) throw std::runtime_error("Unsupported ABI type");
    return it->second;
  }

  /**
   * Get the ABI type for a given ABI type string.
   * @param type The ABI type string.
   * @return The ABI type.
   * @throw std::runtime_error if type is not found.
   */
  inline Types getABIEnumFromString(const std::string& type) {
    static const std::unordered_map<std::string, Types> typeMappings = {
      {"uint8", Types::uint8}, {"uint8[]", Types::uint8Arr},
      {"uint16", Types::uint16}, {"uint16[]", Types::uint16Arr},
      {"uint24", Types::uint24}, {"uint24[]", Types::uint24Arr},
      {"uint32", Types::uint32}, {"uint32[]", Types::uint32Arr},
      {"uint40", Types::uint40}, {"uint40[]", Types::uint40Arr},
      {"uint48", Types::uint48}, {"uint48[]", Types::uint48Arr},
      {"uint56", Types::uint56}, {"uint56[]", Types::uint56Arr},
      {"uint64", Types::uint64}, {"uint64[]", Types::uint64Arr},
      {"uint72", Types::uint72}, {"uint72[]", Types::uint72Arr},
      {"uint80", Types::uint80}, {"uint80[]", Types::uint80Arr},
      {"uint88", Types::uint88}, {"uint88[]", Types::uint88Arr},
      {"uint96", Types::uint96}, {"uint96[]", Types::uint96Arr},
      {"uint104", Types::uint104}, {"uint104[]", Types::uint104Arr},
      {"uint112", Types::uint112}, {"uint112[]", Types::uint112Arr},
      {"uint120", Types::uint120}, {"uint120[]", Types::uint120Arr},
      {"uint128", Types::uint128}, {"uint128[]", Types::uint128Arr},
      {"uint136", Types::uint136}, {"uint136[]", Types::uint136Arr},
      {"uint144", Types::uint144}, {"uint144[]", Types::uint144Arr},
      {"uint152", Types::uint152}, {"uint152[]", Types::uint152Arr},
      {"uint160", Types::uint160}, {"uint160[]", Types::uint160Arr},
      {"uint168", Types::uint168}, {"uint168[]", Types::uint168Arr},
      {"uint176", Types::uint176}, {"uint176[]", Types::uint176Arr},
      {"uint184", Types::uint184}, {"uint184[]", Types::uint184Arr},
      {"uint192", Types::uint192}, {"uint192[]", Types::uint192Arr},
      {"uint200", Types::uint200}, {"uint200[]", Types::uint200Arr},
      {"uint208", Types::uint208}, {"uint208[]", Types::uint208Arr},
      {"uint216", Types::uint216}, {"uint216[]", Types::uint216Arr},
      {"uint224", Types::uint224}, {"uint224[]", Types::uint224Arr},
      {"uint232", Types::uint232}, {"uint232[]", Types::uint232Arr},
      {"uint240", Types::uint240}, {"uint240[]", Types::uint240Arr},
      {"uint248", Types::uint248}, {"uint248[]", Types::uint248Arr},
      {"uint256", Types::uint256}, {"uint256[]", Types::uint256Arr},
      {"int8", Types::int8}, {"int8[]", Types::int8Arr},
      {"int16", Types::int16}, {"int16[]", Types::int16Arr},
      {"int24", Types::int24}, {"int24[]", Types::int24Arr},
      {"int32", Types::int32}, {"int32[]", Types::int32Arr},
      {"int40", Types::int40}, {"int40[]", Types::int40Arr},
      {"int48", Types::int48}, {"int48[]", Types::int48Arr},
      {"int56", Types::int56}, {"int56[]", Types::int56Arr},
      {"int64", Types::int64}, {"int64[]", Types::int64Arr},
      {"int72", Types::int72}, {"int72[]", Types::int72Arr},
      {"int80", Types::int80}, {"int80[]", Types::int80Arr},
      {"int88", Types::int88}, {"int88[]", Types::int88Arr},
      {"int96", Types::int96}, {"int96[]", Types::int96Arr},
      {"int104", Types::int104}, {"int104[]", Types::int104Arr},
      {"int112", Types::int112}, {"int112[]", Types::int112Arr},
      {"int120", Types::int120}, {"int120[]", Types::int120Arr},
      {"int128", Types::int128}, {"int128[]", Types::int128Arr},
      {"int136", Types::int136}, {"int136[]", Types::int136Arr},
      {"int144", Types::int144}, {"int144[]", Types::int144Arr},
      {"int152", Types::int152}, {"int152[]", Types::int152Arr},
      {"int160", Types::int160}, {"int160[]", Types::int160Arr},
      {"int168", Types::int168}, {"int168[]", Types::int168Arr},
      {"int176", Types::int176}, {"int176[]", Types::int176Arr},
      {"int184", Types::int184}, {"int184[]", Types::int184Arr},
      {"int192", Types::int192}, {"int192[]", Types::int192Arr},
      {"int200", Types::int200}, {"int200[]", Types::int200Arr},
      {"int208", Types::int208}, {"int208[]", Types::int208Arr},
      {"int216", Types::int216}, {"int216[]", Types::int216Arr},
      {"int224", Types::int224}, {"int224[]", Types::int224Arr},
      {"int232", Types::int232}, {"int232[]", Types::int232Arr},
      {"int240", Types::int240}, {"int240[]", Types::int240Arr},
      {"int248", Types::int248}, {"int248[]", Types::int248Arr},
      {"int256", Types::int256}, {"int256[]", Types::int256Arr},
      {"address", Types::address}, {"address[]", Types::addressArr},
      {"bool", Types::boolean}, {"bool[]", Types::booleanArr},
      {"bytes", Types::bytes}, {"bytes[]", Types::bytesArr},
      {"string", Types::string}, {"string[]", Types::stringArr},
      {"tuple", Types::tuple}
    };

    auto it = typeMappings.find(type);
    if (it == typeMappings.end()) throw std::runtime_error("Invalid type: " + type);
    return it->second;
  }

  /// Struct for the contract ABI object.
  struct MethodDescription {
    std::string name; ///< Name of the method.
    std::vector<std::pair<std::string, std::string>> inputs; ///< Vector of pairs of input names and types.
    std::vector<std::pair<std::string, std::string>> outputs; ///< Vector of pairs of output names and types.
    std::string stateMutability; ///< State mutability of the method.
    std::string type; ///< Type of the method.
  };

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
     * Encode a function header (functor).
     * @param func The full function header.
     * @return The functor (first 4 bytes of keccak(func)).
     */
    Functor encodeFunction(const std::string_view func);

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
    Bytes encode(const BytesArrView& bytes);

    /**
     * Encode an UTF-8 string.
     * @param str The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const std::string& str);

    /**
     * Encode an array of uint256.
     * @param numV The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const std::vector<uint256_t>& numV);

    /**
     * Encode an array of int256.
     * @param numV The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const std::vector<int256_t>& numV);

    /**
     * Encode an array of addresses.
     * @param addV The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const std::vector<Address>& addV);

    /**
     * Encode an array of booleans.
     * @param bV The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const std::vector<bool>& bV);

    /**
     * Encode an array of raw byte strings.
     * @param bytesV The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const std::vector<Bytes>& bytesV);

    /**
     * Encode an array of UTF-8 strings.
     * @param strV The input to encode.
     * @return The encoded input.
     */
    Bytes encode(const std::vector<std::string>& strV);

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
      } else throw std::runtime_error("The type " + Utils::getRealTypeName<T>() + " is not supported");
    }

    /// Forward declarations.
    template<typename T> struct isTupleOfDynamicTypes;
    template<typename... Ts> struct isTupleOfDynamicTypes<std::tuple<Ts...>>;
    template<typename T> struct isTupleOfDynamicTypes<std::vector<T>>;
        // Type trait to check if T is a std::vector
    template <typename T>
    struct is_vector : std::false_type {};

    template <typename... Args>
    struct is_vector<std::vector<Args...>> : std::true_type {};

    // Helper variable template for is_vector
    template <typename T>
    inline constexpr bool is_vector_v = is_vector<T>::value;

    // Helper to check if a type is a std::tuple
    template<typename T>
    struct is_tuple : std::false_type {};

    template<typename... Ts>
    struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    /**
     * Check if a type is dynamic.
     * @tparam T Any supported ABI type.
     * @return `true` if type is dymanic, `false` otherwise.
     */
    template<typename T> constexpr bool isDynamic() {
      if constexpr (
        std::is_same_v<T, std::vector<uint256_t>> || std::is_same_v<T, std::vector<int256_t>> ||
        std::is_same_v<T, std::vector<Address>> || std::is_same_v<T, std::vector<bool>> ||
        std::is_same_v<T, std::vector<Bytes>> || std::is_same_v<T, std::vector<std::string>> ||
        std::is_same_v<T, BytesArrView> ||
        std::is_same_v<T, std::string> || false
      ) return true;
      if constexpr (is_vector_v<T>) return true;
      if constexpr (isTupleOfDynamicTypes<T>::value) return true;
      return false;
    }

    /// Specialization for a tuple of dynamic types. Defaults to false for unknown types.
    template<typename T> struct isTupleOfDynamicTypes { static constexpr bool value = false; };

    /// Specialization for a tuple of dynamic types, using std::tuple.
    template<typename... Ts> struct isTupleOfDynamicTypes<std::tuple<Ts...>> { static constexpr bool value = (... || isDynamic<Ts>()); };

    /// Specialization for a tuple of dynamic types, using std::vector.
    template<typename T> struct isTupleOfDynamicTypes<std::vector<T>> { static constexpr bool value = isTupleOfDynamicTypes<T>::value; };

    /// Forward declaration.
    template<typename T, typename... Ts> Bytes encode(const T& first, const Ts&... rest);
    template<typename... Ts> Bytes encode(const std::vector<std::tuple<Ts...>>& v);

    /// Calculates the total nextOffset of a given tuple type.
    template<typename T>
    constexpr uint64_t calculateOffsetForType() {
      if constexpr (isDynamic<T>()) {
        return 32;
      } else if constexpr (is_tuple<T>::value) {
        return 32 * std::tuple_size<T>::value;
      } else {
        return 32;
      }
    }

    template <typename... Ts>
    constexpr uint64_t calculateTotalOffset() {
      return (calculateOffsetForType<Ts>() + ...);
    }

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
            nextOffset += 32 * ((packed.size() + 31) / 32);
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

    /// Specialization for encoding a vector of tuples.
    template<typename... Ts> Bytes encode(const std::vector<std::tuple<Ts...>>& v) {
      Bytes result;
      uint64_t nextOffset = 32 * v.size();  // The first 32 bytes are for the length of the dynamic array
      if constexpr (isTupleOfDynamicTypes<std::tuple<Ts...>>::value)
      {
        /// If the tuple is dynamic, we need to account the offsets of each tuple
        Bytes tupleData;
        Bytes tupleOffSets;

        // Encode each tuple.
        for (const auto& t : v) {
          append(tupleOffSets, Utils::uint256ToBytes(nextOffset));
          Bytes tupleBytes = encode(t);  // We're calling the encode function specialized for tuples
          nextOffset += tupleBytes.size();
          tupleData.insert(tupleData.end(), tupleBytes.begin(), tupleBytes.end());
        }

        append(result, Utils::padLeftBytes(Utils::uintToBytes(v.size()), 32));  // Add the array length to the result
        append(result, tupleOffSets);  // Add the tuple offsets
        result.insert(result.end(), tupleData.begin(), tupleData.end());  // Add the tuple data
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
        if constexpr (std::is_same_v<ItemType, Bytes>) { // Convert Bytes to BytesArrView if applicable
          BytesArrView arrView(item.data(), item.size());
          Bytes packed = encode(arrView);  // Call the encode function expecting a BytesArrView
          append(result, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
          nextOffset += 32 * ((packed.size() + 31) / 32);
          dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
        } else if constexpr (isDynamic<ItemType>()) {
          Bytes packed = encode(item);
          append(result, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
          nextOffset += 32 * ((packed.size() + 31) / 32);
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

    // Helper to check if a type is a std::tuple
    template<typename T>
    struct is_tuple : std::false_type {};

    template<typename... Ts>
    struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    // Type trait to check if T is a std::vector
    template <typename T>
    struct is_vector : std::false_type {};

    template <typename... Args>
    struct is_vector<std::vector<Args...>> : std::true_type {};

    // Helper variable template for is_vector
    template <typename T>
    inline constexpr bool is_vector_v = is_vector<T>::value;

    // Type trait to extract the element type of a std::vector
    template <typename T>
    struct vector_element_type {};

    template <typename... Args>
    struct vector_element_type<std::vector<Args...>> {
      using type = typename std::vector<Args...>::value_type;
    };

    // Helper alias template for vector_element_type
    template <typename T>
    using vector_element_type_t = typename vector_element_type<T>::type;

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

    template<typename T>
    struct tuple_type_sequence {};

    template<typename... Args>
    struct tuple_type_sequence<std::tuple<Args...>> {
      using type = std::tuple<Args...>;
    };

    /**
     * Decode a packed std::tuple<Args...> individually
     * This function takes advante of std::tuple_element and template recurssion
     * in order to parse all the items within that given tuple.
     * @param TupleLike The std::tuple<Args...> structure
     * @param I - the current tuple index
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

    /// Forward declarations.
    template<typename T> struct isTupleOfDynamicTypes;
    template<typename... Ts> struct isTupleOfDynamicTypes<std::tuple<Ts...>>;
    template<typename T> struct isTupleOfDynamicTypes<std::vector<T>>;

    /**
     * Check if a type is dynamic.
     * @tparam T Any supported ABI type.
     * @return `true` if type is dymanic, `false` otherwise.
     */
    template<typename T> constexpr bool isDynamic() {
      if constexpr (
        std::is_same_v<T, std::vector<uint256_t>> || std::is_same_v<T, std::vector<int256_t>> ||
        std::is_same_v<T, std::vector<Address>> || std::is_same_v<T, std::vector<bool>> ||
        std::is_same_v<T, std::vector<Bytes>> || std::is_same_v<T, std::vector<std::string>> ||
        std::is_same_v<T, std::string> || false
      ) return true;
      if constexpr (isTupleOfDynamicTypes<T>::value) return true;
      return false;
    }

    /// Specialization for a tuple of dynamic types. Defaults to false for unknown types.
    template<typename T> struct isTupleOfDynamicTypes { static constexpr bool value = false; };

    /// Specialization for a tuple of dynamic types, using std::tuple.
    template<typename... Ts> struct isTupleOfDynamicTypes<std::tuple<Ts...>> { static constexpr bool value = (... || isDynamic<Ts>()); };

    /// Specialization for a tuple of dynamic types, using std::vector.
    template<typename T> struct isTupleOfDynamicTypes<std::vector<T>> { static constexpr bool value = isTupleOfDynamicTypes<T>::value; };

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
      if constexpr (is_tuple<T>::value) {
        T ret;
        if constexpr (isTupleOfDynamicTypes<T>::value)
        {
          if (index +32 > bytes.size()) throw std::runtime_error("Data too short for tuple of dynamic types");
          Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
          uint64_t offset = Utils::fromBigEndian<uint64_t>(tmp);
          index += 32;
          uint64_t newIndex = 0;
          decodeTuple<T>(bytes.subspan(offset), newIndex, ret);
          return ret;
        }
        decodeTuple<T>(bytes, index, ret);
        return ret;
      }

      if constexpr (is_vector_v<T>) {
        using ElementType = vector_element_type_t<T>;
        if constexpr (is_tuple<ElementType>::value) {
          // Handle vector of tuples here
          std::vector<ElementType> retVector;
          // Get array offset
          if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for tuple[]");
          Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
          uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);
          index += 32;

          // Get array length
          tmp.clear();
          if (arrayStart + 32 > bytes.size()) throw std::runtime_error("Data too short for tuple[]");
          tmp.insert(tmp.end(), bytes.begin() + arrayStart, bytes.begin() + arrayStart + 32);
          uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

          for (uint64_t i = 0; i < arrayLength; ++i)
          {
            // Get tuple offset
            tmp.clear();
            tmp.insert(tmp.end(), bytes.begin() + arrayStart + 32 + (i * 32), bytes.begin() + arrayStart + 32 + (i * 32) + 32);
            uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp) + arrayStart + 32;
            ElementType tuple;
            auto view = bytes.subspan(bytesStart);
            uint64_t newIndex = 0;
            decodeTuple<ElementType>(view, newIndex, tuple);
            retVector.emplace_back(tuple);
          }
          return retVector;
        }
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
      } else throw std::runtime_error("The type " + Utils::getRealTypeName<T>() + " is not supported");
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
     * Decode an array of uint256.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline std::vector<uint256_t> decode<std::vector<uint256_t>>(const BytesArrView& bytes, uint64_t& index) {
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for uint[]");
      Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
      uint64_t vectorStart = Utils::fromBigEndian<uint64_t>(tmp);
      index += 32;

      // Get vector length
      tmp.clear();
      if (vectorStart + 32 > bytes.size()) throw std::runtime_error("Data too short for uint[]");
      tmp.insert(tmp.end(), bytes.begin() + vectorStart, bytes.begin() + vectorStart + 32);
      uint64_t vectorLength = Utils::fromBigEndian<uint64_t>(tmp);

      // Size sanity check
      if (vectorStart + 32 + vectorLength * 32 > bytes.size()) throw std::runtime_error("Data too short for uint[]");

      // Get vector data
      std::vector<uint256_t> tmpArr;
      for (uint64_t i = 0; i < vectorLength; i++) {
          tmp.clear();
          tmp.insert(tmp.end(), bytes.begin() + vectorStart + 32 + (i * 32), bytes.begin() + vectorStart + 32 + (i * 32) + 32);
          uint256_t value = Utils::bytesToUint256(tmp);
          tmpArr.emplace_back(value);
      }
      return tmpArr;
    }

    /**
     * Decode an array of int256.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline std::vector<int256_t> decode<std::vector<int256_t>>(const BytesArrView& bytes, uint64_t& index) {
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for int256[]");
      Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
      uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);
      index += 32;
      tmp.clear();

      if (arrayStart + 32 > bytes.size()) throw std::runtime_error("Data too short for int256[]");
      tmp.insert(tmp.end(), bytes.begin() + arrayStart, bytes.begin() + arrayStart + 32);
      uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

      // Size sanity check
      if (arrayStart + 32 + (arrayLength * 32) > bytes.size()) throw std::runtime_error("Data too short for int256[]");

      // Get array data
      std::vector<int256_t> tmpArr;
      for (uint64_t i = 0; i < arrayLength; i++) {
          tmp.clear();
          tmp.insert(tmp.end(), bytes.begin() + arrayStart + 32 + (i * 32), bytes.begin() + arrayStart + 32 + (i * 32) + 32);
          int256_t value = Utils::bytesToInt256(tmp);
          tmpArr.emplace_back(value);
      }
      return tmpArr;
    }

    /**
     * Decode an array of addresses.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline std::vector<Address> decode<std::vector<Address>>(const BytesArrView& bytes, uint64_t& index) {
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for address[]");
      Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
      uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);
      index += 32;

      // Get array length
      tmp.clear();
      if (arrayStart + 32 > bytes.size()) throw std::runtime_error("Data too short for address[]");
      tmp.insert(tmp.end(), bytes.begin() + arrayStart, bytes.begin() + arrayStart + 32);
      uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

      // Size sanity check
      if (arrayStart + 32 + (arrayLength * 32) > bytes.size()) throw std::runtime_error("Data too short for address[]");

      // Get array data
      std::vector<Address> tmpArr;
      for (uint64_t i = 0; i < arrayLength; i++) {
          tmp.clear();
          // Don't forget to skip the first 12 bytes of an address!
          tmp.insert(tmp.end(), bytes.begin() + arrayStart + 32 + (i * 32) + 12, bytes.begin() + arrayStart + 32 + (i * 32) + 32);
          tmpArr.emplace_back(tmp);
      }
      return tmpArr;
    }

    /**
     * Decode an array of booleans.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline std::vector<bool> decode<std::vector<bool>>(const BytesArrView& bytes, uint64_t& index) {
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for bool[]");
      Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
      uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);
      index += 32;

      // Get array length
      tmp.clear();
      if (arrayStart + 32 > bytes.size()) throw std::runtime_error("Data too short for bool[]");
      tmp.insert(tmp.end(), bytes.begin() + arrayStart, bytes.begin() + arrayStart + 32);
      uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

      // Size sanity check
      if (arrayStart + 32 + (arrayLength * 32) > bytes.size()) throw std::runtime_error("Data too short for bool[]");

      // Get array data
      std::vector<bool> tmpArr;
      for (uint64_t i = 0; i < arrayLength; i++) tmpArr.emplace_back((bytes[arrayStart + 32 + (i * 32) + 31] == 0x01));
      return tmpArr;
    }

    /**
     * Decode an array of raw byte strings.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline std::vector<Bytes> decode<std::vector<Bytes>>(const BytesArrView& bytes, uint64_t& index) {
      // Get array offset
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for bytes[]");
      Bytes tmp(bytes.begin() + index, bytes.begin() + index + 32);
      uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);
      index += 32;

      // Get array length
      tmp.clear();
      if (arrayStart + 32 > bytes.size()) throw std::runtime_error("Data too short for bytes[]");
      tmp.insert(tmp.end(), bytes.begin() + arrayStart, bytes.begin() + arrayStart + 32);
      uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

      std::vector<Bytes> tmpVec;
      for (uint64_t i = 0; i < arrayLength; ++i) {
        // Get bytes offset
        tmp.clear();
        tmp.insert(tmp.end(), bytes.begin() + arrayStart + 32 + (i * 32), bytes.begin() + arrayStart + 32 + (i * 32) + 32);
        uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp) + arrayStart + 32;

        // Get bytes length
        tmp.clear();
        tmp.insert(tmp.end(), bytes.begin() + bytesStart, bytes.begin() + bytesStart + 32);
        uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

        // Individual size sanity check
        if (bytesStart + 32 + bytesLength > bytes.size()) throw std::runtime_error("Data too short for bytes[]");

        // Get bytes data
        tmp.clear();
        tmp.insert(tmp.end(), bytes.begin() + bytesStart + 32, bytes.begin() + bytesStart + 32 + bytesLength);
        tmpVec.emplace_back(tmp);
      }
      return tmpVec;
    }

    /**
     * Decode an array of UTF-8 strings.
     * @param bytes The data string to decode.
     * @param index The point on the encoded string to start decoding.
     * @return The decoded data.
     * @throw std::runtime_error if data is too short for the type.
     */
    template <> inline std::vector<std::string> decode<std::vector<std::string>>(const BytesArrView& bytes, uint64_t& index) {
      /// Get array offset
      if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for string[]");
      std::string tmp(bytes.begin() + index, bytes.begin() + index + 32);
      uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);
      index += 32;

      // Get array length
      tmp.clear();
      if (arrayStart + 32 > bytes.size()) throw std::runtime_error("Data too short for string[]");
      tmp.insert(tmp.end(), bytes.begin() + arrayStart, bytes.begin() + arrayStart + 32);
      uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

      std::vector<std::string> tmpVec;
      for (uint64_t i = 0; i < arrayLength; ++i) {
          // Get bytes offset
          tmp.clear();
          tmp.insert(tmp.end(), bytes.begin() + arrayStart + 32 + (i * 32), bytes.begin() + arrayStart + 32 + (i * 32) + 32);
          uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp) + arrayStart + 32;

          // Get bytes length
          tmp.clear();
          tmp.insert(tmp.end(), bytes.begin() + bytesStart, bytes.begin() + bytesStart + 32);
          uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

          // Individual size sanity check
          if (bytesStart + 32 + bytesLength > bytes.size()) throw std::runtime_error("Data too short for string[]");

          // Get bytes data
          tmp.clear();
          tmp.insert(tmp.end(), bytes.begin() + bytesStart + 32, bytes.begin() + bytesStart + 32 + bytesLength);
          tmpVec.emplace_back(tmp);
      }
      return tmpVec;
    }

    // TODO: docs
    template <typename T, typename... Ts> struct TypeList {
      T head;
      TypeList<Ts...> tail;
      TypeList(const BytesArrView& bytes, uint64_t& index) : head(decode<T>(bytes, index)), tail(bytes, index) {}
    };

    // TODO: docs
    template <typename T> struct TypeList<T> {
      T head;
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

    // TODO: docs
    template<typename... Accumulated, typename T, typename... Ts>
    inline auto toTupleHelper(TypeList<T, Ts...>& tl, std::tuple<Accumulated...> acc) {
      return toTupleHelper(tl.tail, std::tuple_cat(acc, std::tuple<T>(tl.head)));
    }

    // TODO: docs
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
        auto typeListResult = TypeList<Args...>(encodedData, index);
        return toTuple(typeListResult);
      }
    }
  };  // namespace Decoder
}; // namespace ABI

#endif // ABI_H
