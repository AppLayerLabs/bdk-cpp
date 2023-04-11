#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string_view>
#include <thread>

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/beast/core/error.hpp>

#include <ethash/keccak.h>
#include <openssl/rand.h>

#include "strings.h"

#include "json.hpp"

class Hash;

using json = nlohmann::ordered_json;
using bigint = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>>;
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

/**
 * Helper function for debugging failed operations over HTTP.
 * @param cl The class where the operation failed.
 * @param func The function where the operation failed.
 * @param ec Boost Beast error code.
 * @param what String explaining what exactly failed.
 */
void fail(std::string_view cl, std::string_view func, boost::beast::error_code ec, const char* what);

/**
 * Namespace with string prefixes for each blockchain module, for printing log/debug messages.
 * Values are as follows:
 * - blockchain = "Blockchain"
 * - storage = "Storage"
 * - snowmanVM = "SnowmanVM"
 * - block = "Block"
 * - db = "DB"
 * - state = "State"
 * - grpcServer = "gRPCServer"
 * - grpcClient = "gRPCClient"
 * - utils = "Utils"
 * - httpServer = "HTTPServer"
 * - rdpos = "rdPoS"
 * - ABI = "ABI"
 * - P2P::ClientSession = "P2P::ClientSession"
 * - P2P::Server = "P2P::Server"
 * - P2P::ServerListener = "P2P::ServerListener"
 * - P2P::ServerSession = "P2P::ServerSession"
 * - P2P::Manager = "P2P::Manager"
 * - P2P::Parser = "P2P::Parser" <- Part of P2P::Manager
 * - contractManager = "ContractManager"
 */
namespace Log {
  const std::string blockchain = "Blockchain";
  const std::string storage = "Storage";
  const std::string snowmanVM = "SnowmanVM";
  const std::string block = "Block";
  const std::string db = "DB";
  const std::string state = "State";
  const std::string grpcServer = "gRPCServer";
  const std::string grpcClient = "gRPCClient";
  const std::string utils = "Utils";
  const std::string httpServer = "HTTPServer";
  const std::string JsonRPCEncoding = "JsonRPC::Encoding";
  const std::string JsonRPCDecoding = "JsonRPC::Decoding";
  const std::string rdPoS = "rdPoS";
  const std::string ABI = "ABI";
  const std::string P2PClientSession = "P2P::ClientSession";
  const std::string P2PServer = "P2P::Server";
  const std::string P2PServerListener = "P2P::ServerListener";
  const std::string P2PServerSession = "P2P::ServerSession";
  const std::string P2PManager = "P2P::Manager";
  const std::string P2PParser = "P2P::Parser";
  const std::string P2PDiscoveryWorker = "P2P::DiscoveryWorker";
  const std::string contractManager = "ContractManager";
  const std::string syncer = "Syncer";
}

/// Enum for network type.
enum Networks { Mainnet, Testnet, LocalTestnet };

/**
 * Abstraction of balance and nonce for a single account.
 * Used with %Address on %State in an unordered_map to track native accounts.
 * See `nativeAccounts` on %State for more info.
 */
struct Account {
  uint256_t balance = 0;
  uint64_t nonce = 0;
  /// Default Constructor
  Account() {}

  /// Copy Constructor
  Account(const uint256_t& balance, const uint64_t& nonce) : balance(balance), nonce(nonce) {}

  /// Move Constructor
  Account(uint256_t&& balance, uint64_t&& nonce) : balance(std::move(balance)), nonce(std::move(nonce)) {}
};

/// Namespace for utility functions.
namespace Utils {
  /**
   * Log a string to a file called `log.txt`.
   * @param str The string to log.
   */
  void logToFile(std::string_view str);

  /**
   * Log a string to a file called `debug.txt`.
   * @param pfx The module prefix where this is being called (see %Log).
   * @param func The function where this is being called.
   * @param data The contents to be stored.
   */
  void logToDebug(std::string_view pfx, std::string_view func, std::string_view data);

  /**
   * Hash a given input using SHA3.
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
   * @param b The bytes string to convert.'
   * @return The converted 256-bit integer.
   */
  uint256_t bytesToUint256(const std::string_view b);

  /**
   * Convert a bytes string to a 128-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 128-bit integer.
   */
  uint160_t bytesToUint160(const std::string_view b);

  /**
   * Convert a bytes string to a 64-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 64-bit integer.
   */
  uint64_t bytesToUint64(const std::string_view b);

  /**
   * Convert a bytes string to a 32-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 32-bit integer.
   */
  uint32_t bytesToUint32(const std::string_view b);

  /**
   * Convert a bytes string to a 16-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 16-bit integer.
   */
  uint16_t bytesToUint16(const std::string_view b);

  /**
   * Convert a bytes string to a 8-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 8-bit integer.
   */
  uint8_t bytesToUint8(const std::string_view b);

  /**
   * Add padding to the left of a string.
   * @param str The string to pad.
   * @param charAmount The total amount of characters the resulting string should have.
   *                   If this is less than the string's original size,
   *                   the string will remain untouched.
   *                   e.g. `padLeft("aaa", 5)` = **"00aaa"**, `padLeft("aaa", 2)` = **"aaa"**
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
   *                   e.g. `padRight("aaa", 5)` = **"aaa00"**, `padRight("aaa", 2)` = **"aaa"**
   * @param sign (optional) The character to use as padding. Defaults to '0'.
   * @return The padded string.
   */
  std::string padRight(std::string str, unsigned int charAmount, char sign = '0');

  /**
   * Convert a big-endian byte-stream represented on a templated collection to a templated integer value.
   * `_In` will typically be either std::string or bytes.
   * `T` will typically by unsigned, u160, u256 or bigint.
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
   * Convert a string to all-lowercase.
   * @param str The string to convert.
   */
  inline void toLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  }

  /**
   * Convert a string to all-uppercase.
   * @param str The string to convert.
   */
  inline void toUpper(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  }

  /**
   * Load HTTP port settings from a config file. Creates the file if it doesn't exist.
   * @note Organize every "ruleset read-only" as an "Settings" defined class to avoid variable redefinition
   * @return A JSON object with the settings.
   */
  json readConfigFile();

  // DEPRECATED FUNCTIONS BELOW

  /**
   * Check if a string is in hex format.
   * @deprecated
   * Please use Hex when working with strings of behaviour type Hex
   *
   * @param input The string to check.
   * @param strict (optional) If `true`, requires the "0x" prefix. Defaults to `false`.
   * @return `true` if the string is a hex string, `false` otherwise.
   */
  bool isHex(std::string_view input, bool strict = false);

  /**
   * Convert an UTF-8 string to a hex string.
   * @deprecated
   * Please use Hex::fromUTF8 and avoid using pure strings
   * @param input The UTF-8 string to convert.
   * @return The converted hex string.
   */
  std::string utf8ToHex(const std::string_view input);

  /**
   * Convert a bytes string to a hex string.
   * @deprecated
   * Please use Hex when working with strings of behaviour type Hex
   *
   * @param b The bytes string to convert
   * @return The converted hex string.
   */
  std::string bytesToHex(const std::string_view b);


  /**
   * Remove the "0x" prefix from a hex string and make it all lowercase.
   * Same as `stripHexPrefix()` + `toLower()`.
   * @deprecated
   * Please use Hex when working with strings of behaviour type Hex
   * @warning
   * This function has an unsafe string manipulation!
   *
   * @param str The hex string to patch.
   */
  std::string patchHex(const std::string& str);

  /**
   * Remove the "0x" prefix from a hex string.
   * @deprecated
   * This function is unsafe due to modifying the string by reference of an Hex
   * @param str The hex string to remove the "0x" from.
   */
  void stripHexPrefix(std::string& str);

  /**
   * Convert a given hex string to a 256-bit unsigned integer.
   * @deprecated
   * Please use Hex::getUint
   * @param hex The hex string to convert.
   * @return The converted unsigned 256-bit integer.
   */
  uint256_t hexToUint(std::string& hex);

  /**
   * Convert a hex string to a bytes string.
   * @deprecated
   * Please use Hex::bytes
   *
   * @param hex The hex string to convert.
   * @return The converted bytes string.
   */
  std::string hexToBytes(std::string_view hex);

  /**
   * Tells how many bytes are required to store a given integer.
   * @param _i The integer to check.
   * @return The number of bytes required to store the integer.
   */
  template <class T>
  inline unsigned bytesRequired(T _i)
  {
  	static_assert(std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "only unsigned types or bigint supported"); //bigint does not carry sign bit on shift
  	unsigned i = 0;
  	for (; _i != 0; ++i, _i >>= 8) {}
  	return i;
  }

  /**
   * Convert Unsigned Integer to Bytes, takes uint as little endian
   * differently than the uintToBytes functions, there is no padding.
   * @param _i The integer to convert.
   * @return The converted bytes string.
   */
  template <class _T> std::string uintToBytes(_T _i)
  {
    std::string ret(bytesRequired(_i), 0x00);
    uint8_t* b = reinterpret_cast<uint8_t*>(&ret.back());
    for (; _i; _i >>= 8)
      *(b--) = (uint8_t)(_i & 0xff);
    return ret;
  }

  /**
   * Check if an ECDSA signature is valid.
   * @deprecated Moved to ecdsa
   * @param r The first half of the ECDSA signature.
   * @param s The second half of the ECDSA signature.
   * @param v The recovery ID.
   * @return `true` if the signature is valid, `false` otherwise.
   */
  bool verifySig(const uint256_t& r, const uint256_t& s, const uint8_t& v);
};

#endif  // UTILS_H
