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


using uint256_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;
using uint160_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<160, 160, boost::multiprecision::unsigned_magnitude, boost::multiprecision::cpp_int_check_type::unchecked, void>>;

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
  std::string _hex;
  std::string_view filter = "0123456789abcdefxABCDEFX";
  bool strict;
  bool isHexValid(const std::string_view& hex = "");
public:
  explicit Hex();
  Hex(std::string&& value, bool strict = false);
  Hex(const std::string_view& value, bool strict = false);
  static Hex fromBytes(std::string_view bytes, bool strict = false);
  static Hex fromString(std::string_view bytes, bool strict = false);
  static Hex fromUTF8(std::string_view bytes, bool strict = false);

  std::string bytes();

  /**
   * Returns the data as std::string, if you need the char collection of bytes
   * use hex.get().data();
   */
  std::string get();

  uint256_t getUint();

  /// String Container operators
  Hex& operator+=(const std::string& hexString);
  Hex& operator+=(const Hex& hex);

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
