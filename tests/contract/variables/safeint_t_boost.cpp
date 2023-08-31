/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeint.h"
#include "../../src/utils/utils.h"
#include <iostream>

// Helper to map the number of bits to the corresponding type.
template<int Size>
struct IntType;

template<> struct IntType<24> { using type = int24_t; };
template<> struct IntType<40> { using type = int40_t; };
template<> struct IntType<48> { using type = int48_t; };
template<> struct IntType<56> { using type = int56_t; };
template<> struct IntType<72> { using type = int72_t; };
template<> struct IntType<80> { using type = int80_t; };
template<> struct IntType<88> { using type = int88_t; };
template<> struct IntType<96> { using type = int96_t; };
template<> struct IntType<104> { using type = int104_t; };
template<> struct IntType<112> { using type = int112_t; };
template<> struct IntType<120> { using type = int120_t; };
template<> struct IntType<128> { using type = int128_t; };
template<> struct IntType<136> { using type = int136_t; };
template<> struct IntType<144> { using type = int144_t; };
template<> struct IntType<152> { using type = int152_t; };
template<> struct IntType<160> { using type = int160_t; };
template<> struct IntType<168> { using type = int168_t; };
template<> struct IntType<176> { using type = int176_t; };
template<> struct IntType<184> { using type = int184_t; };
template<> struct IntType<192> { using type = int192_t; };
template<> struct IntType<200> { using type = int200_t; };
template<> struct IntType<208> { using type = int208_t; };
template<> struct IntType<216> { using type = int216_t; };
template<> struct IntType<224> { using type = int224_t; };
template<> struct IntType<232> { using type = int232_t; };
template<> struct IntType<240> { using type = int240_t; };
template<> struct IntType<248> { using type = int248_t; };
template<> struct IntType<256> { using type = int256_t; };

template <int Size>
struct SafeIntTester {
    using SafeInt = SafeInt_t<Size>;
    using UnderlyingType = typename IntType<Size>::type;

    void operator()() const {
        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> constructor (Commit and Revert") {
            SafeInt commitedValue(UnderlyingType(-356897));
            SafeInt revertedValue(UnderlyingType(-356897));

            commitedValue.commit();
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-356897));
            REQUIRE(revertedValue.get() == UnderlyingType(0));

        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator+") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue = commitedValue + 2257;
            revertedValue = revertedValue + 2257;

            try {
                throwValueOverflow = throwValueOverflow + 1;
            } catch (std::overflow_error &e) {
                overflow = true;
            }
            try {
                throwValueUnderflow = throwValueUnderflow - 1;
            } catch (std::underflow_error &e) {
                underflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-354640));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator-") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue = commitedValue - 2257;
            revertedValue = revertedValue - 2257;

            try {
                throwValueOverflow = throwValueOverflow + 1;
            } catch (std::overflow_error &e) {
                overflow = true;
            }
            try {
                throwValueUnderflow = throwValueUnderflow - 1;
            } catch (std::underflow_error &e) {
                underflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-359154));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator*") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue = commitedValue * 2;
            revertedValue = revertedValue * 2;

            try {
                throwValueOverflow = throwValueOverflow * 2;
            } catch (std::overflow_error &e) {
                overflow = true;
            }
            try {
                throwValueUnderflow = throwValueUnderflow * 2;
            } catch (std::underflow_error &e) {
                underflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-713794));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator/") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::min());
            throwValueOverflow.commit();

            bool overflow = false;
            bool domain_error = false;

            commitedValue = commitedValue / 2;
            revertedValue = revertedValue / 2;

            try {
                throwValueOverflow = throwValueOverflow / UnderlyingType(0);
            } catch (std::domain_error &e) {
                domain_error = true;
            }
            try {
                throwValueOverflow = throwValueOverflow / -1;
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-178448));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(overflow);
            REQUIRE(domain_error);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator%") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();

            bool domain_error = false;

            commitedValue = commitedValue % 2;
            revertedValue = revertedValue % 2;

            try {
                commitedValue = commitedValue % UnderlyingType(0);
            } catch (std::domain_error &e) {
                domain_error = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-1));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(domain_error);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator&") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue & 0x0000FFFF;
            revertedValue = revertedValue & 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(29217));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator|") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue | 0x0000FFFF;
            revertedValue = revertedValue | 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(393215));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator^") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue ^ 0x0000FFFF;
            revertedValue = revertedValue ^ 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(363998));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator!") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = !commitedValue;
            revertedValue = !revertedValue;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator&&") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue && 0x0000FFFF;
            revertedValue = revertedValue && 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator||") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue || 0x0000FFFF;
            revertedValue = revertedValue || 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator==") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue == 0x0000FFFF;
            revertedValue = revertedValue == 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator!=") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue != 0x0000FFFF;
            revertedValue = revertedValue != 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator>") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue > 0x0000FFFF;
            revertedValue = revertedValue > 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator<") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue = commitedValue < 0x0000FFFF;
            revertedValue = revertedValue < 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator>=") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();
            SafeInt commitedValue2(UnderlyingType(0x0000FFFF));
            commitedValue2.commit();
            SafeInt revertedValue2(UnderlyingType(0x0000FFFF));
            revertedValue2.commit();

            commitedValue = commitedValue >= commitedValue2;
            revertedValue = revertedValue >= revertedValue2;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator<=") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();
            SafeInt commitedValue2(UnderlyingType(0x0000FFFF));
            commitedValue2.commit();
            SafeInt revertedValue2(UnderlyingType(0x0000FFFF));
            revertedValue2.commit();

            commitedValue = commitedValue <= commitedValue2;
            revertedValue = revertedValue <= revertedValue2;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator=") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();

            commitedValue = 0x0000FFFF;
            revertedValue = 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(65535));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator+=") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue += 2257;
            revertedValue += 2257;

            try {
                throwValueOverflow += 1;
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            try {
                throwValueUnderflow += -1;
            } catch (std::underflow_error &e) {
                underflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-354640));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator-=") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue -= 2257;
            revertedValue -= 2257;

            try {
                throwValueOverflow -= -1;
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            try {
                throwValueUnderflow -= 1;
            } catch (std::underflow_error &e) {
                underflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-359154));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator*=") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue *= 2;
            revertedValue *= 2;

            try {
                throwValueOverflow *= 2;
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            try {
                throwValueUnderflow *= 2;
            } catch (std::underflow_error &e) {
                underflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-713794));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator/=") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::min());
            throwValueOverflow.commit();

            bool overflow = false;
            bool domain_error = false;

            commitedValue /= 2;
            revertedValue /= 2;

            try {
                throwValueOverflow /= UnderlyingType(0);
            } catch (std::domain_error &e) {
                domain_error = true;
            }

            try {
                throwValueOverflow /= -1;
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-178448));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(overflow);
            REQUIRE(domain_error);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator%=") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            revertedValue.commit();

            bool domain_error = false;

            commitedValue %= 2;
            revertedValue %= 2;

            try {
                commitedValue %= UnderlyingType(0);
            } catch (std::domain_error &e) {
                domain_error = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-1));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(domain_error);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator&=") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue &= 0x0000FFFF;
            revertedValue &= 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(29217));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator|=") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue |= 0x0000FFFF;
            revertedValue |= 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(393215));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator^=") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();

            commitedValue ^= 0x0000FFFF;
            revertedValue ^= 0x0000FFFF;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(363998));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator++") {
            SafeInt commitedValue(UnderlyingType(356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(356897));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();

            bool overflow = false;

            ++commitedValue;
            ++revertedValue;

            try {
                ++throwValueOverflow;
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(356898));
            REQUIRE(revertedValue.get() == UnderlyingType(356897));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator--") {
            SafeInt commitedValue(UnderlyingType(-356897));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-356897));
            revertedValue.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool underflow = false;

            --commitedValue;
            --revertedValue;

            try {
                --throwValueUnderflow;
            } catch (std::underflow_error &e) {
                underflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-356898));
            REQUIRE(revertedValue.get() == UnderlyingType(-356897));
            REQUIRE(underflow);
        }
    }
};

TEST_CASE("SafeInt_t tests", "[contract][variables][safeint_t_boost]") {
    SafeIntTester<24>()();
    SafeIntTester<40>()();
    SafeIntTester<48>()();
    SafeIntTester<56>()();
    SafeIntTester<72>()();
    SafeIntTester<80>()();
    SafeIntTester<88>()();
    SafeIntTester<96>()();
    SafeIntTester<104>()();
    SafeIntTester<112>()();
    SafeIntTester<120>()();
    SafeIntTester<128>()();
    SafeIntTester<136>()();
    SafeIntTester<144>()();
    SafeIntTester<152>()();
    SafeIntTester<160>()();
    SafeIntTester<168>()();
    SafeIntTester<176>()();
    SafeIntTester<184>()();
    SafeIntTester<192>()();
    SafeIntTester<200>()();
    SafeIntTester<208>()();
    SafeIntTester<216>()();
    SafeIntTester<224>()();
    SafeIntTester<232>()();
    SafeIntTester<240>()();
    SafeIntTester<248>()();
    SafeIntTester<256>()();
}