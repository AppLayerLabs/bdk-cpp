/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HEX_H
#define HEX_H

#include <boost/multiprecision/cpp_int.hpp>

#include "bytes.h" // Byte, Bytes, BytesArr

#include "bytes/view.h" // bytes/ranges.h -> ranges -> span libs/zpp_bits.h -> span
#include "utils/view.h"

using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

/// Abstraction of a strictly hex-formatted string (`(0x)[1-9][a-f][A-F]`).
class Hex {
  private:
    std::string hex_;  ///< Internal string data.
    bool strict_;      ///< If `true`, forces `hex` to include the "0x" prefix.

  public:
    /**
     * Empty string constructor.
     * @param strict (optional) If `true`, sets `hex` to "0x". Defaults to `false`.
     */
    explicit Hex(bool strict = false) : strict_(strict) { if (strict) this->hex_ = "0x"; }

    /**
     * Move constructor.
     * @param value The hex string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @throw DynamicException if hex string is invalid.
     */
    Hex(std::string&& value, bool strict = false);

    /**
     * Copy constructor.
     * @param value The hex string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @throw DynamicException if hex string is invalid.
     */
    Hex(const std::string_view value, bool strict = false);

    /**
     * Build a Hex object from a byte string (`\x12\x34` = "1234").
     * @param bytes The byte string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @param upper (optional) If `true`, outputs `ABCDEF` instead of `abcdef`. Defaults to `false`.
     * @return The constructed Hex object.
     */
    static Hex fromBytes(const View<Bytes> bytes, bool strict = false, bool upper = false);

    /**
     * Build a Hex object from a UTF-8 string ("example" = "6578616d706c65").
     * @param bytes The UTF-8 string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @return The constructed Hex object.
     */
    static Hex fromUTF8(std::string_view bytes, bool strict = false);

    /**
     * Build a Hex object from any given unsigned integer.
     * @param value The unsigned integer.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @return The constructed Hex object.
     */
    template <typename T> static Hex fromUint(T value, bool strict = false) {
      std::stringstream ss;
      ss << std::hex << value;
      return Hex(ss.str(), strict);
    }

    /**
     * Check if a given string is a valid hex-formatted string.
     * @param hex The string to check.
     * @param strict (optional) If `true`, also checks if the string has the "0x" prefix.
     * @return `true` if string is a valid hex, `false` otherwise.
     */
    static bool isValid(const std::string_view hex, bool strict = false);

    Bytes bytes() const;  ///< Convert internal hex data to raw bytes.

    /**
     * Static overload of bytes().
     * @param hex The hex string to convert to bytes.
     * @return The converted bytes string.
     * @throw DynamicException if hex string is invalid.
     */
    static Bytes toBytes(const std::string_view hex);

    /// Getter for `hex`.
    inline const std::string& get() const { return this->hex_; }

    /**
     * Getter for `hex`, but converts it back to an unsigned 256-bit integer.
     * @throw std::length_error if hex is too big to be converted to uint256_t.
     */
    inline uint256_t getUint() const {
      Bytes b = Hex::toBytes(this->hex_);
      if (b.size() > 32) throw std::length_error("Hex too big for uint conversion");
      View<Bytes> bV(b.data(), b.size());
      uint256_t ret;
      boost::multiprecision::import_bits(ret, bV.begin(), bV.end(), 8);
      return ret;
    }

    /**
     * Get a substring of the internal hex string.
     * @param pos (optional) Position of the first character to start from. Defaults to start of string.
     * @param len (optional) Number of characters to get. Defaults to end of string.
     * @return The hex substring.
    */
    inline std::string substr(size_t pos = 0, size_t len = std::string::npos) const {
      return this->hex_.substr(pos, len);
    }

    /// Overload of `substr()` that returns a string_view instead.
    inline std::string_view substr_view(size_t pos = 0, size_t len = std::string::npos) const {
      return std::string_view(this->hex_).substr(pos, len);
    }

    /**
     * Convert a given hex char to its integer representation.
     * @param c The hex char to convert
     * @return The hex char as an integer.
     */
    static inline int toInt(char c) {
     if (c >= '0' && c <= '9') return c - '0';
     if (c >= 'a' && c <= 'f') return c - 'a' + 10;
     if (c >= 'A' && c <= 'F')	return c - 'A' + 10;
     return -1;
    }

    /**
     * Return an Ethereum-JSONRPC-friendly hex string. Examples:
     * - 0x41 (65 in decimal)
     * - 0x400 (1024 in decimal)
     * - WRONG: 0x (should always have at least one digit - zero is "0x0")
     * - WRONG: 0x0400 (no leading zeroes allowed)
     * - WRONG: ff (must be prefixed with "0x")
     *
     * @return The internal hex string, modified as above if needed.
     * @see https://ethereum.org/pt/developers/docs/apis/json-rpc/#hex-encoding
     */
    std::string forRPC() const;

    /**
     * Concat operator.
     * @throw DynamicException on invalid concat.
     */
    Hex& operator+=(const std::string& hex);

    /// Concat operator.
    Hex& operator+=(const Hex& other);

    /**
     * Default operator to return the internal hex string directly as a string. Example:
     * ```
     * std::string myHexString = "My Hex is:";
     * Hex hex = Hex::fromString("Hello World");
     * myHexString += hex;
     * std::cout << myHexString << std::endl;
     * $ My Hex is: 48656c6c6f20576f726c64
     * ```
     */
    inline operator std::string() const { return this->hex_; }

    /**
     * Default operator to return the internal hex string directly as a string_view. Example:
     * ```
     * std::string myStringHex = "My Hex is:";
     * Hex hex = Hex::fromString("Hello World");
     * std::string_view myHexString = hex;
     * std::cout << myStringHex << myHexString << std::endl;
     * $ My Hex is: 48656c6c6f20576f726c64
     * ```
     */
    inline operator std::string_view() const { return this->hex_; }

    /**
     * Friend function to allow left shift in output stream. Example:
     * ```
     * Hex hex = Hex("48656c6c6f20576f726c64");
     * std::cout << "My hex is: " << hex << std::endl;
     * $ My hex is: 48656c6c6f20576f726c64
     * ```
     */
    inline friend std::ostream& operator<<(std::ostream& out, const Hex& other) { return out << other.hex_; }
};

#endif // HEX_H
