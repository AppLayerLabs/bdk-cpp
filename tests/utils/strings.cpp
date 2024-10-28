/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/strings.h"
#include "bytes/view.h"

using Catch::Matchers::Equals;

namespace TFixedStr {
  TEST_CASE("FixedBytes Class", "[utils][strings]") {
    SECTION("FixedBytes Default Constructor") {
      FixedBytes<10> str1;
      FixedBytes<20> str2;
      REQUIRE(str1.asBytes() == Bytes(10, 0x00));
      REQUIRE(str2.asBytes() == Bytes(20, 0x00));
    }

    SECTION("FixedBytes Copy Bytes Constructor") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE(str1.asBytes() == Bytes({'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'}));
      REQUIRE(str2.asBytes() == Bytes({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a}));
    }

    SECTION("FixedBytes Copy FixedBytes Constructor") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(str1);
      REQUIRE(str1.asBytes() == Bytes({'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'}));
      REQUIRE(str2.asBytes() == Bytes({'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'}));
    }

    SECTION("FixedBytes Getter") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE(str1 == FixedBytes<10>({'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'}));
      REQUIRE(str2 == FixedBytes<10>({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a}));
    }

    SECTION("FixedBytes const char* getter") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE_THAT(std::string(reinterpret_cast<const char*>(str1.data()), str1.size()), Equals("1234567890"));
      REQUIRE_THAT(std::string(reinterpret_cast<const char*>(str2.data()), str2.size()), Equals("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
    }

    SECTION("FixedBytes hex()") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE_THAT(str1.hex(), Equals("31323334353637383930"));
      REQUIRE_THAT(str2.hex(), Equals("0102030405060708090a"));
    }

    SECTION("FixedBytes size()") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<16> str2(bytes::view("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10"));
      FixedBytes<10> str3;
      REQUIRE(str1.size() == 10);
      REQUIRE(str2.size() == 16);
      REQUIRE(str3.size() == 10);
    }

    SECTION("FixedBytes cbegin()") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE(*str1.cbegin() == '1');
      REQUIRE(*str2.cbegin() == '\x01');
    }

    SECTION("FixedBytes cend()") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE(*(str1.cend() - 1) == '0');
      REQUIRE(*(str2.cend() - 1) == '\x0a');
    }

    SECTION("FixedBytes operator==(FixedBytes)") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("1234567890"));
      FixedBytes<10> str3(bytes::view("1234567890"));
      REQUIRE(str1 == str2);
      REQUIRE(str1 == str3);
      REQUIRE(str2 == str3);
    }

    SECTION("FixedBytes operator!=(FixedBytes)") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("1234567890"));
      FixedBytes<10> str3(bytes::view("1234567891"));
      REQUIRE(str1 != str3);
      REQUIRE(str2 != str3);
    }

    SECTION("FixedBytes operator<(FixedBytes)") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("1234567891"));
      FixedBytes<10> str3(bytes::view("1234567892"));
      REQUIRE(str1 < str2);
      REQUIRE(str1 < str3);
      REQUIRE(str2 < str3);
    }

    SECTION("FixedBytes operator>(FixedBytes)") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("1234567891"));
      FixedBytes<10> str3(bytes::view("1234567892"));
      REQUIRE(str2 > str1);
      REQUIRE(str3 > str2);
      REQUIRE(str3 > str1);
    }

    SECTION("FixedBytes operator<=(FixedBytes)") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("1234567891"));
      FixedBytes<10> str3(bytes::view("1234567891"));
      REQUIRE(str1 <= str2);
      REQUIRE(str2 <= str3);
      REQUIRE(str1 <= str3);
    }

    SECTION("FixedBytes operator>=(FixedBytes)") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("1234567891"));
      FixedBytes<10> str3(bytes::view("1234567891"));
      REQUIRE(str2 >= str1);
      REQUIRE(str3 >= str2);
      REQUIRE(str3 >= str1);
    }

    SECTION("FixedBytes Copy Assigment Operator") {
      FixedBytes<10> str1(bytes::view("1234567890"));
      FixedBytes<10> str2(bytes::view("1234567891"));
      str1 = str2;
      REQUIRE(str1 == str2);
      REQUIRE(str1.asBytes() == Bytes({'1', '2', '3', '4', '5', '6', '7', '8', '9', '1'}));
      REQUIRE(str2.asBytes() == Bytes({'1', '2', '3', '4', '5', '6', '7', '8', '9', '1'}));
    }
  }
}

namespace THash {
  TEST_CASE("Hash Class", "[utils]") {
    SECTION("Hash uint256_t Constructor") {
      uint256_t i = uint256_t("70518832285973061936518038480459635341011381946952877582230426678885538674712");
      Hash hash(i);
      REQUIRE_THAT(hash.hex(), Equals("9be83ea08b549e7c77644c451b55a674bb12e4668d018183ff9723b1de493818"));
    }

    SECTION("Hash toUint256") {
      uint256_t i = uint256_t("70518832285973061936518038480459635341011381946952877582230426678885538674712");
      Hash hash(i);
      REQUIRE(hash.toUint256() == i);
    }

    SECTION("Hash random()") {
      Hash hash1 = Hash::random();
      Hash hash2 = Hash::random();
      REQUIRE(hash1 != hash2);
    }
  }
}

namespace TSignature {
  TEST_CASE("Signature Class", "[utils]") {
    SECTION("Signature r()") {
      Signature sig(bytes::view("70518832285973061936518038480459635341011381946952877582230426678"));
      REQUIRE(sig.r() == uint256_t("24962382450703388064783334469112749050093133395427026078791530264393631937849"));
    }

    SECTION("Signature s()") {
      Signature sig(bytes::view("70518832285973061936518038480459635341011381946952877582230426678"));
      REQUIRE(sig.s() == uint256_t("24515370196810216536934266698278180508572754644750025621044793698089203807799"));
    }

    SECTION("Signature v()") {
      Signature sig(bytes::view("70518832285973061936518038480459635341011381946952877582230426678"));
      REQUIRE(sig.v() == uint8_t(56));
    }
  }
}

namespace TAddress {
  TEST_CASE("Address Class", "[utils]") {
    SECTION("Address Copy Constructor") {
      Address addr1(Bytes({0x71, 0xc7, 0x65, 0x6e, 0xc7, 0xab, 0x88, 0xb0, 0x98, 0xde, 0xfb, 0x75, 0x1b, 0x74, 0x01, 0xb5, 0xf6, 0xd8, 0x97, 0x6f}));
      Address addr2(std::string("\x71\xc7\x65\x6e\xc7\xab\x88\xb0\x98\xde\xfb\x75\x1b\x74\x01\xb5\xf6\xd8\x97\x6f"), true);
      REQUIRE(addr1 == addr2);
      REQUIRE_THAT(addr1.hex(), Equals("71c7656ec7ab88b098defb751b7401b5f6d8976f"));
      REQUIRE(addr2 == Address({0x71, 0xc7, 0x65, 0x6e, 0xc7, 0xab, 0x88, 0xb0, 0x98, 0xde, 0xfb, 0x75, 0x1b, 0x74, 0x01, 0xb5, 0xf6, 0xd8, 0x97, 0x6f}));
    }

    SECTION("Address toChksum") {
      Address inputAddress(std::string("0xfb6916095ca1df60bb79ce92ce3ea74c37c5d359"), false);
      std::string inputChecksum = Address::checksum(inputAddress);
      Address outputAddress(inputChecksum, false);
      Address expectedOutputAddress(std::string("0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359"), false);
      REQUIRE(outputAddress == expectedOutputAddress);
    }

    SECTION("Address isValid") {
      std::string inputHexAddress = "0xfb6916095ca1df60bb79ce92ce3ea74c37c5d359";
      std::string inputBytesAddress = "\xfb\x69\x16\x09\x5c\xa1\xdf\x60\xbb\x79\xce\x92\xce\x3e\xa7\x4c\x37\xc5\xd3\x59";
      REQUIRE(Address::isValid(inputHexAddress, false));
      REQUIRE(Address::isValid(inputBytesAddress, true));
      REQUIRE(!Address::isValid(inputHexAddress, true));
      REQUIRE(!Address::isValid(inputBytesAddress, false));
    }

    SECTION("Address isChksum") {
      std::string inputAddress = "0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359";
      std::string inputUpper = "0xFB6916095CA1DF60BB79CE92CE3EA74C37C5D359";
      std::string inputLower = "0xfb6916095ca1df60bb79ce92ce3ea74c37c5d359";
      std::string inputWrong = "0xFb6916095CA1DF60Bb79cE92Ce3eA74C37C5D359";
      REQUIRE(Address::isChksum(inputAddress));
      REQUIRE(!Address::isChksum(inputUpper));
      REQUIRE(!Address::isChksum(inputLower));
      REQUIRE(!Address::isChksum(inputWrong));
    }
  }
}

