#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint128_t.h"
#include <iostream>


namespace TSafeUint128_t {
  TEST_CASE("SafeUint128_t Class", "[contract][variables][safeuint128_t]") {
    SECTION("SafeUint128_t constructor (Commit and Revert") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346337460"));
      SafeUint128_t revertedValue(uint128_t("34028236692093846346337460"));

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346337460"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("34028236692093846346337460"));
      REQUIRE(revertedValue.get() == uint128_t(0));
    }

    SECTION("SafeUint128_t operator+") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346337460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346337460"));
      revertedValue.commit();
      SafeUint128_t throwValue(std::numeric_limits<uint128_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue + uint128_t("34028236692093846346337460");
      revertedValue = revertedValue + uint128_t("34028236692093846346337460");
      try {
        throwValue = throwValue + 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("68056473384187692692674920"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346337460"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint128_t operator-") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346337460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346337460"));
      revertedValue.commit();
      SafeUint128_t throwValue(uint128_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue - 10000;
      revertedValue = revertedValue - 10000;
      try {
        throwValue = throwValue - 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("34028236692093846346327460"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346337460"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint128_t operator*") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();
      SafeUint128_t throwValue(std::numeric_limits<uint128_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue * 10000;
      revertedValue = revertedValue * 10000;
      try {
        throwValue = throwValue * 2;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t("340282366920938463463274600000"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("340282366920938463463274600000"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint128_t operator/") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();
      SafeUint128_t throwValue(uint128_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue / 10000;
      revertedValue = revertedValue / 10000;
      try {
        throwValue = throwValue / 2;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t("3402823669209384634632"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("3402823669209384634632"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint128_t operator%") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();
      SafeUint128_t throwValue(uint128_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue % 10000;
      revertedValue = revertedValue % 10000;
      try {
        throwValue = throwValue % 2;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t(7460));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t(7460));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint128_t operator&") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();

      commitedValue = commitedValue & 10000;
      revertedValue = revertedValue & 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t(9472));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t(9472));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
    }

    SECTION("SafeUint128_t operator|") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();

      commitedValue = commitedValue | 10000;
      revertedValue = revertedValue | 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327988"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("34028236692093846346327988"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
    }

    SECTION("SafeUint128_t operator^") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();

      commitedValue = commitedValue ^ 10000;
      revertedValue = revertedValue ^ 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346318516"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("34028236692093846346318516"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
    }

    SECTION("SafeUint128_t operator!") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();

      commitedValue = 0;
      revertedValue = 0;

      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(!commitedValue);
      REQUIRE(!(!revertedValue));
    }

    SECTION("SafeUint128_t operator&&") {
      SafeUint128_t trueValue1(uint128_t(1));
      SafeUint128_t trueValue2(uint128_t(5));
      SafeUint128_t falseValue1(uint128_t(0));
      SafeUint128_t falseValue2(uint128_t(0));

      bool result1 = trueValue1 && trueValue2;
      bool result2 = trueValue1 && falseValue1;
      bool result3 = falseValue1 && trueValue2;
      bool result4 = falseValue1 && falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!result2);
      REQUIRE(!result3);
      REQUIRE(!result4);
    }

    SECTION("SafeUint128_t operator||") {
      SafeUint128_t trueValue1(uint128_t(1));
      SafeUint128_t trueValue2(uint128_t(5));
      SafeUint128_t falseValue1(uint128_t(0));
      SafeUint128_t falseValue2(uint128_t(0));

      bool result1 = trueValue1 || trueValue2;
      bool result2 = trueValue1 || falseValue1;
      bool result3 = falseValue1 || trueValue2;
      bool result4 = falseValue1 || falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!(!result2));
      REQUIRE(!(!result3));
      REQUIRE(!result4);
    }

    SECTION("SafeUint128_t operator==") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));

      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint128_t operator!=") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue != revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint128_t operator<") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();

      REQUIRE(revertedValue < commitedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue < commitedValue);
    }

    SECTION("SafeUint128_t operator<=") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();

      REQUIRE(revertedValue <= commitedValue);
      revertedValue = commitedValue / 2;
      REQUIRE(!(commitedValue <= revertedValue));
      revertedValue.revert();
      REQUIRE(revertedValue <= commitedValue);
    }

    SECTION("SafeUint128_t operator>") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue > revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue > revertedValue);
    }

    SECTION("SafeUint128_t operator>=") {
      SafeUint128_t commitedValue(uint128_t(123981581));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue >= revertedValue);
      revertedValue = commitedValue * 2;
      REQUIRE(commitedValue < revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue >= commitedValue);
    }

    SECTION("SafeUint128_t operator=") {
      SafeUint128_t commitedValue(uint128_t(123981581));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();

      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue == revertedValue);
    }

    SECTION("SafeUint128_t operator+=") {
      SafeUint128_t commitedValue(uint128_t(123981581));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();
      SafeUint128_t throwValue(std::numeric_limits<uint128_t>::max());
      throwValue.commit();

      bool overflow = false;

      try {
        throwValue += commitedValue;
      } catch (const std::overflow_error& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      revertedValue += commitedValue;
      REQUIRE(revertedValue == commitedValue * 2);
      revertedValue.revert();
      commitedValue += 20;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 123981601);
    }

    SECTION("SafeUint128_t operator=-") {
      SafeUint128_t commitedValue(uint128_t(123981581));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();
      SafeUint128_t throwValue(std::numeric_limits<uint128_t>::min());
      throwValue.commit();

      bool overflow = false;

      try {
        throwValue -= commitedValue;
      } catch (const std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      revertedValue -= commitedValue;
      REQUIRE(revertedValue == 0);
      revertedValue.revert();
      commitedValue -= 20;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 123981561);
    }

    SECTION("SafeUint128_t operator=*") {
      SafeUint128_t commitedValue(uint128_t(123981581));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();
      SafeUint128_t throwValue(std::numeric_limits<uint128_t>::max());
      throwValue.commit();

      bool overflow = false;

      try {
        throwValue *= commitedValue;
      } catch (const std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      revertedValue *= commitedValue;
      REQUIRE(revertedValue.get() == 15371432427259561);
      revertedValue.revert();
      REQUIRE(revertedValue.get() == 123981581);
      commitedValue *= 20;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 2479631620);
    }

    SECTION("SafeUint128_t operator=/") {
      SafeUint128_t commitedValue(uint128_t(123981581));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();
      SafeUint128_t throwValue(std::numeric_limits<uint128_t>::max());
      throwValue.commit();

      bool overflow = false;

      try {
        throwValue /= 0;
      } catch (const std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      revertedValue /= commitedValue;
      REQUIRE(revertedValue.get() == 1);
      revertedValue.revert();
      REQUIRE(revertedValue.get() == 123981581);
      commitedValue /= 20;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 6199079);
    }

    SECTION("SafeUint128_t operator%=") {
      SafeUint128_t commitedValue(uint128_t(123981581));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t(123981581));
      revertedValue.commit();
      SafeUint128_t throwValue(std::numeric_limits<uint128_t>::max());
      throwValue.commit();

      bool overflow = false;

      try {
        throwValue %= 0;
      } catch (const std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      revertedValue %= commitedValue;
      REQUIRE(revertedValue.get() == 0);
      revertedValue.revert();
      REQUIRE(revertedValue.get() == 123981581);
      commitedValue %= 20;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 1);
    }

    SECTION("SafeUint128_t operator &=") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();

      commitedValue &= 10000;
      revertedValue &= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t(9472));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t(9472));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
    }

    SECTION("SafeUint64-t operator|=") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();

      commitedValue |= 10000;
      revertedValue |= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327988"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("34028236692093846346327988"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
    }

    SECTION("SafeUint128_t operator^=") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();

      commitedValue ^= 10000;
      revertedValue ^= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346318516"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint128_t("34028236692093846346318516"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
    }

    SECTION("SafeUint128_t operator++") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();
      SafeUint128_t throwValue(std::numeric_limits<uint128_t>::max());
      throwValue.commit();

      bool overflow = false;
      try {
        ++throwValue;
      } catch (std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      ++commitedValue;
      ++revertedValue;
      commitedValue.commit();

      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327461"));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint128_t("34028236692093846346327461"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
    }

    SECTION("SafeUint128_t operator--") {
      SafeUint128_t commitedValue(uint128_t("34028236692093846346327460"));
      commitedValue.commit();
      SafeUint128_t revertedValue(uint128_t("34028236692093846346327460"));
      revertedValue.commit();
      SafeUint128_t throwValue(uint128_t(0));
      throwValue.commit();

      bool overflow = false;
      try {
        --throwValue;
      } catch (std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      --commitedValue;
      --revertedValue;
      commitedValue.commit();

      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327459"));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint128_t("34028236692093846346327459"));
      REQUIRE(revertedValue.get() == uint128_t("34028236692093846346327460"));
    }
  }
}