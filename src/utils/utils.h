/*
Copyright (c) [2023-2024] [AppLayer Developers]

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
#include <variant>
#include <evmc/evmc.hpp>

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

// Base case for the recursive helper - now using requires for an empty body function
template<size_t I = 0, typename... Tp>
requires (I == sizeof...(Tp))
void printDurationsHelper(const std::string& id, std::tuple<Tp...>&, const std::array<std::string, sizeof...(Tp)>&) {
    // Empty body, stopping condition for the recursion
}

// Recursive helper function to print each duration - with requires
template<size_t I = 0, typename... Tp>
requires (I < sizeof...(Tp))
void printDurationsHelper(const std::string& id, std::tuple<Tp...>& t, const std::array<std::string, sizeof...(Tp)>& names) {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - std::get<I>(t));
    std::string funcName = names[I]; // Creating a copy to use with std::move
    Logger::logToDebug(LogType::DEBUG, Log::P2PManager, std::move(funcName),
      "Timepoint at: " + id + " for " + names[I] + ": " + std::to_string(std::get<I>(t).time_since_epoch().count()) + "ms "
      + " Duration for " + names[I] + ": " + std::to_string(duration.count()) + "ms, exitted at: " + std::to_string(now.time_since_epoch().count()) + "ms"
    );
    // Recursive call for the next element in the tuple
    printDurationsHelper<I + 1, Tp...>(id, t, names);
}

template<typename... Tp>
struct printAtExit {
    std::tuple<Tp...> timePoints;
    std::array<std::string, sizeof...(Tp)> names;
    const std::string id;

    printAtExit(const std::string& id_, const std::array<std::string, sizeof...(Tp)>& names_, Tp&... timePoints_) :
    timePoints(std::tie(timePoints_...)), names(names_), id(id_) {}

    ~printAtExit() {
        printDurationsHelper(id, timePoints, names);
    }
};

///@{
/** Typedef for primitive integer type. */
using uint24_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<24, 24, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint40_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<40, 40, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint48_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<48, 48, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint56_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<56, 56, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint72_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<72, 72, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint80_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<80, 80, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint88_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<88, 88, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint96_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<96, 96, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint104_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<104, 104, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint112_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<112, 112, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint120_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<120, 120, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint128_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint136_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<136, 136, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint144_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<144, 144, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint152_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<152, 152, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint168_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<168, 168, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint176_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<176, 176, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint184_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<184, 184, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint192_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<192, 192, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint200_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<200, 200, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint208_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<208, 208, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint216_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<216, 216, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint224_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<224, 224, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint232_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<232, 232, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint240_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<240, 240, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint248_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<248, 248, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int24_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<24, 24, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int40_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<40, 40, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int48_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<48, 48, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int56_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<56, 56, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int72_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<72, 72, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int80_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<80, 80, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int88_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<88, 88, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int96_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<96, 96, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int104_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<104, 104, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int112_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<112, 112, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int120_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<120, 120, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int128_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int136_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<136, 136, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int144_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<144, 144, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int152_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<152, 152, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int168_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<168, 168, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int176_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<176, 176, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int184_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<184, 184, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int192_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<192, 192, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int200_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<200, 200, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int208_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<208, 208, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int216_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<216, 216, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int224_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<224, 224, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int232_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<232, 232, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int240_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<240, 240, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int248_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<248, 248, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using int256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::signed_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
///@}

///@{
/** Typedef for SafeVariable integer type. */
using SafeUint8_t = SafeUint_t<8>;
using SafeUint16_t = SafeUint_t<16>;
using SafeUint24_t = SafeUint_t<24>;
using SafeUint32_t = SafeUint_t<32>;
using SafeUint40_t = SafeUint_t<40>;
using SafeUint48_t = SafeUint_t<48>;
using SafeUint56_t = SafeUint_t<56>;
using SafeUint64_t = SafeUint_t<64>;
using SafeUint72_t = SafeUint_t<72>;
using SafeUint80_t = SafeUint_t<80>;
using SafeUint88_t = SafeUint_t<88>;
using SafeUint96_t = SafeUint_t<96>;
using SafeUint104_t = SafeUint_t<104>;
using SafeUint112_t = SafeUint_t<112>;
using SafeUint120_t = SafeUint_t<120>;
using SafeUint128_t = SafeUint_t<128>;
using SafeUint136_t = SafeUint_t<136>;
using SafeUint144_t = SafeUint_t<144>;
using SafeUint152_t = SafeUint_t<152>;
using SafeUint160_t = SafeUint_t<160>;
using SafeUint168_t = SafeUint_t<168>;
using SafeUint176_t = SafeUint_t<176>;
using SafeUint184_t = SafeUint_t<184>;
using SafeUint192_t = SafeUint_t<192>;
using SafeUint200_t = SafeUint_t<200>;
using SafeUint208_t = SafeUint_t<208>;
using SafeUint216_t = SafeUint_t<216>;
using SafeUint224_t = SafeUint_t<224>;
using SafeUint232_t = SafeUint_t<232>;
using SafeUint240_t = SafeUint_t<240>;
using SafeUint248_t = SafeUint_t<248>;
using SafeUint256_t = SafeUint_t<256>;
using SafeInt8_t = SafeInt_t<8>;
using SafeInt16_t = SafeInt_t<16>;
using SafeInt24_t = SafeInt_t<24>;
using SafeInt32_t = SafeInt_t<32>;
using SafeInt40_t = SafeInt_t<40>;
using SafeInt48_t = SafeInt_t<48>;
using SafeInt56_t = SafeInt_t<56>;
using SafeInt64_t = SafeInt_t<64>;
using SafeInt72_t = SafeInt_t<72>;
using SafeInt80_t = SafeInt_t<80>;
using SafeInt88_t = SafeInt_t<88>;
using SafeInt96_t = SafeInt_t<96>;
using SafeInt104_t = SafeInt_t<104>;
using SafeInt112_t = SafeInt_t<112>;
using SafeInt120_t = SafeInt_t<120>;
using SafeInt128_t = SafeInt_t<128>;
using SafeInt136_t = SafeInt_t<136>;
using SafeInt144_t = SafeInt_t<144>;
using SafeInt152_t = SafeInt_t<152>;
using SafeInt160_t = SafeInt_t<160>;
using SafeInt168_t = SafeInt_t<168>;
using SafeInt176_t = SafeInt_t<176>;
using SafeInt184_t = SafeInt_t<184>;
using SafeInt192_t = SafeInt_t<192>;
using SafeInt200_t = SafeInt_t<200>;
using SafeInt208_t = SafeInt_t<208>;
using SafeInt216_t = SafeInt_t<216>;
using SafeInt224_t = SafeInt_t<224>;
using SafeInt232_t = SafeInt_t<232>;
using SafeInt240_t = SafeInt_t<240>;
using SafeInt248_t = SafeInt_t<248>;
using SafeInt256_t = SafeInt_t<256>;
///@}

/**
 * Map with addresses for contracts deployed at protocol level (name -> address).
 * These contracts are deployed at the beginning of the chain and cannot be
 * destroyed or dynamically deployed like other contracts.
 * Instead, they are deployed in the constructor of State.
 */
const std::unordered_map<std::string, Address> ProtocolContractAddresses = {
  {"rdPoS", Address(Hex::toBytes("0xb23aa52dbeda59277ab8a962c69f5971f22904cf"))},           // Sha3("randomDeterministicProofOfStake").substr(0,20)
  {"ContractManager", Address(Hex::toBytes("0x0001cb47ea6d8b55fe44fdd6b1bdb579efb43e61"))}  // Sha3("ContractManager").substr(0,20)
};

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

/// Enum for the type of the contract.
enum ContractType { NOT_A_CONTRACT, EVM, CPP };

/**
 * Abstraction of balance and nonce for a single account.
 * Used with Address on State in an unordered_map to track native accounts.
 * We store both the code and code hash here, but the EVM Storage (map<address,map<bytes32,bytes32>) is
 * directly implemented in the State as map<StorageKey,Hash> to avoid nested maps.
 * @see State
 */
struct Account {
  uint256_t balance = 0;                       ///< Account balance.
  uint64_t nonce = 0;                          ///< Account nonce.
  Hash codeHash = Hash();                      ///< Account code hash (if any)
  Bytes code = Bytes();                        ///< Account code (if any)
  ContractType contractType = ContractType::NOT_A_CONTRACT; ///< Account contract type.

  /// Default constructor.
  Account() = default;

  /// Copy constructor.
  Account(const uint256_t& balance, const uint64_t& nonce) : balance(balance), nonce(nonce) {}

  /// Move constructor.
  Account(uint256_t&& balance, uint64_t&& nonce) : balance(std::move(balance)), nonce(std::move(nonce)) {}

  /// Deserialize constructor.
  Account(const BytesArrView& bytes);

  /// Serialize the account.
  /// We serialize as balance + nonce + codeHash + contractType + code (if any)
  /// 32 bytes + 8 bytes + 32 bytes + 1 byte + N (0 or more bytes) = 73 + N bytes
  Bytes serialize() const;
  bool isContract() const { return contractType != ContractType::NOT_A_CONTRACT; }
};

/**
 * NonNullUniquePtr is a wrapper around std::unique_ptr that ensures the pointer is never null.
 */
template<typename T>
class NonNullUniquePtr {
private:
  std::unique_ptr<T> ptr;

public:
  // Constructor that calls T<Ts...> with the provided arguments.
  template<typename... Ts>
  explicit NonNullUniquePtr(Ts&&... args) : ptr(std::make_unique<T>(std::forward<Ts>(args)...)) {}

  // Move construction and assignment allowed
  NonNullUniquePtr(NonNullUniquePtr&& other) = default;
  NonNullUniquePtr& operator=(NonNullUniquePtr&&) = default;

  // Deleted copy constructor and copy assignment operator to prevent copying
  NonNullUniquePtr(const NonNullUniquePtr&) = delete;
  NonNullUniquePtr& operator=(const NonNullUniquePtr&) = delete;

  // Dereference operator
  T& operator*() const { return *ptr; }

  // Member access operator
  T* operator->() const { return ptr.get(); }

  // Getter for raw pointer (optional, use with care)
  T* get() const { return ptr.get(); }
};


/**
 * Struct for abstracting a Solidity event parameter.
 * @tparam T The parameter's type.
 * @tparam Index Whether the parameter is indexed or not.
 */
template<typename T, bool Index> struct EventParam {
  using type = T; ///< Event param type.
  const T& value; ///< Event param value.
  static constexpr bool isIndexed = Index;  ///< Indexed status.
  EventParam(const T& value) : value(value) {}  ///< Constructor.
};

template<typename T>
class PointerNullifier {
private:
  T*& ptr;

public:
  PointerNullifier(T*& item) : ptr(item) {}
  ~PointerNullifier() { ptr = nullptr; }

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

  /// Helper struct to conditionally append a type to a tuple.
  template <bool Flag, typename T, typename Tuple> struct conditional_tuple_append {
    using type = Tuple; ///< Typedef.
  };

  /// Helper struct to conditionally append a type to a tuple.
  template <typename T, typename... Ts> struct conditional_tuple_append<false, T, std::tuple<Ts...>> {
    using type = std::tuple<Ts..., T>;  ///< Typedef.
  };

  /// Helper struct for making a tuple type.
  template<typename Accumulated, typename... Rest> struct makeTupleTypeHelper {
    using type = Accumulated; ///< Typedef.
  };

  /// Helper struct for making a tuple type.
  template<typename Accumulated, typename T, bool Flag, typename... Rest>
  struct makeTupleTypeHelper<Accumulated, EventParam<T, Flag>, Rest...> {
    using NextAccumulated = typename conditional_tuple_append<Flag, T, Accumulated>::type;  ///< Typedef.
    using type = typename makeTupleTypeHelper<NextAccumulated, Rest...>::type;  ///< Typedef.
  };

  /// Helper struct for making a tuple type.
  template<typename... Args> struct makeTupleType;

  /// Helper struct for making a tuple type.
  template<typename... Args, bool... Flags> struct makeTupleType<EventParam<Args, Flags>...> {
    using type = typename makeTupleTypeHelper<std::tuple<>, EventParam<Args, Flags>...>::type;  ///< Typedef.
  };

  extern std::atomic<bool> logToCout; ///< Indicates whether logging to stdout is allowed (for safePrint()).

  /**
   * %Log a string to a file called `log.txt`.
   * @param str The string to log.
   */
  void logToFile(std::string_view str);

  /**
   * Get the functor of a evmc_message
   * @param msg The evmc_message to get the functor from.
   * @return The functor of the evmc_message (0 if evmc_message size == 0).
   */
  Functor getFunctor(const evmc_message& msg);

  /**
   * Create a Functor based on a std::string with the function signature (e.g. "functionName(uint256,uint256)").
   * @param funtionSignature The function signature.
   * @return The created Functor.
   */
  Functor makeFunctor(const std::string& functionSignature);

  /**
   * Get the BytesArrView representing the function arguments of a given evmc_message.
   * @param msg The evmc_message to get the function arguments from.
   * @return The BytesArrView representing the function arguments.
   */
  BytesArrView getFunctionArgs(const evmc_message& msg);

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
   * Generate a random bytes string of a given size.
   * @param size The size of the string.
   * @return The generated bytes string.
   */
  Bytes randBytes(const int& size);

  /**
   * Special functions to convert to evmc_uint256be types.
   */
  uint256_t evmcUint256ToUint256(const evmc::uint256be& i);
  evmc::uint256be uint256ToEvmcUint256(const uint256_t& i);
  BytesArr<32> evmcUint256ToBytes(const evmc::uint256be& i);
  evmc::uint256be bytesToEvmcUint256(const BytesArrView b);

  evmc::address ecrecover(evmc::bytes32 hash, evmc::bytes32 v, evmc::bytes32 r, evmc::bytes32 s);

  ///@{
  /**
   * Convert a given integer to a bytes string. Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted integer as a bytes string.
   */
  BytesArr<32> uint256ToBytes(const uint256_t& i);
  BytesArr<31> uint248ToBytes(const uint248_t& i);
  BytesArr<30> uint240ToBytes(const uint240_t& i);
  BytesArr<29> uint232ToBytes(const uint232_t& i);
  BytesArr<28> uint224ToBytes(const uint224_t& i);
  BytesArr<27> uint216ToBytes(const uint216_t& i);
  BytesArr<26> uint208ToBytes(const uint208_t& i);
  BytesArr<25> uint200ToBytes(const uint200_t& i);
  BytesArr<24> uint192ToBytes(const uint192_t& i);
  BytesArr<23> uint184ToBytes(const uint184_t& i);
  BytesArr<22> uint176ToBytes(const uint176_t& i);
  BytesArr<21> uint168ToBytes(const uint168_t& i);
  BytesArr<20> uint160ToBytes(const uint160_t& i);
  BytesArr<19> uint152ToBytes(const uint152_t& i);
  BytesArr<18> uint144ToBytes(const uint144_t& i);
  BytesArr<17> uint136ToBytes(const uint136_t& i);
  BytesArr<16> uint128ToBytes(const uint128_t& i);
  BytesArr<15> uint120ToBytes(const uint120_t& i);
  BytesArr<14> uint112ToBytes(const uint112_t& i);
  BytesArr<13> uint104ToBytes(const uint104_t& i);
  BytesArr<12> uint96ToBytes(const uint96_t& i);
  BytesArr<11> uint88ToBytes(const uint88_t& i);
  BytesArr<10> uint80ToBytes(const uint80_t& i);
  BytesArr<9> uint72ToBytes(const uint72_t& i);
  BytesArr<8> uint64ToBytes(const uint64_t& i);
  BytesArr<7> uint56ToBytes(const uint56_t& i);
  BytesArr<6> uint48ToBytes(const uint48_t& i);
  BytesArr<5> uint40ToBytes(const uint40_t& i);
  BytesArr<4> uint32ToBytes(const uint32_t& i);
  BytesArr<3> uint24ToBytes(const uint24_t& i);
  BytesArr<2> uint16ToBytes(const uint16_t& i);
  BytesArr<1> uint8ToBytes(const uint8_t& i);
  BytesArr<32> int256ToBytes(const int256_t& i);
  ///@}

  ///@{
  /**
   * Convert a given bytes string to an integer.
   * @param b The bytes string to convert.
   * @return The converted integer.
   * @throw DynamicException if string size is invalid.
   */
  uint256_t bytesToUint256(const BytesArrView b);
  uint248_t bytesToUint248(const BytesArrView b);
  uint240_t bytesToUint240(const BytesArrView b);
  uint232_t bytesToUint232(const BytesArrView b);
  uint224_t bytesToUint224(const BytesArrView b);
  uint216_t bytesToUint216(const BytesArrView b);
  uint208_t bytesToUint208(const BytesArrView b);
  uint200_t bytesToUint200(const BytesArrView b);
  uint192_t bytesToUint192(const BytesArrView b);
  uint184_t bytesToUint184(const BytesArrView b);
  uint176_t bytesToUint176(const BytesArrView b);
  uint168_t bytesToUint168(const BytesArrView b);
  uint160_t bytesToUint160(const BytesArrView b);
  uint152_t bytesToUint152(const BytesArrView b);
  uint144_t bytesToUint144(const BytesArrView b);
  uint136_t bytesToUint136(const BytesArrView b);
  uint128_t bytesToUint128(const BytesArrView b);
  uint120_t bytesToUint120(const BytesArrView b);
  uint112_t bytesToUint112(const BytesArrView b);
  uint104_t bytesToUint104(const BytesArrView b);
  uint96_t bytesToUint96(const BytesArrView b);
  uint88_t bytesToUint88(const BytesArrView b);
  uint80_t bytesToUint80(const BytesArrView b);
  uint72_t bytesToUint72(const BytesArrView b);
  uint64_t bytesToUint64(const BytesArrView b);
  uint56_t bytesToUint56(const BytesArrView b);
  uint48_t bytesToUint48(const BytesArrView b);
  uint40_t bytesToUint40(const BytesArrView b);
  uint32_t bytesToUint32(const BytesArrView b);
  uint24_t bytesToUint24(const BytesArrView b);
  uint16_t bytesToUint16(const BytesArrView b);
  uint8_t bytesToUint8(const BytesArrView b);
  int256_t bytesToInt256(const BytesArrView b);


  Bytes cArrayToBytes(const uint8_t* arr, size_t size);

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

  /// Overload of padLeftBytes() that works with UTF-8 strings.
  std::string padLeft(std::string str, unsigned int charAmount, char sign = '\x00');

  /// Overload of padRightBytes() that works with UTF-8 strings.
  std::string padRight(std::string str, unsigned int charAmount, char sign = '\x00');

  /**
   * Convert a big-endian byte-stream represented on a templated collection to a templated integer value.
   * `In` will typically be either std::string or bytes.
   * `T` will typically be unsigned, u160, u256 or bigint.
   * @param bytes The byte stream to convert.
   * @return The converted integer type.
   */
  template <class T, class In> T fromBigEndian(const In& bytes) {
    auto ret = (T)0;
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
    for (; i != 0; ++ic, i >>= 8);
    return ic;
  }

  /**
   * Convert an unsigned integer to bytes.
   * Takes uint as little endian and has no padding, as opposed to uintToBytes().
   * @param i The integer to convert.
   * @return The converted bytes string.
   */
  template <class T> Bytes uintToBytes(T i) {
    Bytes ret(bytesRequired(i));
    Bytes::iterator b = ret.end();
    for (; i; i >>= 8) {
      if (b == ret.begin()) break;
      *(--b) = (uint8_t)(i & 0xff);
    }
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
    demangledName = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &status);
    if (status == 0 && demangledName != nullptr) {
      std::string realName(demangledName);
      free(demangledName);
      return realName;
    } else {
      return mangledName;
    }
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
     if (start + size > vec.size()) throw DynamicException("Invalid range for span");
     return BytesArrView(vec.data() + start, size);
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
    if (start + size > arr.size()) throw DynamicException("Invalid range for span");
    return BytesArrView(arr.data() + start, size);
  }

  /**
   * Convert a string to const span.
   * @param str The string to convert.
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
      throw DynamicException("Invalid range for span");
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

  /**
   * Shorthand for obtaining a milliseconds-since-epoch uint64_t timestamp from std::chrono
   */
  inline uint64_t getCurrentTimeMillisSinceEpoch() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  }
};

#endif  // UTILS_H
