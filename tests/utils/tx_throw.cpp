#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/tx.h"

using Catch::Matchers::Equals;

namespace TTX {
  TEST_CASE("TxBlock (Throw)", "[utils][tx][throw]") {
    SECTION("Tx is not a list") {
      bool catched = false;
      TxBlock tx(
        Address(std::string_view("0x13b5c424686de186bc5268d5cfe6aa4200ca9aee"), false),
        Address(std::string_view("0x31Af43C5E5924610a9c02B669c7980D9eBdB9719"), false),
        Hex::toBytes("0xe426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e"),
        uint64_t(8080),                 /// ChainID
        uint256_t("42968208492763873"),     /// Nonce
        uint256_t("166903214424643"),       /// Value
        uint256_t("65612315125671"),        /// maxPriorityFeePerGas
        uint256_t("712471569147246"),       /// maxFeePerGas
        uint256_t("61182866117425671"),     /// gasLimit
        Hex::toBytes("ce974dad85cf9593db9d5c3e89ca8c67ca0f841dc97f2c58c6ea2038e4fa6d8d")
      );
      std::string txStr = tx.rlpSerialize();
      txStr[1] = 0xf6;
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx RLP too short/large") {
      bool catch1 = false;
      bool catch2 = false;
      TxBlock tx(
        Address(std::string_view("0x13b5c424686de186bc5268d5cfe6aa4200ca9aee"), false),
        Address(std::string_view("0x31Af43C5E5924610a9c02B669c7980D9eBdB9719"), false),
        Hex::toBytes("0xe426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e"),
        uint64_t(8080),                 /// ChainID
        uint256_t("42968208492763873"),     /// Nonce
        uint256_t("166903214424643"),       /// Value
        uint256_t("65612315125671"),        /// maxPriorityFeePerGas
        uint256_t("712471569147246"),       /// maxFeePerGas
        uint256_t("61182866117425671"),     /// gasLimit
        Hex::toBytes("ce974dad85cf9593db9d5c3e89ca8c67ca0f841dc97f2c58c6ea2038e4fa6d8d")
      );
      std::string txShortStr = tx.rlpSerialize();
      txShortStr = txShortStr.substr(0, txShortStr.size() - 1);
      std::string txLargeStr = tx.rlpSerialize();
      txLargeStr += "0x00";
      try { TxBlock txShort(txShortStr, 8080); } catch (std::exception &e) { catch1 = true; }
      try { TxBlock txLarge(txLargeStr, 8080); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    SECTION("Tx chainId too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6b71f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); /// "0x...b7" -> not value anymore", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx MaxPriorityFeePerGas too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1b73bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); /// "0x...b7" -> not value anymore", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx Nonce too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f90b798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); /// "0x...b7" -> not value anymore", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx MaxPriorityFeePerGas too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1b73bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); // /// "0x...b7" -> not value anymore", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx MaxFeePerGas too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7b70287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); // /// "0x...b7" -> not value anymore", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }


    SECTION("Tx GasLimit too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56eb7d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); // /// "0x...b7" -> not value anymore", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx to with wrong size") {
      bool catch1 = false;
      bool catch2 = false;
      std::string txBiggerStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079513b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); // "0x..........................95 != 0x..........................94", thus throw
      std::string txSmallerStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079313b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); // "0x..........................93 != 0x..........................94", thus throw
      try { TxBlock tx1(txBiggerStr, 8080); } catch (std::exception &e) { catch1 = true; }
      try { TxBlock tx2(txSmallerStr, 8080); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    SECTION("Tx value too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aeeb797cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); /// "0x...b7" -> not value anymore", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx V too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec002a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); // "0x...02 > 0x01", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx R too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a13f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a702f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a13f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); /// "0x...a1 > 0xa0", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx S too large") {
      bool catched = false;
      std::string txStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa12d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); /// "0x...a1 > 0xa0", thus throw
      try { TxBlock tx(txStr, 8080); } catch (std::exception &e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Tx invalid signature (wrong elliptic curve)") {
      bool catch1 = false;
      bool catch2 = false;
      std::string txWrongRStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a0fffffffffffffffffffffffffffffffffffffffffffffffffffffff1b4c39fdfa02d71eaeba43f0cbd3f9184afc4e0a907c0b1b69b6a78e6af38b7ddf6b6c0e2a7"
      ); // R is zeroed (after second "0xa0"), thus throw
      std::string txWrongSStr = Hex::toBytes(
        "02f8d6821f908798a75ba3d89ae1863bac8ebc67a7870287fd36caa56e87d95d7e1944fa079413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24ec001a03f40d77504fededf1c0f59b1b5aeecd6cb1c9bbca983352f10717441b4c39fdfa0fffffffffffffffffffffffffffffffffffffffffffffffffffffffff6c0e2a7"
      ); // S is zeroed (after third "0xa0"), thus throw
      try { TxBlock txR(txWrongRStr, 8080); } catch (std::exception &e) { catch1 = true; }
      try { TxBlock txS(txWrongSStr, 8080); } catch (std::exception &e) { catch2 = true; }
      REQUIRE(catch1 == true);
      REQUIRE(catch2 == true);
    }

    SECTION("Tx invalid PrivKey (doesn't match from)") {
      bool catched = false;
      try {
        TxBlock tx(
          Address(std::string("0x1234567890123456789012345678901234567890"), false),
          Address(std::string("0x1234567890123456789012345678901234567890"), false),
          std::string(""), 8080, 0, 0, 90, 90, 21000, std::string("12345678901234567890123456789012")
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

