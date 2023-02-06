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

#include "utils.h"

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
    bool strict;                                             ///< If true, the _hex will include "0x" prefix
    bool isHexValid(const std::string_view& v = "") const;   ///< Check if the hex string is valid, if argument empty, check _hex

  public:
    Hex(bool strict = false);                                                   ///< Default constructor (empty string)

    /**
     * Move a Hex object from a hex string "0x1234" or "1234"
     * @param value hex string
     * @param strict if true, the _hex will include "0x" prefix
     */
    Hex(std::string&& value, bool strict = false);

    /**
     * Build a Hex object from a hex string "0x1234" or "1234"
     * @param value hex string_view
     * @param strict if true, the _hex will include "0x" prefix
     */    
    Hex(const std::string_view& value, bool strict = false);


    /**
     * Build a Hex object from a byte string "\x12\x34" would result in "1234"
     * @param value bytes string
     * @param strict if true, the _hex will include "0x" prefix
     */
    static Hex fromBytes(std::string_view bytes, bool strict = false);

    /**
     * Build a Hex object from a string "exemple" would result in "6578656d706c65"
     * @param value bytes string
     * @param strict if true, the _hex will include "0x" prefix
     */
    static Hex fromUTF8(std::string_view bytes, bool strict = false);


    /**
     * Build a Hex value from a uint*_t value
     * @param value uint*_t value
     * @param strict if true, the _hex will include "0x" prefix
     */
    template <typename T>
    static Hex fromUint(T value, bool strict = false) {
      std::stringstream ss;
      ss << std::hex << value;
      return Hex(ss.str(), strict);
    }
    /**
     * Build a bytes string from the Hex object
     * @return bytes string
     */
    std::string bytes() const;

    /**
     * Getter for internal hex string
     * @return hex string
     */
    inline const std::string& get() const { return this->hex; }

    /**
     * Get uint256_t equivalent to hex.
     * @return uint256_t value
     */
    inline uint256_t getUint() const {
      return boost::lexical_cast<HexTo<uint256_t>>(this->hex);
    }

    /**
     * Get substr of hex string
     * @param pos position of the first character to include
     * @param len number of characters to include
     * @return hex string (new string object)
    */
    inline std::string substr(size_t pos = 0, size_t len = std::string::npos) const {
      return this->hex.substr(pos, len);
    }

    /**
     * Get substr of hex string
     * @param pos position of the first character to include
     * @param len number of characters to include
     * @return hex string (string_view object)
    */

    inline std::string_view substr_view(size_t pos = 0, size_t len = std::string::npos) const {
      return std::string_view(this->hex).substr(pos, len);
    }

    Hex& operator+=(const std::string& hex) { ///< operator+= to add a string () to the hex string
      // TODO: check if hex string is a valid hex string.
      this->hex += (
        hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')
      ) ? hex.substr(2) : hex;
      return *this;
    }

    Hex& operator+=(const Hex& other) { ///< operator += to add a hex to another hex.
      this->hex += (
        other.hex[0] == '0' && (other.hex[1] == 'x' || other.hex[1] == 'X')
      ) ? other.hex.substr(2) : other.hex;
      return *this;
    }

    static int hexCharToInt(char c);
    
    /**
     * Default operator to return directly the hex as string
     * @example
     * std::string myHexString = "My Hex is:";
     * Hex hex = Hex::fromString("Hello World");
     * myHexString += hex;
     *
     * std::cout << myStringHex << std::endl;
     *
     * @result
     * [Terminal]
     * My Hex is: 48656c6c6f20576f726c64
     **/
    inline operator std::string() const { return this->hex; }

    /**
     * Default operator to return directly the hex as string_view
     * @example
     * std::string myHexString = "My Hex is:";
     * Hex hex = Hex::fromString("Hello World");
     * std::string_view myHexString = hex;
     *
     * std::cout << myStringHex << myStringHex << std::endl;
     *
     * @result
     * [Terminal]
     * My Hex is: 48656c6c6f20576f726c64
     **/
    inline operator std::string_view() const { return this->hex; }

    /**
     * Friend function to allow the shift to left in output stream
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
