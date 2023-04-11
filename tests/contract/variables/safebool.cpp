#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safebool.h"
#include <iostream>


namespace TSafeBool {
  TEST_CASE("SafeBool class", "[contract][variables][safebool]") {
    SECTION("SafeBool Constructor") {
      SafeBool safeBool(true);
      REQUIRE(safeBool);
      safeBool.revert();
      REQUIRE(!safeBool);
      safeBool = true;
      REQUIRE(safeBool);
      safeBool = false;
      REQUIRE(!safeBool);
      safeBool = true;
      safeBool.commit();
      REQUIRE(safeBool);
      SafeBool anotherSafeBool(safeBool);
      REQUIRE(anotherSafeBool);
      anotherSafeBool = false;
      REQUIRE(!anotherSafeBool);
      anotherSafeBool.commit();
      REQUIRE(!anotherSafeBool);
    }

    SECTION("SafeBool operator=") {
      SafeBool safeBool(true);
      REQUIRE(safeBool);
      safeBool = false;
      REQUIRE(!safeBool);
      safeBool = true;
      REQUIRE(safeBool);
      safeBool.commit();
      REQUIRE(safeBool);
    }

    SECTION("SafeBool operator bool()") {
      SafeBool safeBool(true);
      REQUIRE(safeBool);
      safeBool = false;
      REQUIRE(!safeBool);
      safeBool = true;
      REQUIRE(safeBool);
      safeBool.commit();
      REQUIRE(safeBool);
    }
  }
}