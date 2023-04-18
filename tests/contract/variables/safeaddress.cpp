#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safeaddress.h"
#include "../../src/utils/utils.h"
#include <iostream>

namespace TSafeAddress {
  TEST_CASE("SafeAddress Class", "[contracts][variables][safeaddress]") {
    SECTION("SafeAddress constructor") {
      Address randomAddress(Utils::randBytes(20), true);
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
      Address randomAddress(Utils::randBytes(20), true);
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
