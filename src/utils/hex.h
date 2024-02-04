/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HEX_H
#define HEX_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string_view>
#include <thread>
#include <span>

#include <boost/multiprecision/cpp_int.hpp>

using Byte = uint8_t;
using Bytes = std::vector<Byte>;
template <std::size_t N>
using BytesArr = std::array<Byte, N>;
using BytesArrView = std::span<const Byte, std::dynamic_extent>;
using BytesArrMutableView = std::span<Byte, std::dynamic_extent>;


using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::checked, void>>;

/**
 * Helper struct for use with Boost's lexical_cast to convert hex strings
 * to a given type (`boost::lexical_cast<%HexTo<uint256_t>>(hexStr)`).
 */
template <typename ElemT> struct HexTo {
  ElemT value;  ///< The value to hold.
  operator ElemT() const { return value; } ///< Operator to get the value.
  /// Stream operator.
  friend std::istream& operator>>(std::istream& in, HexTo& out) {
    in >> std::hex >> out.value;
    return in;
  }
};

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
     * @throw std::runtime_error if hex string is invalid.
     */
    Hex(std::string&& value, bool strict = false);

    /**
     * Copy constructor.
     * @param value The hex string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @throw std::runtime_error if hex string is invalid.
     */
    Hex(const std::string_view value, bool strict = false);

    /**
     * Build a Hex object from a byte string (`\x12\x34` = "1234").
     * @param bytes The byte string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @return The constructed Hex object.
     */
    static Hex fromBytes(const BytesArrView bytes, bool strict = false);

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

    /**
     * Convert internal hex data to bytes.
     * @return The converted bytes string.
     */
    Bytes bytes() const;

    /**
     * Static overload of bytes().
     * @param hex The hex string to convert to bytes.
     * @return The converted bytes string.
     * @throw std::runtime_error if hex string is invalid.
     */
    static Bytes toBytes(const std::string_view hex);

    /// Getter for `hex`.
    inline const std::string& get() const { return this->hex_; }

    /// Getter for `hex`, but converts it back to an unsigned integer.
    inline uint256_t getUint() const {
      return boost::lexical_cast<HexTo<uint256_t>>(this->hex_);
    }

    /**
     * Get a substring of the internal hex string.
     * @param pos (optional) The position of the first character to start from.
     *                       Defaults to the start of the string.
     * @param len (optional) The number of characters to get.
     *                       Defaults to the end of the string.
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
    static int toInt(char c);

    /**
     * Return an Ethereum-JSONRPC-friendly hex string.
     *
     * See: https://ethereum.org/pt/developers/docs/apis/json-rpc/#hex-encoding
     *
     * 0x41 (65 in decimal)
     *
     * 0x400 (1024 in decimal)
     *
     * WRONG: 0x (should always have at least one digit - zero is "0x0")
     *
     * WRONG: 0x0400 (no leading zeroes allowed)
     *
     * WRONG: ff (must be prefixed 0x)
     *
     * @return The internal hex string, modified as above if needed.
     */
    std::string forRPC() const;

    /// Concat operator. Throws on invalid concat.
    Hex& operator+=(const std::string& hex) {
      if (this->isValid(hex, (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')))) {
        this->hex_ += (
          hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')
        ) ? hex.substr(2) : hex;
      } else {
        throw std::runtime_error("Invalid Hex concat operation");
      }
      return *this;
    }

    /// Concat operator.
    Hex& operator+=(const Hex& other) {
      this->hex_ += (other.strict_) ? other.hex_.substr(2) : other.hex_;
      return *this;
    }

    /**
     * Default operator to return the internal hex string directly as a string.
     *
     * Example:
     * ```
     * std::string myHexString = "My Hex is:";
     * Hex hex = Hex::fromString("Hello World");
     * myHexString += hex;
     * std::cout << myHexString << std::endl;
     *
     * $ My Hex is: 48656c6c6f20576f726c64
     * ```
     */
    inline operator std::string() const { return this->hex_; }

    /**
     * Default operator to return the internal hex string directly as a string_view.
     *
     * Example:
     * ```
     * std::string myStringHex = "My Hex is:";
     * Hex hex = Hex::fromString("Hello World");
     * std::string_view myHexString = hex;
     * std::cout << myStringHex << myHexString << std::endl;
     *
     * $ My Hex is: 48656c6c6f20576f726c64
     * ```
     */
    inline operator std::string_view() const { return this->hex_; }

    /**
     * Friend function to allow left shift in output stream.
     *
     * Example:
     * ```
     * Hex hex = Hex("48656c6c6f20576f726c64");
     * std::cout << "My hex is: " << hex << std::endl;
     *
     * $ My hex is: 48656c6c6f20576f726c64
     * ```
     */
    inline friend std::ostream& operator<<(std::ostream& out, const Hex& other) {
      return out << other.hex_;
    }
};

#endif // HEX_H
