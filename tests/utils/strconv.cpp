/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/utils/strconv.h"

#include "../../src/utils/hex.h"

using Catch::Matchers::Equals;

namespace TUtils {
  TEST_CASE("StrConv Namespace", "[utils][strconv]") {
    SECTION("padLeft Test") {
      std::string input = "abcdef";
      std::string output = StrConv::padLeft(input, 10, '0');
      std::string output2 = StrConv::padLeft(input, 20, '1');
      std::string expectedOutput = "0000abcdef";
      std::string expectedOutput2 = "11111111111111abcdef";
      REQUIRE(output == expectedOutput);
      REQUIRE(output2 == expectedOutput2);
    }

    SECTION("padRight Test") {
      std::string input = "abcdef";
      std::string output = StrConv::padRight(input, 10, '0');
      std::string output2 = StrConv::padRight(input, 20, '1');
      std::string expectedOutput = "abcdef0000";
      std::string expectedOutput2 = "abcdef11111111111111";
      REQUIRE(output == expectedOutput);
      REQUIRE(output2 == expectedOutput2);
    }

    SECTION("padLeftBytes Test") {
      Bytes inputBytes = Hex::toBytes("0xabcdef");
      Bytes outputBytes = StrConv::padLeftBytes(inputBytes, 10, 0x00);
      Bytes outputBytes2 = StrConv::padLeftBytes(inputBytes, 20, 0x11);
      Bytes expectedOutputBytes = Hex::toBytes("0x00000000000000abcdef");
      Bytes expectedOutputBytes2 = Hex::toBytes("0x1111111111111111111111111111111111abcdef");
      REQUIRE(outputBytes == expectedOutputBytes);
      REQUIRE(outputBytes2 == expectedOutputBytes2);
    }

    SECTION("padRightBytes Test") {
      Bytes inputBytes = Hex::toBytes("0xabcdef");
      Bytes outputBytes = StrConv::padRightBytes(inputBytes, 10, 0x00);
      Bytes outputBytes2 = StrConv::padRightBytes(inputBytes, 20, 0x11);
      Bytes expectedOutputBytes = Hex::toBytes("0xabcdef00000000000000");
      Bytes expectedOutputBytes2 = Hex::toBytes("0xabcdef1111111111111111111111111111111111");
      REQUIRE(outputBytes == expectedOutputBytes);
      REQUIRE(outputBytes2 == expectedOutputBytes2);
    }

    SECTION("toLower Test") {
      std::string inputStr = "ABCDEF";
      std::string outputStr = inputStr;
      StrConv::toLower(outputStr);
      std::string expectedOutputStr = "abcdef";
      REQUIRE_THAT(outputStr, Equals(expectedOutputStr));
    }

    SECTION("toUpper Test") {
      std::string inputStr = "abcdef";
      std::string outputStr = inputStr;
      StrConv::toUpper(outputStr);
      std::string expectedOutputStr = "ABCDEF";
      REQUIRE_THAT(outputStr, Equals(expectedOutputStr));
    }

    SECTION("bytesToString Test") {
      Bytes b1 = Bytes{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};
      Bytes b2 = Bytes{0x30, 0x42, 0x34, 0x48, 0x52, 0x36, 0x33, 0x39};
      std::string s1 = StrConv::bytesToString(b1);
      std::string s2 = StrConv::bytesToString(b2);
      REQUIRE_THAT(s1, Equals("01234567"));
      REQUIRE_THAT(s2, Equals("0B4HR639"));
    }

    SECTION("stringToBytes Test") {
      std::string s1 = "01234567";
      std::string s2 = "0B4HR639";
      Bytes b1 = StrConv::stringToBytes(s1);
      Bytes b2 = StrConv::stringToBytes(s2);
      REQUIRE(b1 == Bytes{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37});
      REQUIRE(b2 == Bytes{0x30, 0x42, 0x34, 0x48, 0x52, 0x36, 0x33, 0x39});
    }
  }
}

