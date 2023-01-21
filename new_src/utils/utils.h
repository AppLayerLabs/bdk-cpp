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

#include <ethash/keccak.h>
#include <openssl/rand.h>

#include "../libs/devcore/CommonData.h"
#include "../libs/devcore/FixedHash.h"
#include "../libs/json.hpp"

using json = nlohmann::ordered_json;
using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;

/**
 * Helper function for debugging failed operations.
 * @param cl The class where the operation failed.
 * @param func The function where the operation failed.
 * @param ec Boost Beast error code.
 * @param what String explaining what exactly failed.
 */
void fail(std::string cl, std::string func, boost::beast::error_code ec, const char* what);

/**
 * Namespace with string prefixes for each blockchain module, for printing log/debug messages.
 * Values are as follows:
 * - blockchain = "Blockchain::"
 * - storage = "Storage::"
 * - snowmanVM = "SnowmanVM::"
 * - block = "Block::"
 * - db = "DB::"
 * - state = "State::"
 * - grpcServer = "gRPCServer::"
 * - grpcClient = "gRPCClient::"
 * - utils = "Utils::"
 * - httpServer = "HTTPServer::"
 * - rdpos = "rdPoS::"
 * - ABI = "ABI::"
 * - P2PClient = "P2PClient::"
 * - P2PServer = "P2PServer::"
 * - P2PManager = "P2PManager::"
 */
namespace Log {
  const std::string blockchain = "Blockchain::";
  const std::string storage = "Storage::";
  const std::string snowmanVM = "SnowmanVM::";
  const std::string block = "Block::";
  const std::string db = "DB::";
  const std::string state = "State::";
  const std::string grpcServer = "gRPCServer::";
  const std::string grpcClient = "gRPCClient::";
  const std::string utils = "Utils::";
  const std::string httpServer = "HTTPServer::";
  const std::string rdpos = "rdPoS::";
  const std::string ABI = "ABI::";
  const std::string P2PClient = "P2PClient::";
  const std::string P2PServer = "P2PServer::";
  const std::string P2PManager = "P2PManager::";
};

/// Enum for network type.
enum Networks { Mainnet, Testnet, LocalTestnet };

/**
 * Abstraction of balance and nonce for a single account.
 * Used with %Address on %State in an unordered_map to track native accounts.
 * See `nativeAccounts` on %State for more info.
 */
struct Account { uint256_t balance = 0; uint32_t nonce = 0; };

/// Namespace for utility functions.
namespace Utils {
  /**
   * Log a string to a file called `log.txt`.
   * @param str The string to log.
   */
  void logToFile(const std::string_view& str);

  /**
   * Log a string to a file called `debug.txt`.
   * @param pfx The module prefix where this is being called (see %Log).
   * @param func The function where this is being called.
   * @param data The string to log.
   */
  void logToDebug(std::string pfx, std::string func, std::string data);

  /**
   * Hash a given input using SHA3.
   * @param input The string to hash.
   * @return The SHA3-hashed string.
   */
  Hash sha3(const std::string_view& input);

  /**
   * Convert a 256-bit unsigned integer to a bytes string.
   * Use `bytesToHex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 256-bit integer as a bytes string.
   */
  std::string uint256ToBytes(const uint256_t& i);

  /**
   * Convert a 160-bit unsigned integer to a bytes string.
   * Use `bytesToHex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 160-bit integer as a bytes string.
   */
  std::string uint160ToBytes(const uint160_t& i);

  /**
   * Convert a 64-bit unsigned integer to a bytes string.
   * Use `bytesToHex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 64-bit integer as a bytes string.
   */
  std::string uint64ToBytes(const uint64_t& i);

  /**
   * Convert a 32-bit unsigned integer to a bytes string.
   * Use `bytesToHex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 32-bit integer as a bytes string.
   */
  std::string uint32ToBytes(const uint32_t& i);

  /**
   * Convert a 16-bit unsigned integer to a bytes string.
   * Use `bytesToHex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 16-bit integer as a bytes string.
   */
  std::string uint16ToBytes(const uint16_t& i);

  /**
   * Convert a 8-bit unsigned integer to a bytes string.
   * Use `bytesToHex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 8-bit integer as a bytes string.
   */
  std::string uint8ToBytes(const uint8_t& i);

  /**
   * Generate a random bytes string of a given size.
   * @param size The size of the string.
   * @return The generated bytes string.
   */
  std::string randBytes(const size_t& size);

  /**
   * Check if a string is in hex format.
   * @param input The string to check.
   * @param strict (optional) If `true`, requires the "0x" prefix. Defaults to `false`.
   * @return `true` if the string is a hex string, `false` otherwise.
   */
  bool isHex(const std::string_view& input, bool strict = false);

  /**
   * Convert an UTF-8 string to a hex string.
   * @param input The UTF-8 string to convert.
   * @return The converted hex string.
   */
  std::string utf8ToHex(const std::string_view& input);

  /**
   * Convert a bytes string to a hex string.
   * @param b The bytes string to convert
   * @return The converted hex string.
   */
  std::string bytesToHex(const std::string_view& b);

  /**
   * Convert a bytes string to a 256-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 256-bit integer.
   */
  uint256_t bytesToUint256(const std::string_view& b);

  /**
   * Convert a bytes string to a 128-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 128-bit integer.
   */
  uint160_t bytesToUint160(const std::string_view& b);

  /**
   * Convert a bytes string to a 64-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 64-bit integer.
   */
  uint64_t bytesToUint64(const std::string_view& b);

  /**
   * Convert a bytes string to a 32-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 32-bit integer.
   */
  uint32_t bytesToUint32(const std::string_view& b);

  /**
   * Convert a bytes string to a 16-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 16-bit integer.
   */
  uint16_t bytesToUint16(const std::string_view& b);

  /**
   * Convert a bytes string to a 8-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 8-bit integer.
   */
  uint8_t bytesToUint8(const std::string_view& b);

  /**
   * Convert a given hex char to its integer representation.
   * @param c The hex char to convert
   * @return The hex char as an integer.
   */
  int hexCharToInt(char c);

  /**
   * Remove the "0x" prefix from a hex string and make it all lowercase.
   * Same as `stripHexPrefix()` + `toLower()`.
   * @param str The hex string to patch.
   */
  void patchHex(std::string& str);

  /**
   * Convert any given unsigned integer to a hex string.
   * Does not handle paddings or the "0x" prefix.
   * @param i The integer to convert.
   * @return The converted integer as a hex string.
   */
  template <typename T> std::string uintToHex(const T& i) {
    std::stringstream ss;
    std::string ret;
    ss << std::hex << i;
    ret = ss.str();
    for (auto &c : ret) if (std::isupper(c)) c = std::tolower(c);
    return ret;
  }

  /**
   * Remove the "0x" prefix from a hex string.
   * @param str The hex string to remove the "0x" from.
   */
  void stripHexPrefix(std::string& str);

  /**
   * Convert a given hex string to a 256-bit unsigned integer.
   * @param hex The hex string to convert.
   * @return The converted unsigned 256-bit integer.
   */
  uint256_t hexToUint(std::string& hex);

  /**
   * Convert a hex string to a bytes string.
   * @param hex The hex string to convert.
   * @return The converted bytes string.
   */
  std::string hexToBytes(std::string hex);

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
      ret = (T)((ret << 8) | (byte)(typename std::make_unsigned<decltype(i)>::type)i);
    }
    return ret;
  }

  /**
   * Convert a string to all-lowercase.
   * @param str The string to convert.
   */
  void toLower(std::string& str);

  /**
   * Convert a string to all-uppercase.
   * @param str The string to convert.
   */
  void toUpper(std::string& str);

  /**
   * Convert a string to checksum format, as per [EIP-55](https://eips.ethereum.org/EIPS/eip-55).
   * @param str The string to convert.
   */
  void toChksum(std::string& str);

  /**
   * Check if string is checksummed, as per [EIP-55](https://eips.ethereum.org/EIPS/eip-55).
   * Uses `toChksum()` internally. Does not alter the original string.
   * @param str The string to check.
   * @return `true` if the string is checksummed, `false` otherwise.
   */
  bool isChksum(const std::string& str);

  /**
   * Check if a given address string is valid. If the address has both upper *and* lowercase letters, will also check the checksum.
   * @param add The address to be checked.
   * @param fromRPC If `true`, considers the address is a string.
   *                If `false`, considers the address is in raw bytes format.
   * @return `true` if the address is valid, `false` otherwise.
   */
  bool isAddress(const std::string& add, bool fromRPC);

  /**
   * Load HTTP port settings from a config file. Creates the file if it doesn't exist.
   * @return A JSON object with the settings.
   */
  json readConfigFile();
};

#endif  // UTILS_H
