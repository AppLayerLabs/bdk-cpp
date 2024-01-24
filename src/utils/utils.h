/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string_view>
#include <thread>
#include <atomic>
#include <array>
#include <span>
#include <cxxabi.h>

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/beast/core/error.hpp>

#include <ethash/keccak.h>
#include <openssl/rand.h>

#include "strings.h"
#include "logger.h"

#include "src/libs/json.hpp"
#include "src/contract/variables/safeuint.h"
#include "src/contract/variables/safeint.h"
#include <variant>

/// @file utils.h

// Forward declaration.
class Hash;

/// Typedef for json.
using json = nlohmann::ordered_json;

/// Typedef for bigint.
using bigint = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>>;

using Byte = uint8_t; ///< Typedef for Byte.
using Bytes = std::vector<Byte>; ///< Typedef for Bytes.
template <std::size_t N> using BytesArr = std::array<Byte, N>; ///< Typedef for BytesArr.
using BytesArrView = std::span<const Byte, std::dynamic_extent>; ///< Typedef for BytesArrView.
using BytesArrMutableView = std::span<Byte, std::dynamic_extent>; ///< Typedef for BytesArrMutableView.

/// Typedef for uint24_t.
using uint24_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<24, 24, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint40_t.
using uint40_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<40, 40, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint48_t.
using uint48_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<48, 48, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint56_t.
using uint56_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<56, 56, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint72_t.
using uint72_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<72, 72, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint80_t.
using uint80_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<80, 80, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint88_t.
using uint88_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<88, 88, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint96_t.
using uint96_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<96, 96, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint104_t.
using uint104_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<104, 104, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint112_t.
using uint112_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<112, 112, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint120_t.
using uint120_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<120, 120, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint128_t.
using uint128_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint136_t.
using uint136_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<136, 136, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint144_t.
using uint144_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<144, 144, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint152_t.
using uint152_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<152, 152, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint160_t.
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint168_t.
using uint168_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<168, 168, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint176_t.
using uint176_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<176, 176, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint184_t.
using uint184_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<184, 184, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint192_t.
using uint192_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<192, 192, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint200_t.
using uint200_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<200, 200, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint208_t.
using uint208_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<208, 208, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint216_t.
using uint216_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<216, 216, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint224_t.
using uint224_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<224, 224, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint232_t.
using uint232_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<232, 232, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint240_t.
using uint240_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<240, 240, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint248_t.
using uint248_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<248, 248, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for uint256_t.
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

/// Typedef for int24_t.
using int24_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<24, 24, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int40_t.
using int40_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<40, 40, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int48_t.
using int48_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<48, 48, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int56_t.
using int56_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<56, 56, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int72_t.
using int72_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<72, 72, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int80_t.
using int80_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<80, 80, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int88_t.
using int88_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<88, 88, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int96_t.
using int96_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<96, 96, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int104_t.
using int104_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<104, 104, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int112_t.
using int112_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<112, 112, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int120_t.
using int120_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<120, 120, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int128_t.
using int128_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int136_t.
using int136_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<136, 136, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int144_t.
using int144_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<144, 144, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int152_t.
using int152_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<152, 152, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int160_t.
using int160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int168_t.
using int168_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<168, 168, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int176_t.
using int176_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<176, 176, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int184_t.
using int184_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<184, 184, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int192_t.
using int192_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<192, 192, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int200_t.
using int200_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<200, 200, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int208_t.
using int208_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<208, 208, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int216_t.
using int216_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<216, 216, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int224_t.
using int224_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<224, 224, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int232_t.
using int232_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<232, 232, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int240_t.
using int240_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<240, 240, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int248_t.
using int248_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<248, 248, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
/// Typedef for int256_t.
using int256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

using SafeUint8_t = SafeUint_t<8>; ///< Typedef for SafeUint8_t.
using SafeUint16_t = SafeUint_t<16>; ///< Typedef for SafeUint16_t.
using SafeUint24_t = SafeUint_t<24>; ///< Typedef for SafeUint24_t.
using SafeUint32_t = SafeUint_t<32>; ///< Typedef for SafeUint32_t.
using SafeUint40_t = SafeUint_t<40>; ///< Typedef for SafeUint40_t.
using SafeUint48_t = SafeUint_t<48>; ///< Typedef for SafeUint48_t.
using SafeUint56_t = SafeUint_t<56>; ///< Typedef for SafeUint56_t.
using SafeUint64_t = SafeUint_t<64>; ///< Typedef for SafeUint64_t.
using SafeUint72_t = SafeUint_t<72>; ///< Typedef for SafeUint72_t.
using SafeUint80_t = SafeUint_t<80>; ///< Typedef for SafeUint80_t.
using SafeUint88_t = SafeUint_t<88>; ///< Typedef for SafeUint88_t.
using SafeUint96_t = SafeUint_t<96>; ///< Typedef for SafeUint96_t.
using SafeUint104_t = SafeUint_t<104>; ///< Typedef for SafeUint104_t.
using SafeUint112_t = SafeUint_t<112>; ///< Typedef for SafeUint112_t.
using SafeUint120_t = SafeUint_t<120>; ///< Typedef for SafeUint120_t.
using SafeUint128_t = SafeUint_t<128>; ///< Typedef for SafeUint128_t.
using SafeUint136_t = SafeUint_t<136>; ///< Typedef for SafeUint136_t.
using SafeUint144_t = SafeUint_t<144>; ///< Typedef for SafeUint144_t.
using SafeUint152_t = SafeUint_t<152>; ///< Typedef for SafeUint152_t.
using SafeUint160_t = SafeUint_t<160>; ///< Typedef for SafeUint160_t.
using SafeUint168_t = SafeUint_t<168>; ///< Typedef for SafeUint168_t.
using SafeUint176_t = SafeUint_t<176>; ///< Typedef for SafeUint176_t.
using SafeUint184_t = SafeUint_t<184>; ///< Typedef for SafeUint184_t.
using SafeUint192_t = SafeUint_t<192>; ///< Typedef for SafeUint192_t.
using SafeUint200_t = SafeUint_t<200>; ///< Typedef for SafeUint200_t.
using SafeUint208_t = SafeUint_t<208>; ///< Typedef for SafeUint208_t.
using SafeUint216_t = SafeUint_t<216>; ///< Typedef for SafeUint216_t.
using SafeUint224_t = SafeUint_t<224>; ///< Typedef for SafeUint224_t.
using SafeUint232_t = SafeUint_t<232>; ///< Typedef for SafeUint232_t.
using SafeUint240_t = SafeUint_t<240>; ///< Typedef for SafeUint240_t.
using SafeUint248_t = SafeUint_t<248>; ///< Typedef for SafeUint248_t.
using SafeUint256_t = SafeUint_t<256>; ///< Typedef for SafeUint256_t.

using SafeInt8_t = SafeInt_t<8>; ///< Typedef for SafeInt8_t.
using SafeInt16_t = SafeInt_t<16>; ///< Typedef for SafeInt16_t.
using SafeInt24_t = SafeInt_t<24>; ///< Typedef for SafeInt24_t.
using SafeInt32_t = SafeInt_t<32>; ///< Typedef for SafeInt32_t.
using SafeInt40_t = SafeInt_t<40>; ///< Typedef for SafeInt40_t.
using SafeInt48_t = SafeInt_t<48>; ///< Typedef for SafeInt48_t.
using SafeInt56_t = SafeInt_t<56>; ///< Typedef for SafeInt56_t.
using SafeInt64_t = SafeInt_t<64>; ///< Typedef for SafeInt64_t.
using SafeInt72_t = SafeInt_t<72>; ///< Typedef for SafeInt72_t.
using SafeInt80_t = SafeInt_t<80>; ///< Typedef for SafeInt80_t.
using SafeInt88_t = SafeInt_t<88>; ///< Typedef for SafeInt88_t.
using SafeInt96_t = SafeInt_t<96>; ///< Typedef for SafeInt96_t.
using SafeInt104_t = SafeInt_t<104>; ///< Typedef for SafeInt104_t.
using SafeInt112_t = SafeInt_t<112>; ///< Typedef for SafeInt112_t.
using SafeInt120_t = SafeInt_t<120>; ///< Typedef for SafeInt120_t.
using SafeInt128_t = SafeInt_t<128>; ///< Typedef for SafeInt128_t.
using SafeInt136_t = SafeInt_t<136>; ///< Typedef for SafeInt136_t.
using SafeInt144_t = SafeInt_t<144>; ///< Typedef for SafeInt144_t.
using SafeInt152_t = SafeInt_t<152>; ///< Typedef for SafeInt152_t.
using SafeInt160_t = SafeInt_t<160>; ///< Typedef for SafeInt160_t.
using SafeInt168_t = SafeInt_t<168>; ///< Typedef for SafeInt168_t.
using SafeInt176_t = SafeInt_t<176>; ///< Typedef for SafeInt176_t.
using SafeInt184_t = SafeInt_t<184>; ///< Typedef for SafeInt184_t.
using SafeInt192_t = SafeInt_t<192>; ///< Typedef for SafeInt192_t.
using SafeInt200_t = SafeInt_t<200>; ///< Typedef for SafeInt200_t.
using SafeInt208_t = SafeInt_t<208>; ///< Typedef for SafeInt208_t.
using SafeInt216_t = SafeInt_t<216>; ///< Typedef for SafeInt216_t.
using SafeInt224_t = SafeInt_t<224>; ///< Typedef for SafeInt224_t.
using SafeInt232_t = SafeInt_t<232>; ///< Typedef for SafeInt232_t.
using SafeInt240_t = SafeInt_t<240>; ///< Typedef for SafeInt240_t.
using SafeInt248_t = SafeInt_t<248>; ///< Typedef for SafeInt248_t.
using SafeInt256_t = SafeInt_t<256>; ///< Typedef for SafeInt256_t.

/**
 * ethCallInfo: tuple of (from, to, gasLimit, gasPrice, value, functor, data).
 * **NOTE**: Be aware that we are using BytesArrView, so you MUST be sure that
 * the data allocated in BytesArrView is valid during the whole life of the tuple.
 * If you need ethCallInfo to own the data, use ethCallInfoAllocated instead.
 */
using ethCallInfo = std::tuple<Address,Address,uint256_t, uint256_t, uint256_t, Functor, BytesArrView>;

/**
 * Same as ethCallInfo, but using Bytes instead of BytesArrView, truly
 * allocating and owning the data. Some places need it such as tests.
 */
using ethCallInfoAllocated = std::tuple<Address,Address,uint256_t, uint256_t, uint256_t, Functor, Bytes>;

/**
* Fail a function with a given message.
* @param cl The class name.
* @param func The function name.
* @param ec The error code.
* @param what The message to print.
*/
void fail(const std::string& cl, std::string&& func, boost::beast::error_code ec, const char* what);

/// Enum for network type.
enum Networks { Mainnet, Testnet, LocalTestnet };

/// Enum for FunctionType
enum FunctionTypes { View, NonPayable, Payable };

/**
 * Abstraction of balance and nonce for a single account.
 * Used with Address on State in an unordered_map to track native accounts.
 * See `nativeAccounts` on State for more info.
 */
struct Account {
  uint256_t balance = 0;  ///< Account balance.
  uint64_t nonce = 0;     ///< Account nonce.

  /// Default Constructor.
  Account() {}

  /// Copy Constructor.
  Account(const uint256_t& balance, const uint64_t& nonce) : balance(balance), nonce(nonce) {}

  /// Move Constructor.
  Account(uint256_t&& balance, uint64_t&& nonce) : balance(std::move(balance)), nonce(std::move(nonce)) {}
};

/**
 * Struct for abstracting a Solidity event parameter.
 * @tparam T The parameter's type.
 * @tparam Index Whether the parameter is indexed or not.
 */
template<typename T, bool Index> struct EventParam {
  using type = T;
  const T& value;
  static constexpr bool isIndexed = Index;
  EventParam(const T& value) : value(value) {}
};

/// Namespace for utility functions.
namespace Utils {
  std::string getTestDumpPath(); ///< Get the path to the test dump folder.

  /**
   * Helper function for removeQualifiers
   * @tparam TTuple The tuple type to remove qualifiers from.
   * @tparam I The index sequence.
   */
  template <typename TTuple, std::size_t... I> auto removeQualifiersImpl(std::index_sequence<I...>) {
    return std::tuple<std::decay_t<std::tuple_element_t<I, TTuple>>...>{};
  }

  /**
   * Remove the qualifiers from a tuple type.
   * @tparam TTuple The tuple type to remove qualifiers from.
   * @return A tuple with the same types but qualifiers removed.
   */
  template <typename TTuple> auto removeQualifiers() {
    return removeQualifiersImpl<TTuple>(std::make_index_sequence<std::tuple_size_v<TTuple>>{});
  }

  /**
   * Template for identifying if a type is a uint between 8 and 256 bits.
   * @tparam T The type to check.
   * @tparam N The number of bits.
   */
  template<typename T, std::size_t N> struct isRangedUint {
    /// Indicates whether the type is a uint between 8 and 256 bits.
    static const bool value = std::is_integral_v<T> && std::is_unsigned_v<T> && (sizeof(T) * 8 <= N);
  };

  /**
   * Template for identifying if a type is an int between 8 and 256 bits.
   * @tparam T The type to check.
   * @tparam N The number of bits.
   */
  template<typename T, std::size_t N> struct isRangedInt {
    /// Indicates whether the type is an int between 8 and 256 bits.
    static const bool value = std::is_integral_v<T> && std::is_signed_v<T> && (sizeof(T) * 8 <= N);
  };

  /**
   * Template for identifying if a type is a tuple.
   * @tparam T The type to check.
   */
  template <typename T> struct is_tuple : std::false_type {};

  /**
   * Template explicit specialization for identifying if a type is a tuple.
   * @tparam Ts The types to check.
   */
  template <typename... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};

  // Helper struct to conditionally append a type to a tuple
    template <bool Flag, typename T, typename Tuple>
    struct conditional_tuple_append {
        using type = Tuple;
    };

    template <typename T, typename... Ts>
    struct conditional_tuple_append<false, T, std::tuple<Ts...>> {
        using type = std::tuple<Ts..., T>;
    };

    template<typename Accumulated, typename... Rest>
    struct makeTupleTypeHelper {
        using type = Accumulated;
    };

    template<typename Accumulated, typename T, bool Flag, typename... Rest>
    struct makeTupleTypeHelper<Accumulated, EventParam<T, Flag>, Rest...> {
        using NextAccumulated = typename conditional_tuple_append<Flag, T, Accumulated>::type;
        using type = typename makeTupleTypeHelper<NextAccumulated, Rest...>::type;
    };

    template<typename... Args>
    struct makeTupleType;

    template<typename... Args, bool... Flags>
    struct makeTupleType<EventParam<Args, Flags>...> {
        using type = typename makeTupleTypeHelper<std::tuple<>, EventParam<Args, Flags>...>::type;
    };

  extern std::atomic<bool> logToCout; ///< Indicates whether logging to stdout is allowed (for safePrint()).

  /**
   * %Log a string to a file called `log.txt`.
   * @param str The string to log.
   */
  void logToFile(std::string_view str);

  /**
   * Print a string to stdout.
   * @param str The string to print.
   */
  void safePrint(std::string_view str);

  /**
   * %Hash a given input using SHA3.
   * @param input The string to hash.
   * @return The SHA3-hashed string.
   */
  Hash sha3(const BytesArrView input);

  /**
   * Convert a 256-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 256-bit integer as a bytes string.
   */
  BytesArr<32> uint256ToBytes(const uint256_t& i);

  /**
   * Convert a 256-bit signed integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 256-bit integer as a bytes string.
   */
  BytesArr<32> int256ToBytes(const int256_t& i);

  /**
   * Convert a 248-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 248-bit integer as a bytes string.
   */
  BytesArr<31> uint248ToBytes(const uint248_t& i);

  /**
   * Convert a 240-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 240-bit integer as a bytes string.
   */
  BytesArr<30> uint240ToBytes(const uint240_t& i);

  /**
   * Convert a 232-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 232-bit integer as a bytes string.
   */
  BytesArr<29> uint232ToBytes(const uint232_t& i);

  /**
   * Convert a 224-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 224-bit integer as a bytes string.
   */
  BytesArr<28> uint224ToBytes(const uint224_t& i);

  /**
   * Convert a 216-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 216-bit integer as a bytes string.
   */
  BytesArr<27> uint216ToBytes(const uint216_t& i);

  /**
   * Convert a 208-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 208-bit integer as a bytes string.
   */
  BytesArr<26> uint208ToBytes(const uint208_t& i);

  /**
   * Convert a 200-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 200-bit integer as a bytes string.
   */
  BytesArr<25> uint200ToBytes(const uint200_t& i);

  /**
   * Convert a 192-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 192-bit integer as a bytes string.
   */
  BytesArr<24> uint192ToBytes(const uint192_t& i);

  /**
   * Convert a 184-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 184-bit integer as a bytes string.
   */
  BytesArr<23> uint184ToBytes(const uint184_t& i);

  /**
   * Convert a 176-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 176-bit integer as a bytes string.
   */
  BytesArr<22> uint176ToBytes(const uint176_t& i);

  /**
   * Convert a 168-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 168-bit integer as a bytes string.
   */
  BytesArr<21> uint168ToBytes(const uint168_t& i);

  /**
   * Convert a 160-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 160-bit integer as a bytes string.
   */
  BytesArr<20> uint160ToBytes(const uint160_t& i);

  /**
   * Convert a 152-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 152-bit integer as a bytes string.
   */
  BytesArr<19> uint152ToBytes(const uint152_t& i);

  /**
   * Convert a 144-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 144-bit integer as a bytes string.
   */
  BytesArr<18> uint144ToBytes(const uint144_t& i);

  /**
   * Convert a 136-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 136-bit integer as a bytes string.
   */
  BytesArr<17> uint136ToBytes(const uint136_t& i);

  /**
   * Convert a 128-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 128-bit integer as a bytes string.
   */
  BytesArr<16> uint128ToBytes(const uint128_t& i);

  /**
   * Convert a 120-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 120-bit integer as a bytes string.
   */
  BytesArr<15> uint120ToBytes(const uint120_t& i);

  /**
   * Convert a 112-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 112-bit integer as a bytes string.
   */
  BytesArr<14> uint112ToBytes(const uint112_t& i);

  /**
   * Convert a 104-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 104-bit integer as a bytes string.
   */
  BytesArr<13> uint104ToBytes(const uint104_t& i);

  /**
   * Convert a 96-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 96-bit integer as a bytes string.
   */
  BytesArr<12> uint96ToBytes(const uint96_t& i);

  /**
   * Convert a 88-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 88-bit integer as a bytes string.
   */
  BytesArr<11> uint88ToBytes(const uint88_t& i);

  /**
   * Convert a 80-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 80-bit integer as a bytes string.
   */
  BytesArr<10> uint80ToBytes(const uint80_t& i);

  /**
   * Convert a 72-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 72-bit integer as a bytes string.
   */
  BytesArr<9> uint72ToBytes(const uint72_t& i);

  /**
   * Convert a 112-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 112-bit integer as a bytes string.
   */
  BytesArr<14> uint112ToBytes(const uint112_t& i);

  /**
   * Convert a 64-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 64-bit integer as a bytes string.
   */
  BytesArr<8> uint64ToBytes(const uint64_t& i);

  /**
   * Convert a 56-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 56-bit integer as a bytes string.
   */
  BytesArr<7> uint56ToBytes(const uint56_t& i);

  /**
   * Convert a 48-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 48-bit integer as a bytes string.
   */
  BytesArr<6> uint48ToBytes(const uint48_t& i);

  /**
   * Convert a 40-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 40-bit integer as a bytes string.
   */
  BytesArr<5> uint40ToBytes(const uint40_t& i);

  /**
   * Convert a 32-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 32-bit integer as a bytes string.
   */
  BytesArr<4> uint32ToBytes(const uint32_t& i);

  /**
   * Convert a 24-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 24-bit integer as a bytes string.
   */
  BytesArr<3> uint24ToBytes(const uint24_t& i);

  /**
   * Convert a 16-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 16-bit integer as a bytes string.
   */
  BytesArr<2> uint16ToBytes(const uint16_t& i);

  /**
   * Convert a 8-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 8-bit integer as a bytes string.
   */
  BytesArr<1> uint8ToBytes(const uint8_t& i);

  /**
   * Generate a random bytes string of a given size.
   * @param size The size of the string.
   * @return The generated bytes string.
   */
  Bytes randBytes(const int& size);

  /**
   * Convert a bytes string to a 256-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 256-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint256_t bytesToUint256(const BytesArrView b);

  /**
   * Convert a bytes string to a 248-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 248-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint248_t bytesToUint248(const BytesArrView b);

  /**
   * Convert a bytes string to a 240-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 240-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint240_t bytesToUint240(const BytesArrView b);

  /**
   * Convert a bytes string to a 232-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 232-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint232_t bytesToUint232(const BytesArrView b);

  /**
   * Convert a bytes string to a 224-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 224-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint224_t bytesToUint224(const BytesArrView b);

  /**
   * Convert a bytes string to a 216-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 216-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint216_t bytesToUint216(const BytesArrView b);

  /**
   * Convert a bytes string to a 208-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 208-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint208_t bytesToUint208(const BytesArrView b);

  /**
   * Convert a bytes string to a 200-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 200-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint200_t bytesToUint200(const BytesArrView b);

  /**
   * Convert a bytes string to a 192-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 192-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint192_t bytesToUint192(const BytesArrView b);

  /**
   * Convert a bytes string to a 184-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 184-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint184_t bytesToUint184(const BytesArrView b);

  /**
   * Convert a bytes string to a 176-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 176-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint176_t bytesToUint176(const BytesArrView b);

  /**
   * Convert a bytes string to a 168-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 168-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint168_t bytesToUint168(const BytesArrView b);

  /**
   * Convert a bytes string to a 160-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 160-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint160_t bytesToUint160(const BytesArrView b);

  /**
   * Convert a bytes string to a 152-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 152-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint152_t bytesToUint152(const BytesArrView b);

  /**
   * Convert a bytes string to a 144-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 144-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint144_t bytesToUint144(const BytesArrView b);

  /**
   * Convert a bytes string to a 136-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 136-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint136_t bytesToUint136(const BytesArrView b);

  /**
   * Convert a bytes string to a 128-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 128-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint128_t bytesToUint128(const BytesArrView b);

  /**
   * Convert a bytes string to a 120-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 120-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint120_t bytesToUint120(const BytesArrView b);

  /**
   * Convert a bytes string to a 112-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 112-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint112_t bytesToUint112(const BytesArrView b);

  /**
   * Convert a bytes string to a 104-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 104-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint104_t bytesToUint104(const BytesArrView b);

  /**
   * Convert a bytes string to a 96-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 96-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint96_t bytesToUint96(const BytesArrView b);

  /**
   * Convert a bytes string to a 88-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 88-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint88_t bytesToUint88(const BytesArrView b);

  /**
   * Convert a bytes string to a 80-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 80-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint80_t bytesToUint80(const BytesArrView b);

  /**
   * Convert a bytes string to a 72-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 72-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint72_t bytesToUint72(const BytesArrView b);

  /**
   * Convert a bytes string to a 112-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 112-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint112_t bytesToUint112(const BytesArrView b);

  /**
   * Convert a bytes string to a 64-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 64-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint64_t bytesToUint64(const BytesArrView b);

  /**
   * Convert a bytes string to a 56-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 56-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint56_t bytesToUint56(const BytesArrView b);

  /**
   * Convert a bytes string to a 48-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 48-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint48_t bytesToUint48(const BytesArrView b);


  /**
   * Convert a bytes string to a 40-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 40-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint40_t bytesToUint40(const BytesArrView b);

  /**
   * Convert a bytes string to a 32-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 32-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint32_t bytesToUint32(const BytesArrView b);

  /**
   * Convert a bytes string to a 24-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 24-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint24_t bytesToUint24(const BytesArrView b);

  /**
   * Convert a bytes string to a 16-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 16-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint16_t bytesToUint16(const BytesArrView b);

  /**
   * Convert a bytes string to a 8-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 8-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint8_t bytesToUint8(const BytesArrView b);

   /**
    * Convert a bytes string to a 256-bit signed integer.
    * @param b The bytes string to convert.
    * @return The converted 256-bit integer.
    * @throw std::runtime_error if string size is invalid.
    */
  int256_t bytesToInt256(const BytesArrView b);

  /**
   * Add padding to the left of a byte vector.
   * @param bytes The vector to pad.
   * @param charAmount The total amount of characters the resulting string should have.
   *                   If this is less than the string's original size,
   *                   the string will remain untouched.
   *                   e.g. `padLeftBytes("aaa", 5)` = "00aaa", `padLeftBytes("aaa", 2)` = "aaa"
   * @param sign (optional) The character to use as padding. Defaults to '0'.
   * @return The padded vector.
   */
  Bytes padLeftBytes(const BytesArrView bytes, unsigned int charAmount, uint8_t sign = 0x00);

  /**
   * Add padding to the right of a byte vector.
   * @param bytes The vector to pad.
   * @param charAmount The total amount of characters the resulting string should have.
   *                   If this is less than the string's original size,
   *                   the string will remain untouched.
   *                   e.g. `padLeftBytes("aaa", 5)` = "aaa00", `padLeftBytes("aaa", 2)` = "aaa"
   * @param sign (optional) The character to use as padding. Defaults to '0'.
   * @return The padded vector.
   */
  Bytes padRightBytes(const BytesArrView bytes, unsigned int charAmount, uint8_t sign = 0x00);

  /// Overload of padLeftBytes() that works with normal strings.
  std::string padLeft(std::string str, unsigned int charAmount, char sign = '\x00');

  /// Overload of padRightBytes() that works with normal strings.
  std::string padRight(std::string str, unsigned int charAmount, char sign = '\x00');

  /**
   * Convert a big-endian byte-stream represented on a templated collection to a templated integer value.
   * `In` will typically be either std::string or bytes.
   * `T` will typically be unsigned, u160, u256 or bigint.
   * @param bytes The byte stream to convert.
   * @return The converted integer type.
   */
  template <class T, class In> T fromBigEndian(const In& bytes) {
    T ret = (T)0;
    for (auto i : bytes) {
      ret = (T)((ret << 8) | (uint8_t)(typename std::make_unsigned<decltype(i)>::type) i);
    }
    return ret;
  }

  /**
   * Convert a string to all-lowercase. Conversion is done in-place.
   * @param str The string to convert.
   */
  inline void toLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  }

  /**
   * Convert a string to all-uppercase. Conversion is done in-place.
   * @param str The string to convert.
   */
  inline void toUpper(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  }

  /**
   * Load HTTP port settings from a config file. Creates the file if it doesn't exist.
   * @return A JSON object with the settings.
   */
  // TODO: Organize every "ruleset read-only" as a "Settings" defined class to avoid variable redefinition
  json readConfigFile();

  /**
   * Tells how many bytes are required to store a given integer.
   * @param i The integer to check.
   * @return The number of bytes required to store the integer.
   */
  template <class T> inline unsigned bytesRequired(T i) {
    // bigint does not carry sign bit on shift
    static_assert(std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "only unsigned types or bigint supported");
    unsigned ic = 0;
    for (; i != 0; ++ic, i >>= 8) {}
    return ic;
  }

  /**
   * Convert an unsigned integer to bytes.
   * Takes uint as little endian, differently than the uintToBytes functions, there is no padding.
   * @param i The integer to convert.
   * @return The converted bytes string.
   */
  template <class T> Bytes uintToBytes(T i) {
    Bytes ret(bytesRequired(i));
    uint8_t *b = reinterpret_cast<uint8_t *>(&ret.back());
    for (; i; i >>= 8) *(b--) = (uint8_t)(i & 0xff);
    return ret;
  }

  /**
   * Get the real type name of a type.
   * For example, `getRealTypeName<std::string>()` will return
   * "std::basic_string<char, std::char_traits<char>, std::allocator<char>>".
   * @tparam T The type to get the name of.
   * @return The real type name.
   */
  template <typename T> std::string getRealTypeName() {
    int status;
    char* demangledName = nullptr;
    std::string mangledName = typeid(T).name();
    demangledName = abi::__cxa_demangle(mangledName.c_str(), 0, 0, &status);
    if (status == 0 && demangledName != nullptr) {
      std::string realName(demangledName);
      free(demangledName);
      return realName;
    } else {
      return mangledName;
    }
  }

  /**
   * Convert a vector to span.
   * @param vec The vector to convert.
   * @return The converted span.
   */
  inline BytesArrMutableView create_span(Bytes& vec) {
    return BytesArrMutableView(vec.data(), vec.size());
  }

  /**
  * Convert a "subvector" to span.
  * @param vec The vector to convert.
  * @param start The start index of the subvector.
  * @param size The size of the subvector.
  * @return The converted span.
  */
  inline BytesArrMutableView create_span(Bytes& vec, size_t start, size_t size) {
    if (start + size > vec.size()) throw std::runtime_error("Invalid range for span");
    return BytesArrMutableView(vec.data() + start, size);
  }

  /**
   * Convert a vector to const span.
   * @param vec The vector to convert.
   * @return The converted span.
   */
  inline BytesArrView create_view_span(const Bytes& vec) {
    return BytesArrView(vec.data(), vec.size());
  }

  /**
   * Convert a "subvector" to const span.
   * @param vec The vector to convert.
   * @param start The start index of the subvector.
   * @param size The size of the subvector.
   * @return The converted span.
   */
   inline BytesArrView create_view_span(const Bytes& vec, size_t start, size_t size) {
     if (start + size > vec.size()) throw std::runtime_error("Invalid range for span");
     return BytesArrView(vec.data() + start, size);
   }

  /**
  * Template for converting a fixed-size array to span.
  * @param arr The array to convert.
  * @return The converted span.
  */
  template<std::size_t N> inline BytesArrMutableView create_span(BytesArr<N>& arr) {
    return BytesArrMutableView(arr.data(), arr.size());
  }

  /**
  * Convert a "subarray" to span.
  * @param arr The array to convert.
  * @param start The start index of the subarray.
  * @param size The size of the subarray.
  * @return The converted span.
  */
  template<std::size_t N> inline BytesArrMutableView create_span(
    BytesArr<N>& arr, size_t start, size_t size
  ) {
    if (start + size > arr.size()) throw std::runtime_error("Invalid range for span");
    return BytesArrMutableView(arr.data() + start, size);
  }

  /**
  * Convert an array to const span.
  * @param arr The array to convert.
  * @return The converted span.
  */
  template<std::size_t N> inline BytesArrView create_view_span(const BytesArr<N>& arr) {
    return BytesArrView(arr.data(), arr.size());
  }

  /**
   * Convert a "subarray" to const span.
   * @param arr The array to convert.
   * @param start The start index of the subarray.
   * @param size The size of the subarray.
   * @return The converted span.
   */
  template<std::size_t N> inline BytesArrView create_view_span(
    const BytesArr<N>& arr, size_t start, size_t size
  ) {
    if (start + size > arr.size()) throw std::runtime_error("Invalid range for span");
    return BytesArrView(arr.data() + start, size);
  }

  /**
   * Convert a string to const span.
   * @param str
   * @return The converted span.
   */
  inline BytesArrView create_view_span(const std::string_view str) {
    return BytesArrView(reinterpret_cast<const uint8_t*>(str.data()), str.size());
  }

  /**
   * Convert a substring to span.
   * @param str The string to convert.
   * @param start The start index of the substring.
   * @param size The size of the substring.
   * @return The converted span.
   */
  inline BytesArrView create_view_span(const std::string_view str, size_t start, size_t size) {
    if (start + size > str.size()) {
      throw std::runtime_error("Invalid range for span");
    }
    return BytesArrView(reinterpret_cast<const uint8_t*>(str.data()) + start, size);
  }

  /**
   * Append a vector to another.
   * This function is a workaround for calling insert() on a vector with a function
   * as a parameter, since that is not possible as we need to call begin() and end()
   * from the returned value, and we are argumenting two different function calls.
   * Inline is used for best performance.
   * @tparam T Can be either std::vector, std::string, std::span, or std::array.
   * @param vec The vector to append to.
   * @param bytes The vector to be appended.
   */
  template<typename T> inline void appendBytes(Bytes& vec, const T& bytes) {
    vec.insert(vec.end(), bytes.cbegin(), bytes.cend());
  }

  /**
   * Convert a given bytes vector/array to a string.
   * Each byte is properly converted to its respective ASCII char value.
   * @tparam T Can be either std::vector, std::span, or std::array.
   * @return The converted bytes as a string.
   */
  template<typename T> inline std::string bytesToString(const T& bytes) {
    return std::string(bytes.cbegin(), bytes.cend());
  }

  /**
   * Convert a given string to a bytes vector.
   * Each ASCII char is properly converted to its respective byte value.
   * @param str The string to convert.
   * @return The converted string as a bytes vector.
   */
  inline Bytes stringToBytes(const std::string& str) { return Bytes(str.cbegin(), str.cend()); }
};

#endif  // UTILS_H
