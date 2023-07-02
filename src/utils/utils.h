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

#include "json.hpp"

/// @file utils.h

// Forward declaration.
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

using Byte = uint8_t; ///< Typedef for Byte.
using Bytes = std::vector<Byte>; ///< Typedef for Bytes.

/**
* Struct for Bytes type that will be encoded in an function return.
*/
struct BytesEncodedStruct {
   Bytes data; ///< Bytes data.
   };
using BytesEncoded = BytesEncodedStruct; ///< Typedef for BytesEncoded.
template <std::size_t N> using BytesArr = std::array<Byte, N>; ///< Typedef for BytesArr.
using BytesArrView = std::span<const Byte, std::dynamic_extent>; ///< Typedef for BytesArrView.
using BytesArrMutableView = std::span<Byte, std::dynamic_extent>; ///< Typedef for BytesArrMutableView.

/**
 * ethCallInfo: tuple of (from, to, gasLimit, gasPrice, value, functor, data).
 * **ATTENTION**: Be aware that we are using BytesArrView, so you MUST
 * be sure that the data allocated in the BytesArrView is valid during
 * the whole life of the tuple.
 * If you need ethCallInfo to own the data, use ethCallInfoAllocated instead.
 */
using ethCallInfo = std::tuple<Address,Address,uint256_t, uint256_t, uint256_t, Functor, BytesArrView>;

/**
 * Same as ethCallInfo, but using Bytes instead of BytesArrView, truly
 * allocating and owning the data. Some places need it such as tests.
 */
using ethCallInfoAllocated = std::tuple<Address,Address,uint256_t, uint256_t, uint256_t, Functor, Bytes>;

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

  /**
  * Template for identifying if a type is a tuple.
  * @tparam T The type to check.
  */
  template <typename T>
  struct is_tuple : std::false_type {};

  /**
  * Template explicit specialization for identifying if a type is a tuple.
  * @tparam Ts The types to check.
  */
  template <typename... Ts>
  struct is_tuple<std::tuple<Ts...>> : std::true_type {};

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
  Hash sha3(const BytesArrView input);

  /**
   * Convert a 256-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 256-bit integer as a bytes string.
   */
  BytesArr<32> uint256ToBytes(const uint256_t& i);

  /**
   * Convert a 128-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 128-bit integer as a bytes string.
   */
  BytesArr<16> uint128ToBytes(const uint128_t& i);

  /**
   * Convert a 160-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 160-bit integer as a bytes string.
   */
  BytesArr<20> uint160ToBytes(const uint160_t& i);

  /**
   * Convert a 64-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 64-bit integer as a bytes string.
   */
  BytesArr<8> uint64ToBytes(const uint64_t& i);

  /**
   * Convert a 32-bit unsigned integer to a bytes string.
   * Use `Hex()` to properly print it.
   * @param i The integer to convert.
   * @return The converted 32-bit integer as a bytes string.
   */
  BytesArr<4> uint32ToBytes(const uint32_t& i);

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
   * Convert a bytes string to a 128-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 128-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint128_t bytesToUint128(const BytesArrView b);

  /**
   * Convert a bytes string to a 128-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 128-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint160_t bytesToUint160(const BytesArrView b);

  /**
   * Convert a bytes string to a 64-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 64-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint64_t bytesToUint64(const BytesArrView b);

  /**
   * Convert a bytes string to a 32-bit unsigned integer.
   * @param b The bytes string to convert.
   * @return The converted 32-bit integer.
   * @throw std::runtime_error if string size is invalid.
   */
  uint32_t bytesToUint32(const BytesArrView b);

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
  template <class T> Bytes uintToBytes(T i) {
    Bytes ret(bytesRequired(i));
    uint8_t *b = reinterpret_cast<uint8_t *>(&ret.back());
    for (; i; i >>= 8) *(b--) = (uint8_t)(i & 0xff);
    return ret;
  }

  /**
  * Get the real type name of a type.
  * For example, `getRealTypeName<std::string>()` will return "std::basic_string<char, std::char_traits<char>, std::allocator<char> >"
  * @tparam T The type to get the name of.
  * @return The real type name.
  */
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
    if (start + size > vec.size()) {
      throw std::runtime_error("Invalid range for span");
    }

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
     if (start + size > vec.size()) {
       throw std::runtime_error("Invalid range for span");
     }

     return BytesArrView(vec.data() + start, size);
   }

  /**
  * Template for converting a fixed-size array to span.
  * @param arr The array to convert.
  * @return The converted span.
  */
  template<std::size_t N>
  inline BytesArrMutableView create_span(BytesArr<N>& arr) {
    return BytesArrMutableView(arr.data(), arr.size());
  }

  /**
  * Convert a "subarray" to span.
  * @param arr The array to convert.
  * @param start The start index of the subarray.
  * @param size The size of the subarray.
  * @return The converted span.
  */
  template<std::size_t N>
  inline BytesArrMutableView create_span(BytesArr<N>& arr, size_t start, size_t size) {
    if (start + size > arr.size()) {
      throw std::runtime_error("Invalid range for span");
    }

    return BytesArrMutableView(arr.data() + start, size);
  }

  /**
  * Convert an array to const span.
  * @param arr The array to convert.
  * @return The converted span.
  */
  template<std::size_t N>
  inline BytesArrView create_view_span(const BytesArr<N>& arr) {
    return BytesArrView(arr.data(), arr.size());
  }

  /**
   * Convert a "subarray" to const span.
   * @param arr The array to convert.
   * @param start The start index of the subarray.
   * @param size The size of the subarray.
   * @return The converted span.
   */
  template<std::size_t N>
  inline BytesArrView create_view_span(const BytesArr<N>& arr, size_t start, size_t size) {
    if (start + size > arr.size()) {
      throw std::runtime_error("Invalid range for span");
    }

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
  template<typename T>
  inline void appendBytes(Bytes& vec, const T& bytes) {
    vec.insert(vec.end(), bytes.cbegin(), bytes.cend());
  }

  /**
   * Convert a given bytes vector/array to a string.
   * Each byte is properly converted to its respective ASCII char value.
   * @tparam T Can be either std::vector, std::span, or std::array.
   * @return The converted bytes as a string.
   */
  template<typename T>
  inline std::string bytesToString(const T& bytes) {
    return std::string(bytes.cbegin(), bytes.cend());
  }

  /**
   * Convert a given string to a bytes vector.
   * Each ASCII char is properly converted to its respective byte value.
   * @param str The string to convert.
   * @return The converted string as a bytes vector.
   */
  inline Bytes stringToBytes(const std::string& str) {
    return Bytes(str.cbegin(), str.cend());
  }
};

#endif  // UTILS_H
