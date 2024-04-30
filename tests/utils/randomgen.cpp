/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <algorithm>

#include "../../src/utils/randomgen.h"
#include "../../src/libs/catch2/catch_amalgamated.hpp"

using Catch::Matchers::Equals;

namespace TRandomGen {
  TEST_CASE("RandomGen Class", "[utils][randomgen]") {
    SECTION("RandomGen Constructor") {
      Bytes bytes(32, 0x00);
      Hash seed(bytes);
      RandomGen generator(seed);

      uint256_t firstRandom = generator();
      uint256_t secondRandom = generator();
      uint256_t thirdRandom = generator();

      REQUIRE(firstRandom == uint256_t("18569430475105882587588266137607568536673111973893317399460219858819262702947"));
      REQUIRE(secondRandom == uint256_t("36662618139459487608036673114889889725324975350372714220936135061884814320089"));
      REQUIRE(thirdRandom == uint256_t("24167556515869808906615918001412365693631812133744141770956806246558790984502"));
    }

    SECTION("RandomGen getSeed") {
      Hash seed(std::string("\xa6\x2a\x86\x47\x2e\x5c\x22\x4a\xa0\xa7\x84\xec\xca\xf7\x94\xab\xb6\x03\x02\xe2\x07\x3d\x52\xae\x0d\x09\x5a\xc5\xd1\x6f\x03\xa6"));
      RandomGen generator(seed);
      REQUIRE(generator.getSeed() == seed);
      auto newSeed = generator();
      REQUIRE(generator.getSeed().toUint256() == newSeed);
      newSeed = generator();
      REQUIRE(generator.getSeed().toUint256() == newSeed);
    }

    SECTION("RandomGen Min/Max") {
      REQUIRE(RandomGen::min() == uint256_t("0"));
      REQUIRE(RandomGen::max() == uint256_t("115792089237316195423570985008687907853269984665640564039457584007913129639935"));
    }

    SECTION("RandomGen Shuffle") {
      std::vector<std::string> vector = {
        "First String", "Second String", "Third String", "Fourth String",
        "Fifth String", "Sixth String", "Seventh String", "Eighth String",
        "Ninth String", "Tenth String"
      };

      Hash seed(std::string("\xa4\xdd\x40\x26\x1f\xba\xbe\x97\x7a\xb6\xff\x77\xa7\xea\x9f\x76\xcd\x3b\x28\x6a\xa6\x62\x90\xb0\xd6\x2b\xdf\x43\x03\xf4\x38\x2b"));
      RandomGen generator(seed);
      generator.shuffle(vector);

      REQUIRE_THAT(vector[0], Equals("Eighth String"));
      REQUIRE_THAT(vector[1], Equals("Fifth String"));
      REQUIRE_THAT(vector[2], Equals("Second String"));
      REQUIRE_THAT(vector[3], Equals("Tenth String"));
      REQUIRE_THAT(vector[4], Equals("Seventh String"));
      REQUIRE_THAT(vector[5], Equals("Third String"));
      REQUIRE_THAT(vector[6], Equals("Fourth String"));
      REQUIRE_THAT(vector[7], Equals("Ninth String"));
      REQUIRE_THAT(vector[8], Equals("First String"));
      REQUIRE_THAT(vector[9], Equals("Sixth String"));
    }

    SECTION("RandomGen randomness") {
      Hash seed = Hash::random();
      RandomGen generator(seed);

      std::vector<uint256_t> randoms;
      for (int i = 0; i < 10000; i++) randoms.push_back(generator());
      std::sort(randoms.begin(), randoms.end()); // Sort for easy comparison

      // Check if duplicate values exist
      REQUIRE(std::adjacent_find(randoms.begin(), randoms.end()) == randoms.end());
    }
  }
}

