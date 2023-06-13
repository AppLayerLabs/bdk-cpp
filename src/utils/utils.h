#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string_view>
#include <thread>
#include <atomic>
#include <cxxabi.h>

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/beast/core/error.hpp>

#include <ethash/keccak.h>
#include <openssl/rand.h>

#include "strings.h"

#include "json.hpp"

/// @file utils.h

class Hash;

/// Typedef for json.
using json = nlohmann::ordered_json;

/// Typedef for bigint.
using bigint = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>>;

/// Typedef for uint256_t.
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

/// Typedef for uint128_t.
using uint128_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

/// Typedef for uint160_t.
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

/// Typedef for tuple of (from, to, gasLimit, gasPrice, value, data).
using ethCallInfo = std::tuple<Address,Address,uint256_t, uint256_t, uint256_t, std::string>;

/**
 * @fn void fail(std::string_view cl, std::string_view func, boost::beast::error_code ec, const char* what)
 * @brief Helper function for debugging failed operations over HTTP.
 * @param cl The class where the operation failed.
 * @param func The function where the operation failed.
 * @param ec Boost Beast error code.
 * @param what String explaining what exactly failed.
 */
void fail(std::string_view cl, std::string_view func, boost::beast::error_code ec, const char* what);

/// Namespace with string prefixes for each blockchain module, for printing log/debug messages.
namespace Log {
  const std::string blockchain = "Blockchain";                    ///< String for `Blockchain`.
  const std::string storage = "Storage";                          ///< String for `Storage`.
  const std::string snowmanVM = "SnowmanVM";                      ///< String for `SnowmanVM`.
  const std::string block = "Block";                              ///< String for `Block`.
  const std::string db = "DB";                                    ///< String for `DB`.
  const std::string state = "State";                              ///< String for `State`.
  const std::string grpcServer = "gRPCServer";                    ///< String for `gRPCServer`.
  const std::string grpcClient = "gRPCClient";                    ///< String for `gRPCClient`.
  const std::string utils = "Utils";                              ///< String for `Utils`.
  const std::string httpServer = "HTTPServer";                    ///< String for `HTTPServer`.
  const std::string JsonRPCEncoding = "JsonRPC::Encoding";        ///< String for `JsonRPC::Encoding`.
  const std::string JsonRPCDecoding = "JsonRPC::Decoding";        ///< String for `JsonRPC::Decoding`.
  const std::string rdPoS = "rdPoS";                              ///< String for `rdPoS`.
  const std::string ABI = "ABI";                                  ///< String for `ABI`.
  const std::string P2PClientSession = "P2P::ClientSession";      ///< String for `P2P::ClientSession`.
  const std::string P2PServer = "P2P::Server";                    ///< String for `P2P::Server`.
  const std::string P2PServerListener = "P2P::ServerListener";    ///< String for `P2P::ServerListener`.
  const std::string P2PServerSession = "P2P::ServerSession";      ///< String for `P2P::ServerSession`.
  const std::string P2PManager = "P2P::Manager";                  ///< String for `P2P::Manager`.
  const std::string P2PParser = "P2P::Parser";                    ///< String for `P2P::Parser`.
  const std::string P2PDiscoveryWorker = "P2P::DiscoveryWorker";  ///< String for `P2P::DiscoveryWorker`.
  const std::string contractManager = "ContractManager";          ///< String for `ContractManager`.
  const std::string syncer = "Syncer";                            ///< String for `Syncer`.
}

/// Enum for network type.
enum Networks { Mainnet, Testnet, LocalTestnet };

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

/// Namespace for utility functions.
namespace Utils {
  extern std::atomic<bool> logToCout; ///< Indicates whether logging to stdout is allowed (for safePrint()).

  /**
   * %Log a string to a file called `log.txt`.
   * @param str The string to log.
   */
  void logToFile(std::string_view str);

  /**
   * %Log a string to a file called `debug.txt`.
   * @param pfx The module prefix where this is being called (see Log).
   * @param func The function where this is being called.
   * @param data The contents to be stored.
   */
  void logToDebug(std::string_view pfx, std::string_view func, std::string_view data);

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
  Hash sha3(const std::string_view input);

  /**
   * Convert a 256-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 256-bit integer as a bytes string.
   */
  std::string uint256ToBytes(const uint256_t& i);

  /**
   * Convert a 128-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 128-bit integer as a bytes string.
   */
  std::string uint128ToBytes(const uint128_t& i);

  /**
   * Convert a 160-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 160-bit integer as a bytes string.
   */
  std::string uint160ToBytes(const uint160_t& i);

  /**
   * Convert a 64-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 64-bit integer as a bytes string.
   */
  std::string uint64ToBytes(const uint64_t& i);

  /**
   * Convert a 32-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 32-bit integer as a bytes string.
   */
  std::string uint32ToBytes(const uint32_t& i);

  /**
   * Convert a 16-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 16-bit integer as a bytes string.
   */
  std::string uint16ToBytes(const uint16_t& i);

  /**
   * Convert a 8-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 8-bit integer as a bytes string.
   */
  std::string uint8ToBytes(const uint8_t& i);

  /**
   * Generate a random bytes string of a given size.
   * @param size The size of the string.
   * @return The generated bytes string.
   */
  std::string randBytes(const int& size);

  /**
   * Convert a bytes string to a 256-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 256-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint256_t bytesToUint256(const std::string_view b);

  /**
   * Convert a bytes string to a 128-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 128-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint128_t bytesToUint128(const std::string_view b);

  /**
   * Convert a bytes string to a 128-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 128-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint160_t bytesToUint160(const std::string_view b);

  /**
   * Convert a bytes string to a 64-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 64-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint64_t bytesToUint64(const std::string_view b);

  /**
   * Convert a bytes string to a 32-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 32-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint32_t bytesToUint32(const std::string_view b);

  /**
   * Convert a bytes string to a 16-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 16-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint16_t bytesToUint16(const std::string_view b);

  /**
   * Convert a bytes string to a 8-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 8-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint8_t bytesToUint8(const std::string_view b);

  /**
   * Add padding to the left of a string.
   * @param str The string to pad.
   * @param charAmount The total amount of characters the resulting string should have.
   *                   If this is less than the string's original size,
   *                   the string will remain untouched.
   *                   e.g. `padLeft("aaa", 5)` = "00aaa", `padLeft("aaa", 2)` = "aaa"
   * @param sign (optional) The character to use as padding. Defaults to '0'.
   * @return The padded string.
   */
  std::string padLeft(std::string str, unsigned int charAmount, char sign = '0');

  /**
   * Add padding to the right of a string.
   * @param str The string to pad.
   * @param charAmount The total amount of characters the resulting string should have.
   *                   If this is less than the string's original size,
   *                   the string will remain untouched.
   *                   e.g. `padRight("aaa", 5)` = "aaa00", `padRight("aaa", 2)` = "aaa"
   * @param sign (optional) The character to use as padding. Defaults to '0'.
   * @return The padded string.
   */
  std::string padRight(std::string str, unsigned int charAmount, char sign = '0');

  /// Overload of padLeft() that works with byte strings.
  std::string padLeftBytes(std::string str, unsigned int charAmount, char sign = '\x00');

  /// Overload of padRight() that works with byte strings.
  std::string padRightBytes(std::string str, unsigned int charAmount, char sign = '\x00');

  /**
   * Convert a big-endian byte-stream represented on a templated collection to a templated integer value.
   * `In` will typically be either std::string or bytes.
   * `T` will typically be unsigned, u160, u256 or bigint.
   * @param bytes The byte stream to convert.
   * @return The converted integer type.
   */
  template <class T, class In> T fromBigEndian(const In& bytes) {
    T ret = (T)0;
    for (auto i: bytes) {
      ret = (T)((ret << 8) | (uint8_t)(typename std::make_unsigned<decltype(i)>::type)i);
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
  template <class T> std::string uintToBytes(T i) {
    std::string ret(bytesRequired(i), 0x00);
    uint8_t* b = reinterpret_cast<uint8_t*>(&ret.back());
    for (; i; i >>= 8) *(b--) = (uint8_t)(i & 0xff);
    return ret;
  }

  template <typename T>
  std::string getRealTypeName() {
    int status;
    char* demangledName = nullptr;

    std::string mangledName = typeid(T).name();
    demangledName = abi::__cxa_demangle(mangledName.c_str(), 0, 0, &status);

    if(status == 0 && demangledName != nullptr) {
        std::string realName(demangledName);
        free(demangledName);
        return realName;
    } else {
        return mangledName;
    }
}
};

#endif  // UTILS_H
