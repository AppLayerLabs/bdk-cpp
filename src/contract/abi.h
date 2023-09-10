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
 * - uintn = uintn (Solidity) = uintn_t (C++), where n <= 256 and n % 8 == 0
 * - uintnArr = uintn[] (Solidity) = std::vector<uintn_t> (C++), where n <= 256 and n % 8 == 0
 * - address = address (Solidity) = Address (C++)
 * - addressArr = address[] (Solidity) = std::vector<Address> (C++)
 * - bool = bool (Solidity) = bool (C++)
 * - boolArr = bool[] (Solidity) = vector<bool> (C++)
 * - bytes = bytes (Solidity) = Bytes (C++)
 * - bytesArr = bytes[] (Solidity) = std::vector<Bytes> (C++)
 * - string = string (Solidity) = std::string (C++)
 * - stringArr = string[] (Solidity) = std::vector<std::string> (C++)
 */
enum Types {
    uint8, uint8Arr,
    uint16, uint16Arr,
    uint24, uint24Arr,
    uint32, uint32Arr,
    uint40, uint40Arr,
    uint48, uint48Arr,
    uint56, uint56Arr,
    uint64, uint64Arr,
    uint72, uint72Arr,
    uint80, uint80Arr,
    uint88, uint88Arr,
    uint96, uint96Arr,
    uint104, uint104Arr,
    uint112, uint112Arr,
    uint120, uint120Arr,
    uint128, uint128Arr,
    uint136, uint136Arr,
    uint144, uint144Arr,
    uint152, uint152Arr,
    uint160, uint160Arr,
    uint168, uint168Arr,
    uint176, uint176Arr,
    uint184, uint184Arr,
    uint192, uint192Arr,
    uint200, uint200Arr,
    uint208, uint208Arr,
    uint216, uint216Arr,
    uint224, uint224Arr,
    uint232, uint232Arr,
    uint240, uint240Arr,
    uint248, uint248Arr,
    uint256, uint256Arr,

    int8, int8Arr,
    int16, int16Arr,
    int24, int24Arr,
    int32, int32Arr,
    int40, int40Arr,
    int48, int48Arr,
    int56, int56Arr,
    int64, int64Arr,
    int72, int72Arr,
    int80, int80Arr,
    int88, int88Arr,
    int96, int96Arr,
    int104, int104Arr,
    int112, int112Arr,
    int120, int120Arr,
    int128, int128Arr,
    int136, int136Arr,
    int144, int144Arr,
    int152, int152Arr,
    int160, int160Arr,
    int168, int168Arr,
    int176, int176Arr,
    int184, int184Arr,
    int192, int192Arr,
    int200, int200Arr,
    int208, int208Arr,
    int216, int216Arr,
    int224, int224Arr,
    int232, int232Arr,
    int240, int240Arr,
    int248, int248Arr,
    int256, int256Arr,

    address, addressArr,
    boolean, booleanArr,
    bytes, bytesArr,
    string, stringArr
};


/**
* Enum for the types of Solidity functions.
*@tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum;

/**
* Helper struct to map a type to an ABI type.
* Specializations for each type are defined below.
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct ABIType {
  static constexpr Types value = Types::uint256; ///< Default ABI type is uint256.
};


/**
* Specialization for Address.
*/
template <>
struct ABIType<Address> {
  static constexpr Types value = Types::address; ///< ABI type is address.
};

/**
* Specialization for std::vector<Address>.
*/
template <>
struct ABIType<std::vector<Address>> {
  static constexpr Types value = Types::addressArr; ///< ABI type is address.
};

/**
* Specialization for bool.
*/
template <>
struct ABIType<bool> {
  static constexpr Types value = Types::boolean; ///< ABI type is boolean.
};

/**
* Specialization for std::string.
*/
template <>
struct ABIType<std::string> {
  static constexpr Types value = Types::string; ///< ABI type is string.
};

/**
* Specialization for Bytes.
*/
template <>
struct ABIType<Bytes> {
  static constexpr Types value = Types::bytes; ///< ABI type is bytes.
};

/**
* Specialization for uint8_t.
*/
template <>
struct ABIType<uint8_t> {
  static constexpr Types value = Types::uint8; ///< ABI type is uint8.
};

/**
* Specialization for uint16_t.
*/
template <>
struct ABIType<uint16_t> {
  static constexpr Types value = Types::uint16; ///< ABI type is uint16.
};

/**
* Specialization for uint24_t.
*/
template <>
struct ABIType<uint24_t> {
  static constexpr Types value = Types::uint24; ///< ABI type is uint24.
};

/**
* Specialization for uint32_t.
*/
template <>
struct ABIType<uint32_t> {
  static constexpr Types value = Types::uint32; ///< ABI type is uint32.
};

/**
* Specialization for uint40_t.
*/
template <>
struct ABIType<uint40_t> {
  static constexpr Types value = Types::uint40; ///< ABI type is uint40.
};

/**
* Specialization for uint48_t.
*/
template <>
struct ABIType<uint48_t> {
  static constexpr Types value = Types::uint48; ///< ABI type is uint48.
};

/**
* Specialization for uint56_t.
*/
template <>
struct ABIType<uint56_t> {
  static constexpr Types value = Types::uint56; ///< ABI type is uint56.
};

/**
* Specialization for uint64_t.
*/
template <>
struct ABIType<uint64_t> {
  static constexpr Types value = Types::uint64; ///< ABI type is uint64.
};

/**
* Specialization for uint72_t.
*/
template <>
struct ABIType<uint72_t> {
  static constexpr Types value = Types::uint72; ///< ABI type is uint72.
};

/**
* Specialization for uint80_t.
*/
template <>
struct ABIType<uint80_t> {
  static constexpr Types value = Types::uint80; ///< ABI type is uint80.
};

/**
* Specialization for uint88_t.
*/
template <>
struct ABIType<uint88_t> {
  static constexpr Types value = Types::uint88; ///< ABI type is uint88.
};

/**
* Specialization for uint96_t.
*/
template <>
struct ABIType<uint96_t> {
  static constexpr Types value = Types::uint96; ///< ABI type is uint96.
};

/**
* Specialization for uint104_t.
*/
template <>
struct ABIType<uint104_t> {
  static constexpr Types value = Types::uint104; ///< ABI type is uint104.
};

/**
* Specialization for uint112_t.
*/
template <>
struct ABIType<uint112_t> {
  static constexpr Types value = Types::uint112; ///< ABI type is uint112.
};

/**
* Specialization for uint120_t.
*/
template <>
struct ABIType<uint120_t> {
  static constexpr Types value = Types::uint120; ///< ABI type is uint120.
};

/**
* Specialization for uint128_t.
*/
template <>
struct ABIType<uint128_t> {
  static constexpr Types value = Types::uint128; ///< ABI type is uint128.
};

/**
* Specialization for uint136_t.
*/
template <>
struct ABIType<uint136_t> {
  static constexpr Types value = Types::uint136; ///< ABI type is uint136.
};

/**
* Specialization for uint144_t.
*/
template <>
struct ABIType<uint144_t> {
  static constexpr Types value = Types::uint144; ///< ABI type is uint144.
};

/**
* Specialization for uint152_t.
*/
template <>
struct ABIType<uint152_t> {
  static constexpr Types value = Types::uint152; ///< ABI type is uint152.
};

/**
* Specialization for uint160_t.
*/
template <>
struct ABIType<uint160_t> {
  static constexpr Types value = Types::uint160; ///< ABI type is uint160.
};

/**
* Specialization for uint168_t.
*/
template <>
struct ABIType<uint168_t> {
  static constexpr Types value = Types::uint168; ///< ABI type is uint168.
};

/**
* Specialization for uint176_t.
*/
template <>
struct ABIType<uint176_t> {
  static constexpr Types value = Types::uint176; ///< ABI type is uint176.
};

/**
* Specialization for uint184_t.
*/
template <>
struct ABIType<uint184_t> {
  static constexpr Types value = Types::uint184; ///< ABI type is uint184.
};

/**
* Specialization for uint192_t.
*/
template <>
struct ABIType<uint192_t> {
  static constexpr Types value = Types::uint192; ///< ABI type is uint192.
};

/**
* Specialization for uint200_t.
*/
template <>
struct ABIType<uint200_t> {
  static constexpr Types value = Types::uint200; ///< ABI type is uint200.
};

/**
* Specialization for uint208_t.
*/
template <>
struct ABIType<uint208_t> {
  static constexpr Types value = Types::uint208; ///< ABI type is uint208.
};

/**
* Specialization for uint216_t.
*/
template <>
struct ABIType<uint216_t> {
  static constexpr Types value = Types::uint216; ///< ABI type is uint216.
};

/**
* Specialization for uint224_t.
*/
template <>
struct ABIType<uint224_t> {
  static constexpr Types value = Types::uint224; ///< ABI type is uint224.
};

/**
* Specialization for uint232_t.
*/
template <>
struct ABIType<uint232_t> {
  static constexpr Types value = Types::uint232; ///< ABI type is uint232.
};

/**
* Specialization for uint240_t.
*/
template <>
struct ABIType<uint240_t> {
  static constexpr Types value = Types::uint240; ///< ABI type is uint240.
};

/**
* Specialization for uint248_t.
*/
template <>
struct ABIType<uint248_t> {
  static constexpr Types value = Types::uint248; ///< ABI type is uint248.
};

/**
* Specialization for int8_t.
*/
template <>
struct ABIType<int8_t> {
  static constexpr Types value = Types::int8; ///< ABI type is int8.
};

/**
* Specialization for int16_t.
*/
template <>
struct ABIType<int16_t> {
  static constexpr Types value = Types::int16; ///< ABI type is int16.
};

/**
* Specialization for int24_t.
*/
template <>
struct ABIType<int24_t> {
  static constexpr Types value = Types::int24; ///< ABI type is int24.
};

/**
* Specialization for int32_t.
*/
template <>
struct ABIType<int32_t> {
  static constexpr Types value = Types::int32; ///< ABI type is int32.
};

/**
* Specialization for int40_t.
*/
template <>
struct ABIType<int40_t> {
  static constexpr Types value = Types::int40; ///< ABI type is int40.
};

/**
* Specialization for int48_t.
*/
template <>
struct ABIType<int48_t> {
  static constexpr Types value = Types::int48; ///< ABI type is int48.
};

/**
* Specialization for int56_t.
*/
template <>
struct ABIType<int56_t> {
  static constexpr Types value = Types::int56; ///< ABI type is int56.
};

/**
* Specialization for int64_t.
*/
template <>
struct ABIType<int64_t> {
  static constexpr Types value = Types::int64; ///< ABI type is int64.
};

/**
* Specialization for int72_t.
*/
template <>
struct ABIType<int72_t> {
  static constexpr Types value = Types::int72; ///< ABI type is int72.
};

/**
* Specialization for int80_t.
*/
template <>
struct ABIType<int80_t> {
  static constexpr Types value = Types::int80; ///< ABI type is int80.
};

/**
* Specialization for int88_t.
*/
template <>
struct ABIType<int88_t> {
  static constexpr Types value = Types::int88; ///< ABI type is int88.
};

/**
* Specialization for int96_t.
*/
template <>
struct ABIType<int96_t> {
  static constexpr Types value = Types::int96; ///< ABI type is int96.
};

/**
* Specialization for int104_t.
*/
template <>
struct ABIType<int104_t> {
  static constexpr Types value = Types::int104; ///< ABI type is int104.
};

/**
* Specialization for int112_t.
*/
template <>
struct ABIType<int112_t> {
  static constexpr Types value = Types::int112; ///< ABI type is int112.
};

/**
* Specialization for int120_t.
*/
template <>
struct ABIType<int120_t> {
  static constexpr Types value = Types::int120; ///< ABI type is int120.
};

/**
* Specialization for int128_t.
*/
template <>
struct ABIType<int128_t> {
  static constexpr Types value = Types::int128; ///< ABI type is int128.
};

/**
* Specialization for int136_t.
*/
template <>
struct ABIType<int136_t> {
  static constexpr Types value = Types::int136; ///< ABI type is int136.
};

/**
* Specialization for int144_t.
*/
template <>
struct ABIType<int144_t> {
  static constexpr Types value = Types::int144; ///< ABI type is int144.
};

/**
* Specialization for int152_t.
*/
template <>
struct ABIType<int152_t> {
  static constexpr Types value = Types::int152; ///< ABI type is int152.
};

/**
* Specialization for int160_t.
*/
template <>
struct ABIType<int160_t> {
  static constexpr Types value = Types::int160; ///< ABI type is int160.
};

/**
* Specialization for int168_t.
*/
template <>
struct ABIType<int168_t> {
  static constexpr Types value = Types::int168; ///< ABI type is int168.
};

/**
* Specialization for int176_t.
*/
template <>
struct ABIType<int176_t> {
  static constexpr Types value = Types::int176; ///< ABI type is int176.
};

/**
* Specialization for int184_t.
*/
template <>
struct ABIType<int184_t> {
  static constexpr Types value = Types::int184; ///< ABI type is int184.
};

/**
* Specialization for int192_t.
*/
template <>
struct ABIType<int192_t> {
  static constexpr Types value = Types::int192; ///< ABI type is int192.
};

/**
* Specialization for int200_t.
*/
template <>
struct ABIType<int200_t> {
  static constexpr Types value = Types::int200; ///< ABI type is int200.
};

/**
* Specialization for int208_t.
*/
template <>
struct ABIType<int208_t> {
  static constexpr Types value = Types::int208; ///< ABI type is int208.
};

/**
* Specialization for int216_t.
*/
template <>
struct ABIType<int216_t> {
  static constexpr Types value = Types::int216; ///< ABI type is int216.
};

/**
* Specialization for int224_t.
*/
template <>
struct ABIType<int224_t> {
  static constexpr Types value = Types::int224; ///< ABI type is int224.
};

/**
* Specialization for int232_t.
*/
template <>
struct ABIType<int232_t> {
  static constexpr Types value = Types::int232; ///< ABI type is int232.
};

/**
* Specialization for int240_t.
*/
template <>
struct ABIType<int240_t> {
  static constexpr Types value = Types::int240; ///< ABI type is int240.
};

/**
* Specialization for int248_t.
*/
template <>
struct ABIType<int248_t> {
  static constexpr Types value = Types::int248; ///< ABI type is int248.
};

/**
* Specialization for int256_t.
*/
template <>
struct ABIType<int256_t> {
  static constexpr Types value = Types::int256; ///< ABI type is int256.
};

/**
* Specialization for std::vector<T>.
* This is used for all vector types, including bytesArr and stringArr.
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct ABIType<std::vector<T>> {
  static constexpr Types value = static_cast<Types>(static_cast<int>(ABIType<T>::value) + 1); ///< ABI type is vector of T.
};


/**
* Struct to map a type to an ABI type.
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum {
  static constexpr Types value = ABIType<T>::value; ///< ABI type.
};

/**
* Specializations for reference types
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum<T&> : TypeToEnum<T> {};

/**
* Specializations for const reference types
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum<const T&> : TypeToEnum<T> {};

/**
* Specializations for vector reference types
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum<std::vector<T>&> : TypeToEnum<std::vector<T>> {};

/**
* Specializations for const vector reference types
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct TypeToEnum<const std::vector<T>&> : TypeToEnum<std::vector<T>> {};

/**
* Map for calling the correct ABI function for a given uint type.
*/
inline std::unordered_map<Types, std::function<std::any(uint256_t)>> castUintFunctions = {
    {Types::uint8, [](uint256_t value) { return std::any(static_cast<uint8_t>(value)); }},
    {Types::uint16, [](uint256_t value) { return std::any(static_cast<uint16_t>(value)); }},
    {Types::uint24, [](uint256_t value) { return std::any(static_cast<uint24_t>(value)); }},
    {Types::uint32, [](uint256_t value) { return std::any(static_cast<uint32_t>(value)); }},
    {Types::uint40, [](uint256_t value) { return std::any(static_cast<uint40_t>(value)); }},
    {Types::uint48, [](uint256_t value) { return std::any(static_cast<uint48_t>(value)); }},
    {Types::uint56, [](uint256_t value) { return std::any(static_cast<uint56_t>(value)); }},
    {Types::uint64, [](uint256_t value) { return std::any(static_cast<uint64_t>(value)); }},
    {Types::uint72, [](uint256_t value) { return std::any(static_cast<uint72_t>(value)); }},
    {Types::uint80, [](uint256_t value) { return std::any(static_cast<uint80_t>(value)); }},
    {Types::uint88, [](uint256_t value) { return std::any(static_cast<uint88_t>(value)); }},
    {Types::uint96, [](uint256_t value) { return std::any(static_cast<uint96_t>(value)); }},
    {Types::uint104, [](uint256_t value) { return std::any(static_cast<uint104_t>(value)); }},
    {Types::uint112, [](uint256_t value) { return std::any(static_cast<uint112_t>(value)); }},
    {Types::uint120, [](uint256_t value) { return std::any(static_cast<uint120_t>(value)); }},
    {Types::uint128, [](uint256_t value) { return std::any(static_cast<uint128_t>(value)); }},
    {Types::uint136, [](uint256_t value) { return std::any(static_cast<uint136_t>(value)); }},
    {Types::uint144, [](uint256_t value) { return std::any(static_cast<uint144_t>(value)); }},
    {Types::uint152, [](uint256_t value) { return std::any(static_cast<uint152_t>(value)); }},
    {Types::uint160, [](uint256_t value) { return std::any(static_cast<uint160_t>(value)); }},
    {Types::uint168, [](uint256_t value) { return std::any(static_cast<uint168_t>(value)); }},
    {Types::uint176, [](uint256_t value) { return std::any(static_cast<uint176_t>(value)); }},
    {Types::uint184, [](uint256_t value) { return std::any(static_cast<uint184_t>(value)); }},
    {Types::uint192, [](uint256_t value) { return std::any(static_cast<uint192_t>(value)); }},
    {Types::uint200, [](uint256_t value) { return std::any(static_cast<uint200_t>(value)); }},
    {Types::uint208, [](uint256_t value) { return std::any(static_cast<uint208_t>(value)); }},
    {Types::uint216, [](uint256_t value) { return std::any(static_cast<uint216_t>(value)); }},
    {Types::uint224, [](uint256_t value) { return std::any(static_cast<uint224_t>(value)); }},
    {Types::uint232, [](uint256_t value) { return std::any(static_cast<uint232_t>(value)); }},
    {Types::uint240, [](uint256_t value) { return std::any(static_cast<uint240_t>(value)); }},
    {Types::uint248, [](uint256_t value) { return std::any(static_cast<uint248_t>(value)); }}
  };

  /**
   * Map for calling the correct ABI function for a given int type.
   */
  inline std::unordered_map<Types, std::function<std::any(int256_t)>> castIntFunctions = {
    {Types::int8, [](int256_t value) { return std::any(static_cast<int8_t>(value)); }},
    {Types::int16, [](int256_t value) { return std::any(static_cast<int16_t>(value)); }},
    {Types::int24, [](int256_t value) { return std::any(static_cast<int24_t>(value)); }},
    {Types::int32, [](int256_t value) { return std::any(static_cast<int32_t>(value)); }},
    {Types::int40, [](int256_t value) { return std::any(static_cast<int40_t>(value)); }},
    {Types::int48, [](int256_t value) { return std::any(static_cast<int48_t>(value)); }},
    {Types::int56, [](int256_t value) { return std::any(static_cast<int56_t>(value)); }},
    {Types::int64, [](int256_t value) { return std::any(static_cast<int64_t>(value)); }},
    {Types::int72, [](int256_t value) { return std::any(static_cast<int72_t>(value)); }},
    {Types::int80, [](int256_t value) { return std::any(static_cast<int80_t>(value)); }},
    {Types::int88, [](int256_t value) { return std::any(static_cast<int88_t>(value)); }},
    {Types::int96, [](int256_t value) { return std::any(static_cast<int96_t>(value)); }},
    {Types::int104, [](int256_t value) { return std::any(static_cast<int104_t>(value)); }},
    {Types::int112, [](int256_t value) { return std::any(static_cast<int112_t>(value)); }},
    {Types::int120, [](int256_t value) { return std::any(static_cast<int120_t>(value)); }},
    {Types::int128, [](int256_t value) { return std::any(static_cast<int128_t>(value)); }},
    {Types::int136, [](int256_t value) { return std::any(static_cast<int136_t>(value)); }},
    {Types::int144, [](int256_t value) { return std::any(static_cast<int144_t>(value)); }},
    {Types::int152, [](int256_t value) { return std::any(static_cast<int152_t>(value)); }},
    {Types::int160, [](int256_t value) { return std::any(static_cast<int160_t>(value)); }},
    {Types::int168, [](int256_t value) { return std::any(static_cast<int168_t>(value)); }},
    {Types::int176, [](int256_t value) { return std::any(static_cast<int176_t>(value)); }},
    {Types::int184, [](int256_t value) { return std::any(static_cast<int184_t>(value)); }},
    {Types::int192, [](int256_t value) { return std::any(static_cast<int192_t>(value)); }},
    {Types::int200, [](int256_t value) { return std::any(static_cast<int200_t>(value)); }},
    {Types::int208, [](int256_t value) { return std::any(static_cast<int208_t>(value)); }},
    {Types::int216, [](int256_t value) { return std::any(static_cast<int216_t>(value)); }},
    {Types::int224, [](int256_t value) { return std::any(static_cast<int224_t>(value)); }},
    {Types::int232, [](int256_t value) { return std::any(static_cast<int232_t>(value)); }},
    {Types::int240, [](int256_t value) { return std::any(static_cast<int240_t>(value)); }},
    {Types::int248, [](int256_t value) { return std::any(static_cast<int248_t>(value)); }}
  };

/**
 * This function returns the ABI type string for a given ABI type.
 * @param type The ABI type.
 * @return The ABI type string.
 */
std::string inline getStringFromABIEnum(Types type) {
  const std::unordered_map<Types, std::string> typeMappings = {
    {Types::uint8, "uint8"},
    {Types::uint8Arr, "uint8[]"},
    {Types::uint16, "uint16"},
    {Types::uint16Arr, "uint16[]"},
    {Types::uint24, "uint24"},
    {Types::uint24Arr, "uint24[]"},
    {Types::uint32, "uint32"},
    {Types::uint32Arr, "uint32[]"},
    {Types::uint40, "uint40"},
    {Types::uint40Arr, "uint40[]"},
    {Types::uint48, "uint48"},
    {Types::uint48Arr, "uint48[]"},
    {Types::uint56, "uint56"},
    {Types::uint56Arr, "uint56[]"},
    {Types::uint64, "uint64"},
    {Types::uint64Arr, "uint64[]"},
    {Types::uint72, "uint72"},
    {Types::uint72Arr, "uint72[]"},
    {Types::uint80, "uint80"},
    {Types::uint80Arr, "uint80[]"},
    {Types::uint88, "uint88"},
    {Types::uint88Arr, "uint88[]"},
    {Types::uint96, "uint96"},
    {Types::uint96Arr, "uint96[]"},
    {Types::uint104, "uint104"},
    {Types::uint104Arr, "uint104[]"},
    {Types::uint112, "uint112"},
    {Types::uint112Arr, "uint112[]"},
    {Types::uint120, "uint120"},
    {Types::uint120Arr, "uint120[]"},
    {Types::uint128, "uint128"},
    {Types::uint128Arr, "uint128[]"},
    {Types::uint136, "uint136"},
    {Types::uint136Arr, "uint136[]"},
    {Types::uint144, "uint144"},
    {Types::uint144Arr, "uint144[]"},
    {Types::uint152, "uint152"},
    {Types::uint152Arr, "uint152[]"},
    {Types::uint160, "uint160"},
    {Types::uint160Arr, "uint160[]"},
    {Types::uint168, "uint168"},
    {Types::uint168Arr, "uint168[]"},
    {Types::uint176, "uint176"},
    {Types::uint176Arr, "uint176[]"},
    {Types::uint184, "uint184"},
    {Types::uint184Arr, "uint184[]"},
    {Types::uint192, "uint192"},
    {Types::uint192Arr, "uint192[]"},
    {Types::uint200, "uint200"},
    {Types::uint200Arr, "uint200[]"},
    {Types::uint208, "uint208"},
    {Types::uint208Arr, "uint208[]"},
    {Types::uint216, "uint216"},
    {Types::uint216Arr, "uint216[]"},
    {Types::uint224, "uint224"},
    {Types::uint224Arr, "uint224[]"},
    {Types::uint232, "uint232"},
    {Types::uint232Arr, "uint232[]"},
    {Types::uint240, "uint240"},
    {Types::uint240Arr, "uint240[]"},
    {Types::uint248, "uint248"},
    {Types::uint248Arr, "uint248[]"},
    {Types::uint256, "uint256"},
    {Types::uint256Arr, "uint256[]"},
    {Types::int8, "int8"},
    {Types::int8Arr, "int8[]"},
    {Types::int16, "int16"},
    {Types::int16Arr, "int16[]"},
    {Types::int24, "int24"},
    {Types::int24Arr, "int24[]"},
    {Types::int32, "int32"},
    {Types::int32Arr, "int32[]"},
    {Types::int40, "int40"},
    {Types::int40Arr, "int40[]"},
    {Types::int48, "int48"},
    {Types::int48Arr, "int48[]"},
    {Types::int56, "int56"},
    {Types::int56Arr, "int56[]"},
    {Types::int64, "int64"},
    {Types::int64Arr, "int64[]"},
    {Types::int72, "int72"},
    {Types::int72Arr, "int72[]"},
    {Types::int80, "int80"},
    {Types::int80Arr, "int80[]"},
    {Types::int88, "int88"},
    {Types::int88Arr, "int88[]"},
    {Types::int96, "int96"},
    {Types::int96Arr, "int96[]"},
    {Types::int104, "int104"},
    {Types::int104Arr, "int104[]"},
    {Types::int112, "int112"},
    {Types::int112Arr, "int112[]"},
    {Types::int120, "int120"},
    {Types::int120Arr, "int120[]"},
    {Types::int128, "int128"},
    {Types::int128Arr, "int128[]"},
    {Types::int136, "int136"},
    {Types::int136Arr, "int136[]"},
    {Types::int144, "int144"},
    {Types::int144Arr, "int144[]"},
    {Types::int152, "int152"},
    {Types::int152Arr, "int152[]"},
    {Types::int160, "int160"},
    {Types::int160Arr, "int160[]"},
    {Types::int168, "int168"},
    {Types::int168Arr, "int168[]"},
    {Types::int176, "int176"},
    {Types::int176Arr, "int176[]"},
    {Types::int184, "int184"},
    {Types::int184Arr, "int184[]"},
    {Types::int192, "int192"},
    {Types::int192Arr, "int192[]"},
    {Types::int200, "int200"},
    {Types::int200Arr, "int200[]"},
    {Types::int208, "int208"},
    {Types::int208Arr, "int208[]"},
    {Types::int216, "int216"},
    {Types::int216Arr, "int216[]"},
    {Types::int224, "int224"},
    {Types::int224Arr, "int224[]"},
    {Types::int232, "int232"},
    {Types::int232Arr, "int232[]"},
    {Types::int240, "int240"},
    {Types::int240Arr, "int240[]"},
    {Types::int248, "int248"},
    {Types::int248Arr, "int248[]"},
    {Types::int256, "int256"},
    {Types::int256Arr, "int256[]"},
    {Types::address, "address"},
    {Types::addressArr, "address[]"},
    {Types::boolean, "bool"},
    {Types::booleanArr, "bool[]"},
    {Types::bytes, "bytes"},
    {Types::bytesArr, "bytes[]"},
    {Types::string, "string"},
    {Types::stringArr, "string[]"}
  };

  auto it = typeMappings.find(type);
  if (it != typeMappings.end()) {
    return it->second;
  } else {
    throw std::runtime_error("Unsupported ABI type");
  }
}

/**
 * This function returns the ABI type for a given ABI type string.
 * @param type The ABI type string.
 * @return The ABI type.
 */
Types inline getABIEnumFromString(const std::string& type) {
  static const std::unordered_map<std::string, Types> typeMappings = {
    {"uint8", Types::uint8},
    {"uint8[]", Types::uint8Arr},
    {"uint16", Types::uint16},
    {"uint16[]", Types::uint16Arr},
    {"uint24", Types::uint24},
    {"uint24[]", Types::uint24Arr},
    {"uint32", Types::uint32},
    {"uint32[]", Types::uint32Arr},
    {"uint40", Types::uint40},
    {"uint40[]", Types::uint40Arr},
    {"uint48", Types::uint48},
    {"uint48[]", Types::uint48Arr},
    {"uint56", Types::uint56},
    {"uint56[]", Types::uint56Arr},
    {"uint64", Types::uint64},
    {"uint64[]", Types::uint64Arr},
    {"uint72", Types::uint72},
    {"uint72[]", Types::uint72Arr},
    {"uint80", Types::uint80},
    {"uint80[]", Types::uint80Arr},
    {"uint88", Types::uint88},
    {"uint88[]", Types::uint88Arr},
    {"uint96", Types::uint96},
    {"uint96[]", Types::uint96Arr},
    {"uint104", Types::uint104},
    {"uint104[]", Types::uint104Arr},
    {"uint112", Types::uint112},
    {"uint112[]", Types::uint112Arr},
    {"uint120", Types::uint120},
    {"uint120[]", Types::uint120Arr},
    {"uint128", Types::uint128},
    {"uint128[]", Types::uint128Arr},
    {"uint136", Types::uint136},
    {"uint136[]", Types::uint136Arr},
    {"uint144", Types::uint144},
    {"uint144[]", Types::uint144Arr},
    {"uint152", Types::uint152},
    {"uint152[]", Types::uint152Arr},
    {"uint160", Types::uint160},
    {"uint160[]", Types::uint160Arr},
    {"uint168", Types::uint168},
    {"uint168[]", Types::uint168Arr},
    {"uint176", Types::uint176},
    {"uint176[]", Types::uint176Arr},
    {"uint184", Types::uint184},
    {"uint184[]", Types::uint184Arr},
    {"uint192", Types::uint192},
    {"uint192[]", Types::uint192Arr},
    {"uint200", Types::uint200},
    {"uint200[]", Types::uint200Arr},
    {"uint208", Types::uint208},
    {"uint208[]", Types::uint208Arr},
    {"uint216", Types::uint216},
    {"uint216[]", Types::uint216Arr},
    {"uint224", Types::uint224},
    {"uint224[]", Types::uint224Arr},
    {"uint232", Types::uint232},
    {"uint232[]", Types::uint232Arr},
    {"uint240", Types::uint240},
    {"uint240[]", Types::uint240Arr},
    {"uint248", Types::uint248},
    {"uint248[]", Types::uint248Arr},
    {"uint256", Types::uint256},
    {"uint256[]", Types::uint256Arr},
    {"int8", Types::int8},
    {"int8[]", Types::int8Arr},
    {"int16", Types::int16},
    {"int16[]", Types::int16Arr},
    {"int24", Types::int24},
    {"int24[]", Types::int24Arr},
    {"int32", Types::int32},
    {"int32[]", Types::int32Arr},
    {"int40", Types::int40},
    {"int40[]", Types::int40Arr},
    {"int48", Types::int48},
    {"int48[]", Types::int48Arr},
    {"int56", Types::int56},
    {"int56[]", Types::int56Arr},
    {"int64", Types::int64},
    {"int64[]", Types::int64Arr},
    {"int72", Types::int72},
    {"int72[]", Types::int72Arr},
    {"int80", Types::int80},
    {"int80[]", Types::int80Arr},
    {"int88", Types::int88},
    {"int88[]", Types::int88Arr},
    {"int96", Types::int96},
    {"int96[]", Types::int96Arr},
    {"int104", Types::int104},
    {"int104[]", Types::int104Arr},
    {"int112", Types::int112},
    {"int112[]", Types::int112Arr},
    {"int120", Types::int120},
    {"int120[]", Types::int120Arr},
    {"int128", Types::int128},
    {"int128[]", Types::int128Arr},
    {"int136", Types::int136},
    {"int136[]", Types::int136Arr},
    {"int144", Types::int144},
    {"int144[]", Types::int144Arr},
    {"int152", Types::int152},
    {"int152[]", Types::int152Arr},
    {"int160", Types::int160},
    {"int160[]", Types::int160Arr},
    {"int168", Types::int168},
    {"int168[]", Types::int168Arr},
    {"int176", Types::int176},
    {"int176[]", Types::int176Arr},
    {"int184", Types::int184},
    {"int184[]", Types::int184Arr},
    {"int192", Types::int192},
    {"int192[]", Types::int192Arr},
    {"int200", Types::int200},
    {"int200[]", Types::int200Arr},
    {"int208", Types::int208},
    {"int208[]", Types::int208Arr},
    {"int216", Types::int216},
    {"int216[]", Types::int216Arr},
    {"int224", Types::int224},
    {"int224[]", Types::int224Arr},
    {"int232", Types::int232},
    {"int232[]", Types::int232Arr},
    {"int240", Types::int240},
    {"int240[]", Types::int240Arr},
    {"int248", Types::int248},
    {"int248[]", Types::int248Arr},
    {"int256", Types::int256},
    {"int256[]", Types::int256Arr},
    {"address", Types::address},
    {"address[]", Types::addressArr},
    {"bool", Types::boolean},
    {"bool[]", Types::booleanArr},
    {"bytes", Types::bytes},
    {"bytes[]", Types::bytesArr},
    {"string", Types::string},
    {"string[]", Types::stringArr}
  };

  auto it = typeMappings.find(type);
  if (it != typeMappings.end()) {
    return it->second;
  } else {
    throw std::runtime_error("Invalid type: " + type);
  }
}

  /**
  * This struct contains the structure for the contract ABI object.
  */
  struct MethodDescription {
    std::string name; ///< Name of the method.
    std::vector<std::pair<std::string, std::string>> inputs; ///< Vector of pairs of input names and types.
    std::vector<std::pair<std::string, std::string>> outputs; ///< Vector of pairs of output names and types.
    std::string stateMutability; ///< State mutability of the method.
    std::string type; ///< Type of the method.
  };
  namespace Encoder {

    void appendVector(Bytes& dest, const Bytes& src);

    Functor encodeFunction(const std::string_view func);

    Bytes encodeUint(const uint256_t& num);

    Bytes encodeInt(const int256_t& num);

    Bytes encode(const Address& add);

    Bytes encode(const bool& b);

    Bytes encode(const BytesArrView& bytes);

    Bytes encode(const std::string& str);

    Bytes encode(const std::vector<uint256_t>& numV);

    Bytes encode(const std::vector<int256_t>& numV);

    Bytes encode(const std::vector<Address>& addV);

    Bytes encode(const std::vector<bool>& bV);

    Bytes encode(const std::vector<Bytes>& bytesV);

    Bytes encode(const std::vector<std::string>& strV);

    template <typename T>
    Bytes encode(const T& num) {
        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int24_t> ||
                        std::is_same_v<T, int32_t> || std::is_same_v<T, int40_t> || std::is_same_v<T, int48_t> ||
                        std::is_same_v<T, int56_t> || std::is_same_v<T, int64_t> || std::is_same_v<T, int72_t> ||
                        std::is_same_v<T, int80_t> || std::is_same_v<T, int88_t> || std::is_same_v<T, int96_t> ||
                        std::is_same_v<T, int104_t> || std::is_same_v<T, int112_t> || std::is_same_v<T, int120_t> ||
                        std::is_same_v<T, int128_t> || std::is_same_v<T, int136_t> || std::is_same_v<T, int144_t> ||
                        std::is_same_v<T, int152_t> || std::is_same_v<T, int160_t> || std::is_same_v<T, int168_t> ||
                        std::is_same_v<T, int176_t> || std::is_same_v<T, int184_t> || std::is_same_v<T, int192_t> ||
                        std::is_same_v<T, int200_t> || std::is_same_v<T, int208_t> || std::is_same_v<T, int216_t> ||
                        std::is_same_v<T, int224_t> || std::is_same_v<T, int232_t> || std::is_same_v<T, int240_t> ||
                        std::is_same_v<T, int248_t> || std::is_same_v<T, int256_t>) {
            return encodeInt(num);
        } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint24_t> ||
                        std::is_same_v<T, uint32_t> || std::is_same_v<T, uint40_t> || std::is_same_v<T, uint48_t> ||
                        std::is_same_v<T, uint56_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, uint72_t> ||
                        std::is_same_v<T, uint80_t> || std::is_same_v<T, uint88_t> || std::is_same_v<T, uint96_t> ||
                        std::is_same_v<T, uint104_t> || std::is_same_v<T, uint112_t> || std::is_same_v<T, uint120_t> ||
                        std::is_same_v<T, uint128_t> || std::is_same_v<T, uint136_t> || std::is_same_v<T, uint144_t> ||
                        std::is_same_v<T, uint152_t> || std::is_same_v<T, uint160_t> || std::is_same_v<T, uint168_t> ||
                        std::is_same_v<T, uint176_t> || std::is_same_v<T, uint184_t> || std::is_same_v<T, uint192_t> ||
                        std::is_same_v<T, uint200_t> || std::is_same_v<T, uint208_t> || std::is_same_v<T, uint216_t> ||
                        std::is_same_v<T, uint224_t> || std::is_same_v<T, uint232_t> || std::is_same_v<T, uint240_t> ||
                        std::is_same_v<T, uint248_t> || std::is_same_v<T, uint256_t>) {
            return encodeUint(num);
        } else {
            throw std::runtime_error("The type " + Utils::getRealTypeName<T>() + " is not supported");
        }
    }

    template<typename T>
    struct isTupleOfDynamicTypes;

    template<typename... Ts>
    struct isTupleOfDynamicTypes<std::tuple<Ts...>>;

    template<typename T>
    struct isTupleOfDynamicTypes<std::vector<T>>;

    // Function to check if a type is dynamic
    template<typename T>
    constexpr bool isDynamic() {
        if constexpr (std::is_same_v<T, std::vector<uint256_t>> || 
                              std::is_same_v<T, std::vector<int256_t>>  ||
                              std::is_same_v<T, std::vector<Address>>   ||
                              std::is_same_v<T, std::vector<bool>>      ||
                              std::is_same_v<T, std::vector<Bytes>> ||
                              std::is_same_v<T, std::vector<std::string>> ||
                              std::is_same_v<T, std::string> ||
                              false) {
            return true;
        }
        else if constexpr (isTupleOfDynamicTypes<T>::value) {
            return true;
        }
        else {
            return false;
        }
    }

    template<typename T>
    struct isTupleOfDynamicTypes {
        static constexpr bool value = false; // default to false for unknown types
    };

    template<typename... Ts>
    struct isTupleOfDynamicTypes<std::tuple<Ts...>> {
        static constexpr bool value = (... || isDynamic<Ts>());
    };

    template<typename T>
    struct isTupleOfDynamicTypes<std::vector<T>> {
        static constexpr bool value = isTupleOfDynamicTypes<T>::value;
    };

    // Forward declaration
    template<typename T, typename... Ts>
    Bytes encode(const T& first, const Ts&... rest);

    // Specialization for encoding a tuple... Expand and call back encode<T,Ts...>
    template<typename... Ts>
    Bytes encode(const std::tuple<Ts...>& t) {
        Bytes result;
        Bytes dynamicBytes;
        uint64_t nextOffset = 32 * sizeof...(Ts);

        std::apply([&](const auto&... args) {
            auto encodeItem = [&](auto&& item) {
                using ItemType = std::decay_t<decltype(item)>;
                if (isDynamic<ItemType>()) {
                    Bytes packed = encode(item);
                    appendVector(result, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
                    nextOffset += 32 * ((packed.size() + 31) / 32);
                    dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
                } else {
                    appendVector(result, encode(item));
                }
            };
            
            (encodeItem(args), ...);
        }, t);
        
        result.insert(result.end(), dynamicBytes.begin(), dynamicBytes.end());
        return result;
    }

    // Specialization for encoding a vector of tuples
    template<typename... Ts>
    Bytes encode(const std::vector<std::tuple<Ts...>>& v) {
        Bytes result;
        uint64_t nextOffset = 32;  // The first 32 bytes are for the length of the dynamic array
        Bytes dynamicBytes;
        Bytes tupleData;

        // Encode each tuple
        for (const auto& t : v) {
            Bytes tupleBytes = encode(t);  // We're calling the encode function specialized for tuples
            nextOffset += tupleBytes.size();
            tupleData.insert(tupleData.end(), tupleBytes.begin(), tupleBytes.end());
        }

        // Add the array length to the result
        appendVector(result, Utils::padLeftBytes(Utils::uintToBytes(v.size()), 32));

        // Add the tuple data
        result.insert(result.end(), tupleData.begin(), tupleData.end());
        
        return result;
    }

    // The main encode function
    template<typename T, typename... Ts>
     Bytes encodeData(const T& first, const Ts&... rest) {
      Bytes result;
      uint64_t nextOffset = 32 * (sizeof...(Ts) + 1);
      Bytes dynamicBytes;

      auto encodeItem = [&](auto&& item) {
        using ItemType = std::decay_t<decltype(item)>;

        // Convert Bytes to BytesArrView if applicable
        if constexpr (std::is_same_v<ItemType, Bytes>) {
          BytesArrView arrView(item.data(), item.size());
          Bytes packed = encode(arrView);  // Call the encode function expecting a BytesArrView
          appendVector(result, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
          nextOffset += 32 * ((packed.size() + 31) / 32);
          dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
        }
        else if constexpr (isDynamic<ItemType>()) {
          Bytes packed = encode(item);
          appendVector(result, Utils::padLeftBytes(Utils::uintToBytes(nextOffset), 32));
          nextOffset += 32 * ((packed.size() + 31) / 32);
          dynamicBytes.insert(dynamicBytes.end(), packed.begin(), packed.end());
        }
        else {
          appendVector(result, encode(item));
        }
      };

      encodeItem(first);
      (encodeItem(rest), ...);

      result.insert(result.end(), dynamicBytes.begin(), dynamicBytes.end());
      return result;
    }
  }

  namespace Decoder {

    template <typename T, typename... Ts>
    struct TypeList;

    template<typename T>
    inline T decode(const BytesArrView& bytes, uint64_t& index);

    uint256_t decodeUint(const BytesArrView& bytes, uint64_t& index);

    int256_t decodeInt(const BytesArrView& bytes, uint64_t& index);

    std::vector<BaseTypes> decodeDataTypes(const std::vector<ABI::Types>& types, const BytesArrView& encodedData);

    template <typename T> T getData(std::vector<BaseTypes> data, const uint64_t index) {
        if (index >= data.size()) throw std::out_of_range("Index out of range");
        if (std::holds_alternative<T>(data[index])) return std::get<T>(data[index]);
        throw std::runtime_error("Type mismatch");
    }

    std::any inline getDataDispatch(int index, Types type, std::vector<BaseTypes> data){
      switch (type) {
        case Types::uint256:
        case Types::uint8:
        case Types::uint16:
        case Types::uint24:
        case Types::uint32:
        case Types::uint40:
        case Types::uint48:
        case Types::uint56:
        case Types::uint64:
        case Types::uint72:
        case Types::uint80:
        case Types::uint88:
        case Types::uint96:
        case Types::uint104:
        case Types::uint112:
        case Types::uint120:
        case Types::uint128:
        case Types::uint136:
        case Types::uint144:
        case Types::uint152:
        case Types::uint160:
        case Types::uint168:
        case Types::uint176:
        case Types::uint184:
        case Types::uint192:
        case Types::uint200:
        case Types::uint208:
        case Types::uint216:
        case Types::uint224:
        case Types::uint232:
        case Types::uint240:
        case Types::uint248:
        return getData<uint256_t>(data, index);
        case Types::int256:
        case Types::int8:
        case Types::int16:
        case Types::int24:
        case Types::int32:
        case Types::int40:
        case Types::int48:
        case Types::int56:
        case Types::int64:
        case Types::int72:
        case Types::int80:
        case Types::int88:
        case Types::int96:
        case Types::int104:
        case Types::int112:
        case Types::int120:
        case Types::int128:
        case Types::int136:
        case Types::int144:
        case Types::int152:
        case Types::int160:
        case Types::int168:
        case Types::int176:
        case Types::int184:
        case Types::int192:
        case Types::int200:
        case Types::int208:
        case Types::int216:
        case Types::int224:
        case Types::int232:
        case Types::int240:
        case Types::int248:
        return getData<int256_t>(data, index);
        case Types::uint8Arr:
        case Types::uint16Arr:
        case Types::uint24Arr:
        case Types::uint32Arr:
        case Types::uint40Arr:
        case Types::uint48Arr:
        case Types::uint56Arr:
        case Types::uint64Arr:
        case Types::uint72Arr:
        case Types::uint80Arr:
        case Types::uint88Arr:
        case Types::uint96Arr:
        case Types::uint104Arr:
        case Types::uint112Arr:
        case Types::uint120Arr:
        case Types::uint128Arr:
        case Types::uint136Arr:
        case Types::uint144Arr:
        case Types::uint152Arr:
        case Types::uint160Arr:
        case Types::uint168Arr:
        case Types::uint176Arr:
        case Types::uint184Arr:
        case Types::uint192Arr:
        case Types::uint200Arr:
        case Types::uint208Arr:
        case Types::uint216Arr:
        case Types::uint224Arr:
        case Types::uint232Arr:
        case Types::uint240Arr:
        case Types::uint248Arr:
        case Types::uint256Arr:
        return getData<std::vector<uint256_t>>(data, index);
        case Types::int8Arr:
        case Types::int16Arr:
        case Types::int24Arr:
        case Types::int32Arr:
        case Types::int40Arr:
        case Types::int48Arr:
        case Types::int56Arr:
        case Types::int64Arr:
        case Types::int72Arr:
        case Types::int80Arr:
        case Types::int88Arr:
        case Types::int96Arr:
        case Types::int104Arr:
        case Types::int112Arr:
        case Types::int120Arr:
        case Types::int128Arr:
        case Types::int136Arr:
        case Types::int144Arr:
        case Types::int152Arr:
        case Types::int160Arr:
        case Types::int168Arr:
        case Types::int176Arr:
        case Types::int184Arr:
        case Types::int192Arr:
        case Types::int200Arr:
        case Types::int208Arr:
        case Types::int216Arr:
        case Types::int224Arr:
        case Types::int232Arr:
        case Types::int240Arr:
        case Types::int248Arr:
        case Types::int256Arr:
        return getData<std::vector<int256_t>>(data, index);
        case Types::address: return getData<Address>(data, index);
        case Types::addressArr: return getData<std::vector<Address>>(data, index);
        case Types::boolean: return getData<bool>(data, index);
        case Types::booleanArr: return getData<std::vector<bool>>(data, index);
        case Types::bytes: return getData<Bytes>(data, index);
        case Types::bytesArr: return getData<std::vector<Bytes>>(data, index);
        case Types::string: return getData<std::string>(data, index);
        case Types::stringArr: return getData<std::vector<std::string>>(data, index);
        default: throw std::runtime_error("Invalid ABI::Types type: " + getStringFromABIEnum(type));
      }
    }

    template <typename T>
    inline T decode(const BytesArrView& bytes, uint64_t& index) {
        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int24_t> ||
                        std::is_same_v<T, int32_t> || std::is_same_v<T, int40_t> || std::is_same_v<T, int48_t> ||
                        std::is_same_v<T, int56_t> || std::is_same_v<T, int64_t> || std::is_same_v<T, int72_t> ||
                        std::is_same_v<T, int80_t> || std::is_same_v<T, int88_t> || std::is_same_v<T, int96_t> ||
                        std::is_same_v<T, int104_t> || std::is_same_v<T, int112_t> || std::is_same_v<T, int120_t> ||
                        std::is_same_v<T, int128_t> || std::is_same_v<T, int136_t> || std::is_same_v<T, int144_t> ||
                        std::is_same_v<T, int152_t> || std::is_same_v<T, int160_t> || std::is_same_v<T, int168_t> ||
                        std::is_same_v<T, int176_t> || std::is_same_v<T, int184_t> || std::is_same_v<T, int192_t> ||
                        std::is_same_v<T, int200_t> || std::is_same_v<T, int208_t> || std::is_same_v<T, int216_t> ||
                        std::is_same_v<T, int224_t> || std::is_same_v<T, int232_t> || std::is_same_v<T, int240_t> ||
                        std::is_same_v<T, int248_t> || std::is_same_v<T, int256_t>) {
            return decodeInt(bytes, index);
        } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint24_t> ||
                        std::is_same_v<T, uint32_t> || std::is_same_v<T, uint40_t> || std::is_same_v<T, uint48_t> ||
                        std::is_same_v<T, uint56_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, uint72_t> ||
                        std::is_same_v<T, uint80_t> || std::is_same_v<T, uint88_t> || std::is_same_v<T, uint96_t> ||
                        std::is_same_v<T, uint104_t> || std::is_same_v<T, uint112_t> || std::is_same_v<T, uint120_t> ||
                        std::is_same_v<T, uint128_t> || std::is_same_v<T, uint136_t> || std::is_same_v<T, uint144_t> ||
                        std::is_same_v<T, uint152_t> || std::is_same_v<T, uint160_t> || std::is_same_v<T, uint168_t> ||
                        std::is_same_v<T, uint176_t> || std::is_same_v<T, uint184_t> || std::is_same_v<T, uint192_t> ||
                        std::is_same_v<T, uint200_t> || std::is_same_v<T, uint208_t> || std::is_same_v<T, uint216_t> ||
                        std::is_same_v<T, uint224_t> || std::is_same_v<T, uint232_t> || std::is_same_v<T, uint240_t> ||
                        std::is_same_v<T, uint248_t> || std::is_same_v<T, uint256_t>) {
            return decodeUint(bytes, index);
        } else {
            throw std::runtime_error("The type " + Utils::getRealTypeName<T>() + " is not supported");
        }
    }

    template <>
    inline Address decode<Address>(const BytesArrView& bytes, uint64_t& index) {
        BytesArrView data(bytes);
        if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for address");
        Address result = Address(data.subspan(index + 12, 20));
        index += 32;
        return result;
    }

    template <>
    inline bool decode<bool>(const BytesArrView& bytes, uint64_t& index) {
        BytesArrView data(bytes);
        if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for bool");
        bool result = (data[index + 31] == 0x01);
        index += 32;
        return result;
    }

    template <>
    inline Bytes decode<Bytes>(const BytesArrView& bytes, uint64_t& index) {
        BytesArrView data(bytes);
        if (index + 32 > data.size()) throw std::runtime_error("Data too short for bytes");
        Bytes tmp(data.begin() + index, data.begin() + index + 32);
        uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp);

        index += 32;

        // Get bytes length
        tmp.clear();
        if (bytesStart + 32 > data.size()) throw std::runtime_error("Data too short for bytes");
        tmp.insert(tmp.end(), data.begin() + bytesStart, data.begin() + bytesStart + 32);
        uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

        // Size sanity check
        if (bytesStart + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short for bytes");

        // Get bytes data
        tmp.clear();
        tmp.insert(tmp.end(), data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength);
        return tmp;
    }

    template <>
    inline std::vector<uint256_t> decode<std::vector<uint256_t>>(const BytesArrView& bytes, uint64_t& index) {
        BytesArrView data(bytes);
        if (index + 32 > data.size()) throw std::runtime_error("Data too short for uint[]");
        Bytes tmp(data.begin() + index, data.begin() + index + 32);
        uint64_t vectorStart = Utils::fromBigEndian<uint64_t>(tmp);

        index += 32;

        // Get vector length
        tmp.clear();
        if (vectorStart + 32 > data.size()) throw std::runtime_error("Data too short for uint[]");
        tmp.insert(tmp.end(), data.begin() + vectorStart, data.begin() + vectorStart + 32);
        uint64_t vectorLength = Utils::fromBigEndian<uint64_t>(tmp);

        // Size sanity check
        if (vectorStart + 32 + vectorLength * 32 > data.size()) throw std::runtime_error("Data too short for uint[]");

        // Get vector data
        std::vector<uint256_t> tmpArr;
        for (uint64_t i = 0; i < vectorLength; i++) {
            tmp.clear();
            tmp.insert(tmp.end(), data.begin() + vectorStart + 32 + (i * 32), data.begin() + vectorStart + 32 + (i * 32) + 32);
            uint256_t value = Utils::bytesToUint256(tmp);
            tmpArr.emplace_back(value);
        }

        return tmpArr;
    }

    template <>
    inline std::vector<int256_t> decode<std::vector<int256_t>>(const BytesArrView& bytes, uint64_t& index) {
        BytesArrView data(bytes);
        if (index + 32 > data.size()) throw std::runtime_error("Data too short for int256[]");
        Bytes tmp(data.begin() + index, data.begin() + index + 32);
        uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

        index += 32;
        tmp.clear();

        if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for int256[]");
        tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
        uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

        // Size sanity check
        if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short for int256[]");

        // Get array data
        std::vector<int256_t> tmpArr;
        for (uint64_t i = 0; i < arrayLength; i++) {
            tmp.clear();
            tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32);
            int256_t value = Utils::bytesToInt256(tmp); // Change this line to use bytesToInt256
            tmpArr.emplace_back(value);
        }

        return tmpArr;
    }

    template <>
    inline std::vector<Address> decode<std::vector<Address>>(const BytesArrView& bytes, uint64_t& index) {
        BytesArrView data(bytes);
        if (index + 32 > data.size()) throw std::runtime_error("Data too short for address[]");
        Bytes tmp(data.begin() + index, data.begin() + index + 32);
        uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

        index += 32;

        // Get array length
        tmp.clear();
        if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for address[]");
        tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
        uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

        // Size sanity check
        if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short for address[]");

        // Get array data
        std::vector<Address> tmpArr;
        for (uint64_t i = 0; i < arrayLength; i++) {
            tmp.clear();
            // Don't forget to skip the first 12 bytes of an address!
            tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32) + 12, data.begin() + arrayStart + 32 + (i * 32) + 32);
            tmpArr.emplace_back(tmp);
        }

        return tmpArr;
    }

    template <>
    inline std::vector<bool> decode<std::vector<bool>>(const BytesArrView& bytes, uint64_t& index) {
        BytesArrView data(bytes);
        if (index + 32 > data.size()) throw std::runtime_error("Data too short for bool[]");
        Bytes tmp(data.begin() + index, data.begin() + index + 32);
        uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

        index += 32;

        // Get array length
        tmp.clear();
        if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for bool[]");
        tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
        uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

        // Size sanity check
        if (arrayStart + 32 + (arrayLength * 32) > data.size()) throw std::runtime_error("Data too short for bool[]");

        // Get array data
        std::vector<bool> tmpArr;
        for (uint64_t i = 0; i < arrayLength; i++) tmpArr.emplace_back(
            (data[arrayStart + 32 + (i * 32) + 31] == 0x01)
        );
        return tmpArr;
    }

    template <>
    inline std::vector<Bytes> decode<std::vector<Bytes>>(const BytesArrView& bytes, uint64_t& index) {
        // Get array offset
        BytesArrView data(bytes);
        if (index + 32 > data.size()) throw std::runtime_error("Data too short for bytes[]");
        Bytes tmp(data.begin() + index, data.begin() + index + 32);
        uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

        index += 32;

        // Get array length
        tmp.clear();
        if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for bytes[]");
        tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
        uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

        std::vector<Bytes> tmpVec;
        for (uint64_t i = 0; i < arrayLength; ++i) {
            // Get bytes offset
            tmp.clear();
            tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32);
            uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp) + arrayStart + 32;

            // Get bytes length
            tmp.clear();
            tmp.insert(tmp.end(), data.begin() + bytesStart, data.begin() + bytesStart + 32);
            uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

            // Individual size sanity check
            if (bytesStart + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short for bytes[]");

            // Get bytes data
            tmp.clear();
            tmp.insert(tmp.end(), data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength);
            tmpVec.emplace_back(tmp);
        }

        return tmpVec;
    }

    template <>
    inline std::vector<std::string> decode<std::vector<std::string>>(const BytesArrView& bytes, uint64_t& index) {
        /// Get array offset
        BytesArrView data(bytes);
        if (index + 32 > data.size()) throw std::runtime_error("Data too short for string[]");
        std::string tmp(data.begin() + index, data.begin() + index + 32);
        uint64_t arrayStart = Utils::fromBigEndian<uint64_t>(tmp);

        index += 32;

        // Get array length
        tmp.clear();
        if (arrayStart + 32 > data.size()) throw std::runtime_error("Data too short for string[]");
        tmp.insert(tmp.end(), data.begin() + arrayStart, data.begin() + arrayStart + 32);
        uint64_t arrayLength = Utils::fromBigEndian<uint64_t>(tmp);

        std::vector<std::string> tmpVec;
        for (uint64_t i = 0; i < arrayLength; ++i) {
            // Get bytes offset
            tmp.clear();
            tmp.insert(tmp.end(), data.begin() + arrayStart + 32 + (i * 32), data.begin() + arrayStart + 32 + (i * 32) + 32);
            uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp) + arrayStart + 32;

            // Get bytes length
            tmp.clear();
            tmp.insert(tmp.end(), data.begin() + bytesStart, data.begin() + bytesStart + 32);
            uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

            // Individual size sanity check
            if (bytesStart + 32 + bytesLength > data.size()) throw std::runtime_error("Data too short for string[]");

            // Get bytes data
            tmp.clear();
            tmp.insert(tmp.end(), data.begin() + bytesStart + 32, data.begin() + bytesStart + 32 + bytesLength);
            tmpVec.emplace_back(tmp);
        }

        return tmpVec;
    }

    template <>
    inline std::string decode<std::string>(const BytesArrView& bytes, uint64_t& index) {
        if (index + 32 > bytes.size()) throw std::runtime_error("Data too short for string");
        std::string tmp(bytes.begin() + index, bytes.begin() + index + 32);
        uint64_t bytesStart = Utils::fromBigEndian<uint64_t>(tmp);

        index += 32;  // Move index to next 32 bytes

        // Get bytes length
        tmp.clear();
        if (bytesStart + 32 > bytes.size()) throw std::runtime_error("Data too short for string");
        tmp.insert(tmp.end(), bytes.begin() + bytesStart, bytes.begin() + bytesStart + 32);
        uint64_t bytesLength = Utils::fromBigEndian<uint64_t>(tmp);

        // Size sanity check
        if (bytesStart + 32 + bytesLength > bytes.size()) throw std::runtime_error("Data too short for string");

        // Get bytes data
        tmp.clear();
        tmp.insert(tmp.end(), bytes.begin() + bytesStart + 32, bytes.begin() + bytesStart + 32 + bytesLength);
        return tmp;
    }



    template <typename T, typename... Ts>
    struct TypeList {
        T head;
        TypeList<Ts...> tail;

        TypeList(const BytesArrView& bytes, uint64_t& index) : head(decode<T>(bytes, index)), tail(bytes, index) {}
    };

    template <typename T>
    struct TypeList<T> {
        T head;

        TypeList(const BytesArrView& bytes, uint64_t& index) : head(decode<T>(bytes, index)) {}
    };

    template <typename... Args>
    inline std::tuple<Args...> toTuple(TypeList<Args...>& tl) {
        return toTupleHelper(tl, std::tuple<>());
    }

    template<typename... Accumulated, typename T, typename... Ts>
    inline auto toTupleHelper(TypeList<T, Ts...>& tl, std::tuple<Accumulated...> acc) {
        return toTupleHelper(tl.tail, std::tuple_cat(acc, std::tuple<T>(tl.head)));
    }

    template<typename... Accumulated, typename T>
    inline auto toTupleHelper(TypeList<T>& tl, std::tuple<Accumulated...> acc) {
        return std::tuple_cat(acc, std::tuple<T>(tl.head));
    }

    template<typename... Args>
    inline std::tuple<Args...> decodeData(const BytesArrView& encodedData, uint64_t index = 0) {
        auto typeListResult = TypeList<Args...>(encodedData, index);
        return toTuple(typeListResult);
    }
  }
}; // namespace ABI

#endif // ABI_H
