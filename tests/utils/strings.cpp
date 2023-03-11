#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/strings.h"

using Catch::Matchers::Equals;

namespace TFixedStr {
  TEST_CASE("FixedStr Class", "[utils]") {
    SECTION("FixedStr Default Constructor Test") {
      FixedStr<10> str1;
      FixedStr<20> str2;
      REQUIRE_THAT(str1.get(),  Equals(std::string(10, 0x00)));
      REQUIRE_THAT(str2.get(), Equals(std::string(20, 0x00)));
    }

    SECTION("FixedStr Copy String Constructor Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE_THAT(str1.get(),  Equals("1234567890"));
      REQUIRE_THAT(str2.get(), Equals("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
    }

    SECTION("FixedStr Move String Constructor Test") {
      std::string inputStr1 = "1234567890";
      std::string inputStr2 = "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a";
      FixedStr<10> str1(std::move(inputStr1));
      FixedStr<10> str2(std::move(inputStr2));
      REQUIRE_THAT(str1.get(), Equals("1234567890"));
      REQUIRE_THAT(str2.get(), Equals("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE_THAT(inputStr1, Equals(""));
      REQUIRE_THAT(inputStr2, Equals(""));
    }

    SECTION("FixedStr Copy FixedStr Constructor Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(str1);
      REQUIRE_THAT(str1.get(), Equals("1234567890"));
      REQUIRE_THAT(str2.get(), Equals("1234567890"));
    }

    SECTION("FixedStr Move FixedStr Constructor Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::move(str1));
      REQUIRE_THAT(str1.get(), Equals(""));
      REQUIRE_THAT(str2.get(), Equals("1234567890"));
    }

    SECTION("FixedStr Getter Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE_THAT(str1.get(), Equals("1234567890"));
      REQUIRE_THAT(str2.get(), Equals("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
    }

    SECTION("FixedStr const char* getter Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE_THAT(str1.raw(), Equals("1234567890"));
      REQUIRE_THAT(str2.raw(), Equals("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
    }

    SECTION("FixedStr hex() Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE_THAT(str1.hex(), Equals("31323334353637383930"));
      REQUIRE_THAT(str2.hex(), Equals("0102030405060708090a"));
    }

    SECTION("FixedStr empty() Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      FixedStr<0> str3;
      REQUIRE(str1.empty() == false);
      REQUIRE(str2.empty() == false);
      REQUIRE(str3.empty() == true);
    }

    SECTION("FixedStr size() Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<16> str2(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10"));
      FixedStr<10> str3;
      REQUIRE(str1.size() == 10);
      REQUIRE(str2.size() == 16);
      REQUIRE(str3.size() == 10);
    }

    SECTION("FixedStr cbegin() Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE(*str1.cbegin() == '1');
      REQUIRE(*str2.cbegin() == '\x01');
    }

    SECTION("FixedStr cend() Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a"));
      REQUIRE(*(str1.cend() - 1) == '0');
      REQUIRE(*(str2.cend() - 1) == '\x0a');
    }

    SECTION("FixedStr operator==(FixedStr) Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("1234567890"));
      FixedStr<10> str3(std::string("1234567890"));
      REQUIRE(str1 == str2);
      REQUIRE(str1 == str3);
      REQUIRE(str2 == str3);
    }

    SECTION("FixedStr operator!=(FixedStr) Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("1234567890"));
      FixedStr<10> str3(std::string("1234567891"));
      REQUIRE(str1 != str3);
      REQUIRE(str2 != str3);
    }

    SECTION("FixedStr operator<(FixedStr) Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("1234567891"));
      FixedStr<10> str3(std::string("1234567892"));
      REQUIRE(str1 < str2);
      REQUIRE(str1 < str3);
      REQUIRE(str2 < str3);
    }

    SECTION("FixedStr operator>(FixedStr) Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("1234567891"));
      FixedStr<10> str3(std::string("1234567892"));
      REQUIRE(str2 > str1);
      REQUIRE(str3 > str2);
      REQUIRE(str3 > str1);
    }

    SECTION("FixedStr operator<=(FixedStr) Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("1234567891"));
      FixedStr<10> str3(std::string("1234567891"));
      REQUIRE(str1 <= str2);
      REQUIRE(str2 <= str3);
      REQUIRE(str1 <= str3);
    }

    SECTION("FixedStr operator>=(FixedStr) Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("1234567891"));
      FixedStr<10> str3(std::string("1234567891"));
      REQUIRE(str2 >= str1);
      REQUIRE(str3 >= str2);
      REQUIRE(str3 >= str1);
    }

    SECTION("FixedStr Copy Assigment Operator Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2(std::string("1234567891"));
      str1 = str2;
      REQUIRE(str1 == str2);
      REQUIRE_THAT(str1.get(), Equals("1234567891"));
      REQUIRE_THAT(str2.get(), Equals("1234567891"));
    }

    SECTION("FixedStr Copy Move Operator Test") {
      FixedStr<10> str1(std::string("1234567890"));
      FixedStr<10> str2;
      str2 = std::move(str1);
      REQUIRE_THAT(str2.get(), Equals("1234567890"));
      REQUIRE_THAT(str1.get(), Equals(""));
    }
  }
}

namespace THash {
  TEST_CASE("Hash Class", "[utils]") {
    SECTION("Hash uint256_t Constructor Test") {
      uint256_t i = uint256_t("70518832285973061936518038480459635341011381946952877582230426678885538674712");
      Hash hash(i);
      REQUIRE_THAT(hash.hex(), Equals("9be83ea08b549e7c77644c451b55a674bb12e4668d018183ff9723b1de493818"));
    }

    SECTION("Hash toUint256 Test") {
      uint256_t i = uint256_t("70518832285973061936518038480459635341011381946952877582230426678885538674712");
      Hash hash(i);
      REQUIRE(hash.toUint256() == i);
    }

    SECTION("Hash random() Test") {
      Hash hash1 = Hash::random();
      Hash hash2 = Hash::random();
      REQUIRE(hash1 != hash2);
    }
  }
}

namespace TSignature {
  TEST_CASE("Signature Class", "[utils]") {
    SECTION("Signature r()") {
      Signature sig(std::string("70518832285973061936518038480459635341011381946952877582230426678"));
      REQUIRE(sig.r() == uint256_t("24962382450703388064783334469112749050093133395427026078791530264393631937849"));
    }

    SECTION("Signature s()") {
      Signature sig(std::string("70518832285973061936518038480459635341011381946952877582230426678"));
      REQUIRE(sig.s() == uint256_t("24515370196810216536934266698278180508572754644750025621044793698089203807799"));
    }

    SECTION("Signature v()") {
      Signature sig(std::string("70518832285973061936518038480459635341011381946952877582230426678"));
      REQUIRE(sig.v() == uint8_t(56));
    }
  }
}

namespace TAddress {
  TEST_CASE("Address Class", "[utils]") {
    SECTION("Address Copy Constructor Test") {
      Address addr1(std::string("0x71c7656ec7ab88b098defb751b7401b5f6d8976f"), false);
      Address addr2(std::string("\x71\xc7\x65\x6e\xc7\xab\x88\xb0\x98\xde\xfb\x75\x1b\x74\x01\xb5\xf6\xd8\x97\x6f"), true);
      REQUIRE(addr1 == addr2);
      REQUIRE_THAT(addr1.hex(), Equals("71c7656ec7ab88b098defb751b7401b5f6d8976f"));
      REQUIRE_THAT(addr2.get(), Equals("\x71\xc7\x65\x6e\xc7\xab\x88\xb0\x98\xde\xfb\x75\x1b\x74\x01\xb5\xf6\xd8\x97\x6f"));
    }
  
    SECTION("Address Move String Constructor Test") {
      std::string str("0x71c7656ec7ab88b098defb751b7401b5f6d8976f");
      Address addr1(std::move(str), false);
      REQUIRE_THAT(addr1.hex(), Equals("71c7656ec7ab88b098defb751b7401b5f6d8976f"));
      REQUIRE_THAT(str, Equals(""));
    }
  
    SECTION("Address Move Address Constructor Test") {
      Address addr1(std::string("0x71c7656ec7ab88b098defb751b7401b5f6d8976f"), false);
      Address addr2(std::move(addr1));
      REQUIRE_THAT(addr1.get(), Equals(""));
      REQUIRE_THAT(addr2.hex(), Equals("71c7656ec7ab88b098defb751b7401b5f6d8976f"));
    }

    SECTION("Address::toChksum Test") {
      Address inputAddress(std::string("0xfb6916095ca1df60bb79ce92ce3ea74c37c5d359"), false);
      std::string inputChecksum = inputAddress.toChksum();
      Address outputAddress(inputChecksum, false);
      Address expectedOutputAddress(std::string("0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359"), false);
      REQUIRE_THAT(outputAddress.get(), Equals(expectedOutputAddress.get()));
    }

    SECTION("isChksum Test") {
      std::string inputAddress = "0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359";
      REQUIRE(Address::isChksum(inputAddress));
    }

    SECTION("isAddress Test") {
      std::string inputHexAddress = "0xfb6916095ca1df60bb79ce92ce3ea74c37c5d359";
      std::string inputBytesAddress = "\xfb\x69\x16\x09\x5c\xa1\xdf\x60\xbb\x79\xce\x92\xce\x3e\xa7\x4c\x37\xc5\xd3\x59";
      REQUIRE(Address::isValid(inputHexAddress, false));
      REQUIRE(Address::isValid(inputBytesAddress, true));
      REQUIRE(!Address::isValid(inputHexAddress, true));
      REQUIRE(!Address::isValid(inputBytesAddress, false));
    }
  }
}

