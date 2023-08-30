/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeaddress.h"
#include "../../src/utils/utils.h"
#include <iostream>

namespace TSafeAddress {
  TEST_CASE("SafeAddress Class", "[contract][variables][safeaddress]") {
    SECTION("SafeAddress constructor") {
      Address randomAddress(Utils::randBytes(20));
      SafeAddress commitedAddress(randomAddress);
      SafeAddress revertedAddress(randomAddress);

      REQUIRE(commitedAddress.get() == randomAddress);
      REQUIRE(revertedAddress.get() == randomAddress);

      commitedAddress.commit();
      revertedAddress.revert();

      REQUIRE(commitedAddress.get() == randomAddress);
      REQUIRE(revertedAddress.get() == Address());
    }

    SECTION("SafeAddress operator=") {
      Address randomAddress(Utils::randBytes(20));
      SafeAddress commitedAddress;
      SafeAddress revertedAddress;

      commitedAddress = randomAddress;
      revertedAddress = randomAddress;

      REQUIRE(commitedAddress.get() == randomAddress);
      REQUIRE(revertedAddress.get() == randomAddress);

      commitedAddress.commit();
      revertedAddress.revert();

      REQUIRE(commitedAddress.get() == randomAddress);
      REQUIRE(revertedAddress.get() == Address());
    }
  }
}
