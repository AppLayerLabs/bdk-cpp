#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint.h"
#include <iostream>

using SafeUint16_t = SafeUint_t<16>;
namespace TSafeUint16_t {
  TEST_CASE("SafeUint16_t Class", "[contract][variables][safeuint16_t]") {
    SECTION("SafeUint16_t constructor (Commit and Revert") {
      SafeUint_t<16> commitedValue(uint16_t(2847));
      SafeUint16_t revertedValue(uint16_t(2847));

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(2847));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(2847));
      REQUIRE(revertedValue.get() == uint16_t(0));
    }

    SECTION("SafeUint16_t operator+") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(std::numeric_limits<uint16_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue + 1000;
      revertedValue = revertedValue + 1000;
      try {
        throwValue = throwValue + 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(3847));
      REQUIRE(revertedValue.get() == uint16_t(2847));
      REQUIRE(overflow);
    }

    SECTION("SafeUint16_t operator-") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(uint16_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue - 1000;
      revertedValue = revertedValue - 1000;
      try {
        throwValue = throwValue - 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(1847));
      REQUIRE(revertedValue.get() == uint16_t(2847));
      REQUIRE(overflow);
    }

    SECTION("SafeUint16_t operator*") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(std::numeric_limits<uint16_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue * 7;
      revertedValue = revertedValue * 7;
      try {
        throwValue = throwValue * 2;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(19929));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(19929));
      REQUIRE(revertedValue.get() == uint16_t(2847));
      REQUIRE(overflow);
    }

    SECTION("SafeUint16_t operator/") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(uint16_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue / 1000;
      revertedValue = revertedValue / 1000;
      try {
        throwValue = throwValue / 0;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(2));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(2));
      REQUIRE(revertedValue.get() == uint16_t(2847));
      REQUIRE(overflow);
    }

    SECTION("SafeUint16_t operator%") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(uint16_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue % 1000;
      revertedValue = revertedValue % 1000;
      try {
        throwValue = throwValue % 2;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(847));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(847));
      REQUIRE(revertedValue.get() == uint16_t(2847));
      REQUIRE(overflow);
    }

    SECTION("SafeUint16_t operator&") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue = commitedValue & 1000;
      revertedValue = revertedValue & 1000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(776));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(776));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator|") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue = commitedValue | 1000;
      revertedValue = revertedValue | 1000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(3071));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(3071));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator^") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue = commitedValue ^ 1000;
      revertedValue = revertedValue ^ 1000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(2295));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(2295));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator<<") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue = commitedValue << 4;
      revertedValue = revertedValue << 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(45552));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(45552));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator>>") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue = commitedValue >> 4;
      revertedValue = revertedValue >> 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(177));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(177));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator!") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue = 0;
      revertedValue = 0;

      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(!commitedValue);
      REQUIRE(!(!revertedValue));
    }

    SECTION("SafeUint16_t operator&&") {
      SafeUint16_t trueValue1(uint16_t(1));
      SafeUint16_t trueValue2(uint16_t(5));
      SafeUint16_t falseValue1(uint16_t(0));
      SafeUint16_t falseValue2(uint16_t(0));

      bool result1 = trueValue1 && trueValue2;
      bool result2 = trueValue1 && falseValue1;
      bool result3 = falseValue1 && trueValue2;
      bool result4 = falseValue1 && falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!result2);
      REQUIRE(!result3);
      REQUIRE(!result4);
    }

    SECTION("SafeUint16_t operator||") {
      SafeUint16_t trueValue1(uint16_t(1));
      SafeUint16_t trueValue2(uint16_t(5));
      SafeUint16_t falseValue1(uint16_t(0));
      SafeUint16_t falseValue2(uint16_t(0));

      bool result1 = trueValue1 || trueValue2;
      bool result2 = trueValue1 || falseValue1;
      bool result3 = falseValue1 || trueValue2;
      bool result4 = falseValue1 || falseValue2;

      REQUIRE(!(!result1)); // Assuming your class has a method to check if the object represents true.
      REQUIRE(!(!result2));
      REQUIRE(!(!result3));
      REQUIRE(!result4);
    }

    SECTION("SafeUint16_t operator==") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));

      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint16_t operator!=") {
      SafeUint16_t commitedValue(uint16_t(1234));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      REQUIRE(commitedValue != revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint16_t operator<") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(1234));
      revertedValue.commit();

      REQUIRE(revertedValue < commitedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue < commitedValue);
    }

    SECTION("SafeUint16_t operator<=") {
      SafeUint16_t commitedValue(uint32_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint32_t(2847));
      revertedValue.commit();

      REQUIRE(revertedValue <= commitedValue);
      revertedValue = commitedValue / 2;
      REQUIRE(!(commitedValue <= revertedValue));
      REQUIRE(!(commitedValue <= revertedValue));
      revertedValue.revert();
      REQUIRE(revertedValue <= commitedValue);
    }
    
    SECTION("SafeUint16_t operator>") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(1234));
      revertedValue.commit();

      REQUIRE(commitedValue > revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue > revertedValue);
    }

    SECTION("SafeUint16_t operator>=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      REQUIRE(commitedValue >= revertedValue);
      revertedValue = commitedValue * 2;
      REQUIRE(commitedValue < revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue >= commitedValue);
    }

    SECTION("SafeUint16_t operator=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue == revertedValue);
    }

    SECTION("SafeUint16_t operator+=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(std::numeric_limits<uint16_t>::max());
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
      REQUIRE(commitedValue.get() == 2867);
    }

    SECTION("SafeUint16_t operator=-") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(std::numeric_limits<uint16_t>::min());
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
      REQUIRE(commitedValue.get() == 2827);
    }

    SECTION("SafeUint16_t operator=*") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(std::numeric_limits<uint16_t>::max());
      throwValue.commit();

      bool overflow = false;

      try {
        throwValue *= commitedValue;
      } catch (const std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      revertedValue *= 7;
      REQUIRE(revertedValue.get() == 19929);
      revertedValue.revert();
      REQUIRE(revertedValue.get() == 2847);
      commitedValue *= 7;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 19929);
    }

    SECTION("SafeUint16_t operator=/") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(std::numeric_limits<uint16_t>::max());
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
      REQUIRE(revertedValue.get() == 2847);
      commitedValue /= 20;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 142);
    }

    SECTION("SafeUint16_t operator%=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(std::numeric_limits<uint16_t>::max());
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
      REQUIRE(revertedValue.get() == 2847);
      commitedValue %= 20;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 7);
    }

    SECTION("SafeUint16_t operator &=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue &= 1000;
      revertedValue &= 1000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(776));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(776));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint64-t operator|=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue |= 1000;
      revertedValue |= 1000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(3071));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(3071));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator^=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue ^= 1000;
      revertedValue ^= 1000;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(2295));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(2295));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator<<=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue <<= 4;
      revertedValue <<= 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(45552));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(45552));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator>>=") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();

      commitedValue >>= 4;
      revertedValue >>= 4;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint16_t(177));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint16_t(177));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator++") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(std::numeric_limits<uint16_t>::max());
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

      REQUIRE(revertedValue.get() == uint16_t(2848));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint16_t(2848));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }

    SECTION("SafeUint16_t operator--") {
      SafeUint16_t commitedValue(uint16_t(2847));
      commitedValue.commit();
      SafeUint16_t revertedValue(uint16_t(2847));
      revertedValue.commit();
      SafeUint16_t throwValue(uint16_t(0));
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

      REQUIRE(revertedValue.get() == uint16_t(2846));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint16_t(2846));
      REQUIRE(revertedValue.get() == uint16_t(2847));
    }
  }
}