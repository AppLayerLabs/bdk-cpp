#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint160_t.h"
#include <iostream>


namespace TSafeUint160_t {
  TEST_CASE("SafeUint160_t Class", "[contracts][variables][safeuint160_t]") {
    SECTION("SafeUint160_t constructor (Commit and Revert") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
      REQUIRE(revertedValue.get() == uint160_t(0));
    }

    SECTION("SafeUint160_t operator+") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();
      SafeUint160_t throwValue(std::numeric_limits<uint160_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue + uint160_t("146150163733090291820368483271628301965593254");
      revertedValue = revertedValue + uint160_t("146150163733090291820368483271628301965593254");
      try {
        throwValue = throwValue + 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t("292300327466180583640736966543256603931186508"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint160_t operator-") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();
      SafeUint160_t throwValue(uint160_t(0));
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

      REQUIRE(commitedValue.get() == uint160_t("146150163733090291820368483271628301965583254"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint160_t operator*") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();
      SafeUint160_t throwValue(std::numeric_limits<uint160_t>::max());
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
      REQUIRE(revertedValue.get() == uint160_t("1461501637330902918203684832716283019655932540000"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t("1461501637330902918203684832716283019655932540000"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint160_t operator/") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();
      SafeUint160_t throwValue(uint160_t(0));
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
      REQUIRE(revertedValue.get() == uint160_t("14615016373309029182036848327162830196559"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t("14615016373309029182036848327162830196559"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint160_t operator%") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();
      SafeUint160_t throwValue(uint160_t(0));
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
      REQUIRE(revertedValue.get() == uint160_t(3254));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t(3254));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint160_t operator&") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();

      commitedValue = commitedValue & 10000;
      revertedValue = revertedValue & 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint160_t(512));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t(512));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
    }

    SECTION("SafeUint160_t operator|") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();

      commitedValue = commitedValue | 10000;
      revertedValue = revertedValue | 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965602742"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t("146150163733090291820368483271628301965602742"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
    }

    SECTION("SafeUint160_t operator^") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();

      commitedValue = commitedValue ^ 10000;
      revertedValue = revertedValue ^ 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965602230"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t("146150163733090291820368483271628301965602230"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
    }

    SECTION("SafeUint160_t operator!") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();

      commitedValue = 0;
      revertedValue = 0;

      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(!commitedValue);
      REQUIRE(!(!revertedValue));
    }

    SECTION("SafeUint160_t operator&&") {
      SafeUint160_t trueValue1(uint160_t(1));
      SafeUint160_t trueValue2(uint160_t(5));
      SafeUint160_t falseValue1(uint160_t(0));
      SafeUint160_t falseValue2(uint160_t(0));

      bool result1 = trueValue1 && trueValue2;
      bool result2 = trueValue1 && falseValue1;
      bool result3 = falseValue1 && trueValue2;
      bool result4 = falseValue1 && falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!result2);
      REQUIRE(!result3);
      REQUIRE(!result4);
    }

    SECTION("SafeUint160_t operator||") {
      SafeUint160_t trueValue1(uint160_t(1));
      SafeUint160_t trueValue2(uint160_t(5));
      SafeUint160_t falseValue1(uint160_t(0));
      SafeUint160_t falseValue2(uint160_t(0));

      bool result1 = trueValue1 || trueValue2;
      bool result2 = trueValue1 || falseValue1;
      bool result3 = falseValue1 || trueValue2;
      bool result4 = falseValue1 || falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!(!result2));
      REQUIRE(!(!result3));
      REQUIRE(!result4);
    }

    SECTION("SafeUint160_t operator==") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));

      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint160_t operator!=") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue != revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint160_t operator<") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();

      REQUIRE(revertedValue < commitedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue < commitedValue);
    }

    SECTION("SafeUint160_t operator<=") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();

      REQUIRE(revertedValue <= commitedValue);
      revertedValue = commitedValue / 2;
      REQUIRE(!(commitedValue <= revertedValue));
      revertedValue.revert();
      REQUIRE(revertedValue <= commitedValue);
    }

    SECTION("SafeUint160_t operator>") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue > revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue > revertedValue);
    }

    SECTION("SafeUint160_t operator>=") {
      SafeUint160_t commitedValue(uint160_t(123981581));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue >= revertedValue);
      revertedValue = commitedValue * 2;
      REQUIRE(commitedValue < revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue >= commitedValue);
    }

    SECTION("SafeUint160_t operator=") {
      SafeUint160_t commitedValue(uint160_t(123981581));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();

      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue == revertedValue);
    }

    SECTION("SafeUint160_t operator+=") {
      SafeUint160_t commitedValue(uint160_t(123981581));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();
      SafeUint160_t throwValue(std::numeric_limits<uint160_t>::max());
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

    SECTION("SafeUint160_t operator=-") {
      SafeUint160_t commitedValue(uint160_t(123981581));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();
      SafeUint160_t throwValue(std::numeric_limits<uint160_t>::min());
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

    SECTION("SafeUint160_t operator=*") {
      SafeUint160_t commitedValue(uint160_t(123981581));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();
      SafeUint160_t throwValue(std::numeric_limits<uint160_t>::max());
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

    SECTION("SafeUint160_t operator=/") {
      SafeUint160_t commitedValue(uint160_t(123981581));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();
      SafeUint160_t throwValue(std::numeric_limits<uint160_t>::max());
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

    SECTION("SafeUint160_t operator%=") {
      SafeUint160_t commitedValue(uint160_t(123981581));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t(123981581));
      revertedValue.commit();
      SafeUint160_t throwValue(std::numeric_limits<uint160_t>::max());
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

    SECTION("SafeUint160_t operator &=") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();

      commitedValue &= 10000;
      revertedValue &= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint160_t(512));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t(512));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
    }

    SECTION("SafeUint64-t operator|=") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();

      commitedValue |= 10000;
      revertedValue |= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965602742"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t("146150163733090291820368483271628301965602742"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
    }

    SECTION("SafeUint160_t operator^=") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();

      commitedValue ^= 10000;
      revertedValue ^= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965602230"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint160_t("146150163733090291820368483271628301965602230"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
    }

    SECTION("SafeUint160_t operator++") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();
      SafeUint160_t throwValue(std::numeric_limits<uint160_t>::max());
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

      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593255"));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint160_t("146150163733090291820368483271628301965593255"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
    }

    SECTION("SafeUint160_t operator--") {
      SafeUint160_t commitedValue(uint160_t("146150163733090291820368483271628301965593254"));
      commitedValue.commit();
      SafeUint160_t revertedValue(uint160_t("146150163733090291820368483271628301965593254"));
      revertedValue.commit();
      SafeUint160_t throwValue(uint160_t(0));
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

      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593253"));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint160_t("146150163733090291820368483271628301965593253"));
      REQUIRE(revertedValue.get() == uint160_t("146150163733090291820368483271628301965593254"));
    }
  }
}