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
    std::string hex;                                         ///< Internal string data.
    std::string_view filter = "0123456789abcdefxABCDEFX";    ///< Filter for hex string
    bool strict;                                             ///< If `true`, hex includes "0x"

    /**
     * Check if a given string is valid hex.
     * @param v (optional) The string to check. If empty, check hex. Defaults to empty.
     * @return `true` if string is a valid hex, `false` otherwise.
     */
    bool isValid(const std::string_view& v = "") const;

  public:
    /**
     * Default constructor (empty string).
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     */
    Hex(bool strict = false) : strict(strict) { if (strict) this->hex = "0x"; }

    /**
     * Move constructor.
     * @param value The hex string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     */
    Hex(std::string&& value, bool strict = false);

    /**
     * Copy constructor.
     * @param value The hex string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     */
    Hex(const std::string_view& value, bool strict = false);

    /**
     * Build a Hex object from a byte string.
     * "\x12\x34" would result in "1234".
     * @param bytes The byte string.
     * @param strict (optional) If `true`, includes "0x". Defaults to `false`.
     * @return The constructed Hex object.
     */
    static Hex fromBytes(std::string_view bytes, bool strict = false);

    /**
     * Build a Hex object from a UTF-8 string.
     * "example" would result in "6578616d706c65".
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
     * Convert the Hex data to bytes.
     * @return The bytes string.
     */
    std::string bytes() const;

    /// Getter for `hex`.
    inline const std::string& get() const { return this->hex; }

    /// Getter for `hex`, but converts it back to an unsigned integer.
    inline uint256_t getUint() const {
      return boost::lexical_cast<HexTo<uint256_t>>(this->hex);
    }

    /**
     * Get a substring of the hex string.
     * @param pos (optional) The position of the first character to start from.
     *                       Defaults to the start of the string.
     * @param len (optional) The number of characters to get.
     *                       Defaults to the end of the string.
     * @return The hex substring.
    */
    inline std::string substr(size_t pos = 0, size_t len = std::string::npos) const {
      return this->hex.substr(pos, len);
    }

    /// Overload of `substr()` for string_view.
    inline std::string_view substr_view(size_t pos = 0, size_t len = std::string::npos) const {
      return std::string_view(this->hex).substr(pos, len);
    }

    /// Concat operator. Throws on invalid concat.
    Hex& operator+=(const std::string& hex) {
      if (this->isValid(hex)) {
        this->hex += (
          hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')
        ) ? hex.substr(2) : hex;
      } else {
        throw std::runtime_error("Invalid Hex concat operation");
      }
      return *this;
    }

    /// Concat operator. Throws on invalid concat.
    Hex& operator+=(const Hex& other) {
      if (this->isValid(other.hex)) {
        this->hex += (
          other.hex[0] == '0' && (other.hex[1] == 'x' || other.hex[1] == 'X')
        ) ? other.hex.substr(2) : other.hex;
      } else {
        throw std::runtime_error("Invalid Hex concat operation");
      }
      return *this;
    }

    /**
     * Convert a given hex char to its integer representation.
     * @param c The hex char to convert
     * @return The hex char as an integer.
     */
    static int toInt(char c);

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
     * Friend function to allow left shift in output stream.
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
