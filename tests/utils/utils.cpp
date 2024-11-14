/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/utils/utils.h" // (strings.h -> hex.h), (bytes/join.h -> bytes/range.h -> bytes/view.h)

using Catch::Matchers::Equals;

namespace TUtils {
  TEST_CASE("Utils Namespace", "[utils][utilsitself]") {
    SECTION("Sha3 Test") {
      std::string sha3Input = "My SHA3 Input";
      auto sha3Output = Utils::sha3(Utils::stringToBytes(sha3Input));
      Bytes sha3ExpectedOutput = Bytes{0x10, 0x11, 0x40, 0xd6, 0xe7, 0x50, 0x6f, 0x80, 0x4c, 0xf7, 0xb0, 0x37, 0x0f, 0xa9, 0x0b, 0x04, 0xc5, 0xe9, 0x37, 0x4d, 0xdb, 0x0c, 0x8c, 0xbe, 0x12, 0xaf, 0x15, 0x0c, 0x8f, 0xf3, 0xee, 0x36};
      REQUIRE(sha3Output == Hash(sha3ExpectedOutput));
    }

    SECTION("randBytes Test") {
      Bytes randBytesOutput = Utils::randBytes(32);
      REQUIRE(randBytesOutput.size() == 32);
    }

    SECTION("fromBigEndian Test") {
      std::string_view inputBytes("\x10\x11\x40\xd6\xe7\x50\x6f\x80\x4c\xf7\xb0\x37\x0f\xa9\x0b\x04\xc5\xe9\x37\x4d\xdb\x0c\x8c\xbe\x12\xaf\x15\x0c\x8f\xf3\xee\x36");

      auto uint256Output = Utils::fromBigEndian<uint256_t>(inputBytes);
      auto uint64Ouput12To20 = Utils::fromBigEndian<uint64_t>(inputBytes.substr(12, 8));
      auto uint64Ouput20To28 = Utils::fromBigEndian<uint64_t>(inputBytes.substr(20, 8));
      auto uint64Output24To28 = Utils::fromBigEndian<uint64_t>(inputBytes.substr(24, 4));
      auto uint32Output28To32 = Utils::fromBigEndian<uint32_t>(inputBytes.substr(28, 4));
      auto uint160Output5To25 = Utils::fromBigEndian<uint160_t>(inputBytes.substr(5, 20));
      uint256_t uint256ExpectedOutput = uint256_t("7267489482988504755957722036644729207517128093499486419604741885099068616246");
      uint64_t uint64ExpectedOutput12To20 = uint64_t(1128445296761190221);
      uint64_t uint64ExpectedOutput20To28 = uint64_t(15784145542011884812ULL);
      uint64_t uint64ExpectedOutput24To28 = uint64_t(313464076);
      uint32_t uint32ExpectedOutput28To32 = uint32_t(2415128118);
      uint160_t uint160ExpectedOutput5To25 = uint160_t("459205820946237488389499242237511570682479951378");
      REQUIRE(uint256Output == uint256ExpectedOutput);
      REQUIRE(uint64Ouput12To20 == uint64ExpectedOutput12To20);
      REQUIRE(uint64Ouput20To28 == uint64ExpectedOutput20To28);
      REQUIRE(uint64Output24To28 == uint64ExpectedOutput24To28);
      REQUIRE(uint32Output28To32 == uint32ExpectedOutput28To32);
      REQUIRE(uint160Output5To25 == uint160ExpectedOutput5To25);
    }

    SECTION("appendBytes Test") {
      Bytes int1 = Bytes{0x78, 0xF0, 0xB2, 0x91};
      Bytes int2 = Bytes{0xAC, 0x26, 0x0E, 0x43};
      Bytes res = Bytes{0x78, 0xF0, 0xB2, 0x91, 0xAC, 0x26, 0x0E, 0x43};
      Utils::appendBytes(int1, int2);
      REQUIRE(int1 == res);
    }

    SECTION("bytesToString Test") {
      Bytes b1 = Bytes{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};
      Bytes b2 = Bytes{0x30, 0x42, 0x34, 0x48, 0x52, 0x36, 0x33, 0x39};
      std::string s1 = Utils::bytesToString(b1);
      std::string s2 = Utils::bytesToString(b2);
      REQUIRE_THAT(s1, Equals("01234567"));
      REQUIRE_THAT(s2, Equals("0B4HR639"));
    }

    SECTION("stringToBytes Test") {
      std::string s1 = "01234567";
      std::string s2 = "0B4HR639";
      Bytes b1 = Utils::stringToBytes(s1);
      Bytes b2 = Utils::stringToBytes(s2);
      REQUIRE(b1 == Bytes{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37});
      REQUIRE(b2 == Bytes{0x30, 0x42, 0x34, 0x48, 0x52, 0x36, 0x33, 0x39});
    }
  }
}

