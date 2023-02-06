#ifndef HEX_H
#define HEX_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string_view>
#include <thread>

#include <boost/multiprecision/cpp_int.hpp>

using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;

// TODO: document this
template <typename ElemT> struct HexTo {
  ElemT value;
  operator ElemT() const { return value; }
  friend std::istream& operator>>(std::istream& in, HexTo& out) {
    in >> std::hex >> out.value;
    return in;
  }
};

class Hex {
  private:
    std::string hex;                                       ///< Internal string data.
    std::string_view filter = "0123456789abcdefxABCDEFX";  ///< Filter for hex string validity.
    bool strict;                                           ///< If true, hex will include "0x" prefix.

    /**
     * Check if hex string is valid.
     * @param v (optional) The string to check. If empty, checks hex. Defaults to empty.
     * @return `true` if hex is valid, `false` otherwise.
     */
    bool isValid(const std::string_view& v = "") const;

  public:
    /**
     * Default constructor (empty string).
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     */
    inline Hex(bool strict = false) : strict(strict) { if (strict) this->hex = "0x"; }

    /**
     * Copy constructor.
     * Throws on invalid hex param.
     * @param v The hex string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     */
    Hex(const std::string_view& v, bool strict = false);

    /**
     * Move constructor.
     * Throws on invalid hex param.
     * @param v The hex string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     */
    Hex(std::string&& v, bool strict = false);

    /**
     * Build a Hex object from a byte string.
     * "\x12\x34" would result in "1234".
     * @param bytes The byte string to build from.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @return The constructed Hex object.
     */
    static Hex fromBytes(std::string_view bytes, bool strict = false);

    /**
     * Build a Hex object from a UTF-8 string.
     * "example" would result in "6578616d706c65".
     * @param str The UTF-8 string to build from.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @return The constructed Hex object.
     */
    static Hex fromUTF8(std::string_view str, bool strict = false);

    /**
     * Build a Hex object from any given uint value.
     * @param v The uint to use.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     */
    template <typename T> static Hex fromUint(T v, bool strict = false) {
      std::stringstream ss;
      ss << std::hex << v;
      return Hex(ss.str(), strict);
    }

    /**
     * Convert a given hex char to its integer representation.
     * @param c The hex char to convert
     * @return The hex char as an integer.
     */
    static int toInt(char c);

    /**
     * Convert the Hex data to a bytes string.
     * Throws on invalid chars.
     * @return The converted bytes string.
     */
    std::string bytes() const;

    /// Getter for `hex`.
    inline const std::string& get() const { return this->hex; }

    /// Getter for `hex`, but converts the value to a 256-bit unsigned integer.
    uint256_t getUint() const;

    /**
     * Get a substring of the Hex data string.
     * @param pos The position of the first character to get.
     * @param len The number of characters to get.
     * @return The specified substring.
    */
    inline std::string substr(size_t pos = 0, size_t len = std::string::npos) const {
      return this->hex.substr(pos, len);
    }

    /// Overload of `substr()`, for string_view.
    inline std::string_view substr_view(size_t pos = 0, size_t len = std::string::npos) const {
      return std::string_view(this->hex).substr(pos, len);
    }

    /// Concat operator.
    Hex& operator+=(const std::string& hex) {
      if (this->isValid(hex)) {
        this->hex += (
          hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')
        ) ? hex.substr(2) : hex;
      }
      return *this;
    }

    /// Concat operator.
    Hex& operator+=(const Hex& other) {
      if (this->isValid(hex)) {
        this->hex += (
          other.hex[0] == '0' && (other.hex[1] == 'x' || other.hex[1] == 'X')
        ) ? other.hex.substr(2) : other.hex;
      }
      return *this;
    }

    /**
     * Default operator to return the hex directly as a string.
     *
     * @example
     * std::string myHexString = "My Hex is:";
     * Hex hex = Hex::fromString("Hello World");
     * myHexString += hex;
     * std::cout << myStringHex << std::endl;
     *
     * @result
     * [Terminal]
     * My Hex is: 48656c6c6f20576f726c64
     **/
    inline operator std::string() const { return this->hex; }

    /**
     * Default operator to return the hex directly as a string_view.
     *
     * @example
     * std::string myHexString = "My Hex is:";
     * Hex hex = Hex::fromString("Hello World");
     * std::string_view myHexString = hex;
     * std::cout << myStringHex << myStringHex << std::endl;
     *
     * @result
     * [Terminal]
     * My Hex is: 48656c6c6f20576f726c64
     **/
    inline operator std::string_view() const { return this->hex; }

    /**
     * Friend function to allow bitwise left shift in output stream.
     *
     * @example
     * Hex hex = Hex("48656c6c6f20576f726c64");
     * std::cout << "My hex is: " << hex << std::endl;
     *
     * @result
     * [Terminal]
     * My hex is: 48656c6c6f20576f726c64
     **/
    inline friend std::ostream& operator<<(std::ostream& out, const Hex& other) {
      return out << other.hex;
    }
};

#endif // HEX_H
