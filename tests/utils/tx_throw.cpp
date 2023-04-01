#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/tx.h"

using Catch::Matchers::Equals;

namespace TTX {
  TEST_CASE("TxBlock (Throw)", "[utils][tx]") {
    SECTION("Tx is not a list") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf66b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"
      ); // "0xf6.. < 0xf7..", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx RLP too short/large") {
      bool catch1 = false;
      bool catch2 = false;
      std::string txShortStr = Hex::toBytes(
        "0xf88f8085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x..8f < 0x..90", thus throw
      std::string txLargeStr = Hex::toBytes(
        "0xf8918085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x..91 > 0x..90", thus throw
      try { TxBlock txShort(txShortStr, 8080); } catch (std::exception &e) { catch1 = true; }
      try { TxBlock txLarge(txLargeStr, 8080); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    SECTION("Tx GasPrice too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf89080f5178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x......f5 > 0x......85", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx GasLimit too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf8908085178411b200f303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x..................f3 > 0x..................83", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx to with wrong size") {
      bool catch1 = false;
      bool catch2 = false;
      std::string txBiggerStr = Hex::toBytes(
        "0xf8908085178411b2008303f15595bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x..........................95 != 0x..........................94", thus throw
      std::string txSmallerStr = Hex::toBytes(
        "0xf8908085178411b2008303f15593bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x..........................93 != 0x..........................94", thus throw
      try { TxBlock tx1(txBiggerStr, 8080); } catch (std::exception &e) { catch1 = true; }
      try { TxBlock tx2(txSmallerStr, 8080); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    SECTION("Tx value too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22f76a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x...8ccd22f7 < 0x...8ccd2287", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx V too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d79ffa05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x...0a4d79ff > 0x...0a4d7925", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx R too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925ff5ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x...0a4d7925ff > 0x...0a4d7925a0", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx S too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27ff727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x...96fc27ff > 0x...96fc27a0", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    // TODO: how to actually test "chainId too high"?
    /*
    SECTION("Tx chainId too high") {
      bool catched = false;
      // ...
      REQUIRE(catched == true);
    }
    */

    SECTION("Tx invalid signature (wrong V)") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7910a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // "0x...0a4d7910" < "0x...0a4d7925" (v is not 27 or 28), thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx invalid signature (wrong elliptic curve)") {
      bool catch1 = false;
      bool catch2 = false;
      std::string txWrongRStr = Hex::toBytes(
        "0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a00000000000000000000000000000000000000000000000000000000000000000a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"
      ); // R is zeroed (after second "0xa0"), thus throw
      std::string txWrongSStr = Hex::toBytes(
        "0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a00000000000000000000000000000000000000000000000000000000000000000"
      ); // S is zeroed (after third "0xa0"), thus throw
      try { TxBlock txR(txWrongRStr, 8080); } catch (std::exception &e) { catch1 = true; }
      try { TxBlock txS(txWrongSStr, 8080); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    // TODO: can't do this one unless Secp256k1::recover() can also be done
    /*
    SECTION(Tx invalid signature (can't recover pubkey)") {
      bool catched = false;
      // ...
      REQUIRE(catched == true);
    }
    */

    SECTION("Tx invalid PrivKey (doesn't match from)") {
      bool catched = false;
      try {
        TxBlock tx(
          Address(std::string("0x1234567890123456789012345678901234567890"), false),
          Address(std::string("0x1234567890123456789012345678901234567890"), false),
          std::string(""), 8080, 0, 0, 90, 21000, std::string("12345678901234567890123456789012")
        );
      } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    // TODO: not sure how to test last two throws (recover() and verifySig()) - see todo above and from ecdsa.cpp
  }

  TEST_CASE("TxValidator (Throw)", "[utils][tx]") {
    SECTION("Tx is not a list") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf645808026a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"
      ); // "0xf6.. < 0xf8..", thus throw
      try { TxValidator tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx RLP too short/large") {
      bool catch1 = false;
      bool catch2 = false;
      std::string txShortStr = Hex::toBytes(
        "0xf844808026a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"
      ); // "0x..44 < 0x..45", thus throw
      std::string txLargeStr = Hex::toBytes(
        "0xf846808026a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"
      ); // "0x..46 > 0x..45", thus throw
      try { TxValidator txShort(txShortStr, 8080); } catch (std::exception &e) { catch1 = true; }
      try { TxValidator txLarge(txLargeStr, 8080); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    SECTION("Tx V too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf8458080ffa08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"
      ); // "0x........ff > 0x........26", thus throw
      try { TxValidator tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx R too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf845808026ff8a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"
      ); // "0x..........ff > 0x..........a0", thus throw
      try { TxValidator tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx S too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf845808026a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34ff77a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"
      ); // "0x...ee0b34ff > 0x...ee0b34a0", thus throw
      try { TxValidator tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    // TODO: how to actually test "chainId too high"?
    /*
    SECTION("Tx chainId too high") {
      bool catched = false;
      // ...
      REQUIRE(catched == true);
    }
    */

    SECTION("Tx invalid signature (wrong V)") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "0xf845808006a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"
      ); // "0x........06" < "0x........26" (v is not 27 or 28), thus throw
      try { TxValidator tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx invalid signature (wrong elliptic curve)") {
      bool catch1 = false;
      bool catch2 = false;
      std::string txWrongRStr = Hex::toBytes(
        "0xf845808026a00000000000000000000000000000000000000000000000000000000000000000a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"
      ); // R is zeroed (after first "0xa0"), thus throw
      std::string txWrongSStr = Hex::toBytes(
        "0xf845808026a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a00000000000000000000000000000000000000000000000000000000000000000"
      ); // S is zeroed (after "0x...ee0b34a0"), thus throw
      try { TxValidator txR(txWrongRStr, 8080); } catch (std::exception &e) { catch1 = true; }
      try { TxValidator txS(txWrongSStr, 8080); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    // TODO: can't do this one unless Secp256k1::recover() can also be done
    /*
    SECTION(Tx invalid signature (can't recover pubkey)") {
      bool catched = false;
      // ...
      REQUIRE(catched == true);
    }
    */

    SECTION("Tx invalid PrivKey (doesn't match from)") {
      bool catched = false;
      try {
        TxValidator tx(
          Address(std::string("0x1234567890123456789012345678901234567890"), false),
          std::string(""), 8080, 0, std::string("12345678901234567890123456789012")
        );
      } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    // TODO: not sure how to test last two throws (recover() and verifySig()) - see todo above and from ecdsa.cpp
  }
}

