#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint.h"
#include "../../src/utils/utils.h"
#include <iostream>

using SafeUint256_t = SafeUint_t<256>;
namespace TSafeUint256_t {
  TEST_CASE("SafeUint256_t Class", "[contract][variables][safeuint256_t]") {
    SECTION("SafeUint256_t constructor (Commit and Revert") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
      REQUIRE(revertedValue.get() == uint256_t(0));
    }

    SECTION("SafeUint256_t operator+") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();
      SafeUint256_t throwValue(std::numeric_limits<uint256_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue + uint256_t("1927831865120318940191371489123952378115126713");
      revertedValue = revertedValue + uint256_t("1927831865120318940191371489123952378115126713");
      try {
        throwValue = throwValue + 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t("3855663730240637880382742978247904756230253426"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint256_t operator-") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();
      SafeUint256_t throwValue(uint256_t(0));
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

      REQUIRE(commitedValue.get() == uint256_t("1927831865120318940191371489123952378115116713"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint256_t operator*") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();
      SafeUint256_t throwValue(std::numeric_limits<uint256_t>::max());
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
      REQUIRE(revertedValue.get() == uint256_t("19278318651203189401913714891239523781151267130000"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t("19278318651203189401913714891239523781151267130000"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint256_t operator/") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();
      SafeUint256_t throwValue(uint256_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue / 10000;
      revertedValue = revertedValue / 10000;
      try {
        throwValue = throwValue / 0;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint256_t("192783186512031894019137148912395237811512"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t("192783186512031894019137148912395237811512"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint256_t operator%") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();
      SafeUint256_t throwValue(uint256_t(0));
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
      REQUIRE(revertedValue.get() == uint256_t(6713));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t(6713));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
      REQUIRE(overflow);
    }

    SECTION("SafeUint256_t operator&") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();

      commitedValue = commitedValue & 10000;
      revertedValue = revertedValue & 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint256_t(9488));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t(9488));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
    }

    SECTION("SafeUint256_t operator|") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();

      commitedValue = commitedValue | 10000;
      revertedValue = revertedValue | 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115127225"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t("1927831865120318940191371489123952378115127225"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
    }

    SECTION("SafeUint256_t operator^") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();

      commitedValue = commitedValue ^ 10000;
      revertedValue = revertedValue ^ 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115117737"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t("1927831865120318940191371489123952378115117737"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
    }

    SECTION("SafeUint256_t operator!") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();

      commitedValue = 0;
      revertedValue = 0;

      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(!commitedValue);
      REQUIRE(!(!revertedValue));
    }

    SECTION("SafeUint256_t operator&&") {
      SafeUint256_t trueValue1(uint256_t(1));
      SafeUint256_t trueValue2(uint256_t(5));
      SafeUint256_t falseValue1(uint256_t(0));
      SafeUint256_t falseValue2(uint256_t(0));

      bool result1 = trueValue1 && trueValue2;
      bool result2 = trueValue1 && falseValue1;
      bool result3 = falseValue1 && trueValue2;
      bool result4 = falseValue1 && falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!result2);
      REQUIRE(!result3);
      REQUIRE(!result4);
    }

    SECTION("SafeUint256_t operator||") {
      SafeUint256_t trueValue1(uint256_t(1));
      SafeUint256_t trueValue2(uint256_t(5));
      SafeUint256_t falseValue1(uint256_t(0));
      SafeUint256_t falseValue2(uint256_t(0));

      bool result1 = trueValue1 || trueValue2;
      bool result2 = trueValue1 || falseValue1;
      bool result3 = falseValue1 || trueValue2;
      bool result4 = falseValue1 || falseValue2;

      REQUIRE(!(!result1)); // Assuming your class has a method to check if the object represents true.
      REQUIRE(!(!result2));
      REQUIRE(!(!result3));
      REQUIRE(!result4);
    }

    SECTION("SafeUint256_t operator==") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));

      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint256_t operator!=") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue != revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint256_t operator<") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();

      REQUIRE(revertedValue < commitedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue < commitedValue);
    }

    SECTION("SafeUint256_t operator<=") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();

      REQUIRE(revertedValue <= commitedValue);
      revertedValue = commitedValue / 2;
      REQUIRE(!(commitedValue <= revertedValue));
      revertedValue.revert();
      REQUIRE(revertedValue <= commitedValue);
    }

    SECTION("SafeUint256_t operator>") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue > revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue > revertedValue);
    }

    SECTION("SafeUint256_t operator>=") {
      SafeUint256_t commitedValue(uint256_t(123981581));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue >= revertedValue);
      revertedValue = commitedValue * 2;
      REQUIRE(commitedValue < revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue >= commitedValue);
    }

    SECTION("SafeUint256_t operator=") {
      SafeUint256_t commitedValue(uint256_t(123981581));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();

      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue == revertedValue);
    }

    SECTION("SafeUint256_t operator+=") {
      SafeUint256_t commitedValue(uint256_t(123981581));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();
      SafeUint256_t throwValue(std::numeric_limits<uint256_t>::max());
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

    SECTION("SafeUint256_t operator=-") {
      SafeUint256_t commitedValue(uint256_t(123981581));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();
      SafeUint256_t throwValue(std::numeric_limits<uint256_t>::min());
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

    SECTION("SafeUint256_t operator=*") {
      SafeUint256_t commitedValue(uint256_t(123981581));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();
      SafeUint256_t throwValue(std::numeric_limits<uint256_t>::max());
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

    SECTION("SafeUint256_t operator=/") {
      SafeUint256_t commitedValue(uint256_t(123981581));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();
      SafeUint256_t throwValue(std::numeric_limits<uint256_t>::max());
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

    SECTION("SafeUint256_t operator%=") {
      SafeUint256_t commitedValue(uint256_t(123981581));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t(123981581));
      revertedValue.commit();
      SafeUint256_t throwValue(std::numeric_limits<uint256_t>::max());
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

    SECTION("SafeUint256_t operator &=") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();

      commitedValue &= 10000;
      revertedValue &= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint256_t(9488));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t(9488));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
    }

    SECTION("SafeUint64-t operator|=") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();

      commitedValue |= 10000;
      revertedValue |= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115127225"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t("1927831865120318940191371489123952378115127225"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
    }

    SECTION("SafeUint256_t operator^=") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();

      commitedValue ^= 10000;
      revertedValue ^= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115117737"));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint256_t("1927831865120318940191371489123952378115117737"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
    }

    SECTION("SafeUint256_t operator++") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();
      SafeUint256_t throwValue(std::numeric_limits<uint256_t>::max());
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

      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126714"));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint256_t("1927831865120318940191371489123952378115126714"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
    }

    SECTION("SafeUint256_t operator--") {
      SafeUint256_t commitedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      commitedValue.commit();
      SafeUint256_t revertedValue(uint256_t("1927831865120318940191371489123952378115126713"));
      revertedValue.commit();
      SafeUint256_t throwValue(uint256_t(0));
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

      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126712"));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint256_t("1927831865120318940191371489123952378115126712"));
      REQUIRE(revertedValue.get() == uint256_t("1927831865120318940191371489123952378115126713"));
    }
  }
}