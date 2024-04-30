/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/hex.h"

using Catch::Matchers::Equals;

namespace THex {
  TEST_CASE("Hex Class", "[utils][hex]") {
    SECTION("Hex Default Constructor") {
      Hex hex;
      Hex hexStrict(true);
      REQUIRE_THAT(hex.get(), Equals(""));
      REQUIRE_THAT(hexStrict.get(), Equals("0x"));
    }

    SECTION("Hex Copy Constructor") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      REQUIRE_THAT(hex.get(), Equals("1234"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x1234"));
      REQUIRE_THAT(hexStr, Equals("0x1234"));

      bool catch1 = false;
      bool catch2 = false;
      std::string wrongStr = "xyzw";
      try { Hex wrongHex(wrongStr, false); } catch (std::exception &e) { catch1 = true; }
      try { Hex wrongHex2(wrongStr, true); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    SECTION("Hex Move Constructor") {
      std::string hexStr = "1234";
      std::string hexStrStrict = "0x1234";
      Hex hex(std::move(hexStr), false);
      Hex hexStrict(std::move(hexStrStrict), true);
      REQUIRE_THAT(hex.get(), Equals("1234"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x1234"));
      REQUIRE_THAT(hexStr, Equals(""));
      REQUIRE_THAT(hexStrStrict, Equals(""));

      bool catch1 = false;
      bool catch2 = false;
      std::string wrongStr = "xyzw";
      std::string wrongStr2 = "xyzw";
      try { Hex wrongHex(std::move(wrongStr), false); } catch (std::exception &e) { catch1 = true; }
      try { Hex wrongHex2(std::move(wrongStr2), true); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    SECTION("Hex FromBytes") {
      Bytes bytes{0x12, 0x34};
      Hex hex = Hex::fromBytes(bytes, false);
      Hex hexStrict = Hex::fromBytes(bytes, true);
      REQUIRE_THAT(hex.get(), Equals("1234"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x1234"));
    }

    SECTION("Hex FromUTF8") {
      std::string utf8 = "exemple";
      Hex hex = Hex::fromUTF8(utf8, false);
      Hex hexStrict = Hex::fromUTF8(utf8, true);
      REQUIRE_THAT(hex.get(), Equals("6578656d706c65"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x6578656d706c65"));
    }

    SECTION("Hex FromUint") {
      uint256_t value = 4660;
      Hex hex = Hex::fromUint(value, false);
      Hex hexStrict = Hex::fromUint(value, true);
      REQUIRE_THAT(hex.get(), Equals("1234"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x1234"));
    }

    SECTION("Hex IsValid") {
      REQUIRE(Hex::isValid("0x1a2b3c4d5e6f7890", true));
      REQUIRE(Hex::isValid("1a2b3c4d5e6f7890", false));
      REQUIRE(!Hex::isValid("0x81684g837h3892j", true));
      REQUIRE(!Hex::isValid("81684g837h3892j", false));
      REQUIRE(!Hex::isValid("1a2b3c4d5e6f7890", true));
      REQUIRE(!Hex::isValid("0x1a2b3c4d5e6f7890", false));
    }

    SECTION("Hex ToBytes") {
      std::string_view hexStr("0x1234");
      std::string_view hexStr2("5678");
      Bytes bytesStr = Hex::toBytes(hexStr);
      Bytes bytesStr2 = Hex::toBytes(hexStr2);
      REQUIRE(Hex::fromBytes(bytesStr).get() == "1234");
      REQUIRE(Hex::fromBytes(bytesStr2).get() == "5678");

      bool catched = false;
      std::string_view wrongStr("xyzw");
      try { Bytes s = Hex::toBytes(wrongStr); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Hex Bytes") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      REQUIRE(hex.bytes() == Bytes{0x12, 0x34});
      REQUIRE(hexStrict.bytes() == Bytes{0x12, 0x34});
    }

    SECTION("Hex Get") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      REQUIRE_THAT(hex.get(), Equals("1234"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x1234"));
    }

    SECTION("Hex GetUint") {
      std::string hexStr = "0x1234";
      std::string oddHexStr = "0xfffff";
      std::string evenHexStr = "0x0fffff";
      std::string tooBigHexStr = "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"; // 33 bytes
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      Hex oddHex(oddHexStr, true);
      Hex evenHex(evenHexStr, true);
      Hex tooBigHex(tooBigHexStr, true);
      REQUIRE(hex.getUint() == uint256_t(4660));
      REQUIRE(hexStrict.getUint() == uint256_t(4660));
      REQUIRE(oddHex.getUint() == uint256_t(1048575));
      REQUIRE(evenHex.getUint() == uint256_t(1048575));
      try {
        uint256_t wrongNumber = tooBigHex.getUint();
      } catch (std::length_error& e) {
        REQUIRE(e.what() == std::string("Hex too big for uint conversion"));
      }
    }

    SECTION("Hex Substr") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      REQUIRE_THAT(hex.substr(0, 2), Equals("12"));
      REQUIRE_THAT(hex.substr(2, 2), Equals("34"));
      REQUIRE_THAT(hexStrict.substr(0, 2), Equals("0x"));
      REQUIRE_THAT(hexStrict.substr(2, 2), Equals("12"));
      REQUIRE_THAT(hexStrict.substr(4, 2), Equals("34"));
    }

    // Cannot test string_view::substr with Catch2
    // REQUIRE_THAT(hex.substr_view(0, 2), Equals("12")); throws template errors

    SECTION("Hex ToInt") {
      // Yes I know this is overkill (it could be more tho) but you can never be too sure lol
      std::unordered_map<char, int> map = {
        {'0', 0}, {'1', 1}, {'2', 2}, {'3', 3}, {'4', 4}, {'5', 5}, {'6', 6},
        {'7', 7}, {'8', 8}, {'9', 9}, {'a', 10}, {'A', 10}, {'b', 11}, {'B', 11},
        {'c', 12}, {'C', 12}, {'d', 13}, {'D', 13}, {'e', 14}, {'E', 14},
        {'f', 15}, {'F', 15}, {'g', -1}, {'G', -1}, {'h', -1}, {'H', -1},
        {'!', -1}, {'@', -1}, {'#', -1}, {'$', -1}, {'%', -1}, {'&', -1},
        {'*', -1}, {'(', -1}, {')', -1}, {'-', -1}, {'+', -1}, {'_', -1},
        {'=', -1}, {',', -1}, {'.', -1}, {'<', -1}, {'>', -1}, {';', -1},
        {':', -1}, {'/', -1}, {'?', -1}, {'~', -1}, {'^', -1}, {'[', -1},
        {']', -1}, {'{', -1}, {'}', -1}, {'"', -1}, {'|', -1}
      };
      for (std::pair<char, int> p : map) REQUIRE(Hex::toInt(p.first) == p.second);
    }

    SECTION("Hex ForRPC") {
      Hex h1(std::string("0x41"), true);
      Hex h2(std::string("0x400"), true);
      Hex h3(std::string("0x"), true);
      Hex h4(std::string("0x0400"), true);
      Hex h5(std::string("ff"), false);
      Hex h6(std::string("0x00007a6f00"), true);
      REQUIRE_THAT(h1.forRPC(), Equals("0x41"));
      REQUIRE_THAT(h2.forRPC(), Equals("0x400"));
      REQUIRE_THAT(h3.forRPC(), Equals("0x0"));
      REQUIRE_THAT(h4.forRPC(), Equals("0x400"));
      REQUIRE_THAT(h5.forRPC(), Equals("0xff"));
      REQUIRE_THAT(h6.forRPC(), Equals("0x7a6f00"));
    }

    SECTION("Hex operator+= (std::string)") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      hex += std::string("0x5678");
      hexStrict += std::string("0x5678");
      REQUIRE_THAT(hex.get(), Equals("12345678"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x12345678"));

      bool catched = false;
      try { hex += std::string("xyzw"); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Hex operator+= (Hex)") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      hex += Hex(std::move(std::string("0x5678")), false);
      hexStrict += Hex(std::move(std::string("0x5678")), true);
      REQUIRE_THAT(hex.get(), Equals("12345678"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x12345678"));
    }
  }
}

