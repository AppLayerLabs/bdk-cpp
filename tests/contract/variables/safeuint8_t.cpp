#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint8_t.h"
#include <iostream>


namespace TSafeUint8_t {
  TEST_CASE("SafeUint8_t Class", "[contracts][variables][safeuint8_t]") {
    SECTION("SafeUint8_t constructor (Commit and Revert") {
      SafeUint8_t commitedValue(uint8_t(17));
      SafeUint8_t revertedValue(uint8_t(17));

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(17));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(17));
      REQUIRE(revertedValue.get() == uint8_t(0));
    }

    SECTION("SafeUint8_t operator+") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(std::numeric_limits<uint8_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue + 5;
      revertedValue = revertedValue + 5;
      try {
        throwValue = throwValue + 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(22));
      REQUIRE(revertedValue.get() == uint8_t(17));
      REQUIRE(overflow);
    }

    SECTION("SafeUint8_t operator-") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(uint8_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue - 5;
      revertedValue = revertedValue - 5;
      try {
        throwValue = throwValue - 1;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(12));
      REQUIRE(revertedValue.get() == uint8_t(17));
      REQUIRE(overflow);
    }

    SECTION("SafeUint8_t operator*") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(std::numeric_limits<uint8_t>::max());
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue * 8;
      revertedValue = revertedValue * 8;
      try {
        throwValue = throwValue * 2;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(136));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(136));
      REQUIRE(revertedValue.get() == uint8_t(17));
      REQUIRE(overflow);
    }

    SECTION("SafeUint8_t operator/") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(uint8_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue / 3;
      revertedValue = revertedValue / 3;
      try {
        throwValue = throwValue / 2;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(5));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(5));
      REQUIRE(revertedValue.get() == uint8_t(17));
      REQUIRE(overflow);
    }

    SECTION("SafeUint8_t operator%") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(uint8_t(0));
      throwValue.commit();

      bool overflow = false;

      commitedValue = commitedValue % 23;
      revertedValue = revertedValue % 23;
      try {
        throwValue = throwValue % 2;
      } catch (std::exception& e) {
        overflow = true;
      }
      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(17));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(17));
      REQUIRE(revertedValue.get() == uint8_t(17));
      REQUIRE(overflow);
    }

    SECTION("SafeUint8_t operator&") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue = commitedValue & 23;
      revertedValue = revertedValue & 23;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(17));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(17));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator|") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue = commitedValue | 23;
      revertedValue = revertedValue | 23;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(23));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(23));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator^") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue = commitedValue ^ 23;
      revertedValue = revertedValue ^ 23;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(6));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(6));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator<<") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue = commitedValue << 1;
      revertedValue = revertedValue << 1;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(34));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(34));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator>>") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue = commitedValue >> 1;
      revertedValue = revertedValue >> 1;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(8));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(8));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator!") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue = 0;
      revertedValue = 0;

      commitedValue.commit();
      revertedValue.revert();

      REQUIRE(!commitedValue);
      REQUIRE(!(!revertedValue));
    }

    SECTION("SafeUint8_t operator&&") {
      SafeUint8_t trueValue1(uint8_t(1));
      SafeUint8_t trueValue2(uint8_t(5));
      SafeUint8_t falseValue1(uint8_t(0));
      SafeUint8_t falseValue2(uint8_t(0));

      bool result1 = trueValue1 && trueValue2;
      bool result2 = trueValue1 && falseValue1;
      bool result3 = falseValue1 && trueValue2;
      bool result4 = falseValue1 && falseValue2;

      REQUIRE(!(!result1));
      REQUIRE(!result2);
      REQUIRE(!result3);
      REQUIRE(!result4);
    }

    SECTION("SafeUint8_t operator||") {
      SafeUint8_t trueValue1(uint8_t(1));
      SafeUint8_t trueValue2(uint8_t(5));
      SafeUint8_t falseValue1(uint8_t(0));
      SafeUint8_t falseValue2(uint8_t(0));

      bool result1 = trueValue1 || trueValue2;
      bool result2 = trueValue1 || falseValue1;
      bool result3 = falseValue1 || trueValue2;
      bool result4 = falseValue1 || falseValue2;

      REQUIRE(!(!result1)); // Assuming your class has a method to check if the object represents true.
      REQUIRE(!(!result2));
      REQUIRE(!(!result3));
      REQUIRE(!result4);
    }

    SECTION("SafeUint8_t operator==") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));

      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint8_t operator!=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(12));
      revertedValue.commit();

      REQUIRE(commitedValue != revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue != revertedValue);
    }

    SECTION("SafeUint8_t operator<") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(12));
      revertedValue.commit();

      REQUIRE(revertedValue < commitedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue < commitedValue);
    }
    
    SECTION("SafeUint8_t operator>") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(12));
      revertedValue.commit();

      REQUIRE(commitedValue > revertedValue);
      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue > revertedValue);
    }

    SECTION("SafeUint8_t operator>=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      REQUIRE(commitedValue >= revertedValue);
      revertedValue = commitedValue * 2;
      REQUIRE(commitedValue < revertedValue);
      revertedValue.revert();
      REQUIRE(revertedValue >= commitedValue);
    }

    SECTION("SafeUint8_t operator=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      revertedValue = commitedValue;
      REQUIRE(commitedValue == revertedValue);
      revertedValue.revert();
      REQUIRE(commitedValue == revertedValue);
    }

    SECTION("SafeUint8_t operator+=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(std::numeric_limits<uint8_t>::max());
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
      commitedValue += 7;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 24);
    }

    SECTION("SafeUint8_t operator=-") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(std::numeric_limits<uint8_t>::min());
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
      commitedValue -= 10;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 7);
    }

    SECTION("SafeUint8_t operator=*") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(std::numeric_limits<uint8_t>::max());
      throwValue.commit();

      bool overflow = false;

      try {
        throwValue *= commitedValue;
      } catch (const std::exception& e) {
        overflow = true;
      }

      REQUIRE(overflow);
      revertedValue *= 7;
      REQUIRE(revertedValue.get() == 119);
      revertedValue.revert();
      REQUIRE(revertedValue.get() == 17);
      commitedValue *= 3;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 51);
    }

    SECTION("SafeUint8_t operator=/") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(std::numeric_limits<uint8_t>::max());
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
      REQUIRE(revertedValue.get() == 17);
      commitedValue /= 3;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 5);
    }

    SECTION("SafeUint8_t operator%=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(std::numeric_limits<uint8_t>::max());
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
      REQUIRE(revertedValue.get() == 17);
      commitedValue %= 3;
      commitedValue.commit();
      REQUIRE(commitedValue.get() == 2);
    }

    SECTION("SafeUint8_t operator &=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue &= 23;
      revertedValue &= 23;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(17));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(17));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint64-t operator|=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue |= 23;
      revertedValue |= 23;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(23));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(23));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator^=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue ^= 23;
      revertedValue ^= 23;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(6));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(6));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator<<=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue <<= 1;
      revertedValue <<= 1;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(34));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(34));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator>>=") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();

      commitedValue >>= 1;
      revertedValue >>= 1;

      commitedValue.commit();
      REQUIRE(revertedValue.get() == uint8_t(8));
      revertedValue.revert();

      REQUIRE(commitedValue.get() == uint8_t(8));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator++") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(std::numeric_limits<uint8_t>::max());
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

      REQUIRE(revertedValue.get() == uint8_t(18));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint8_t(18));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }

    SECTION("SafeUint8_t operator--") {
      SafeUint8_t commitedValue(uint8_t(17));
      commitedValue.commit();
      SafeUint8_t revertedValue(uint8_t(17));
      revertedValue.commit();
      SafeUint8_t throwValue(uint8_t(0));
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

      REQUIRE(revertedValue.get() == uint8_t(16));
      revertedValue.revert();
      REQUIRE(commitedValue.get() == uint8_t(16));
      REQUIRE(revertedValue.get() == uint8_t(17));
    }
  }
}