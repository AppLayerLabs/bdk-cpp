#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint.h"
#include <iostream>

using SafeUint64_t = SafeUint_t<64>;

namespace TSafeUint64_t {
  TEST_CASE("SafeUint64_t Class", "[contract][variables][safeuint64_t]") {
    SECTION("SafeUint64_t constructor (Commit and Revert") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      SafeUint64_t revertedValue(uint64_t(192381851023));

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(192381851023));
      REQUIRE(revertedValue.get() == uint64_t(0));
    }

    SECTION("SafeUint64_t operator+") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();
      SafeUint64_t throwValue(std::numeric_limits<uint64_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue + 10000;
      revertedValue = revertedValue + 10000;
      try {
        throwValue = throwValue + 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(192381861023));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
      REQUIRE(overflow);
    }

    SECTION("SafeUint64_t operator-") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();
      SafeUint64_t throwValue(uint64_t(0));
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

      REQUIRE(commitedValue.get() == uint64_t(192381841023));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
      REQUIRE(overflow);
    }

    SECTION("SafeUint64_t operator*") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();
      SafeUint64_t throwValue(std::numeric_limits<uint64_t>::max());
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
      REQUIRE(revertedValue.get() == uint64_t(1923818510230000));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(1923818510230000));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
      REQUIRE(overflow);
    }

    SECTION("SafeUint64_t operator/") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();
      SafeUint64_t throwValue(uint64_t(0));
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
      REQUIRE(revertedValue.get() == uint64_t(19238185));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(19238185));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
      REQUIRE(overflow);
    }

    SECTION("SafeUint64_t operator%") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();
      SafeUint64_t throwValue(uint64_t(0));
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
      REQUIRE(revertedValue.get() == uint64_t(1023));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(1023));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
      REQUIRE(overflow);
    }

    SECTION("SafeUint64_t operator&") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue = commitedValue & 10000;
      revertedValue = revertedValue & 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(1280));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(1280));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator|") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue = commitedValue | 10000;
      revertedValue = revertedValue | 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(192381859743));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(192381859743));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator^") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue = commitedValue ^ 10000;
      revertedValue = revertedValue ^ 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(192381858463));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(192381858463));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator<<") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue = commitedValue << 4;
      revertedValue = revertedValue << 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(3078109616368));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(3078109616368));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator>>") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue = commitedValue >> 4;
      revertedValue = revertedValue >> 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(12023865688));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(12023865688));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator!") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue = 0;
      revertedValue = 0;

      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(!commitedValue);
      REQUIRE(!(!revertedValue));
    }

    SECTION("SafeUint64_t operator&&") {
      SafeUint64_t trueValue1(uint64_t(1));
      SafeUint64_t trueValue2(uint64_t(5));
      SafeUint64_t falseValue1(uint64_t(0));
      SafeUint64_t falseValue2(uint64_t(0));

      bool result1 = trueValue1 && trueValue2;
      bool result2 = trueValue1 && falseValue1;
      bool result3 = falseValue1 && trueValue2;
      bool result4 = falseValue1 && falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!result2);
      REQUIRE(!result3);
      REQUIRE(!result4);
    }

    SECTION("SafeUint64_t operator||") {
      SafeUint64_t trueValue1(uint64_t(1));
      SafeUint64_t trueValue2(uint64_t(5));
      SafeUint64_t falseValue1(uint64_t(0));
      SafeUint64_t falseValue2(uint64_t(0));

      bool result1 = trueValue1 || trueValue2;
      bool result2 = trueValue1 || falseValue1;
      bool result3 = falseValue1 || trueValue2;
      bool result4 = falseValue1 || falseValue2;

      REQUIRE(!(!result1)); // Assuming your class has a method to check if the object represents true.
      REQUIRE(!(!result2));
      REQUIRE(!(!result3));
      REQUIRE(!result4);
    }

    SECTION("SafeUint64_t operator==") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));

      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint64_t operator!=") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue != revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint64_t operator<") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();

      REQUIRE(revertedValue < commitedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue < commitedValue);
    }

    SECTION("SafeUint64_t operator<=") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      REQUIRE(revertedValue <= commitedValue);
      revertedValue = commitedValue / 2;
      REQUIRE(!(commitedValue <= revertedValue));
      revertedValue.revert();
      REQUIRE(revertedValue <= commitedValue);
    }
    
    SECTION("SafeUint64_t operator>") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue > revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue > revertedValue);
    }

    SECTION("SafeUint64_t operator>=") {
      SafeUint64_t commitedValue(uint64_t(123981581));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue >= revertedValue);
      revertedValue = commitedValue * 2;
      REQUIRE(commitedValue < revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue >= commitedValue);
    }

    SECTION("SafeUint64_t operator=") {
      SafeUint64_t commitedValue(uint64_t(123981581));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();

      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue == revertedValue);
    }

    SECTION("SafeUint64_t operator+=") {
      SafeUint64_t commitedValue(uint64_t(123981581));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();
      SafeUint64_t throwValue(std::numeric_limits<uint64_t>::max());
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

    SECTION("SafeUint64_t operator=-") {
      SafeUint64_t commitedValue(uint64_t(123981581));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();
      SafeUint64_t throwValue(std::numeric_limits<uint64_t>::min());
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

    SECTION("SafeUint64_t operator=*") {
      SafeUint64_t commitedValue(uint64_t(123981581));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();
      SafeUint64_t throwValue(std::numeric_limits<uint64_t>::max());
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

    SECTION("SafeUint64_t operator=/") {
      SafeUint64_t commitedValue(uint64_t(123981581));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();
      SafeUint64_t throwValue(std::numeric_limits<uint64_t>::max());
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

    SECTION("SafeUint64_t operator%=") {
      SafeUint64_t commitedValue(uint64_t(123981581));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(123981581));
      revertedValue.commit();
      SafeUint64_t throwValue(std::numeric_limits<uint64_t>::max());
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

    SECTION("SafeUint64_t operator &=") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue &= 10000;
      revertedValue &= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(1280));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(1280));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64-t operator|=") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue |= 10000;
      revertedValue |= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(192381859743));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(192381859743));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator^=") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue ^= 10000;
      revertedValue ^= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(192381858463));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(192381858463));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator<<=") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue <<= 4;
      revertedValue <<= 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(3078109616368));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(3078109616368));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator>>=") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();

      commitedValue >>= 4;
      revertedValue >>= 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint64_t(12023865688));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint64_t(12023865688));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator++") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();
      SafeUint64_t throwValue(std::numeric_limits<uint64_t>::max());
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

      REQUIRE(revertedValue.get() == uint64_t(192381851024));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint64_t(192381851024));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }

    SECTION("SafeUint64_t operator--") {
      SafeUint64_t commitedValue(uint64_t(192381851023));
      commitedValue.commit();
      SafeUint64_t revertedValue(uint64_t(192381851023));
      revertedValue.commit();
      SafeUint64_t throwValue(uint64_t(0));
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

      REQUIRE(revertedValue.get() == uint64_t(192381851022));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint64_t(192381851022));
      REQUIRE(revertedValue.get() == uint64_t(192381851023));
    }
  }
}