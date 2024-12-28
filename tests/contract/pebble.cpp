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
    SECTION("Pebble creation + dump") {
      Address pebbleAddr;
      std::unique_ptr<Options> options;
      {
        SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testPebbleCreation");
        pebbleAddr = sdk.deployContract<Pebble>(uint256_t(100000));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::name) == "Pebble");
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::symbol) == "PBL");
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalSupply) == uint256_t(0));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::maxSupply) == uint256_t(100000));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalNormal) == uint64_t(0));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalGold) == uint64_t(0));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalDiamond) == uint64_t(0));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::raritySeed) == uint256_t(1000000));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::diamondRarity) == uint256_t(1));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::goldRarity) == uint256_t(10));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::getAuthorizer) == Address());
        // Dump to database
        options = std::make_unique<Options>(sdk.getOptions());
        sdk.getState().saveToDB();
      }

      // SDKTestSuite should automatically load the state from the DB if we construct it with an Options object
      // (The createNewEnvironment DELETES the DB if any is found)
      SDKTestSuite sdk(*options);
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::name) == "Pebble");
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::symbol) == "PBL");
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalSupply) == uint256_t(0));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::maxSupply) == uint256_t(100000));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalNormal) == uint64_t(0));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalGold) == uint64_t(0));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalDiamond) == uint64_t(0));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::raritySeed) == uint256_t(1000000));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::diamondRarity) == uint256_t(1));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::goldRarity) == uint256_t(10));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::getAuthorizer) == Address());
    }

    SECTION("Pebble ownership transfer (Ownable coverage)") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testPebbleOwnershipTransfer");
      Address pebbleAddr = sdk.deployContract<Pebble>(uint256_t(100000));
      REQUIRE_THROWS(sdk.callFunction(pebbleAddr, &Pebble::transferOwnership, Address())); // cannot transfer to zero address
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::owner) == Address("0x00dead00665771855a34155f5e7405489df2c3c6", false));
      Address newOwner("0x1234567890123456789012345678901234567890", false);
      sdk.callFunction(pebbleAddr, &Pebble::transferOwnership, newOwner);
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::owner) == newOwner);
    }

    SECTION("Pebble ownership renounce (Ownable coverage)") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testPebbleOwnershipTransfer");
      Address pebbleAddr = sdk.deployContract<Pebble>(uint256_t(100000));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::owner) == Address("0x00dead00665771855a34155f5e7405489df2c3c6", false));
      sdk.callFunction(pebbleAddr, &Pebble::renounceOwnership);
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::owner) == Address());
    }

    SECTION("Pebble minting") {
      std::unique_ptr<Options> opts = nullptr;
      TestAccount authorizerAccount = TestAccount::newRandomAccount();
      TestAccount minterAccount = TestAccount::newRandomAccount();
      TestAccount anotherAccount = TestAccount::newRandomAccount();
      Address pebbleAddr;
      {
        SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testPebbleMinting", {authorizerAccount, minterAccount, anotherAccount});
        pebbleAddr = sdk.deployContract<Pebble>(uint256_t(100000));

        REQUIRE_NOTHROW(sdk.callFunction(pebbleAddr, &Pebble::changeAuthorizer, authorizerAccount.address));
        REQUIRE_NOTHROW(sdk.callFunction(pebbleAddr, 0, authorizerAccount, &Pebble::addMinter, minterAccount.address));

        auto mintTx = sdk.callFunction(pebbleAddr, 0, minterAccount, &Pebble::mintNFT, minterAccount.address, uint64_t(1));

        auto events = sdk.getEventsEmittedByTxTup(mintTx, &Pebble::MintedNFT);
        REQUIRE(events.size() == 1);
        auto event = events[0];
        REQUIRE(std::get<0>(event) == minterAccount.address);
        REQUIRE(std::get<1>(event) == uint256_t(0));
        // Derive the same randomness as the one generated to create the rarity
        // then check against the rarity inside the event.
        auto latestBlock = sdk.getStorage().latest();
        auto latestRandomness = latestBlock->getBlockRandomness().toUint256();
        auto expectedRarity = sdk.callViewFunction(pebbleAddr, &Pebble::determineRarity, latestRandomness);
        REQUIRE(std::get<2>(event) == expectedRarity);
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalSupply) == uint256_t(1));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::ownerOf, uint256_t(0)) == minterAccount.address);
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::getTokenRarity, uint256_t(0)) == sdk.callViewFunction(pebbleAddr, &Pebble::rarityToString, expectedRarity));
        if (expectedRarity == Pebble::Rarity::Normal) {
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalNormal) == uint64_t(1));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalGold) == uint64_t(0));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalDiamond) == uint64_t(0));
        } else if (expectedRarity == Pebble::Rarity::Gold) {
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalNormal) == uint64_t(0));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalGold) == uint64_t(1));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalDiamond) == uint64_t(0));
        } else if (expectedRarity == Pebble::Rarity::Diamond) {
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalNormal) == uint64_t(0));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalGold) == uint64_t(0));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalDiamond) == uint64_t(1));
        }

        // Change the rarity so it is ALWAYS a diamond
        sdk.callFunction(pebbleAddr, &Pebble::setDiamondRarity, uint256_t(sdk.callViewFunction(pebbleAddr, &Pebble::raritySeed) + 1));
        // Try minting again
        mintTx = sdk.callFunction(pebbleAddr, 0, minterAccount, &Pebble::mintNFT, minterAccount.address, uint64_t(1));
        // NFT must be a diamond, check the event

        events = sdk.getEventsEmittedByTxTup(mintTx, &Pebble::MintedNFT);
        REQUIRE(events.size() == 1);
        event = events[0];
        REQUIRE(std::get<0>(event) == minterAccount.address);
        REQUIRE(std::get<1>(event) == uint256_t(1));
        REQUIRE(std::get<2>(event) == Pebble::Rarity::Diamond);
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalSupply) == uint256_t(2));
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::ownerOf, uint256_t(1)) == minterAccount.address);
        REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::getTokenRarity, uint256_t(1)) == sdk.callViewFunction(pebbleAddr, &Pebble::rarityToString, Pebble::Rarity::Diamond));
        if (expectedRarity == Pebble::Rarity::Normal) {
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalNormal) == uint64_t(1));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalGold) == uint64_t(0));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalDiamond) == uint64_t(1));
        } else if (expectedRarity == Pebble::Rarity::Gold) {
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalNormal) == uint64_t(0));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalGold) == uint64_t(1));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalDiamond) == uint64_t(1));
        } else if (expectedRarity == Pebble::Rarity::Diamond) {
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalNormal) == uint64_t(0));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalGold) == uint64_t(0));
          REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalDiamond) == uint64_t(2));
        }
        // Check if another account cannot change the rarity
        REQUIRE_THROWS(sdk.callFunction(pebbleAddr, 0, anotherAccount, &Pebble::setDiamondRarity, uint256_t(1)));
        // Check throw against non authorized mint
        REQUIRE_THROWS(sdk.callFunction(pebbleAddr, 0, anotherAccount, &Pebble::mintNFT, anotherAccount.address, uint64_t(1)));
        opts = std::make_unique<Options>(sdk.getOptions());
      }

      auto sdk = SDKTestSuite(*opts);
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::totalSupply) == uint256_t(2));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::ownerOf, uint256_t(1)) == minterAccount.address);
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::getTokenRarity, uint256_t(1)) == sdk.callViewFunction(pebbleAddr, &Pebble::rarityToString, Pebble::Rarity::Diamond));
      REQUIRE(sdk.callViewFunction(pebbleAddr, &Pebble::getAuthorizer) == authorizerAccount.address);
    }
  }
}

