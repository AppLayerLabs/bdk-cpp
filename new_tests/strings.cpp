#include "../new_src/libs/catch2/catch_amalgamated.hpp"
#include "../new_src/utils/strings.h"

using Catch::Matchers::Equals;

namespace TFixedStr {
  TEST_CASE("FixedStr Class") {
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
  TEST_CASE("Hash Class") {
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
  TEST_CASE("Signature Class") {
    SECTION("Signature r()") {
      Signature sig(std::string("70518832285973061936518038480459635341011381946952877582230426678"));
      FixedStr<32> rStr(std::string("70518832285973061936518038480459"));
      REQUIRE(sig.r().get() == rStr.get());
    }
    
    SECTION("Signature s()") {
      Signature sig(std::string("70518832285973061936518038480459635341011381946952877582230426678"));
      FixedStr<32> sStr(std::string("63534101138194695287758223042667"));
      REQUIRE(sig.s().get() == sStr.get());
    }

    SECTION("Signature v()") {
      Signature sig(std::string("70518832285973061936518038480459635341011381946952877582230426678"));
      std::cout << sig.get() << std::endl;
      FixedStr<1> vStr(std::string("8"));
      REQUIRE(sig.v().get() == vStr.get());
    }
  }
}

namespace TAddress {
  TEST_CASE("Address Copy Constructor Test") {
    Address addr1(std::string("0x71c7656ec7ab88b098defb751b7401b5f6d8976f"), true);
    Address addr2(std::string("\x71\xc7\x65\x6e\xc7\xab\x88\xb0\x98\xde\xfb\x75\x1b\x74\x01\xb5\xf6\xd8\x97\x6f"), false);
    REQUIRE(addr1 == addr2);
    REQUIRE_THAT(addr1.get(), Equals("\x71\xc7\x65\x6e\xc7\xab\x88\xb0\x98\xde\xfb\x75\x1b\x74\x01\xb5\xf6\xd8\x97\x6f"));
    REQUIRE_THAT(addr2.hex(), Equals("71c7656ec7ab88b098defb751b7401b5f6d8976f"));
  }

  TEST_CASE("Address Move String Constructor Test") {
    std::string str("0x71c7656ec7ab88b098defb751b7401b5f6d8976f");
    Address addr1(std::move(str), true);
    REQUIRE_THAT(addr1.hex(), Equals("71c7656ec7ab88b098defb751b7401b5f6d8976f"));
    REQUIRE_THAT(str, Equals(""));
  }

  TEST_CASE("Address Move Address Constructor Test") {
    Address addr1(std::string("0x71c7656ec7ab88b098defb751b7401b5f6d8976f"), true);
    Address addr2(std::move(addr1));
    REQUIRE_THAT(addr2.hex(), Equals("71c7656ec7ab88b098defb751b7401b5f6d8976f"));
    REQUIRE_THAT(addr1.hex(), Equals(""));
  }
}