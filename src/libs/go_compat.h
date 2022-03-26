#pragma once
#include "Common.h"
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

        }

        uint8_t version()
    }
}
