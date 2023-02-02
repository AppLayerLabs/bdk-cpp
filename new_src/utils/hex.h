#ifndef HEX_H
#define HEX_H

#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
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
  std::string _hex; ///< Internal string data.
  std::string_view filter = "0123456789abcdefxABCDEFX"; ///< Filter for hex string
  bool strict; ///< If true, the _hex will include "0x" prefix
  bool isHexValid(const std::string_view& hex = ""); ///< Check if the hex string is valid, if argument empty, check _hex
public:
  explicit Hex(); ///< Default constructor (empty string)
  
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
   * Build a bytes string from the Hex object
   * @return bytes string
   */
  std::string bytes() const;

  /**
   * Getter for internal hex string
   * @return hex string
   */
  inline const std::string& get() const;

  /**
   * Get uint256_t equivalent to hex.
   * @return uint256_t value
   */
  uint256_t getUint() const;

  inline std::string substr(size_t pos = 0, size_t len = std::string::npos) const {
    return _hex.substr(pos, len);
  }
  inline std::string_view substr_view(size_t pos = 0, size_t len = std::string::npos) const {
    return std::string_view(_hex).substr(pos, len);
  }

  /// String Container operators
  Hex& operator+=(const std::string& hexString);
  Hex& operator+=(const Hex& hex);

  /**
   * Convert any given unsigned integer to a hex string.
   * @param i The integer to convert.
   * @return The converted integer as a hex string.
   */
  template <typename T> Hex uintToHex(const T& i, bool strict) {
    std::stringstream ss;
    std::string ret;
    ss << std::hex << i;
    ret = ss.str();
    for (auto &c : ret) if (std::isupper(c)) c = std::tolower(c);
    return Hex(std::move(ret), strict);
  }

  /**
   * Default operator to return directly the _hex as string
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
  operator std::string() const {
    return _hex;
  }

  /**
   * Default operator to return directly the _hex as string_view
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
  operator std::string_view () const {
    return _hex;
  }

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
  friend std::ostream& operator <<(std::ostream &out, const Hex &hex);
  ~Hex();
};


#endif //HEX_H
