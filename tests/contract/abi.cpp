/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/contract/abi.h"
#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/libs/json.hpp"
#include "../../src/utils/strings.h"
#include "../../src/utils/utils.h"

#include <boost/filesystem.hpp>
#include <fstream>

using Catch::Matchers::Equals;

namespace TABI {
TEST_CASE("ABI Namespace", "[contract][abi]") {
  SECTION("Encode Uint256 (Single)") {
      auto functor = ABI::NewEncoder::encodeFunction("testUint(uint256)");
      Bytes eS = ABI::NewEncoder::encodeData(uint256_t("12038189571283151234217456623442137"));

      REQUIRE(functor == Hex::toBytes("c7a16965"));
      REQUIRE(eS == Hex::toBytes(
        "0000000000000000000000000000000000025187505f9a7cca5c5178e81858d9"
      ));
    }

  SECTION("Encode Uint256 (Multiple)") {
      auto eS = ABI::NewEncoder::encodeData(uint256_t("985521342366467353964568564348544758443523147426"),
        uint256_t("3453441424448154428346543455122894428593523456453894523"),
        uint256_t("238745423894452554435879784534423784946532544278453254451345"));
      Functor functor = ABI::NewEncoder::encodeFunction("testMultipleUint(uint256,uint256,uint256)");

      REQUIRE(functor == Hex::toBytes("aab4c13b"));

      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "000000000000000000000000aca04e2e6a9c731a64f56964ab72e6c8270786a2"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "000000000000000000240e3c8296e085da6a626254ed08dd8b03286c83bbe17b"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "000000000000002608ca87ae5c3b4312e205f41a89f288579ccd19f908317091"
      ));
    }

  SECTION("Encode Uint256 (Array)") {

      auto eS = ABI::NewEncoder::encodeData(std::vector<uint256_t>{
        uint256_t("19283178512315252514312458124312935128381523"),
        uint256_t("31482535189448189541125434144"),
        uint256_t("1123444185124184138124378143891242186794252455823414458"),
        uint256_t("215345189442554346421356134551234851234484")});

      auto functor = ABI::NewEncoder::encodeFunction("testUintArr(uint256[])");

      REQUIRE(functor == Hex::toBytes("d1a4e446"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000020"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000004"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "0000000000000000000000000000dd5c2b23fbb5fb408500075ff573e1383853"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 3, eS.begin() + 32 * 4) == Hex::toBytes(
        "000000000000000000000000000000000000000065b9be246336b3f36607ab20"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 4, eS.begin() + 32 * 5) == Hex::toBytes(
        "0000000000000000000bbab3b46bd0328e1d17617db2abfefc19046a083e14ba"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 5, eS.begin() + 32 * 6) == Hex::toBytes(
        "00000000000000000000000000000278d7b6df6a4c94873a7d6559bcab95e2b4"
      ));
    }

  SECTION("Encode Int256 (Single)") {

    auto eS = ABI::NewEncoder::encodeData(int256_t("-123456789012345678901234567890123456789012345678901234567890"));
    auto functor = ABI::NewEncoder::encodeFunction("testInt(int256)");

      REQUIRE(functor.asBytes() == Hex::toBytes("6017d51d"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "ffffffffffffffec550afb1b43e19de8c0785bc873c84b6373300e6931c0f52e"
      ));
    }

  SECTION("Encode Int256 (Multiple)") {

    auto eS = ABI::NewEncoder::encodeData(int256_t("-123456789012345678901234567890123456789012345678901234567890"),
      int256_t("123456789012345678901234567890123456789012345678901234567890"),
      int256_t("-56789012345678901234567890123456789012345678901234567890"),
      int256_t("56789012345678901234567890123456789012345678901234567890"));

      Functor functor = ABI::NewEncoder::encodeFunction("testMultipleInt(int256,int256)");

      REQUIRE(functor.asBytes() == Hex::toBytes("c402855a"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "ffffffffffffffec550afb1b43e19de8c0785bc873c84b6373300e6931c0f52e"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "0000000000000013aaf504e4bc1e62173f87a4378c37b49c8ccff196ce3f0ad2"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "fffffffffffffffffdaf1854f62f44391b682b86c9106cac85300e6931c0f52e"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 3, eS.begin() + 32 * 4) == Hex::toBytes(
        "00000000000000000250e7ab09d0bbc6e497d47936ef93537acff196ce3f0ad2"
      ));
    }

  SECTION("Encode Int256 (Array)") {

    auto eS = ABI::NewEncoder::encodeData(std::vector<int256_t>{
      int256_t("-123456789012345678901234567890123456789012345678901234567890"),
      int256_t("123456789012345678901234567890123456789012345678901234567890"),
      int256_t("-56789012345678901234567890123456789012345678901234567890"),
      int256_t("56789012345678901234567890123456789012345678901234567890")});

      Functor functor = ABI::NewEncoder::encodeFunction("testIntArr(int256[])");

      REQUIRE(functor == Hex::toBytes("47406546"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000020"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000004"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "ffffffffffffffec550afb1b43e19de8c0785bc873c84b6373300e6931c0f52e"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 3, eS.begin() + 32 * 4) == Hex::toBytes(
        "0000000000000013aaf504e4bc1e62173f87a4378c37b49c8ccff196ce3f0ad2"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 4, eS.begin() + 32 * 5) == Hex::toBytes(
        "fffffffffffffffffdaf1854f62f44391b682b86c9106cac85300e6931c0f52e"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 5, eS.begin() + 32 * 6) == Hex::toBytes(
        "00000000000000000250e7ab09d0bbc6e497d47936ef93537acff196ce3f0ad2"
      ));
    }

  SECTION("Encode String (Single)") {

      auto eS = ABI::NewEncoder::encodeData(std::string("Hello World!"));
   
      Functor functor = ABI::NewEncoder::encodeFunction("testString(string)");

      REQUIRE(functor.asBytes() == Hex::toBytes("61cb5a01"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000020"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000c"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "48656c6c6f20576f726c64210000000000000000000000000000000000000000"
      ));
    }

  SECTION("Encode Bool (Multiple)") {

      auto eS = ABI::NewEncoder::encodeData(true, false, true);

      Functor functor = ABI::NewEncoder::encodeFunction("testMultipleBool(bool,bool,bool)");

      REQUIRE(functor.asBytes() == Hex::toBytes("49fdef10"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000001"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000001"
      ));
    }

    SECTION("Encode String (Array) + Uint256 (Array)") {

      auto eS = ABI::NewEncoder::encodeData(std::vector<std::string>{
        "First String", "Second String", "Third String"
      }, std::vector<uint256_t>{
        uint256_t("129838151824165123321245841287434198"),
        uint256_t("2134584124125984418451243118545129854235"),
        uint256_t("1234812315823541285534458693557693548423844235"),
        uint256_t("32452893445892345238552138945234454324523194514")});

      Functor functor = ABI::NewEncoder::encodeFunction("testStringArrWithUintArr(string[],uint256[])");

      REQUIRE(functor.asBytes() == Hex::toBytes("023c4a5e"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000040"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000180"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000003"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 3, eS.begin() + 32 * 4) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000060"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 4, eS.begin() + 32 * 5) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000000a0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 5, eS.begin() + 32 * 6) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000000e0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 6, eS.begin() + 32 * 7) == Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000c"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 7, eS.begin() + 32 * 8) == Hex::toBytes(
        "466972737420537472696e670000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 8, eS.begin() + 32 * 9) == Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000d"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 9, eS.begin() + 32 * 10) == Hex::toBytes(
        "5365636f6e6420537472696e6700000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 10, eS.begin() + 32 * 11) == Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000c"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 11, eS.begin() + 32 * 12) == Hex::toBytes(
        "546869726420537472696e670000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 12, eS.begin() + 32 * 13) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000004"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 13, eS.begin() + 32 * 14) == Hex::toBytes(
        "0000000000000000000000000000000000190183df26aa795c3b01e079ae4fd6"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 14, eS.begin() + 32 * 15) == Hex::toBytes(
        "0000000000000000000000000000000645e1f2c6dad3d9f1675c8163df32551b"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 15, eS.begin() + 32 * 16)== Hex::toBytes(
        "00000000000000000000000000375ef34102454b2b5222061ed99b03a148918b"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 16, eS.begin() + 32 * 17) == Hex::toBytes(
        "00000000000000000000000005af3cf248a13bc919ac44299b86c8a94ba65892"
      ));
    }

    SECTION("Encode Address (Single)") {

      auto eS = ABI::NewEncoder::encodeData(Address(Hex::toBytes("0x873630b0fAE5F8c69392Abdabb3B15270D137Ca1")));

      Functor functor = ABI::NewEncoder::encodeFunction("testAddress(address)");

      REQUIRE(functor.asBytes() == Hex::toBytes("42f45790"));
      REQUIRE(Bytes(eS.begin(), eS.end()) == Hex::toBytes(
        "000000000000000000000000873630b0fae5f8c69392abdabb3b15270d137ca1"
      ));
    }

  SECTION("Encode Bytes (Single)") {

      auto eS = ABI::NewEncoder::encodeData(Hex::toBytes("0xc8191d2e98e7cd9201cef777f85bf857"));

      Functor functor = ABI::NewEncoder::encodeFunction("testBytes(bytes)");

      REQUIRE(functor.asBytes() == Hex::toBytes("3ca8b1a7"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000020"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "c8191d2e98e7cd9201cef777f85bf85700000000000000000000000000000000"
      ));
    }

    SECTION("Encode Bytes (Array) + String (Array)") {

      auto eS = ABI::NewEncoder::encodeData(std::vector<Bytes>{
        Hex::toBytes("0x81a1217428d6d8ff7a419e87cfc948d2"),
        Hex::toBytes("0x2d96cf448d1d455d9013572ac07edefc"),
        Hex::toBytes("0xc584d0de5dbddca6e74686a3c154bb28"),
        Hex::toBytes("0xdb6f06ea16ab61dca14053001c6b5815")},
        std::vector<std::string>{
        "First String", "Second String", "Third String",
        "Fourth String"
      });

      Functor functor = ABI::NewEncoder::encodeFunction("testBytesArrWithStrArr(bytes[],string[])");

      REQUIRE(functor.asBytes() == Hex::toBytes("f1881d9f"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000040"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000001e0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000004"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 3, eS.begin() + 32 * 4) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000080"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 4, eS.begin() + 32 * 5) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000000c0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 5, eS.begin() + 32 * 6) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000100"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 6, eS.begin() + 32 * 7) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000140"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 7, eS.begin() + 32 * 8) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 8, eS.begin() + 32 * 9) == Hex::toBytes(
        "81a1217428d6d8ff7a419e87cfc948d200000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 9, eS.begin() + 32 * 10) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 10, eS.begin() + 32 * 11) == Hex::toBytes(
        "2d96cf448d1d455d9013572ac07edefc00000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 11, eS.begin() + 32 * 12) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 12, eS.begin() + 32 * 13) == Hex::toBytes(
        "c584d0de5dbddca6e74686a3c154bb2800000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 13, eS.begin() + 32 * 14) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 14, eS.begin() + 32 * 15) == Hex::toBytes(
        "db6f06ea16ab61dca14053001c6b581500000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 15, eS.begin() + 32 * 16) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000004"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 16, eS.begin() + 32 * 17) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000080"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 17, eS.begin() + 32 * 18) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000000c0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 18, eS.begin() + 32 * 19) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000100"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 19, eS.begin() + 32 * 20) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000140"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 20, eS.begin() + 32 * 21) == Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000c"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 21, eS.begin() + 32 * 22) == Hex::toBytes(
        "466972737420537472696e670000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 22, eS.begin() + 32 * 23) == Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000d"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 23, eS.begin() + 32 * 24) == Hex::toBytes(
        "5365636f6e6420537472696e6700000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 24, eS.begin() + 32 * 25)== Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000c"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 25, eS.begin() + 32 * 26) == Hex::toBytes(
        "546869726420537472696e670000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 26, eS.begin() + 32 * 27) == Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000d"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 27, eS.begin() + 32 * 28) == Hex::toBytes(
        "466f7572746820537472696e6700000000000000000000000000000000000000"
      ));
    }

  SECTION("Encode All") {

    auto eS = ABI::NewEncoder::encodeData(uint256_t("19283816759128317851231551416451212"),
      std::vector<uint256_t>{
        uint256_t("1239381517249318561241694412"),
        uint256_t("2395843472138412758912309213482574123672567"),
        uint256_t("9138482765346472349817634647689124123"),
        uint256_t("9234782382341248283491")
      },
      true,
      std::vector<bool>{false, true, false},
      Address(Hex::toBytes("0x873630b0fAE5F8c69392Abdabb3B15270D137Ca1")),
      std::vector<Address>{
        Address(Hex::toBytes("0x2D061c095b06efed6A54b6e9B3f50f1b55cce2FF")),
        Address(Hex::toBytes("0x873630b0fAE5F8c69392Abdabb3B15270D137Ca1")),
        Address(Hex::toBytes("0xA462f6A66CC4465fA2d5E90EFA6757f615125760"))
      },
      Hex::toBytes("0xec05537ed99fc9053e29368726573b25"),
      std::vector<Bytes>{
        Hex::toBytes("0xadfae295d92644d19f69e4f20f28d0ae"),
        Hex::toBytes("0x6777b56cd127407ae1b1cc309905521e"),
        Hex::toBytes("0x52719fe16375c2446b109dfcf9336c38"),
        Hex::toBytes("0x6763b32cbd1c695a694d66fe2e729c97")},
      std::string("This is a string"),
      std::vector<std::string>{"Yes", "This", "Is", "A", "String",
                                "Array", "How stupid lol"});

      Functor functor = ABI::NewEncoder::encodeFunction("testAll(uint256,uint256[],bool,bool[],address,address[],bytes,bytes[],string,string[])");

      REQUIRE(functor == Hex::toBytes("d8d2684c"));
      REQUIRE(Bytes(eS.begin(), eS.begin() + 32) == Hex::toBytes(
        "000000000000000000000000000000000003b6c3fc7f2d151440685a319c408c"
      ));
      REQUIRE(Bytes(eS.begin() + 32, eS.begin() + 32 * 2) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000140"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 2, eS.begin() + 32 * 3) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000001"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 3, eS.begin() + 32 * 4) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000001e0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 4, eS.begin() + 32 * 5) == Hex::toBytes(
        "000000000000000000000000873630b0fae5f8c69392abdabb3b15270d137ca1"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 5, eS.begin() + 32 * 6) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000260"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 6, eS.begin() + 32 * 7) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000002e0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 7, eS.begin() + 32 * 8) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000320"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 8, eS.begin() + 32 * 9) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000004c0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 9, eS.begin() + 32 * 10) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000500"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 10, eS.begin() + 32 * 11) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000004"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 11, eS.begin() + 32 * 12) == Hex::toBytes(
        "00000000000000000000000000000000000000000401313ead502c4caecd00cc"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 12, eS.begin() + 32 * 13) == Hex::toBytes(
        "00000000000000000000000000001b80c04c816f5d3f60d46e2c568de014a3f7"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 13, eS.begin() + 32 * 14) == Hex::toBytes(
        "0000000000000000000000000000000006e001fc95fc94cdd826174c57e3d91b"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 14, eS.begin() + 32 * 15) == Hex::toBytes(
        "0000000000000000000000000000000000000000000001f49e59b0c3edac7363"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 15, eS.begin() + 32 * 16) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000003"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 16, eS.begin() + 32 * 17) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 17, eS.begin() + 32 * 18) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000001"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 18, eS.begin() + 32 * 19) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 19, eS.begin() + 32 * 20) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000003"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 20, eS.begin() + 32 * 21) == Hex::toBytes(
        "0000000000000000000000002d061c095b06efed6a54b6e9b3f50f1b55cce2ff"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 21, eS.begin() + 32 * 22) == Hex::toBytes(
        "000000000000000000000000873630b0fae5f8c69392abdabb3b15270d137ca1"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 22, eS.begin() + 32 * 23)== Hex::toBytes(
        "000000000000000000000000a462f6a66cc4465fa2d5e90efa6757f615125760"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 23, eS.begin() + 32 * 24) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 24, eS.begin() + 32 * 25) == Hex::toBytes(
        "ec05537ed99fc9053e29368726573b2500000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 25, eS.begin() + 32 * 26) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000004"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 26, eS.begin() + 32 * 27) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000080"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 27, eS.begin() + 32 * 28) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000000c0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 28, eS.begin() + 32 * 29) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000100"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 29, eS.begin() + 32 * 30) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000140"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 30, eS.begin() + 32 * 31) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 31, eS.begin() + 32 * 32) == Hex::toBytes(
        "adfae295d92644d19f69e4f20f28d0ae00000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 32, eS.begin() + 32 * 33) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 33, eS.begin() + 32 * 34) == Hex::toBytes(
        "6777b56cd127407ae1b1cc309905521e00000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 34, eS.begin() + 32 * 35) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 35, eS.begin() + 32 * 36) == Hex::toBytes(
        "52719fe16375c2446b109dfcf9336c3800000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 36, eS.begin() + 32 * 37) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 37, eS.begin() + 32 * 38)== Hex::toBytes(
        "6763b32cbd1c695a694d66fe2e729c9700000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 38, eS.begin() + 32 * 39) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000010"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 39, eS.begin() + 32 * 40) == Hex::toBytes(
        "54686973206973206120737472696e6700000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 40, eS.begin() + 32 * 41) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000007"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 41, eS.begin() + 32 * 42) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000000e0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 42, eS.begin() + 32 * 43) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000120"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 43, eS.begin() + 32 * 44) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000160"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 44, eS.begin() + 32 * 45) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000001a0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 45, eS.begin() + 32 * 46) == Hex::toBytes(
        "00000000000000000000000000000000000000000000000000000000000001e0"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 46, eS.begin() + 32 * 47) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000220"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 47, eS.begin() + 32 * 48) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000260"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 48, eS.begin() + 32 * 49) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000003"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 49, eS.begin() + 32 * 50) == Hex::toBytes(
        "5965730000000000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 50, eS.begin() + 32 * 51) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000004"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 51, eS.begin() + 32 * 52) == Hex::toBytes(
        "5468697300000000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 52, eS.begin() + 32 * 53) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000002"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 53, eS.begin() + 32 * 54) == Hex::toBytes(
        "4973000000000000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 54, eS.begin() + 32 * 55) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000001"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 55, eS.begin() + 32 * 56) == Hex::toBytes(
        "4100000000000000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 56, eS.begin() + 32 * 57) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000006"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 57, eS.begin() + 32 * 58) == Hex::toBytes(
        "537472696e670000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 58, eS.begin() + 32 * 59) == Hex::toBytes(
        "0000000000000000000000000000000000000000000000000000000000000005"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 59, eS.begin() + 32 * 60) == Hex::toBytes(
        "4172726179000000000000000000000000000000000000000000000000000000"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 60, eS.begin() + 32 * 61) == Hex::toBytes(
        "000000000000000000000000000000000000000000000000000000000000000e"
      ));
      REQUIRE(Bytes(eS.begin() + 32 * 61, eS.begin() + 32 * 62) == Hex::toBytes(
        "486f7720737475706964206c6f6c000000000000000000000000000000000000"
      ));
    }

    SECTION("Decode Uint256 (Array)") {
      Bytes ABI = Hex::toBytes("0x"
        "0000000000000000000000000000000000000000000000000000000000000020"
        "0000000000000000000000000000000000000000000000000000000000000003"
        "0000000000000000000000000000000000000000000000002017594d84130397"
        "0000000000000000000000000000000000000000000000000000027cae776d75"
        "00000000000000000000000000000000000000000000000000016201a9fce5dd"
      );
      std::vector<ABI::Types> types = {ABI::Types::uint256Arr};

    ABI::Decoder d(types, ABI);
    std::vector<uint256_t> dV = d.getData<std::vector<uint256_t>>(0);

    REQUIRE(dV[0] == uint256_t(2312415123141231511));
    REQUIRE(dV[1] == uint256_t(2734526262645));
    REQUIRE(dV[2] == uint256_t(389234263123421));
  }

    SECTION("Decode Int256") {
      Bytes ABI = Hex::toBytes("0x"
        "fffffffffffffffffdaf1854f62f44391b682b86c9106cac85300e6931c0f52e"
      );
      std::vector<ABI::Types> types = {ABI::Types::int256};

    ABI::Decoder d(types, ABI);
    int256_t dV = d.getData<int256_t>(0);

    REQUIRE(dV == int256_t("-56789012345678901234567890123456789012345678901234567890"));
  }

    SECTION("Decode Int256 (Array)") {
      Bytes ABI = Hex::toBytes("0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
       "0000000000000000000000000000000000000000000000000000000000000004"
       "ffffffffffffffec550afb1b43e19de8c0785bc873c84b6373300e6931c0f52e0000000000000013aaf504e4bc1e62173f87a4378c37b49c8ccff196ce3f0ad2fffffffffffffffffdaf1854f62f44391b682b86c9106cac85300e6931c0f52e00000000000000000250e7ab09d0bbc6e497d47936ef93537acff196ce3f0ad2"
      );
      std::vector<ABI::Types> types = {ABI::Types::int256Arr};

    ABI::Decoder d(types, ABI);
    std::vector<int256_t> dV = d.getData<std::vector<int256_t>>(0);

    REQUIRE(dV[0] == int256_t("-123456789012345678901234567890123456789012345678901234567890"));
    REQUIRE(dV[1] == int256_t("123456789012345678901234567890123456789012345678901234567890"));
    REQUIRE(dV[2] == int256_t("-56789012345678901234567890123456789012345678901234567890"));
    REQUIRE(dV[3] == int256_t("56789012345678901234567890123456789012345678901234567890"));
  }

    SECTION("Decode Address (Array)") {
      Bytes ABI = Hex::toBytes("0x"
        "0000000000000000000000000000000000000000000000000000000000000020"
        "0000000000000000000000000000000000000000000000000000000000000003"
        "0000000000000000000000005b38da6a701c568545dcfcb03fcb875f56beddc4"
        "000000000000000000000000ab8483f64d9c6d1ecf9b849ae677dd3315835cb2"
        "0000000000000000000000004b20993bc481177ec7e8f571cecae8a9e22c02db"
      );
      std::vector<ABI::Types> types = {ABI::Types::addressArr};

    ABI::Decoder d(types, ABI);
    std::vector<Address> dV = d.getData<std::vector<Address>>(0);

      REQUIRE(dV[0] == Address(Hex::toBytes("0x5B38Da6a701c568545dCfcB03FcB875f56beddC4")));
      REQUIRE(dV[1] == Address(Hex::toBytes("0xAb8483F64d9C6d1EcF9b849Ae677dD3315835cb2")));
      REQUIRE(dV[2] == Address(Hex::toBytes("0x4B20993Bc481177ec7E8f571ceCaE8A9e22C02db")));
    }

    SECTION("Decode Bool (Array)") {
      Bytes ABI = Hex::toBytes("0x"
        "0000000000000000000000000000000000000000000000000000000000000020"
        "0000000000000000000000000000000000000000000000000000000000000003"
        "0000000000000000000000000000000000000000000000000000000000000001"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000001"
      );
      std::vector<ABI::Types> types = {ABI::Types::booleanArr};

    ABI::Decoder d(types, ABI);
    std::vector<bool> dV = d.getData<std::vector<bool>>(0);

    REQUIRE(dV[0] == true);
    REQUIRE(dV[1] == false);
    REQUIRE(dV[2] == true);
  }

    SECTION("Decode Bytes (Single)") {
      Bytes ABI = Hex::toBytes("0x"
        "0000000000000000000000000000000000000000000000000000000000000020"
        "0000000000000000000000000000000000000000000000000000000000000004"
        "0adf1f1a00000000000000000000000000000000000000000000000000000000"
      );
      std::vector<ABI::Types> types = {ABI::Types::bytes};

      ABI::Decoder d(types, ABI);
      Bytes bytes = d.getData<Bytes>(0);

    REQUIRE(bytes == Hex::toBytes("0x0adf1f1a"));
  }

    SECTION("Decode Bytes (Array)") {
      Bytes ABI = Hex::toBytes("0x"
        "0000000000000000000000000000000000000000000000000000000000000020"
        "0000000000000000000000000000000000000000000000000000000000000004"
        "0000000000000000000000000000000000000000000000000000000000000080"
        "00000000000000000000000000000000000000000000000000000000000000c0"
        "0000000000000000000000000000000000000000000000000000000000000100"
        "0000000000000000000000000000000000000000000000000000000000000140"
        "0000000000000000000000000000000000000000000000000000000000000004"
        "0adf1f1a00000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000004"
        "fffadcba00000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000006"
        "0113ffedc2310000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000002"
        "aaaa000000000000000000000000000000000000000000000000000000000000"
      );
      std::vector<ABI::Types> types = {ABI::Types::bytesArr};

      ABI::Decoder d(types, ABI);
      std::vector<Bytes> dV = d.getData<std::vector<Bytes>>(0);

    REQUIRE(dV[0] == Hex::toBytes("0x0adf1f1a"));
    REQUIRE(dV[1] == Hex::toBytes("0xfffadcba"));
    REQUIRE(dV[2] == Hex::toBytes("0x0113ffedc231"));
    REQUIRE(dV[3] == Hex::toBytes("0xaaaa"));
  }

    SECTION("Decode String (Single)") {
      Bytes ABI = Hex::toBytes("0x"
        "0000000000000000000000000000000000000000000000000000000000000020"
        "000000000000000000000000000000000000000000000000000000000000000e"
        "5468697320697320612074657374000000000000000000000000000000000000"
      );
      std::vector<ABI::Types> types = {ABI::Types::string};

    ABI::Decoder d(types, ABI);
    std::string str = d.getData<std::string>(0);

    REQUIRE(str == "This is a test");
  }

    SECTION("Decode String (Array)") {
      Bytes ABI = Hex::toBytes("0x"
        "0000000000000000000000000000000000000000000000000000000000000020"
        "0000000000000000000000000000000000000000000000000000000000000004"
        "0000000000000000000000000000000000000000000000000000000000000080"
        "00000000000000000000000000000000000000000000000000000000000000c0"
        "0000000000000000000000000000000000000000000000000000000000000100"
        "0000000000000000000000000000000000000000000000000000000000000140"
        "0000000000000000000000000000000000000000000000000000000000000016"
        "5468697320697320746865206669727374207465737400000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000017"
        "5468697320697320746865207365636f6e642074657374000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000016"
        "5468697320697320746865207468697264207465737400000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000016"
        "546869732069732074686520666f727468207465737400000000000000000000"
      );
      std::vector<ABI::Types> types = {ABI::Types::stringArr};

    ABI::Decoder d(types, ABI);
    std::vector<std::string> dV = d.getData<std::vector<std::string>>(0);

    REQUIRE(dV[0] == "This is the first test");
    REQUIRE(dV[1] == "This is the second test");
    REQUIRE(dV[2] == "This is the third test");
    REQUIRE(dV[3] == "This is the forth test"); // Someone fix this typo for the
                                                // love of the cosmos
  }

    SECTION("Decode Bytes (Array) + String (Array)") {
      Bytes ABI = Hex::toBytes("0x"
                               "0000000000000000000000000000000000000000000000000000000000000040"
                               "00000000000000000000000000000000000000000000000000000000000001e0"
                               "0000000000000000000000000000000000000000000000000000000000000004"
                               "0000000000000000000000000000000000000000000000000000000000000080"
                               "00000000000000000000000000000000000000000000000000000000000000c0"
                               "0000000000000000000000000000000000000000000000000000000000000100"
                               "0000000000000000000000000000000000000000000000000000000000000140"
                               "0000000000000000000000000000000000000000000000000000000000000016"
                               "5468697320697320746865206669727374207465737400000000000000000000"
                               "0000000000000000000000000000000000000000000000000000000000000017"
                               "5468697320697320746865207365636f6e642074657374000000000000000000"
                               "0000000000000000000000000000000000000000000000000000000000000016"
                               "5468697320697320746865207468697264207465737400000000000000000000"
                               "0000000000000000000000000000000000000000000000000000000000000016"
                               "546869732069732074686520666f727468207465737400000000000000000000"
                               "0000000000000000000000000000000000000000000000000000000000000004"
                               "0000000000000000000000000000000000000000000000000000000000000080"
                               "00000000000000000000000000000000000000000000000000000000000000c0"
                               "0000000000000000000000000000000000000000000000000000000000000100"
                               "0000000000000000000000000000000000000000000000000000000000000140"
                               "0000000000000000000000000000000000000000000000000000000000000004"
                               "0adf1f1a00000000000000000000000000000000000000000000000000000000"
                               "0000000000000000000000000000000000000000000000000000000000000004"
                               "fffadcba00000000000000000000000000000000000000000000000000000000"
                               "0000000000000000000000000000000000000000000000000000000000000006"
                               "0113ffedc2310000000000000000000000000000000000000000000000000000"
                               "0000000000000000000000000000000000000000000000000000000000000002"
                               "aaaa000000000000000000000000000000000000000000000000000000000000"
      );

      std::vector<ABI::Types> types = {ABI::Types::stringArr, ABI::Types::bytesArr};
      ABI::Decoder d(types, ABI);
      std::vector<std::string> stringArr = d.getData<std::vector<std::string>>(0);
      std::vector<Bytes> bytesArr = d.getData<std::vector<Bytes>>(1);

      REQUIRE(stringArr[0] == "This is the first test");
      REQUIRE(stringArr[1] == "This is the second test");
      REQUIRE(stringArr[2] == "This is the third test");
      REQUIRE(stringArr[3] ==
              "This is the forth test"); // Someone fix this typo for the love of
      // the cosmos
      REQUIRE(bytesArr[0] == Hex::toBytes("0x0adf1f1a"));
      REQUIRE(bytesArr[1] == Hex::toBytes("0xfffadcba"));
      REQUIRE(bytesArr[2] == Hex::toBytes("0x0113ffedc231"));
      REQUIRE(bytesArr[3] == Hex::toBytes("0xaaaa"));
    }
  };
} // namespace TABI
