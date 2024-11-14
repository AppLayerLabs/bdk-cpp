/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/utils/strconv.h"

#include "../../src/utils/hex.h"

using Catch::Matchers::Equals;

// TODO: missing tests for padLeft/padRight (no bytes) and cArrayToBytes

namespace TUtils {
  TEST_CASE("StrConv Namespace", "[utils][strconv]") {
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
  }
}

