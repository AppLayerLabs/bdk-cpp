#ifndef ABI_H
#define ABI_H

#include <string>
#include <any>

#include "../utils/hex.h"
#include "../utils/json.hpp"
#include "../utils/utils.h"

/// Namespace for Solidity ABI-related operations.
namespace ABI {
  /**
 * Enum for the types of Solidity variables.
 * Equivalency is as follows:
 * - uint8 = uint8 (Solidity) = uint8_t (C++)
 * - uint8Arr = uint8[] (Solidity) = std::vector<uint8_t> (C++)
 * - uint16 = uint16 (Solidity) = uint16_t (C++)
 * - uint16Arr = uint16[] (Solidity) = std::vector<uint16_t> (C++)
 * - uint24 = uint24 (Solidity) = uint24_t (C++)
  * - uint24Arr = uint24[] (Solidity) = std::vector<uint24_t> (C++)
 * - uint32 = uint32 (Solidity) = uint32_t (C++)
 * - uint32Arr = uint32[] (Solidity) = std::vector<uint32_t> (C++)
 * - uint40 = uint40 (Solidity) = uint40_t (C++)
 * - uint40Arr = uint40[] (Solidity) = std::vector<uint40_t> (C++)
 * - uint48 = uint48 (Solidity) = uint48_t (C++)
 * - uint48Arr = uint48[] (Solidity) = std::vector<uint48_t> (C++)
 * - uint56 = uint56 (Solidity) = uint56_t (C++)
 * - uint56Arr = uint56[] (Solidity) = std::vector<uint56_t> (C++)
 * - uint64 = uint64 (Solidity) = uint64_t (C++)
 * - uint64Arr = uint64[] (Solidity) = std::vector<uint64_t> (C++)
 * - uint72 = uint72 (Solidity) = uint72_t (C++)
 * - uint72Arr = uint72[] (Solidity) = std::vector<uint72_t> (C++)
 * - uint80 = uint80 (Solidity) = uint80_t (C++)
 * - uint80Arr = uint80[] (Solidity) = std::vector<uint80_t> (C++)
 * - uint88 = uint88 (Solidity) = uint88_t (C++)
 * - uint88Arr = uint88[] (Solidity) = std::vector<uint88_t> (C++)
 * - uint96 = uint96 (Solidity) = uint96_t (C++)
 * - uint96Arr = uint96[] (Solidity) = std::vector<uint96_t> (C++)
 * - uint104 = uint104 (Solidity) = uint104_t (C++)
 * - uint104Arr = uint104[] (Solidity) = std::vector<uint104_t> (C++)
 * - uint112 = uint112 (Solidity) = uint112_t (C++)
 * - uint112Arr = uint112[] (Solidity) = std::vector<uint112_t> (C++)
 * - uint120 = uint120 (Solidity) = uint120_t (C++)
 * - uint120Arr = uint120[] (Solidity) = std::vector<uint120_t> (C++)
 * - uint128 = uint128 (Solidity) = uint128_t (C++)
 * - uint128Arr = uint128[] (Solidity) = std::vector<uint128_t> (C++)
 * - uint136 = uint136 (Solidity) = uint136_t (C++)
 * - uint136Arr = uint136[] (Solidity) = std::vector<uint136_t> (C++)
 * - uint144 = uint144 (Solidity) = uint144_t (C++)
 * - uint144Arr = uint144[] (Solidity) = std::vector<uint144_t> (C++)
 * - uint152 = uint152 (Solidity) = uint152_t (C++)
 * - uint152Arr = uint152[] (Solidity) = std::vector<uint152_t> (C++)
 * - uint160 = uint160 (Solidity) = uint160_t (C++)
 * - uint160Arr = uint160[] (Solidity) = std::vector<uint160_t> (C++)
 * - uint168 = uint168 (Solidity) = uint168_t (C++)
 * - uint168Arr = uint168[] (Solidity) = std::vector<uint168_t> (C++)
 * - uint176 = uint176 (Solidity) = uint176_t (C++)
 * - uint176Arr = uint176[] (Solidity) = std::vector<uint176_t> (C++)
 * - uint184 = uint184 (Solidity) = uint184_t (C++)
 * - uint184Arr = uint184[] (Solidity) = std::vector<uint184_t> (C++)
 * - uint192 = uint192 (Solidity) = uint192_t (C++)
 * - uint192Arr = uint192[] (Solidity) = std::vector<uint192_t> (C++)
 * - uint200 = uint200 (Solidity) = uint200_t (C++)
 * - uint200Arr = uint200[] (Solidity) = std::vector<uint200_t> (C++)
 * - uint208 = uint208 (Solidity) = uint208_t (C++)
 * - uint208Arr = uint208[] (Solidity) = std::vector<uint208_t> (C++)
 * - uint216 = uint216 (Solidity) = uint216_t (C++)
 * - uint216Arr = uint216[] (Solidity) = std::vector<uint216_t> (C++)
 * - uint224 = uint224 (Solidity) = uint224_t (C++)
 * - uint224Arr = uint224[] (Solidity) = std::vector<uint224_t> (C++)
 * - uint232 = uint232 (Solidity) = uint232_t (C++)
 * - uint232Arr = uint232[] (Solidity) = std::vector<uint232_t> (C++)
 * - uint240 = uint240 (Solidity) = uint240_t (C++)
 * - uint240Arr = uint240[] (Solidity) = std::vector<uint240_t> (C++)
 * - uint248 = uint248 (Solidity) = uint248_t (C++)
 * - uint248Arr = uint248[] (Solidity) = std::vector<uint248_t> (C++)
 * - uint256 = uint256 (Solidity) = uint256_t (C++)
 * - uint256Arr = uint256[] (Solidity) = std::vector<uint256_t> (C++)
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
  uint8,
  uint8Arr,
  uint16,
  uint16Arr,
  uint24,
  uint24Arr,
  uint32,
  uint32Arr,
  uint40,
  uint40Arr,
  uint48,
  uint48Arr,
  uint56,
  uint56Arr,
  uint64,
  uint64Arr,
  uint72,
  uint72Arr,
  uint80,
  uint80Arr,
  uint88,
  uint88Arr,
  uint96,
  uint96Arr,
  uint104,
  uint104Arr,
  uint112,
  uint112Arr,
  uint120,
  uint120Arr,
  uint128,
  uint128Arr,
  uint136,
  uint136Arr,
  uint144,
  uint144Arr,
  uint152,
  uint152Arr,
  uint160,
  uint160Arr,
  uint168,
  uint168Arr,
  uint176,
  uint176Arr,
  uint184,
  uint184Arr,
  uint192,
  uint192Arr,
  uint200,
  uint200Arr,
  uint208,
  uint208Arr,
  uint216,
  uint216Arr,
  uint224,
  uint224Arr,
  uint232,
  uint232Arr,
  uint240,
  uint240Arr,
  uint248,
  uint248Arr,
  uint256,
  uint256Arr,
  address,
  addressArr,
  boolean,
  booleanArr,
  bytes,
  bytesArr,
  string,
  stringArr
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
* Specialization for std::vector<T>.
* This is used for all vector types, including bytesArr and stringArr.
* @tparam T The type to map to an ABI type.
*/
template <typename T>
struct ABIType<std::vector<T>> {
  static constexpr Types value = Types::uint256Arr; ///< ABI type is uint256Arr.
};

/**
* Specialization for address.
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
* Map for calling the correct ABI function for a given ABI type.
*/
inline std::unordered_map<Types, std::function<std::any(uint256_t)>> castFunctions = {
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

  /// Class that encodes and packs native data types into Solidity ABI strings.
  class Encoder {
    private:
      Bytes data_; ///< Encoded Solidity ABI string, as RAW BYTES. Use Hex::fromBytes().get() to print it properly.
      Functor functor; ///< Functor of the function to call. (if any)

      /**
       * Encode a function header into Solidity ABI format.
       * Requires the full function header, no spaces between args
       * (e.g. `func(arg1,arg2)`). The function will SHA3-hash the header
       * and return the first 4 hex bytes of the hash (aka the "functor").
       * @param func The function header to encode.
       * @return The encoded functor hex string.
       */
      Functor encodeFunction(const std::string_view func) const;

      /**
       * Encode a 256-bit unsigned integer into Solidity ABI format.
       * @param num The 256-bit unsigned integer to encode.
       * @return The encoded uint256 hex string, padded 32 hex bytes to the LEFT.
       */
      Bytes encodeUint256(const uint256_t& num) const;

      /**
       * Encode a 20-byte address into Solidity ABI format.
       * @param add The 20-byte address to encode.
       * @return The encoded address hex string, padded 32 bytes to the LEFT.
       */
      Bytes encodeAddress(const Address& add) const;

      /**
       * Encode a boolean into Solidity ABI format.
       * @param b The boolean to encode.
       * @return The encoded boolean hex string, padded 32 bytes to the LEFT.
       */
      Bytes encodeBool(bool b) const;

      /**
       * Encode a raw bytes or UTF-8 string into Solidity ABI format.
       * Solidity bytes and string types are parsed the exact same way,
       * so we can use the same function for encoding both types.
       * @param bytes The raw bytes or UTF-8 string to encode.
       * @return The encoded hex bytes or string,
       *         padded to the nearest multiple of 32 bytes to the RIGHT.
       */
      Bytes encodeBytes(const BytesArrView bytes) const;

      /**
       * Encode a 256-bit unsigned integer array into Solidity ABI format.
       * @param numV The 256-bit unsigned integer array to encode.
       * @return The encoded uint256[] hex string, with the proper offsets and lengths.
       */
      Bytes encodeUint256Arr(const std::vector<uint256_t>& numV) const;

      /**
       * Encode a 20-byte address array into Solidity ABI format.
       * @param addV The 20-byte address array to encode.
       * @return The encoded address[] hex string, with the proper offsets and lengths.
       */
      Bytes encodeAddressArr(const std::vector<Address>& addV) const;

      /**
       * Encode a boolean array into Solidity ABI format.
       * @param bV The boolean array to encode.
       * @return The encoded bool[] hex string, with the proper offsets and lengths.
       */
      Bytes encodeBoolArr(const std::vector<bool>& bV) const;

      /**
       * Encode a raw bytes or UTF-8 string array into Solidity ABI format.
       * See `encodeBytes()` for more details.
       * @param bytesV The raw bytes or UTF-8 string array to encode.
       * @return The encoded bytes[] or string[] hex string, with the proper offsets and lengths.
       */
      Bytes encodeBytesArr(const std::vector<BytesArrView>& bytesV) const;

    public:
      /// Alias for variant type, for easier handling.
      typedef std::vector<BaseTypes> EncVar;

      /**
       * Constructor.
       * Automatically encodes the data during construction.
       * Throws on error.
       * @param data A list of variables to encode.
       * @param func (optional) The full function header to encode.
       *             Defaults to an empty string.
       *@throw std::runtime_error if the function header is invalid or if header
       *and data do not match.
       * TODO: change std::string_view func to be only the function name, and derive the respective argument types from data.
       */
      Encoder(const ABI::Encoder::EncVar& data, const std::string_view func = "");

      /// Getter for `data`.
      const Bytes& getData() const { return this->data_; }

      /// Getter for 'functor'
      const Functor& getFunctor() const { return this->functor; }

      /**
       * Get the length of `data`.
       * @return The total size of the data string.
       */
      size_t size() const { return this->data_.size(); }
  };

  /// Class that unpacks and decodes a Solidity ABI string into their original data types.
  class Decoder {
    private:
      /// List with the decoded native data types.
      std::vector<BaseTypes> data_;

      /**
       * Decode a 256-bit unsigned integer from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded 256-bit unsigned integer.
       *@throw std::runtime_error if data is too short.
       */
      uint256_t decodeUint256(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a 20-byte address from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded 20-byte address.
       *@throw std::runtime_error if data is too short.
       */
      Address decodeAddress(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a boolean from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded boolean.
       *@throw std::runtime_error if data is too short.
       */
      bool decodeBool(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a raw bytes from the given Solidity data string.
       * Decoding bytes and string in Solidity is done the exact same way,
       * as we are dealing with data as raw bytes anyway.
       * We differentiate the return types for convenience.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded raw bytes
       * @throws std::runtime_error if data is too short.
       */
      Bytes decodeBytes(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a raw UTF-8 string from the given Solidity data string.
       * Decoding bytes and string in Solidity is done the exact same way,
       * as we are dealing with data as raw bytes anyway.
       * We differentiate the return types for convenience.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded string
       * @throw std::runtime_error if data is too short.
       */
      std::string decodeString(const BytesArrView data, const uint64_t& start) const;

      /**
       * Decode a 256-bit unsigned integer array from the given Solidity data
       * string. Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded 256-bit unsigned integer array.
       * @throw std::runtime_error if data is too short.
       */
      std::vector<uint256_t> decodeUint256Arr(
        const BytesArrView data, const uint64_t& start
      ) const;

      /**
       * Decode a 20-byte address array from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded 20-byte address array.
       * @throw std::runtime_error if data is too short.
       */
      std::vector<Address> decodeAddressArr(
        const BytesArrView data, const uint64_t& start
      ) const;

      /**
       * Decode a boolean array from the given Solidity data string.
       * Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded boolean array.
       * @throw std::runtime_error if data is too short.
       */
      std::vector<bool> decodeBoolArr(
        const BytesArrView data, const uint64_t& start
      ) const;

      /**
       * Decode a raw bytes array from the given Solidity data
       * string. See `decodeBytes()` for more details. Throws if data is too short.
       * @param data The Solidity data bytes to decode.
       * @param start The index of the vector to start decoding from.
       * @return The decoded raw bytes.
       * @throw std::runtime_error if data is too short.
       */
      std::vector<Bytes> decodeBytesArr(
        const BytesArrView data, const uint64_t& start
      ) const;

      /**
        * Decode a raw bytes array from the given Solidity data
        * string. See `decodeBytes()` for more details. Throws if data is too short.
        * @param data The Solidity data bytes to decode.
        * @param start The index of the vector to start decoding from.
        * @return The decoded raw bytes.
        * @throw std::runtime_error if data is too short.
        */
      std::vector<std::string> decodeStringArr(
        const BytesArrView data, const uint64_t& start
      ) const;

  public:
      /**
       * Constructor. Automatically decodes the data during construction.
       * @param types An ordered list of expected Solidity types to decode.
       * @param bytes The full Solidity ABI string to decode, AS A RAW BYTES STRING.
       */
      Decoder(const std::vector<Types>& types, const BytesArrView bytes);

      /**
       * Get a specific data type from the decoded `data` list.
       * @param index The index of the data type to get.
       * @return The decoded data type.
       * @throw std::out_of_range if index is out of range.
       * @throw std::runtime_error if type mismatch.
       */
      template <typename T> T getData(const uint64_t &index) const {
        if (index >= this->data_.size()) throw std::out_of_range("Index out of range");
        if (std::holds_alternative<T>(this->data_[index])) return std::get<T>(this->data_[index]);
        throw std::runtime_error("Type mismatch");
      }

    /**
     * Get a specific data type from the decoded `data` list.
     * @param index The index of the data type to get.
     * @param type The expected Solidity type of the data.
     * @return The decoded data type.
     * @throw std::runtime_error if type mismatch.
     */
    std::any getDataDispatch(int index, Types type) {
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
        return this->getData<uint256_t>(index);
        case Types::uint8Arr:
        case Types::uint16Arr:
        case Types::uint32Arr:
        case Types::uint64Arr:
        case Types::uint256Arr:
        return this->getData<std::vector<uint256_t>>(index);
        case Types::address: return this->getData<Address>(index);
        case Types::addressArr: return this->getData<std::vector<Address>>(index);
        case Types::boolean: return this->getData<bool>(index);
        case Types::booleanArr: return this->getData<std::vector<bool>>(index);
        case Types::bytes: return this->getData<Bytes>(index);
        case Types::bytesArr: return this->getData<std::vector<Bytes>>(index);
        case Types::string: return this->getData<std::string>(index);
        case Types::stringArr: return this->getData<std::vector<std::string>>(index);
        default: throw std::runtime_error("Invalid ABI::Types type: " + getStringFromABIEnum(type));
      }
    }

      /**
       * Get the size of the `data` list.
       * @return The total number of decoded types.
       */
      size_t getDataSize() const { return this->data_.size(); }
  };
}; // namespace ABI

#endif // ABI_H
