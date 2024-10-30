/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/contract/templates/pebble.h"

#include "../sdktestsuite.hpp"

namespace TPEBBLE {
  TEST_CASE("Pebble Class", "[contract][pebble]") {
    SECTION("Pebble creation") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testPebbleCreation");
      Address pebbleAddr = sdk.deployContract<Pebble>(uint256_t(100000));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::name) == "Pebble");
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::symbol) == "PBL");
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalSupply) == uint256_t(0));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::maxSupply) == uint256_t(100000));
    }
    SECTION("Pebble minting") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testPebbleMinting");
      Address pebbleAddr = sdk.deployContract<Pebble>(uint256_t(100000));

      auto mintTx = sdk.callFunction(pebbleAddr, &Pebble::mintNFT, sdk.getChainOwnerAccount().address);

      auto events = sdk.getEventsEmittedByTxTup(mintTx, &Pebble::MintedNFT);
      REQUIRE(events.size() == 1);
      auto event = events[0];
      REQUIRE(std::get<0>(event) == sdk.getChainOwnerAccount().address);
      REQUIRE(std::get<1>(event) == uint256_t(0));
      // Derive the same randomness as the one generated to create the rarity
      // then check against the rarity inside the event.
      auto latestBlock = sdk.getStorage().latest();
      auto latestRandomness = latestBlock->getBlockRandomness().toUint256();
      auto expectedRarity = Pebble::determineRarity_(latestRandomness);
      REQUIRE(std::get<2>(event) == expectedRarity);
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalSupply) == uint256_t(1));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::ownerOf, uint256_t(0)) == sdk.getChainOwnerAccount().address);
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::getTokenRarity, uint256_t(0)) == Pebble::rarityToString_(expectedRarity));
    }
  }
}

