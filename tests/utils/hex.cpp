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

    SECTION("Hex Move Constructor") {
      std::string hexStr = "1234";
      std::string hexStrStrict = "0x1234";
      Hex hex(std::move(hexStr), false);
      Hex hexStrict(std::move(hexStrStrict), true);
      REQUIRE_THAT(hex.get(), Equals("1234"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x1234"));
      REQUIRE_THAT(hexStr, Equals(""));
      REQUIRE_THAT(hexStrStrict, Equals(""));
    }

    SECTION("Hex Copy Constructor") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      REQUIRE_THAT(hex.get(), Equals("1234"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x1234"));
      REQUIRE_THAT(hexStr, Equals("0x1234"));
    }

    SECTION("Hex FromBytes") {
      std::string bytes = "\x12\x34";
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

    SECTION("Hex Bytes") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      REQUIRE_THAT(hex.bytes(), Equals("\x12\x34"));
      REQUIRE_THAT(hexStrict.bytes(), Equals("\x12\x34"));
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
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      REQUIRE(hex.getUint() == uint256_t(4660));
      REQUIRE(hexStrict.getUint() == uint256_t(4660));
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

    SECTION("Hex operator+=(std::string)") {
      std::string hexStr = "0x1234";
      Hex hex(hexStr, false);
      Hex hexStrict(hexStr, true);
      hex += std::string("0x5678");
      hexStrict += std::string("0x5678");
      REQUIRE_THAT(hex.get(), Equals("12345678"));
      REQUIRE_THAT(hexStrict.get(), Equals("0x12345678"));
    }

    SECTION("Hex operator +=(Hex)") {
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
