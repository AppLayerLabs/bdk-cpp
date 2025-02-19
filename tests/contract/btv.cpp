/*
Copyright (c) [2023-2024] [AppLayer Developers]
  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/contract/templates/btvenergy.h"
#include "../../src/contract/templates/btvplayer.h"
#include "../../src/contract/templates/btvcommon.h"
#include "../sdktestsuite.hpp"
#include "contract/templates/btvproposals.h"


namespace TBuildTheVoid {
  TEST_CASE("Build The Void Tests", "[contract][buildthevoid]") {
    SECTION("BuildTheVoid Test Energy/Player/Proposals") {
      std::unique_ptr<Options> options = nullptr;
      Address playerAddress = Address();
      Address energyAddress = Address();
      Address proposalAddress = Address();
      {
        SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20CreationEVM");
        options = std::make_unique<Options>(sdk.getOptions());
        playerAddress = sdk.deployContract<BTVPlayer>(std::string("Players"), std::string("PLYS"));
        energyAddress = sdk.deployContract<BTVEnergy>(std::string("Energy"), std::string("NRG"), uint8_t(18));
        proposalAddress = sdk.deployContract<BTVProposals>();

        sdk.callFunction(playerAddress, &BTVPlayer::setEnergyContract, energyAddress);
        sdk.callFunction(playerAddress, &BTVPlayer::setProposalContract, proposalAddress);
        sdk.callFunction(proposalAddress, &BTVProposals::setPlayerContract, playerAddress);
        sdk.callFunction(proposalAddress, &BTVProposals::setEnergyContract, energyAddress);

        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::name) == "Players");
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::symbol) == "PLYS");
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::totalSupply) == 0);

        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::name) == "Energy");
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::symbol) == "NRG");
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::decimals) == 18);
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::totalSupply) == 0);

        sdk.callFunction(energyAddress, &BTVEnergy::mint, sdk.getChainOwnerAccount().address, uint256_t("100000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::totalSupply) == uint256_t("100000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("100000000000000000000"));
        sdk.callFunction(playerAddress, &BTVPlayer::mintPlayer, std::string("Alice"), sdk.getChainOwnerAccount().address);
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::totalSupply) == 1);
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerName, uint64_t(0)) == "Alice");
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == 0);
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::playerExists, std::string("Alice")) == true);
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::ownerOf, uint256_t(0)) == sdk.getChainOwnerAccount().address);
        sdk.callFunction(energyAddress, &BTVEnergy::approve, playerAddress, uint256_t("100000000000000000000"));

        sdk.callFunction(playerAddress, &BTVPlayer::addPlayerEnergy, uint64_t(0), uint256_t("100000000000000000000"));
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == uint256_t("100000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, playerAddress) == uint256_t("100000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("0"));

        sdk.callFunction(proposalAddress, &BTVProposals::setProposalPrice, uint256_t("10000000000000000000")); // 10.00 NRG
        // Mint another player with "Bob" name, with no energy
        sdk.callFunction(playerAddress, &BTVPlayer::mintPlayer, std::string("Bob"), sdk.getChainOwnerAccount().address);
        sdk.callFunction(playerAddress, &BTVPlayer::approveProposalSpend);


        auto proposalPrice = sdk.callViewFunction(proposalAddress, &BTVProposals::getProposalPrice);
        REQUIRE(proposalPrice == uint256_t("10000000000000000000"));

        REQUIRE_THROWS(sdk.callFunction(playerAddress, &BTVPlayer::createProposal, uint64_t(1), std::string("Test Proposal"), std::string("This is a test proposal")));

        sdk.callFunction(playerAddress, &BTVPlayer::createProposal, uint64_t(0), std::string("Test Proposal"), std::string("This is a test proposal"));

        auto proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getActiveProposals);
        REQUIRE(proposals.size() == 1);
        REQUIRE(std::get<0>(proposals[0]) == sdk.callViewFunction(proposalAddress, &BTVProposals::getProposalPrice));
        REQUIRE(std::get<1>(proposals[0]) == "Test Proposal");
        REQUIRE(std::get<2>(proposals[0]) == "This is a test proposal");

        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == uint256_t("90000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, playerAddress) == uint256_t("90000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, proposalAddress) == uint256_t("10000000000000000000"));

        auto proposalVotes = sdk.callViewFunction(proposalAddress, &BTVProposals::getProposalVotes, uint64_t(0));
        REQUIRE(proposalVotes.empty());

        // Now try voting
        // First with token 1 that has no energy, and technically cannot vote
        REQUIRE_THROWS(sdk.callFunction(playerAddress, &BTVPlayer::voteOnProposal, uint64_t(1), uint64_t(0), uint256_t("1")));

        // Now, vote EVERYTHING from token 0
        sdk.callFunction(playerAddress, &BTVPlayer::voteOnProposal, uint64_t(0), uint64_t(0), uint256_t("90000000000000000000"));

        proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getActiveProposals);
        REQUIRE(proposals.size() == 1);
        REQUIRE(std::get<0>(proposals[0]) == uint256_t("100000000000000000000"));
        REQUIRE(std::get<1>(proposals[0]) == "Test Proposal");
        REQUIRE(std::get<2>(proposals[0]) == "This is a test proposal");

        proposalVotes = sdk.callViewFunction(proposalAddress, &BTVProposals::getProposalVotes, uint64_t(0));
        REQUIRE(proposalVotes.size() == 1);
        REQUIRE(std::get<0>(proposalVotes[0]) == 0);
        REQUIRE(std::get<1>(proposalVotes[0]) == uint256_t("90000000000000000000"));

        // Check the balance on the player and ERC20 contract
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == uint256_t("0"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, playerAddress) == uint256_t("0"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, proposalAddress) == uint256_t("100000000000000000000"));

        // Now, lets try REMOVING the vote
        REQUIRE_THROWS(sdk.callFunction(playerAddress, &BTVPlayer::removeVote, uint64_t(1), uint64_t(0), uint256_t("1")));
        REQUIRE_THROWS(sdk.callFunction(playerAddress, &BTVPlayer::removeVote, uint64_t(0), uint64_t(1), uint256_t("1")));
        sdk.callFunction(playerAddress, &BTVPlayer::removeVote, uint64_t(0), uint64_t(0), uint256_t("90000000000000000000"));
        REQUIRE_THROWS(sdk.callFunction(playerAddress, &BTVPlayer::removeVote, uint64_t(0), uint64_t(0), uint256_t("1")));

        proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getActiveProposals);
        REQUIRE(proposals.size() == 1);
        REQUIRE(std::get<0>(proposals[0]) == uint256_t("10000000000000000000"));
        REQUIRE(std::get<1>(proposals[0]) == "Test Proposal");
        REQUIRE(std::get<2>(proposals[0]) == "This is a test proposal");

        proposalVotes = sdk.callViewFunction(proposalAddress, &BTVProposals::getProposalVotes, uint64_t(0));
        REQUIRE(proposalVotes.empty());

        // Check player energy and ERC20 contract balance
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == uint256_t("90000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, playerAddress) == uint256_t("90000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, proposalAddress) == uint256_t("10000000000000000000"));

        // Now, lets vote again but then complete the proposal
        sdk.callFunction(playerAddress, &BTVPlayer::voteOnProposal, uint64_t(0), uint64_t(0), uint256_t("90000000000000000000"));
        sdk.callFunction(proposalAddress, &BTVProposals::completeProposal, uint64_t(0));

        // We check again the proposal, it should be in the completed proposals list
        proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getActiveProposals);
        REQUIRE(proposalVotes.empty());
        proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getCompletedProposals);
        REQUIRE(proposals.size() == 1);
        REQUIRE(std::get<0>(proposals[0]) == uint256_t("100000000000000000000"));
        REQUIRE(std::get<1>(proposals[0]) == "Test Proposal");
        REQUIRE(std::get<2>(proposals[0]) == "This is a test proposal");

        proposalVotes = sdk.callViewFunction(proposalAddress, &BTVProposals::getProposalVotes, uint64_t(0));
        REQUIRE(proposalVotes.size() == 1);

        // Now, we claim that vote since the proposal is completed
        sdk.callFunction(playerAddress, &BTVPlayer::removeVote, uint64_t(0), uint64_t(0), uint256_t("90000000000000000000"));

        proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getActiveProposals);
        REQUIRE(proposals.empty());
        proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getCompletedProposals);
        REQUIRE(proposals.size() == 1);
        REQUIRE(std::get<0>(proposals[0]) == uint256_t("100000000000000000000"));
        REQUIRE(std::get<1>(proposals[0]) == "Test Proposal");
        REQUIRE(std::get<2>(proposals[0]) == "This is a test proposal");

        proposalVotes = sdk.callViewFunction(proposalAddress, &BTVProposals::getProposalVotes, uint64_t(0));
        REQUIRE(proposalVotes.empty());
        // Check the balance on the player and ERC20 contract
        REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == uint256_t("90000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, playerAddress) == uint256_t("90000000000000000000"));
        REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, proposalAddress) == uint256_t("10000000000000000000"));

        sdk.getState().saveToDB();
      }
      // Load the SDKTestSuite again using the same environment
      SDKTestSuite sdk(*options);

      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::name) == "Players");
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::symbol) == "PLYS");
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::totalSupply) == 2);
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerName, uint64_t(0)) == "Alice");
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == uint256_t("90000000000000000000"));
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::playerExists, std::string("Alice")) == true);
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::ownerOf, uint256_t(0)) == sdk.getChainOwnerAccount().address);

      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(1)) == uint256_t("0"));
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::playerExists, std::string("Bob")) == true);
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::ownerOf, uint256_t(1)) == sdk.getChainOwnerAccount().address);

      REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::name) == "Energy");
      REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::symbol) == "NRG");
      REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::decimals) == 18);
      REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::totalSupply) == uint256_t("100000000000000000000"));
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == uint256_t("90000000000000000000"));
      REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, playerAddress) == uint256_t("90000000000000000000"));
      REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, proposalAddress) == uint256_t("10000000000000000000"));
      REQUIRE(sdk.callViewFunction(energyAddress, &BTVEnergy::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("0"));
      auto proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getActiveProposals);
      REQUIRE(proposals.empty());
      proposals = sdk.callViewFunction(proposalAddress, &BTVProposals::getCompletedProposals);
      REQUIRE(proposals.size() == 1);
      REQUIRE(std::get<0>(proposals[0]) == uint256_t("100000000000000000000"));
      REQUIRE(std::get<1>(proposals[0]) == "Test Proposal");
      REQUIRE(std::get<2>(proposals[0]) == "This is a test proposal");
    }

    SECTION("BuildTheVoid Test Chunk/World") {
      BTVUtils::Chunk chunk;
      BTVUtils::World world;

      // Set Y 16 of the entire chunk to be of WALL type
      // And set its owner to be 1000
      for (int x = 0; x < BTVUtils::Chunk::WIDTH; x++) {
        for (int z = 0; z < BTVUtils::Chunk::LENGTH; z++) {
          chunk.blocks[x][16][z].setBlockType(BTVUtils::BlockType::WALL);
          chunk.blocks[x][16][z].setPlacer(1000);
        }
      }

      // Now, check if everything was properly set
      for (int x = 0; x < BTVUtils::Chunk::WIDTH; x++) {
        for (int z = 0; z < BTVUtils::Chunk::LENGTH; z++) {
          REQUIRE(chunk.blocks[x][16][z].type == BTVUtils::BlockType::WALL);
          REQUIRE(chunk.blocks[x][16][z].getPlacer().value() == 1000);
        }
      }
      // Check if everything else is set as AIR and without a placer
      for (int x = 0; x < BTVUtils::Chunk::WIDTH; x++) {
        for (int y = 0; y < BTVUtils::Chunk::HEIGHT; y++) {
          for (int z = 0; z < BTVUtils::Chunk::LENGTH; z++) {
            if (y != 16) {
              REQUIRE(chunk.blocks[x][y][z].type == BTVUtils::BlockType::AIR);
              REQUIRE_FALSE(chunk.blocks[x][y][z].getPlacer().has_value());
            }
          }
        }
      }

      // Now, test serialization and deserialization
      auto serialized = chunk.serialize();
      auto deserialized = BTVUtils::Chunk::deserialize(serialized);

      REQUIRE(chunk == deserialized);
      for (int x = 0; x < BTVUtils::Chunk::WIDTH; x++) {
        for (int z = 0; z < BTVUtils::Chunk::LENGTH; z++) {
          REQUIRE(deserialized.blocks[x][16][z].type == BTVUtils::BlockType::WALL);
          REQUIRE(deserialized.blocks[x][16][z].getPlacer().value() == 1000);
        }
      }
      for (int x = 0; x < BTVUtils::Chunk::WIDTH; x++) {
        for (int y = 0; y < BTVUtils::Chunk::HEIGHT; y++) {
          for (int z = 0; z < BTVUtils::Chunk::LENGTH; z++) {
            if (y != 16) {
              REQUIRE(deserialized.blocks[x][y][z].type == BTVUtils::BlockType::AIR);
              REQUIRE_FALSE(deserialized.blocks[x][y][z].getPlacer().has_value());
            }
          }
        }
      }

      auto floorDiv = [](int64_t val, int64_t size) {
        if (val >= 0) {
          return val / size;
        } else {
          return (val - (size - 1)) / size;
        }
      };
      // Now, test the world
      for (uint64_t i = 0; i < 1000; ++i) {
        // Generate a random WORLD POSITION, it can range from -511 to 511 (X,Y) and 0 to 63 (Y)
        BTVUtils::WorldBlockPos wp;
        // I know that rand() is not a good random number generator, but it's good enough for this test
        wp.x = rand() % 1024 - 512;
        wp.y = rand() % 64;
        wp.z = rand() % 1024 - 512;

        // Convert the world position to a local position
        auto lp = world.worldToLocal(wp);
        // Check if the local position is correct
        REQUIRE(lp.cx == floorDiv(wp.x, BTVUtils::Chunk::WIDTH));
        REQUIRE(lp.cy == floorDiv(wp.z, BTVUtils::Chunk::LENGTH));
        REQUIRE(lp.x == wp.x - (lp.cx * BTVUtils::Chunk::WIDTH));
        REQUIRE(lp.y == wp.y);
        REQUIRE(lp.z == wp.z - (lp.cy * BTVUtils::Chunk::LENGTH));
        // Get the neighbors of the world position
        auto worldNeighbors = world.getNeighborsWorld(wp);
        // Also get the neighbors of the local position
        auto localNeighbors = world.getNeighborsLocal(lp);
        // Now check if all the neighbors are pointing to the SAME block
        REQUIRE(worldNeighbors.front.first == localNeighbors.front.first);
        REQUIRE(worldNeighbors.back.first == localNeighbors.back.first);
        REQUIRE(worldNeighbors.top.first == localNeighbors.top.first);
        REQUIRE(worldNeighbors.bottom.first == localNeighbors.bottom.first);
        REQUIRE(worldNeighbors.left.first == localNeighbors.left.first);
        REQUIRE(worldNeighbors.right.first == localNeighbors.right.first);
        // Now check if the positions are correct
        REQUIRE(worldNeighbors.front.second == BTVUtils::World::localToWorld(localNeighbors.front.second));
        REQUIRE(worldNeighbors.back.second == BTVUtils::World::localToWorld(localNeighbors.back.second));
        REQUIRE(worldNeighbors.top.second == BTVUtils::World::localToWorld(localNeighbors.top.second));
        REQUIRE(worldNeighbors.bottom.second == BTVUtils::World::localToWorld(localNeighbors.bottom.second));
        REQUIRE(worldNeighbors.left.second == BTVUtils::World::localToWorld(localNeighbors.left.second));
        REQUIRE(worldNeighbors.right.second == BTVUtils::World::localToWorld(localNeighbors.right.second));

        // Same but with worldToLocal
        REQUIRE(BTVUtils::World::worldToLocal(worldNeighbors.front.second) == localNeighbors.front.second);
        REQUIRE(BTVUtils::World::worldToLocal(worldNeighbors.back.second) == localNeighbors.back.second);
        REQUIRE(BTVUtils::World::worldToLocal(worldNeighbors.top.second) == localNeighbors.top.second);
        REQUIRE(BTVUtils::World::worldToLocal(worldNeighbors.bottom.second) == localNeighbors.bottom.second);
        REQUIRE(BTVUtils::World::worldToLocal(worldNeighbors.left.second) == localNeighbors.left.second);
        REQUIRE(BTVUtils::World::worldToLocal(worldNeighbors.right.second) == localNeighbors.right.second);

        if (BTVUtils::World::isOutOfBounds(worldNeighbors.front.second)) {
          REQUIRE(worldNeighbors.front.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(worldNeighbors.back.second)) {
          REQUIRE(worldNeighbors.back.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(worldNeighbors.top.second)) {
          REQUIRE(worldNeighbors.top.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second)) {
          REQUIRE(worldNeighbors.bottom.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(worldNeighbors.left.second)) {
          REQUIRE(worldNeighbors.left.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(worldNeighbors.right.second)) {
          REQUIRE(worldNeighbors.right.first == nullptr);
        }

        if (BTVUtils::World::isOutOfBounds(localNeighbors.front.second)) {
          REQUIRE(localNeighbors.front.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(localNeighbors.back.second)) {
          REQUIRE(localNeighbors.back.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(localNeighbors.top.second)) {
          REQUIRE(localNeighbors.top.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second)) {
          REQUIRE(localNeighbors.bottom.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(localNeighbors.left.second)) {
          REQUIRE(localNeighbors.left.first == nullptr);
        }
        if (BTVUtils::World::isOutOfBounds(localNeighbors.right.second)) {
          REQUIRE(localNeighbors.right.first == nullptr);
        }
      }

      // Now test each "border" of the world
      // We must check all 8 corners of the world
      // Get their neighbors and verify if they are correct and out of bounrds
      {
        BTVUtils::WorldBlockPos wp;
        wp.x = 511;
        wp.y = 0;
        wp.z = 511;

        auto worldNeighbors = world.getNeighborsWorld(wp);
        auto localNeighbors = world.getNeighborsLocal(world.worldToLocal(wp));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.front.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.back.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.top.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.left.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.right.second));

        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.front.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.back.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.top.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.left.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.right.second));

        REQUIRE(worldNeighbors.front.first == localNeighbors.front.first);
        REQUIRE(worldNeighbors.back.first == localNeighbors.back.first);
        REQUIRE(worldNeighbors.top.first == localNeighbors.top.first);
        REQUIRE(worldNeighbors.bottom.first == localNeighbors.bottom.first);
        REQUIRE(worldNeighbors.left.first == localNeighbors.left.first);
        REQUIRE(worldNeighbors.right.first == localNeighbors.right.first);

        // Check if +x, -y and +z are out of bounds
        REQUIRE(worldNeighbors.front.first == nullptr);
        REQUIRE(worldNeighbors.bottom.first == nullptr);
        REQUIRE(worldNeighbors.right.first == nullptr);
        REQUIRE(worldNeighbors.back.first != nullptr);
        REQUIRE(worldNeighbors.top.first != nullptr);
        REQUIRE(worldNeighbors.left.first != nullptr);
        // Check if the positions are correct for each neighbor
        REQUIRE(worldNeighbors.front.second.x == 512);
        REQUIRE(worldNeighbors.front.second.y == 0);
        REQUIRE(worldNeighbors.front.second.z == 511);
        REQUIRE(worldNeighbors.back.second.x == 510);
        REQUIRE(worldNeighbors.back.second.y == 0);
        REQUIRE(worldNeighbors.back.second.z == 511);
        REQUIRE(worldNeighbors.top.second.x == 511);
        REQUIRE(worldNeighbors.top.second.y == 1);
        REQUIRE(worldNeighbors.top.second.z == 511);
        REQUIRE(worldNeighbors.bottom.second.x == 511);
        REQUIRE(worldNeighbors.bottom.second.y == -1);
        REQUIRE(worldNeighbors.bottom.second.z == 511);
        REQUIRE(worldNeighbors.left.second.x == 511);
        REQUIRE(worldNeighbors.left.second.y == 0);
        REQUIRE(worldNeighbors.left.second.z == 510);
        REQUIRE(worldNeighbors.right.second.x == 511);
        REQUIRE(worldNeighbors.right.second.y == 0);
        REQUIRE(worldNeighbors.right.second.z == 512);
        // Also check the local positions are correct, INCLUDING THE CHUNK COORDINATES
        REQUIRE(localNeighbors.front.second.cx == 32);
        REQUIRE(localNeighbors.front.second.cy == 31);
        REQUIRE(localNeighbors.front.second.x == 0);
        REQUIRE(localNeighbors.front.second.y == 0);
        REQUIRE(localNeighbors.front.second.z == 15);

        REQUIRE(localNeighbors.back.second.cx == 31);
        REQUIRE(localNeighbors.back.second.cy == 31);
        REQUIRE(localNeighbors.back.second.x == 14);
        REQUIRE(localNeighbors.back.second.y == 0);
        REQUIRE(localNeighbors.back.second.z == 15);

        REQUIRE(localNeighbors.top.second.cx == 31);
        REQUIRE(localNeighbors.top.second.cy == 31);
        REQUIRE(localNeighbors.top.second.x == 15);
        REQUIRE(localNeighbors.top.second.y == 1);
        REQUIRE(localNeighbors.top.second.z == 15);

        REQUIRE(localNeighbors.bottom.second.cx == 31);
        REQUIRE(localNeighbors.bottom.second.cy == 31);
        REQUIRE(localNeighbors.bottom.second.x == 15);
        REQUIRE(localNeighbors.bottom.second.y == -1);
        REQUIRE(localNeighbors.bottom.second.z == 15);

        REQUIRE(localNeighbors.left.second.cx == 31);
        REQUIRE(localNeighbors.left.second.cy == 31);
        REQUIRE(localNeighbors.left.second.x == 15);
        REQUIRE(localNeighbors.left.second.y == 0);
        REQUIRE(localNeighbors.left.second.z == 14);

        REQUIRE(localNeighbors.right.second.cx == 31);
        REQUIRE(localNeighbors.right.second.cy == 32);
        REQUIRE(localNeighbors.right.second.x == 15);
        REQUIRE(localNeighbors.right.second.y == 0);
        REQUIRE(localNeighbors.right.second.z == 0);
      }
      {
        // Top Left Corner
        BTVUtils::WorldBlockPos wp;
        wp.x = -512;
        wp.y = 0;
        wp.z = 511;

        auto worldNeighbors = world.getNeighborsWorld(wp);
        auto localNeighbors = world.getNeighborsLocal(world.worldToLocal(wp));

        // Out-of-bounds checks:
        //   front   => (x+1) = -511  => In bounds
        //   back    => (x-1) = -513  => Out of bounds
        //   top     => (y+1) = 1     => In bounds
        //   bottom  => (y-1) = -1    => Out of bounds
        //   left    => (z-1) = 510   => In bounds
        //   right   => (z+1) = 512   => Out of bounds

        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.front.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.back.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.top.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.left.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.right.second));

        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.front.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.back.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.top.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.left.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.right.second));

        // Check that the pointers are null where out-of-bounds, and not-null otherwise
        REQUIRE(worldNeighbors.front.first != nullptr);
        REQUIRE(worldNeighbors.back.first == nullptr);
        REQUIRE(worldNeighbors.top.first != nullptr);
        REQUIRE(worldNeighbors.bottom.first == nullptr);
        REQUIRE(worldNeighbors.left.first != nullptr);
        REQUIRE(worldNeighbors.right.first == nullptr);

        // Check that the world neighbor positions are correct
        REQUIRE(worldNeighbors.front.second.x == -511);
        REQUIRE(worldNeighbors.front.second.y == 0);
        REQUIRE(worldNeighbors.front.second.z == 511);

        REQUIRE(worldNeighbors.back.second.x == -513);
        REQUIRE(worldNeighbors.back.second.y == 0);
        REQUIRE(worldNeighbors.back.second.z == 511);

        REQUIRE(worldNeighbors.top.second.x == -512);
        REQUIRE(worldNeighbors.top.second.y == 1);
        REQUIRE(worldNeighbors.top.second.z == 511);

        REQUIRE(worldNeighbors.bottom.second.x == -512);
        REQUIRE(worldNeighbors.bottom.second.y == -1);
        REQUIRE(worldNeighbors.bottom.second.z == 511);

        REQUIRE(worldNeighbors.left.second.x == -512);
        REQUIRE(worldNeighbors.left.second.y == 0);
        REQUIRE(worldNeighbors.left.second.z == 510);

        REQUIRE(worldNeighbors.right.second.x == -512);
        REQUIRE(worldNeighbors.right.second.y == 0);
        REQUIRE(worldNeighbors.right.second.z == 512);

        // Check the local positions (including chunk coords)
        // Base local pos of (x=-512,z=511,y=0) => (cx=-32,cy=31, x=0,y=0,z=15)
        //   front => x+1 => (cx=-32, x=1, same chunk)
        //   back => x-1 => shift => (cx=-33, x=15)
        //   right => z+1 => shift => (cy=32, z=0), etc.

        REQUIRE(localNeighbors.front.second.cx == -32);
        REQUIRE(localNeighbors.front.second.cy == 31);
        REQUIRE(localNeighbors.front.second.x == 1);
        REQUIRE(localNeighbors.front.second.y == 0);
        REQUIRE(localNeighbors.front.second.z == 15);

        REQUIRE(localNeighbors.back.second.cx == -33);
        REQUIRE(localNeighbors.back.second.cy == 31);
        REQUIRE(localNeighbors.back.second.x == 15);
        REQUIRE(localNeighbors.back.second.y == 0);
        REQUIRE(localNeighbors.back.second.z == 15);

        REQUIRE(localNeighbors.top.second.cx == -32);
        REQUIRE(localNeighbors.top.second.cy == 31);
        REQUIRE(localNeighbors.top.second.x == 0);
        REQUIRE(localNeighbors.top.second.y == 1);
        REQUIRE(localNeighbors.top.second.z == 15);

        REQUIRE(localNeighbors.bottom.second.cx == -32);
        REQUIRE(localNeighbors.bottom.second.cy == 31);
        REQUIRE(localNeighbors.bottom.second.x == 0);
        REQUIRE(localNeighbors.bottom.second.y == -1);
        REQUIRE(localNeighbors.bottom.second.z == 15);

        REQUIRE(localNeighbors.left.second.cx == -32);
        REQUIRE(localNeighbors.left.second.cy == 31);
        REQUIRE(localNeighbors.left.second.x == 0);
        REQUIRE(localNeighbors.left.second.y == 0);
        REQUIRE(localNeighbors.left.second.z == 14);

        REQUIRE(localNeighbors.right.second.cx == -32);
        REQUIRE(localNeighbors.right.second.cy == 32);
        REQUIRE(localNeighbors.right.second.x == 0);
        REQUIRE(localNeighbors.right.second.y == 0);
        REQUIRE(localNeighbors.right.second.z == 0);
      }
      {
        // Bottom Left Corner
        BTVUtils::WorldBlockPos wp;
        wp.x = -512;
        wp.y = 0;
        wp.z = -512;

        auto worldNeighbors = world.getNeighborsWorld(wp);
        auto localNeighbors = world.getNeighborsLocal(world.worldToLocal(wp));

        // Out-of-bounds checks:
        //   front   => (x+1) = -511  => In bounds
        //   back    => (x-1) = -513  => Out of bounds
        //   top     => (y+1) = 1     => In bounds
        //   bottom  => (y-1) = -1    => Out of bounds
        //   left    => (z-1) = -513  => Out of bounds
        //   right   => (z+1) = -511  => In bounds

        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.front.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.back.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.top.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.left.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.right.second));

        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.front.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.back.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.top.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.left.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.right.second));

        // Pointer checks
        REQUIRE(worldNeighbors.front.first != nullptr);
        REQUIRE(worldNeighbors.back.first == nullptr);
        REQUIRE(worldNeighbors.top.first != nullptr);
        REQUIRE(worldNeighbors.bottom.first == nullptr);
        REQUIRE(worldNeighbors.left.first == nullptr);
        REQUIRE(worldNeighbors.right.first != nullptr);

        // World neighbor positions
        REQUIRE(worldNeighbors.front.second.x == -511);
        REQUIRE(worldNeighbors.front.second.y == 0);
        REQUIRE(worldNeighbors.front.second.z == -512);

        REQUIRE(worldNeighbors.back.second.x == -513);
        REQUIRE(worldNeighbors.back.second.y == 0);
        REQUIRE(worldNeighbors.back.second.z == -512);

        REQUIRE(worldNeighbors.top.second.x == -512);
        REQUIRE(worldNeighbors.top.second.y == 1);
        REQUIRE(worldNeighbors.top.second.z == -512);

        REQUIRE(worldNeighbors.bottom.second.x == -512);
        REQUIRE(worldNeighbors.bottom.second.y == -1);
        REQUIRE(worldNeighbors.bottom.second.z == -512);

        REQUIRE(worldNeighbors.left.second.x == -512);
        REQUIRE(worldNeighbors.left.second.y == 0);
        REQUIRE(worldNeighbors.left.second.z == -513);

        REQUIRE(worldNeighbors.right.second.x == -512);
        REQUIRE(worldNeighbors.right.second.y == 0);
        REQUIRE(worldNeighbors.right.second.z == -511);

        // Local positions
        // Base local pos => (cx=-32, cy=-32, x=0, y=0, z=0)

        REQUIRE(localNeighbors.front.second.cx == -32);
        REQUIRE(localNeighbors.front.second.cy == -32);
        REQUIRE(localNeighbors.front.second.x == 1);
        REQUIRE(localNeighbors.front.second.y == 0);
        REQUIRE(localNeighbors.front.second.z == 0);

        REQUIRE(localNeighbors.back.second.cx == -33);
        REQUIRE(localNeighbors.back.second.cy == -32);
        REQUIRE(localNeighbors.back.second.x == 15);
        REQUIRE(localNeighbors.back.second.y == 0);
        REQUIRE(localNeighbors.back.second.z == 0);

        REQUIRE(localNeighbors.top.second.cx == -32);
        REQUIRE(localNeighbors.top.second.cy == -32);
        REQUIRE(localNeighbors.top.second.x == 0);
        REQUIRE(localNeighbors.top.second.y == 1);
        REQUIRE(localNeighbors.top.second.z == 0);

        REQUIRE(localNeighbors.bottom.second.cx == -32);
        REQUIRE(localNeighbors.bottom.second.cy == -32);
        REQUIRE(localNeighbors.bottom.second.x == 0);
        REQUIRE(localNeighbors.bottom.second.y == -1);
        REQUIRE(localNeighbors.bottom.second.z == 0);

        REQUIRE(localNeighbors.left.second.cx == -32);
        REQUIRE(localNeighbors.left.second.cy == -33);
        REQUIRE(localNeighbors.left.second.x == 0);
        REQUIRE(localNeighbors.left.second.y == 0);
        REQUIRE(localNeighbors.left.second.z == 15);

        REQUIRE(localNeighbors.right.second.cx == -32);
        REQUIRE(localNeighbors.right.second.cy == -32);
        REQUIRE(localNeighbors.right.second.x == 0);
        REQUIRE(localNeighbors.right.second.y == 0);
        REQUIRE(localNeighbors.right.second.z == 1);
      }
      {
        // Bottom Right Corner
        BTVUtils::WorldBlockPos wp;
        wp.x = 511;
        wp.y = 0;
        wp.z = -512;

        auto worldNeighbors = world.getNeighborsWorld(wp);
        auto localNeighbors = world.getNeighborsLocal(world.worldToLocal(wp));

        // Out-of-bounds checks:
        //   front   => x+1 = 512   => Out of bounds
        //   back    => x-1 = 510   => In bounds
        //   top     => y+1 = 1     => In bounds
        //   bottom  => y-1 = -1    => Out of bounds
        //   left    => z-1 = -513  => Out of bounds
        //   right   => z+1 = -511  => In bounds

        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.front.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.back.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.top.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.left.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.right.second));

        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.front.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.back.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.top.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.left.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.right.second));

        // Pointer checks
        REQUIRE(worldNeighbors.front.first == nullptr);
        REQUIRE(worldNeighbors.back.first != nullptr);
        REQUIRE(worldNeighbors.top.first != nullptr);
        REQUIRE(worldNeighbors.bottom.first == nullptr);
        REQUIRE(worldNeighbors.left.first == nullptr);
        REQUIRE(worldNeighbors.right.first != nullptr);

        // World neighbor positions
        REQUIRE(worldNeighbors.front.second.x == 512);
        REQUIRE(worldNeighbors.front.second.y == 0);
        REQUIRE(worldNeighbors.front.second.z == -512);

        REQUIRE(worldNeighbors.back.second.x == 510);
        REQUIRE(worldNeighbors.back.second.y == 0);
        REQUIRE(worldNeighbors.back.second.z == -512);

        REQUIRE(worldNeighbors.top.second.x == 511);
        REQUIRE(worldNeighbors.top.second.y == 1);
        REQUIRE(worldNeighbors.top.second.z == -512);

        REQUIRE(worldNeighbors.bottom.second.x == 511);
        REQUIRE(worldNeighbors.bottom.second.y == -1);
        REQUIRE(worldNeighbors.bottom.second.z == -512);

        REQUIRE(worldNeighbors.left.second.x == 511);
        REQUIRE(worldNeighbors.left.second.y == 0);
        REQUIRE(worldNeighbors.left.second.z == -513);

        REQUIRE(worldNeighbors.right.second.x == 511);
        REQUIRE(worldNeighbors.right.second.y == 0);
        REQUIRE(worldNeighbors.right.second.z == -511);

        // Local positions
        // Base => (cx=31, cy=-32, x=15, y=0, z=0)

        REQUIRE(localNeighbors.front.second.cx == 32);
        REQUIRE(localNeighbors.front.second.cy == -32);
        REQUIRE(localNeighbors.front.second.x == 0);
        REQUIRE(localNeighbors.front.second.y == 0);
        REQUIRE(localNeighbors.front.second.z == 0);

        REQUIRE(localNeighbors.back.second.cx == 31);
        REQUIRE(localNeighbors.back.second.cy == -32);
        REQUIRE(localNeighbors.back.second.x == 14);
        REQUIRE(localNeighbors.back.second.y == 0);
        REQUIRE(localNeighbors.back.second.z == 0);

        REQUIRE(localNeighbors.top.second.cx == 31);
        REQUIRE(localNeighbors.top.second.cy == -32);
        REQUIRE(localNeighbors.top.second.x == 15);
        REQUIRE(localNeighbors.top.second.y == 1);
        REQUIRE(localNeighbors.top.second.z == 0);

        REQUIRE(localNeighbors.bottom.second.cx == 31);
        REQUIRE(localNeighbors.bottom.second.cy == -32);
        REQUIRE(localNeighbors.bottom.second.x == 15);
        REQUIRE(localNeighbors.bottom.second.y == -1);
        REQUIRE(localNeighbors.bottom.second.z == 0);

        REQUIRE(localNeighbors.left.second.cx == 31);
        REQUIRE(localNeighbors.left.second.cy == -33);
        REQUIRE(localNeighbors.left.second.x == 15);
        REQUIRE(localNeighbors.left.second.y == 0);
        REQUIRE(localNeighbors.left.second.z == 15);

        REQUIRE(localNeighbors.right.second.cx == 31);
        REQUIRE(localNeighbors.right.second.cy == -32);
        REQUIRE(localNeighbors.right.second.x == 15);
        REQUIRE(localNeighbors.right.second.y == 0);
        REQUIRE(localNeighbors.right.second.z == 1);
      }
{
        BTVUtils::WorldBlockPos wp;
        wp.x = 511;
        wp.y = 63;
        wp.z = 511;

        auto worldNeighbors = world.getNeighborsWorld(wp);
        auto localNeighbors = world.getNeighborsLocal(world.worldToLocal(wp));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.front.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.back.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.top.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.left.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.right.second));

        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.front.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.back.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.top.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.left.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.right.second));

        REQUIRE(worldNeighbors.front.first == localNeighbors.front.first);
        REQUIRE(worldNeighbors.back.first == localNeighbors.back.first);
        REQUIRE(worldNeighbors.top.first == localNeighbors.top.first);
        REQUIRE(worldNeighbors.bottom.first == localNeighbors.bottom.first);
        REQUIRE(worldNeighbors.left.first == localNeighbors.left.first);
        REQUIRE(worldNeighbors.right.first == localNeighbors.right.first);

        // Check if +x, -y and +z are out of bounds
        REQUIRE(worldNeighbors.front.first == nullptr);
        REQUIRE(worldNeighbors.bottom.first != nullptr);
        REQUIRE(worldNeighbors.right.first == nullptr);
        REQUIRE(worldNeighbors.back.first != nullptr);
        REQUIRE(worldNeighbors.top.first == nullptr);
        REQUIRE(worldNeighbors.left.first != nullptr);
        // Check if the positions are correct for each neighbor
        REQUIRE(worldNeighbors.front.second.x == 512);
        REQUIRE(worldNeighbors.front.second.y == 63);
        REQUIRE(worldNeighbors.front.second.z == 511);
        REQUIRE(worldNeighbors.back.second.x == 510);
        REQUIRE(worldNeighbors.back.second.y == 63);
        REQUIRE(worldNeighbors.back.second.z == 511);
        REQUIRE(worldNeighbors.top.second.x == 511);
        REQUIRE(worldNeighbors.top.second.y == 64);
        REQUIRE(worldNeighbors.top.second.z == 511);
        REQUIRE(worldNeighbors.bottom.second.x == 511);
        REQUIRE(worldNeighbors.bottom.second.y == 62);
        REQUIRE(worldNeighbors.bottom.second.z == 511);
        REQUIRE(worldNeighbors.left.second.x == 511);
        REQUIRE(worldNeighbors.left.second.y == 63);
        REQUIRE(worldNeighbors.left.second.z == 510);
        REQUIRE(worldNeighbors.right.second.x == 511);
        REQUIRE(worldNeighbors.right.second.y == 63);
        REQUIRE(worldNeighbors.right.second.z == 512);
        // Also check the local positions are correct, INCLUDING THE CHUNK COORDINATES
        REQUIRE(localNeighbors.front.second.cx == 32);
        REQUIRE(localNeighbors.front.second.cy == 31);
        REQUIRE(localNeighbors.front.second.x == 0);
        REQUIRE(localNeighbors.front.second.y == 63);
        REQUIRE(localNeighbors.front.second.z == 15);

        REQUIRE(localNeighbors.back.second.cx == 31);
        REQUIRE(localNeighbors.back.second.cy == 31);
        REQUIRE(localNeighbors.back.second.x == 14);
        REQUIRE(localNeighbors.back.second.y == 63);
        REQUIRE(localNeighbors.back.second.z == 15);

        REQUIRE(localNeighbors.top.second.cx == 31);
        REQUIRE(localNeighbors.top.second.cy == 31);
        REQUIRE(localNeighbors.top.second.x == 15);
        REQUIRE(localNeighbors.top.second.y == 64);
        REQUIRE(localNeighbors.top.second.z == 15);

        REQUIRE(localNeighbors.bottom.second.cx == 31);
        REQUIRE(localNeighbors.bottom.second.cy == 31);
        REQUIRE(localNeighbors.bottom.second.x == 15);
        REQUIRE(localNeighbors.bottom.second.y == 62);
        REQUIRE(localNeighbors.bottom.second.z == 15);

        REQUIRE(localNeighbors.left.second.cx == 31);
        REQUIRE(localNeighbors.left.second.cy == 31);
        REQUIRE(localNeighbors.left.second.x == 15);
        REQUIRE(localNeighbors.left.second.y == 63);
        REQUIRE(localNeighbors.left.second.z == 14);

        REQUIRE(localNeighbors.right.second.cx == 31);
        REQUIRE(localNeighbors.right.second.cy == 32);
        REQUIRE(localNeighbors.right.second.x == 15);
        REQUIRE(localNeighbors.right.second.y == 63);
        REQUIRE(localNeighbors.right.second.z == 0);
      }
      {
        // Top Left Corner at y=63
        BTVUtils::WorldBlockPos wp;
        wp.x = -512;
        wp.y = 63;
        wp.z = 511;

        auto worldNeighbors = world.getNeighborsWorld(wp);
        auto localNeighbors = world.getNeighborsLocal(world.worldToLocal(wp));

        // Out-of-bounds checks:
        //   front   => x+1 = -511      => In bounds
        //   back    => x-1 = -513      => Out of bounds
        //   top     => y+1 = 64        => Out of bounds (since max y=63)
        //   bottom  => y-1 = 62        => In bounds
        //   left    => z-1 = 510       => In bounds
        //   right   => z+1 = 512       => Out of bounds

        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.front.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.back.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.top.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.left.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.right.second));

        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.front.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.back.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.top.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.left.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.right.second));

        // Pointer checks
        REQUIRE(worldNeighbors.front.first != nullptr);
        REQUIRE(worldNeighbors.back.first == nullptr);
        REQUIRE(worldNeighbors.top.first == nullptr);
        REQUIRE(worldNeighbors.bottom.first != nullptr);
        REQUIRE(worldNeighbors.left.first != nullptr);
        REQUIRE(worldNeighbors.right.first == nullptr);

        // World neighbor positions
        REQUIRE(worldNeighbors.front.second.x == -511);
        REQUIRE(worldNeighbors.front.second.y == 63);
        REQUIRE(worldNeighbors.front.second.z == 511);

        REQUIRE(worldNeighbors.back.second.x == -513);
        REQUIRE(worldNeighbors.back.second.y == 63);
        REQUIRE(worldNeighbors.back.second.z == 511);

        REQUIRE(worldNeighbors.top.second.x == -512);
        REQUIRE(worldNeighbors.top.second.y == 64);
        REQUIRE(worldNeighbors.top.second.z == 511);

        REQUIRE(worldNeighbors.bottom.second.x == -512);
        REQUIRE(worldNeighbors.bottom.second.y == 62);
        REQUIRE(worldNeighbors.bottom.second.z == 511);

        REQUIRE(worldNeighbors.left.second.x == -512);
        REQUIRE(worldNeighbors.left.second.y == 63);
        REQUIRE(worldNeighbors.left.second.z == 510);

        REQUIRE(worldNeighbors.right.second.x == -512);
        REQUIRE(worldNeighbors.right.second.y == 63);
        REQUIRE(worldNeighbors.right.second.z == 512);

        // Local positions
        // base => for (x=-512,z=511,y=63):
        //   -512 / 16 = -32 in X-chunk space (assuming floorDiv logic)
        //    511 / 16 = 31 in Z-chunk space
        //    In-chunk x = 0, z = 15, y=63
        // (Exact chunk coords can vary if your floorDiv handles negatives differently, but pattern is the same.)

        REQUIRE(localNeighbors.front.second.cx == -32);
        REQUIRE(localNeighbors.front.second.cy == 31);
        REQUIRE(localNeighbors.front.second.x == 1);
        REQUIRE(localNeighbors.front.second.y == 63);
        REQUIRE(localNeighbors.front.second.z == 15);

        REQUIRE(localNeighbors.back.second.cx == -33);
        REQUIRE(localNeighbors.back.second.cy == 31);
        REQUIRE(localNeighbors.back.second.x == 15);
        REQUIRE(localNeighbors.back.second.y == 63);
        REQUIRE(localNeighbors.back.second.z == 15);

        REQUIRE(localNeighbors.top.second.cx == -32);
        REQUIRE(localNeighbors.top.second.cy == 31);
        REQUIRE(localNeighbors.top.second.x == 0);
        REQUIRE(localNeighbors.top.second.y == 64);
        REQUIRE(localNeighbors.top.second.z == 15);

        REQUIRE(localNeighbors.bottom.second.cx == -32);
        REQUIRE(localNeighbors.bottom.second.cy == 31);
        REQUIRE(localNeighbors.bottom.second.x == 0);
        REQUIRE(localNeighbors.bottom.second.y == 62);
        REQUIRE(localNeighbors.bottom.second.z == 15);

        REQUIRE(localNeighbors.left.second.cx == -32);
        REQUIRE(localNeighbors.left.second.cy == 31);
        REQUIRE(localNeighbors.left.second.x == 0);
        REQUIRE(localNeighbors.left.second.y == 63);
        REQUIRE(localNeighbors.left.second.z == 14);

        REQUIRE(localNeighbors.right.second.cx == -32);
        REQUIRE(localNeighbors.right.second.cy == 32);
        REQUIRE(localNeighbors.right.second.x == 0);
        REQUIRE(localNeighbors.right.second.y == 63);
        REQUIRE(localNeighbors.right.second.z == 0);
      }
      {
        // Bottom Left Corner at y=63
        BTVUtils::WorldBlockPos wp;
        wp.x = -512;
        wp.y = 63;
        wp.z = -512;

        auto worldNeighbors = world.getNeighborsWorld(wp);
        auto localNeighbors = world.getNeighborsLocal(world.worldToLocal(wp));

        // Out-of-bounds checks:
        //   front   => x+1 = -511  => In bounds
        //   back    => x-1 = -513  => Out of bounds
        //   top     => y+1 = 64    => Out of bounds
        //   bottom  => y-1 = 62    => In bounds
        //   left    => z-1 = -513  => Out of bounds
        //   right   => z+1 = -511  => In bounds

        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.front.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.back.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.top.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.left.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.right.second));

        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.front.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.back.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.top.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.left.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.right.second));

        // Pointer checks
        REQUIRE(worldNeighbors.front.first != nullptr);
        REQUIRE(worldNeighbors.back.first == nullptr);
        REQUIRE(worldNeighbors.top.first == nullptr);
        REQUIRE(worldNeighbors.bottom.first != nullptr);
        REQUIRE(worldNeighbors.left.first == nullptr);
        REQUIRE(worldNeighbors.right.first != nullptr);

        // World neighbor positions
        REQUIRE(worldNeighbors.front.second.x == -511);
        REQUIRE(worldNeighbors.front.second.y == 63);
        REQUIRE(worldNeighbors.front.second.z == -512);

        REQUIRE(worldNeighbors.back.second.x == -513);
        REQUIRE(worldNeighbors.back.second.y == 63);
        REQUIRE(worldNeighbors.back.second.z == -512);

        REQUIRE(worldNeighbors.top.second.x == -512);
        REQUIRE(worldNeighbors.top.second.y == 64);
        REQUIRE(worldNeighbors.top.second.z == -512);

        REQUIRE(worldNeighbors.bottom.second.x == -512);
        REQUIRE(worldNeighbors.bottom.second.y == 62);
        REQUIRE(worldNeighbors.bottom.second.z == -512);

        REQUIRE(worldNeighbors.left.second.x == -512);
        REQUIRE(worldNeighbors.left.second.y == 63);
        REQUIRE(worldNeighbors.left.second.z == -513);

        REQUIRE(worldNeighbors.right.second.x == -512);
        REQUIRE(worldNeighbors.right.second.y == 63);
        REQUIRE(worldNeighbors.right.second.z == -511);

        // Local positions
        // base => (x=-512, z=-512, y=63)
        //   => (cx=-32, cy=-32, x=0, z=0, y=63) with your floorDiv logic

        REQUIRE(localNeighbors.front.second.cx == -32);
        REQUIRE(localNeighbors.front.second.cy == -32);
        REQUIRE(localNeighbors.front.second.x == 1);
        REQUIRE(localNeighbors.front.second.y == 63);
        REQUIRE(localNeighbors.front.second.z == 0);

        REQUIRE(localNeighbors.back.second.cx == -33);
        REQUIRE(localNeighbors.back.second.cy == -32);
        REQUIRE(localNeighbors.back.second.x == 15);
        REQUIRE(localNeighbors.back.second.y == 63);
        REQUIRE(localNeighbors.back.second.z == 0);

        REQUIRE(localNeighbors.top.second.cx == -32);
        REQUIRE(localNeighbors.top.second.cy == -32);
        REQUIRE(localNeighbors.top.second.x == 0);
        REQUIRE(localNeighbors.top.second.y == 64);
        REQUIRE(localNeighbors.top.second.z == 0);

        REQUIRE(localNeighbors.bottom.second.cx == -32);
        REQUIRE(localNeighbors.bottom.second.cy == -32);
        REQUIRE(localNeighbors.bottom.second.x == 0);
        REQUIRE(localNeighbors.bottom.second.y == 62);
        REQUIRE(localNeighbors.bottom.second.z == 0);

        REQUIRE(localNeighbors.left.second.cx == -32);
        REQUIRE(localNeighbors.left.second.cy == -33);
        REQUIRE(localNeighbors.left.second.x == 0);
        REQUIRE(localNeighbors.left.second.y == 63);
        REQUIRE(localNeighbors.left.second.z == 15);

        REQUIRE(localNeighbors.right.second.cx == -32);
        REQUIRE(localNeighbors.right.second.cy == -32);
        REQUIRE(localNeighbors.right.second.x == 0);
        REQUIRE(localNeighbors.right.second.y == 63);
        REQUIRE(localNeighbors.right.second.z == 1);
        }
        {
        // Bottom Right Corner at y=63
        BTVUtils::WorldBlockPos wp;
        wp.x = 511;
        wp.y = 63;
        wp.z = -512;

        auto worldNeighbors = world.getNeighborsWorld(wp);
        auto localNeighbors = world.getNeighborsLocal(world.worldToLocal(wp));

        // Out-of-bounds checks:
        //   front   => x+1=512     => Out of bounds
        //   back    => x-1=510     => In bounds
        //   top     => y+1=64      => Out of bounds
        //   bottom  => y-1=62      => In bounds
        //   left    => z-1=-513    => Out of bounds
        //   right   => z+1=-511    => In bounds

        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.front.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.back.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.top.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.bottom.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(localNeighbors.left.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(localNeighbors.right.second));

        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.front.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.back.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.top.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.bottom.second));
        REQUIRE(BTVUtils::World::isOutOfBounds(worldNeighbors.left.second));
        REQUIRE(!BTVUtils::World::isOutOfBounds(worldNeighbors.right.second));

        // Pointer checks
        REQUIRE(worldNeighbors.front.first == nullptr);
        REQUIRE(worldNeighbors.back.first != nullptr);
        REQUIRE(worldNeighbors.top.first == nullptr);
        REQUIRE(worldNeighbors.bottom.first != nullptr);
        REQUIRE(worldNeighbors.left.first == nullptr);
        REQUIRE(worldNeighbors.right.first != nullptr);

        // World neighbor positions
        REQUIRE(worldNeighbors.front.second.x == 512);
        REQUIRE(worldNeighbors.front.second.y == 63);
        REQUIRE(worldNeighbors.front.second.z == -512);

        REQUIRE(worldNeighbors.back.second.x == 510);
        REQUIRE(worldNeighbors.back.second.y == 63);
        REQUIRE(worldNeighbors.back.second.z == -512);

        REQUIRE(worldNeighbors.top.second.x == 511);
        REQUIRE(worldNeighbors.top.second.y == 64);
        REQUIRE(worldNeighbors.top.second.z == -512);

        REQUIRE(worldNeighbors.bottom.second.x == 511);
        REQUIRE(worldNeighbors.bottom.second.y == 62);
        REQUIRE(worldNeighbors.bottom.second.z == -512);

        REQUIRE(worldNeighbors.left.second.x == 511);
        REQUIRE(worldNeighbors.left.second.y == 63);
        REQUIRE(worldNeighbors.left.second.z == -513);

        REQUIRE(worldNeighbors.right.second.x == 511);
        REQUIRE(worldNeighbors.right.second.y == 63);
        REQUIRE(worldNeighbors.right.second.z == -511);

        // Local positions
        // base => (x=511, z=-512, y=63)
        //   => (cx=31, cy=-32, x=15, z=0, y=63), with your floorDiv logic

        REQUIRE(localNeighbors.front.second.cx == 32);
        REQUIRE(localNeighbors.front.second.cy == -32);
        REQUIRE(localNeighbors.front.second.x == 0);
        REQUIRE(localNeighbors.front.second.y == 63);
        REQUIRE(localNeighbors.front.second.z == 0);

        REQUIRE(localNeighbors.back.second.cx == 31);
        REQUIRE(localNeighbors.back.second.cy == -32);
        REQUIRE(localNeighbors.back.second.x == 14);
        REQUIRE(localNeighbors.back.second.y == 63);
        REQUIRE(localNeighbors.back.second.z == 0);

        REQUIRE(localNeighbors.top.second.cx == 31);
        REQUIRE(localNeighbors.top.second.cy == -32);
        REQUIRE(localNeighbors.top.second.x == 15);
        REQUIRE(localNeighbors.top.second.y == 64);
        REQUIRE(localNeighbors.top.second.z == 0);

        REQUIRE(localNeighbors.bottom.second.cx == 31);
        REQUIRE(localNeighbors.bottom.second.cy == -32);
        REQUIRE(localNeighbors.bottom.second.x == 15);
        REQUIRE(localNeighbors.bottom.second.y == 62);
        REQUIRE(localNeighbors.bottom.second.z == 0);

        REQUIRE(localNeighbors.left.second.cx == 31);
        REQUIRE(localNeighbors.left.second.cy == -33);
        REQUIRE(localNeighbors.left.second.x == 15);
        REQUIRE(localNeighbors.left.second.y == 63);
        REQUIRE(localNeighbors.left.second.z == 15);

        REQUIRE(localNeighbors.right.second.cx == 31);
        REQUIRE(localNeighbors.right.second.cy == -32);
        REQUIRE(localNeighbors.right.second.x == 15);
        REQUIRE(localNeighbors.right.second.z == 1);
        REQUIRE(localNeighbors.right.second.y == 63);
      }
    }
  }
}


