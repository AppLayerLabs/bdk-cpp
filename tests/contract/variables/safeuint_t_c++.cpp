#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint.h"
#include <iostream>

template <int Size>
struct UnderlyingType;

template <>
struct UnderlyingType<8> { using type = uint8_t; };

template <>
struct UnderlyingType<16> { using type = uint16_t; };

template <>
struct UnderlyingType<32> { using type = uint32_t; };

template <>
struct UnderlyingType<64> { using type = uint64_t; };

template <int Size>
struct SafeUintTester {
    using SafeUint = SafeUint_t<Size>;
    using UnderlyingType = typename UnderlyingType<Size>::type;

    void operator()() const {
        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> constructor (Commit and Revert") {
            SafeUint commitedValue(UnderlyingType(17));
            SafeUint revertedValue(UnderlyingType(17));

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(17));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(17));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator+") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
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

            REQUIRE(commitedValue.get() == UnderlyingType(22));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
            REQUIRE(overflow);
        }
        
        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator-") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(UnderlyingType(0));
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

            REQUIRE(commitedValue.get() == UnderlyingType(12));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator*") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
            throwValue.commit();

            bool overflow = false;

            commitedValue = commitedValue * 5;
            revertedValue = revertedValue * 5;
            try {
                throwValue = throwValue * 2;
            } catch (std::exception& e) {
                overflow = true;
            }
            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(85));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator/") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(UnderlyingType(0));
            throwValue.commit();

            bool overflow = false;

            commitedValue = commitedValue / 5;
            revertedValue = revertedValue / 5;
            try {
                throwValue = throwValue / 0;
            } catch (std::exception& e) {
                overflow = true;
            }
            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(3));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator%") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(UnderlyingType(0));
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
            REQUIRE(commitedValue.get() == UnderlyingType(17));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(17));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator&") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue = commitedValue & 23;
            revertedValue = revertedValue & 23;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(17));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(17));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator|") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue = commitedValue | 23;
            revertedValue = revertedValue | 23;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(23));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(23));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator^") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue = commitedValue ^ 23;
            revertedValue = revertedValue ^ 23;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(6));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(6));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator<<") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue = commitedValue << 1;
            revertedValue = revertedValue << 1;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(34));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(34));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator>>") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue = commitedValue >> 1;
            revertedValue = revertedValue >> 1;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(8));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(8));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator!") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue = 0;
            revertedValue = 0;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(!commitedValue);
            REQUIRE(!(!revertedValue));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator&&") {
            SafeUint trueValue1(UnderlyingType(1));
            SafeUint trueValue2(UnderlyingType(5));
            SafeUint falseValue1(UnderlyingType(0));
            SafeUint falseValue2(UnderlyingType(0));

            bool result1 = trueValue1 && trueValue2;
            bool result2 = trueValue1 && falseValue1;
            bool result3 = falseValue1 && trueValue2;
            bool result4 = falseValue1 && falseValue2;

            REQUIRE(!(!result1));
            REQUIRE(!result2);
            REQUIRE(!result3);
            REQUIRE(!result4);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator||") {
            SafeUint trueValue1(UnderlyingType(1));
            SafeUint trueValue2(UnderlyingType(5));
            SafeUint falseValue1(UnderlyingType(0));
            SafeUint falseValue2(UnderlyingType(0));

            bool result1 = trueValue1 || trueValue2;
            bool result2 = trueValue1 || falseValue1;
            bool result3 = falseValue1 || trueValue2;
            bool result4 = falseValue1 || falseValue2;

            REQUIRE(!(!result1));
            REQUIRE(!(!result2));
            REQUIRE(!(!result3));
            REQUIRE(!result4);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator==") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));

            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue != revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator!=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12));
            revertedValue.commit();

            REQUIRE(commitedValue != revertedValue);
            revertedValue = commitedValue;
            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue != revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator<") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12));
            revertedValue.commit();

            REQUIRE(revertedValue < commitedValue);
            revertedValue = commitedValue;
            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(revertedValue < commitedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator>") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12));
            revertedValue.commit();

            REQUIRE(commitedValue > revertedValue);
            revertedValue = commitedValue;
            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue > revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator>=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            REQUIRE(commitedValue >= revertedValue);
            revertedValue = commitedValue * 2;
            REQUIRE(commitedValue < revertedValue);
            revertedValue.revert();
            REQUIRE(revertedValue >= commitedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator<=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            REQUIRE(commitedValue <= revertedValue);
            revertedValue = commitedValue * 2;
            REQUIRE(commitedValue < revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue <= revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            revertedValue = commitedValue;
            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue == revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator+=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
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

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator-=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::min());
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

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator*=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
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

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator/=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
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

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator%=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
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

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator&=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue &= 23;
            revertedValue &= 23;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(17));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(17));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator|=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue |= 23;
            revertedValue |= 23;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(23));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(23));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator^=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue ^= 23;
            revertedValue ^= 23;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(6));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(6));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator<<=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue <<= 1;
            revertedValue <<= 1;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(34));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(34));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator>>=") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();

            commitedValue >>= 1;
            revertedValue >>= 1;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(8));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(8));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator++") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
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

            REQUIRE(revertedValue.get() == UnderlyingType(18));
            revertedValue.revert();
            REQUIRE(commitedValue.get() == UnderlyingType(18));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator--") {
            SafeUint commitedValue(UnderlyingType(17));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(17));
            revertedValue.commit();
            SafeUint throwValue(UnderlyingType(0));
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

            REQUIRE(revertedValue.get() == UnderlyingType(16));
            revertedValue.revert();
            REQUIRE(commitedValue.get() == UnderlyingType(16));
            REQUIRE(revertedValue.get() == UnderlyingType(17));
        }

    }
};

TEST_CASE("SafeUint_t tests", "[contract][variables][safeuint_t]") {
    SafeUintTester<8>()();
    SafeUintTester<16>()();
    SafeUintTester<32>()();
    SafeUintTester<64>()();
}
