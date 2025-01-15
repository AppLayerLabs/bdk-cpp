/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/reentrancyguard.h"

namespace TReentrancyGuard {
  TEST_CASE("ReentrancyGuard class", "[contract][variables][reentrancyguard]") {
    SECTION("ReentrancyGuard constructor") {
      bool lock = false;
      ReentrancyGuard guard(lock);
      REQUIRE(lock == true);
      REQUIRE_THROWS(ReentrancyGuard(lock));
      guard.~ReentrancyGuard();
      REQUIRE(lock == false);
    }
  }
}

