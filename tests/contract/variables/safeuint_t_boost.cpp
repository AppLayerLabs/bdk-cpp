/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeuint.h"
#include "../../src/utils/utils.h"
#include <iostream>

// Helper to map the number of bits to the corresponding type.
template<int Size>
struct UintType;

template<> struct UintType<24> { using type = uint24_t; };
template<> struct UintType<40> { using type = uint40_t; };
template<> struct UintType<48> { using type = uint48_t; };
template<> struct UintType<56> { using type = uint56_t; };
template<> struct UintType<72> { using type = uint72_t; };
template<> struct UintType<80> { using type = uint80_t; };
template<> struct UintType<88> { using type = uint88_t; };
template<> struct UintType<96> { using type = uint96_t; };
template<> struct UintType<104> { using type = uint104_t; };
template<> struct UintType<112> { using type = uint112_t; };
template<> struct UintType<120> { using type = uint120_t; };
template<> struct UintType<128> { using type = uint128_t; };
template<> struct UintType<136> { using type = uint136_t; };
template<> struct UintType<144> { using type = uint144_t; };
template<> struct UintType<152> { using type = uint152_t; };
template<> struct UintType<160> { using type = uint160_t; };
template<> struct UintType<168> { using type = uint168_t; };
template<> struct UintType<176> { using type = uint176_t; };
template<> struct UintType<184> { using type = uint184_t; };
template<> struct UintType<192> { using type = uint192_t; };
template<> struct UintType<200> { using type = uint200_t; };
template<> struct UintType<208> { using type = uint208_t; };
template<> struct UintType<216> { using type = uint216_t; };
template<> struct UintType<224> { using type = uint224_t; };
template<> struct UintType<232> { using type = uint232_t; };
template<> struct UintType<240> { using type = uint240_t; };
template<> struct UintType<248> { using type = uint248_t; };
template<> struct UintType<256> { using type = uint256_t; };

template<int Size>
struct SafeUintTester {
    using SafeUint = SafeUint_t<Size>;
    using UnderlyingType = typename UintType<Size>::type;

    void operator()() const {
        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> constructor (Commit and Revert") {
            SafeUint commitedValue(UnderlyingType("356897"));
            SafeUint revertedValue(UnderlyingType("356897"));

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType("356897"));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator+") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
            throwValue.commit();

            bool overflow = false;

            commitedValue = commitedValue + UnderlyingType("356897");
            revertedValue = revertedValue + UnderlyingType("356897");
            try {
                throwValue = throwValue + 1;
            } catch (std::exception& e) {
                overflow = true;
            }
            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType("713794"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator-") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();
            SafeUint throwValue(UnderlyingType(0));
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

            REQUIRE(commitedValue.get() == UnderlyingType("346897"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator*") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();
            SafeUint throwValue(std::numeric_limits<UnderlyingType>::max());
            throwValue.commit();

            bool overflow = false;

            commitedValue = commitedValue * 10;
            revertedValue = revertedValue * 10;
            try {
                throwValue = throwValue * 2;
            } catch (std::exception& e) {
                overflow = true;
            }
            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType("3568970"));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType("3568970"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator/") {
            SafeUint commitedValue(UnderlyingType("3568970"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("3568970"));
            revertedValue.commit();
            SafeUint throwValue(UnderlyingType(0));
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
            REQUIRE(revertedValue.get() == UnderlyingType("356"));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType("356"));
            REQUIRE(revertedValue.get() == UnderlyingType("3568970"));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator%") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();
            SafeUint throwValue(UnderlyingType(0));
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
            REQUIRE(revertedValue.get() == UnderlyingType(6897));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(6897));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator&") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();

            commitedValue = commitedValue & 10000;
            revertedValue = revertedValue & 10000;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(8704));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(8704));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator|") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();

            commitedValue = commitedValue | 10000;
            revertedValue = revertedValue | 10000;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType("358193"));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType("358193"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator^") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();

            commitedValue = commitedValue ^ 10000;
            revertedValue = revertedValue ^ 10000;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType("349489"));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType("349489"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator!") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
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
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));

            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue != revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator!=") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12398158));
            revertedValue.commit();

            REQUIRE(commitedValue != revertedValue);
            revertedValue = commitedValue;
            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue != revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator<") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12398));
            revertedValue.commit();

            REQUIRE(revertedValue < commitedValue);
            revertedValue = commitedValue;
            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(revertedValue < commitedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator<=") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();

            REQUIRE(revertedValue <= commitedValue);
            revertedValue = commitedValue / 2;
            REQUIRE(!(commitedValue <= revertedValue));
            revertedValue.revert();
            REQUIRE(revertedValue <= commitedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator>") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12398));
            revertedValue.commit();

            REQUIRE(commitedValue > revertedValue);
            revertedValue = commitedValue;
            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue > revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator>=") {
            SafeUint commitedValue(UnderlyingType(123981));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(123981));
            revertedValue.commit();

            REQUIRE(commitedValue >= revertedValue);
            revertedValue = commitedValue * 2;
            REQUIRE(commitedValue < revertedValue);
            revertedValue.revert();
            REQUIRE(revertedValue >= commitedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator=") {
            SafeUint commitedValue(UnderlyingType(12398158));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12398158));
            revertedValue.commit();

            revertedValue = commitedValue;
            REQUIRE(commitedValue == revertedValue);
            revertedValue.revert();
            REQUIRE(commitedValue == revertedValue);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator+=") {
            SafeUint commitedValue(UnderlyingType(123981));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(123981));
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
            commitedValue += 20;
            commitedValue.commit();
            REQUIRE(commitedValue.get() == 124001);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator-=") {
            SafeUint commitedValue(UnderlyingType(12398158));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12398158));
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
            commitedValue -= 20;
            commitedValue.commit();
            REQUIRE(commitedValue.get() == 12398138);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator*=") {
            SafeUint commitedValue(UnderlyingType(1239));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(1239));
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
            revertedValue *= commitedValue;
            REQUIRE(revertedValue.get() == 1535121);
            revertedValue.revert();
            REQUIRE(revertedValue.get() == 1239);
            commitedValue *= 20;
            commitedValue.commit();
            REQUIRE(commitedValue.get() == 24780);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator/=") {
            SafeUint commitedValue(UnderlyingType(12398158));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12398158));
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
            REQUIRE(revertedValue.get() == 12398158);
            commitedValue /= 20;
            commitedValue.commit();
            REQUIRE(commitedValue.get() == 619907);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator%=") {
            SafeUint commitedValue(UnderlyingType(12398158));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType(12398158));
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
            REQUIRE(revertedValue.get() == 12398158);
            commitedValue %= 20;
            commitedValue.commit();
            REQUIRE(commitedValue.get() == 18);
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator&=") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();

            commitedValue &= 10000;
            revertedValue &= 10000;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(8704));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(8704));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator|=") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();

            commitedValue |= 10000;
            revertedValue |= 10000;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType("358193"));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType("358193"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator^=") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
            revertedValue.commit();

            commitedValue ^= 10000;
            revertedValue ^= 10000;

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType("349489"));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType("349489"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator++") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
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

            REQUIRE(revertedValue.get() == UnderlyingType("356898"));
            revertedValue.revert();
            REQUIRE(commitedValue.get() == UnderlyingType("356898"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
        }

        SECTION(std::string("SafeUint_t<") + std::to_string(Size) + "> operator--") {
            SafeUint commitedValue(UnderlyingType("356897"));
            commitedValue.commit();
            SafeUint revertedValue(UnderlyingType("356897"));
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

            REQUIRE(revertedValue.get() == UnderlyingType("356896"));
            revertedValue.revert();
            REQUIRE(commitedValue.get() == UnderlyingType("356896"));
            REQUIRE(revertedValue.get() == UnderlyingType("356897"));
        }

    }
};

TEST_CASE("SafeUint_t tests", "[contract][variables][safeuint_t_boost]") {
    SafeUintTester<24>()();
    SafeUintTester<40>()();
    SafeUintTester<48>()();
    SafeUintTester<56>()();
    SafeUintTester<72>()();
    SafeUintTester<80>()();
    SafeUintTester<88>()();
    SafeUintTester<96>()();
    SafeUintTester<104>()();
    SafeUintTester<112>()();
    SafeUintTester<120>()();
    SafeUintTester<128>()();
    SafeUintTester<136>()();
    SafeUintTester<144>()();
    SafeUintTester<152>()();
    SafeUintTester<160>()();
    SafeUintTester<168>()();
    SafeUintTester<176>()();
    SafeUintTester<184>()();
    SafeUintTester<192>()();
    SafeUintTester<200>()();
    SafeUintTester<208>()();
    SafeUintTester<216>()();
    SafeUintTester<224>()();
    SafeUintTester<232>()();
    SafeUintTester<240>()();
    SafeUintTester<248>()();
    SafeUintTester<256>()();
}
