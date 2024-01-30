/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeint.h"
#include <iostream>
#include <stdexcept>

template <int Size>
struct UnderlyingType;

template <>
struct UnderlyingType<8> { using type = int8_t; };

template <>
struct UnderlyingType<16> { using type = int16_t; };

template <>
struct UnderlyingType<32> { using type = int32_t; };

template <>
struct UnderlyingType<64> { using type = int64_t; };

template <int Size>
struct SafeIntTester {
    using SafeInt = SafeInt_t<Size>;
    using UnderlyingType = typename UnderlyingType<Size>::type;

    void operator()() const {
        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> constructor (Commit and Revert") {
            SafeInt commitedValue(UnderlyingType(-42));
            SafeInt revertedValue(UnderlyingType(-42));

            commitedValue.commit();
            REQUIRE(commitedValue.get() == UnderlyingType(-42));
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-42));
            REQUIRE(revertedValue.get() == 0);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator+") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue = commitedValue + 5;
            revertedValue = revertedValue + 5;

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

            REQUIRE(commitedValue.get() == UnderlyingType(-37));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator-") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue = commitedValue - 5;
            revertedValue = revertedValue - 5;

            // To test overflow, subtract a negative number from the max value.
            try {
                throwValueOverflow = throwValueOverflow - (-1);
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            // To test underflow, subtract a positive number from the min value.
            try {
                throwValueUnderflow = throwValueUnderflow - 1;
            } catch (std::underflow_error &e) {
                underflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-47));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator*") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
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

            REQUIRE(commitedValue.get() == UnderlyingType(-84));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
            REQUIRE(overflow);
            REQUIRE(underflow);

        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator/") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool domain_error = false;
            bool overflow = false;

            commitedValue = commitedValue / 2;
            revertedValue = revertedValue / 2;

            try {
                throwValueUnderflow = throwValueUnderflow / 0;
            } catch (std::domain_error &e) {
                domain_error = true;
            }

            try {
                throwValueUnderflow = throwValueUnderflow / -1;
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(-21));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
            REQUIRE(domain_error);
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator%") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();
            
            bool domain_error = false;

            commitedValue = commitedValue % 2;
            revertedValue = revertedValue % 2;

            try {
                commitedValue = commitedValue % 0;
            } catch (std::domain_error &e) {
                domain_error = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
            REQUIRE(domain_error);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator&") {
            SafeInt commitedValue(UnderlyingType(0b10101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b10101010));
            revertedValue.commit();

            commitedValue = commitedValue & UnderlyingType(0b11110000);
            revertedValue = revertedValue & UnderlyingType(0b11110000);

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b10100000));
            REQUIRE(revertedValue.get() == UnderlyingType(0b10101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator|") {
            SafeInt commitedValue(UnderlyingType(0b10101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b10101010));
            revertedValue.commit();

            commitedValue = commitedValue | UnderlyingType(0b11110000);
            revertedValue = revertedValue | UnderlyingType(0b11110000);

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b11111010));
            REQUIRE(revertedValue.get() == UnderlyingType(0b10101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator^") {
            SafeInt commitedValue(UnderlyingType(0b10101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b10101010));
            revertedValue.commit();

            commitedValue = commitedValue ^ UnderlyingType(0b11110000);
            revertedValue = revertedValue ^ UnderlyingType(0b11110000);

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b01011010));
            REQUIRE(revertedValue.get() == UnderlyingType(0b10101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator<<") {
            SafeInt commitedValue(UnderlyingType(0b10101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b10101010));
            revertedValue.commit();

            commitedValue = commitedValue << 2;
            revertedValue = revertedValue << 2;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b1010101000));
            REQUIRE(revertedValue.get() == UnderlyingType(0b10101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator>>") {
            SafeInt commitedValue(UnderlyingType(0b00101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b00101010));
            revertedValue.commit();

            commitedValue = commitedValue >> 2;
            revertedValue = revertedValue >> 2;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b00001010)); 
            REQUIRE(revertedValue.get() == UnderlyingType(0b00101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator!") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();

            commitedValue = !commitedValue;
            revertedValue = !revertedValue;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator&&") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();

            commitedValue = commitedValue && SafeInt(UnderlyingType(1));
            revertedValue = revertedValue && SafeInt(UnderlyingType(1));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator||") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();

            commitedValue = commitedValue || SafeInt(UnderlyingType(1));
            revertedValue = revertedValue || SafeInt(UnderlyingType(1));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator==") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();

            commitedValue = commitedValue == SafeInt(UnderlyingType(1));
            revertedValue = revertedValue == SafeInt(UnderlyingType(1));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator!=") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();

            commitedValue = commitedValue != SafeInt(UnderlyingType(1));
            revertedValue = revertedValue != SafeInt(UnderlyingType(1));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator<") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();

            commitedValue = commitedValue < SafeInt(UnderlyingType(-41));
            revertedValue = revertedValue < SafeInt(UnderlyingType(-41));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator>") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();

            commitedValue = commitedValue > SafeInt(UnderlyingType(-41));
            revertedValue = revertedValue > SafeInt(UnderlyingType(-41));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator>=") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();

            commitedValue = commitedValue >= SafeInt(UnderlyingType(-41));
            revertedValue = revertedValue >= SafeInt(UnderlyingType(-41));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator<=") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();

            commitedValue = commitedValue <= SafeInt(UnderlyingType(-41));
            revertedValue = revertedValue <= SafeInt(UnderlyingType(-41));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator=") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();

            commitedValue = SafeInt(UnderlyingType(1));
            revertedValue = SafeInt(UnderlyingType(1));

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(1));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator+=") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue += 5;
            revertedValue += 5;

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

            REQUIRE(commitedValue.get() == UnderlyingType(5));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator-=") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();
            SafeInt throwValueOverflow(std::numeric_limits<UnderlyingType>::max());
            throwValueOverflow.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool overflow = false;
            bool underflow = false;

            commitedValue -= 5;
            revertedValue -= 5;

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

            REQUIRE(commitedValue.get() == UnderlyingType(-5));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator*=") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
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

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
            REQUIRE(overflow);
            REQUIRE(underflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator/=") {
            SafeInt commitedValue(UnderlyingType(0));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0));
            revertedValue.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool domain_error = false;
            bool overflow = false;

            commitedValue /= 2;
            revertedValue /= 2;

            try {
                throwValueUnderflow /= 0;
            } catch (std::domain_error &e) {
                domain_error = true;
            }

            try {
                throwValueUnderflow /= -1;
            } catch (std::overflow_error &e) {
                overflow = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(0));
            REQUIRE(domain_error);
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator%=") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
            revertedValue.commit();
            SafeInt throwValueUnderflow(std::numeric_limits<UnderlyingType>::min());
            throwValueUnderflow.commit();

            bool domain_error = false;

            commitedValue %= 2;
            revertedValue %= 2;

            try {
                throwValueUnderflow %= 0;
            } catch (std::domain_error &e) {
                domain_error = true;
            }

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
            REQUIRE(domain_error);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator&=") {
            SafeInt commitedValue(UnderlyingType(0b10101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b10101010));
            revertedValue.commit();

            commitedValue &= UnderlyingType(0b11110000);
            revertedValue &= UnderlyingType(0b11110000);

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b10100000));
            REQUIRE(revertedValue.get() == UnderlyingType(0b10101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator|=") {
            SafeInt commitedValue(UnderlyingType(0b10101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b10101010));
            revertedValue.commit();

            commitedValue |= UnderlyingType(0b11110000);
            revertedValue |= UnderlyingType(0b11110000);

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b11111010));
            REQUIRE(revertedValue.get() == UnderlyingType(0b10101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator^=") {
            SafeInt commitedValue(UnderlyingType(0b10101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b10101010));
            revertedValue.commit();

            commitedValue ^= UnderlyingType(0b11110000);
            revertedValue ^= UnderlyingType(0b11110000);

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b01011010));
            REQUIRE(revertedValue.get() == UnderlyingType(0b10101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator<<=") {
            SafeInt commitedValue(UnderlyingType(0b10101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b10101010));
            revertedValue.commit();

            commitedValue <<= 2;
            revertedValue <<= 2;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b1010101000));
            REQUIRE(revertedValue.get() == UnderlyingType(0b10101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator>>=") {
            SafeInt commitedValue(UnderlyingType(0b00101010));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(0b00101010));
            revertedValue.commit();

            commitedValue >>= 2;
            revertedValue >>= 2;

            commitedValue.commit();
            revertedValue.revert();

            REQUIRE(commitedValue.get() == UnderlyingType(0b00001010));
            REQUIRE(revertedValue.get() == UnderlyingType(0b00101010));
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator++") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
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

            REQUIRE(commitedValue.get() == UnderlyingType(-41));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
            REQUIRE(overflow);
        }

        SECTION(std::string("SafeInt_t<") + std::to_string(Size) + "> operator--") {
            SafeInt commitedValue(UnderlyingType(-42));
            commitedValue.commit();
            SafeInt revertedValue(UnderlyingType(-42));
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

            REQUIRE(commitedValue.get() == UnderlyingType(-43));
            REQUIRE(revertedValue.get() == UnderlyingType(-42));
            REQUIRE(underflow);
        }


    }
};

TEST_CASE("SafeInt_t tests", "[contract][variables][safeint_t]") {
    SafeIntTester<8>()();
    SafeIntTester<16>()();
    SafeIntTester<32>()();
    SafeIntTester<64>()();
}
