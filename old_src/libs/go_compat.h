#pragma once
#include "../../include/web3cpp/devcore/Common.h"
#include <string>
#include <chrono>

namespace go {
    namespace error {
        typedef std::string error;
        const std::string ok = "";
        std::string inline err(std::string m) {
            return m;
        }
    }

    class time {
        dev::u128 value;

        time(
            uint8_t version,
            uint64_t seconds,
            uint32_t nanoseconds,
            uint16_t offset_minutes
        ) {
            value = dev::u128(version) << 120 &
                    dev::u128(seconds) << 56 &
                    dev::u128(nanoseconds) << 24 &
                    dev::u128(offset_minutes);

        }

        time() {
            // Convert current timestamp to golang Marshal bullshittery

            namespace chrono = std::chrono;

            chrono::system_clock::time_point now = chrono::system_clock::now();
            auto since_year_1 = dev::u256(chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch()).count()) + dev::u256(chrono::duration<long, chrono::nanoseconds>(6.21355968e+19));
            // discard most significant bytes
            uint16_t nanos = since_year_1;
        }

    };
}
