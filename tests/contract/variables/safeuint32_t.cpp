#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint.h"
#include <iostream>

using SafeUint32_t = SafeUint_t<32>;
namespace TSafeUint32_t {
  TEST_CASE("SafeUint32_t Class", "[contract][variables][safeuint32_t]") {
    SECTION("SafeUint32_t constructor (Commit and Revert") {
      SafeUint32_t commitedValue(uint32_t(429496));
      SafeUint32_t revertedValue(uint32_t(429496));

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(429496));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(429496));
      REQUIRE(revertedValue.get() == uint32_t(0));
    }

    SECTION("SafeUint32_t operator+") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();
      SafeUint32_t throwValue(std::numeric_limits<uint32_t>::max());
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

      REQUIRE(commitedValue.get() == uint32_t(439496));
      REQUIRE(revertedValue.get() == uint32_t(429496));
      REQUIRE(overflow);
    }

    SECTION("SafeUint32_t operator-") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();
      SafeUint32_t throwValue(uint32_t(0));
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

      REQUIRE(commitedValue.get() == uint32_t(419496));
      REQUIRE(revertedValue.get() == uint32_t(429496));
      REQUIRE(overflow);
    }

    SECTION("SafeUint32_t operator*") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();
      SafeUint32_t throwValue(std::numeric_limits<uint32_t>::max());
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
      REQUIRE(revertedValue.get() == uint32_t(4294960000));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(4294960000));
      REQUIRE(revertedValue.get() == uint32_t(429496));
      REQUIRE(overflow);
    }

    SECTION("SafeUint32_t operator/") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();
      SafeUint32_t throwValue(uint32_t(0));
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
      REQUIRE(revertedValue.get() == uint32_t(42));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(42));
      REQUIRE(revertedValue.get() == uint32_t(429496));
      REQUIRE(overflow);
    }

    SECTION("SafeUint32_t operator%") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();
      SafeUint32_t throwValue(uint32_t(0));
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
      REQUIRE(revertedValue.get() == uint32_t(9496));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(9496));
      REQUIRE(revertedValue.get() == uint32_t(429496));
      REQUIRE(overflow);
    }

    SECTION("SafeUint32_t operator&") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue = commitedValue & 10000;
      revertedValue = revertedValue & 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(1296));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(1296));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator|") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue = commitedValue | 10000;
      revertedValue = revertedValue | 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(438200));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(438200));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator^") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue = commitedValue ^ 10000;
      revertedValue = revertedValue ^ 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(436904));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(436904));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator<<") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue = commitedValue << 4;
      revertedValue = revertedValue << 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(6871936));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(6871936));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator>>") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue = commitedValue >> 4;
      revertedValue = revertedValue >> 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(26843));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(26843));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator!") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue = 0;
      revertedValue = 0;

      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(!commitedValue);
      REQUIRE(!(!revertedValue));
    }

    SECTION("SafeUint32_t operator&&") {
      SafeUint32_t trueValue1(uint32_t(1));
      SafeUint32_t trueValue2(uint32_t(5));
      SafeUint32_t falseValue1(uint32_t(0));
      SafeUint32_t falseValue2(uint32_t(0));

      bool result1 = trueValue1 && trueValue2;
      bool result2 = trueValue1 && falseValue1;
      bool result3 = falseValue1 && trueValue2;
      bool result4 = falseValue1 && falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!result2);
      REQUIRE(!result3);
      REQUIRE(!result4);
    }

    SECTION("SafeUint32_t operator||") {
      SafeUint32_t trueValue1(uint32_t(1));
      SafeUint32_t trueValue2(uint32_t(5));
      SafeUint32_t falseValue1(uint32_t(0));
      SafeUint32_t falseValue2(uint32_t(0));

      bool result1 = trueValue1 || trueValue2;
      bool result2 = trueValue1 || falseValue1;
      bool result3 = falseValue1 || trueValue2;
      bool result4 = falseValue1 || falseValue2;

      REQUIRE(!(!result1)); // Assuming your class has a method to check if the object represents true.
      REQUIRE(!(!result2));
      REQUIRE(!(!result3));
      REQUIRE(!result4);
    }

    SECTION("SafeUint32_t operator==") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));

      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint32_t operator!=") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue != revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint32_t operator<") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(319384));
      revertedValue.commit();

      REQUIRE(revertedValue < commitedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue < commitedValue);
    }

    SECTION("SafeUint32_t operator<=") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      REQUIRE(revertedValue <= commitedValue);
      revertedValue = commitedValue / 2;
      REQUIRE(!(commitedValue <= revertedValue));
      REQUIRE(!(commitedValue <= revertedValue));
      revertedValue.revert();
      REQUIRE(revertedValue <= commitedValue);
    }
    
    SECTION("SafeUint32_t operator>") {
      SafeUint32_t commitedValue(uint32_t(1239881));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      REQUIRE(commitedValue > revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue > revertedValue);
    }

    SECTION("SafeUint32_t operator>=") {
      SafeUint32_t commitedValue(uint32_t(123981581));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(123981581));
      revertedValue.commit();

      REQUIRE(commitedValue >= revertedValue);
      revertedValue = commitedValue * 2;
      REQUIRE(commitedValue < revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue >= commitedValue);
    }

    SECTION("SafeUint32_t operator=") {
      SafeUint32_t commitedValue(uint32_t(123981581));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(123981581));
      revertedValue.commit();

      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue == revertedValue);
    }

    SECTION("SafeUint32_t operator+=") {
      SafeUint32_t commitedValue(uint32_t(123981581));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(123981581));
      revertedValue.commit();
      SafeUint32_t throwValue(std::numeric_limits<uint32_t>::max());
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

    SECTION("SafeUint32_t operator=-") {
      SafeUint32_t commitedValue(uint32_t(123981581));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(123981581));
      revertedValue.commit();
      SafeUint32_t throwValue(std::numeric_limits<uint32_t>::min());
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

    SECTION("SafeUint32_t operator=*") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();
      SafeUint32_t throwValue(std::numeric_limits<uint32_t>::max());
      throwValue.commit();

      bool overflow = false;

      try {
        throwValue *= commitedValue;
      } catch (const std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      revertedValue *= 100;
      REQUIRE(revertedValue.get() == 42949600);
      revertedValue.revert();
      REQUIRE(revertedValue.get() == 429496);
      commitedValue *= 20;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 8589920);
    }

    SECTION("SafeUint32_t operator=/") {
      SafeUint32_t commitedValue(uint32_t(123981581));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(123981581));
      revertedValue.commit();
      SafeUint32_t throwValue(std::numeric_limits<uint32_t>::max());
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

    SECTION("SafeUint32_t operator%=") {
      SafeUint32_t commitedValue(uint32_t(123981581));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(123981581));
      revertedValue.commit();
      SafeUint32_t throwValue(std::numeric_limits<uint32_t>::max());
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

    SECTION("SafeUint32_t operator &=") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue &= 10000;
      revertedValue &= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(1296));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(1296));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint64-t operator|=") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue |= 10000;
      revertedValue |= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(438200));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(438200));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator^=") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue ^= 10000;
      revertedValue ^= 10000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(436904));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(436904));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator<<=") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue <<= 4;
      revertedValue <<= 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(6871936));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(6871936));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator>>=") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();

      commitedValue >>= 4;
      revertedValue >>= 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint32_t(26843));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint32_t(26843));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator++") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();
      SafeUint32_t throwValue(std::numeric_limits<uint32_t>::max());
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

      REQUIRE(revertedValue.get() == uint32_t(429497));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint32_t(429497));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }

    SECTION("SafeUint32_t operator--") {
      SafeUint32_t commitedValue(uint32_t(429496));
      commitedValue.commit();
      SafeUint32_t revertedValue(uint32_t(429496));
      revertedValue.commit();
      SafeUint32_t throwValue(uint32_t(0));
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

      REQUIRE(revertedValue.get() == uint32_t(429495));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint32_t(429495));
      REQUIRE(revertedValue.get() == uint32_t(429496));
    }
  }
}