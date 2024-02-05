#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/dynamicexception.h"

using Catch::Matchers::Equals;

namespace TDynamicException {
    TEST_CASE("DynamicException Class", "[utils][dynamicexception]") {
        SECTION("Exception message is set and retrieved correctly") {
            const std::string expectedMessage = "Test error message";
            DynamicException exception(expectedMessage);

            REQUIRE(std::string(exception.what()) == expectedMessage);
        }

        SECTION("File name is set and retrieved correctly") {
            const std::string expectedFile = "test.cpp";
            DynamicException exception("Test error message", expectedFile);

            REQUIRE(exception.getFile() == expectedFile);
        }

        SECTION("Line number is set and retrieved correctly") {
            const int expectedLine = 42;
            DynamicException exception("Test error message", "test.cpp", expectedLine);

            REQUIRE(exception.getLine() == expectedLine);
        }

        SECTION("Function name is set and retrieved correctly") {
            const std::string expectedFunction = "testFunction";
            DynamicException exception("Test error message", "test.cpp", 42, expectedFunction);

            REQUIRE(exception.getFunction() == expectedFunction);
        }

        SECTION("Timestamp is set correctly") {
            DynamicException exception("Test error message", "test.cpp", 42, "testFunction");
            
            std::string timestamp = exception.getTimestamp();
            REQUIRE(timestamp.length() == 19);
            REQUIRE(timestamp[4] == '-');
            REQUIRE(timestamp[7] == '-');
            REQUIRE(timestamp[10] == ' ');
            REQUIRE(timestamp[13] == ':');
            REQUIRE(timestamp[16] == ':');
        }
    }
}
