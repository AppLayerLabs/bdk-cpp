/*
Copyright (c) [2023-2024] [AppLayer Developers]
  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/contract/templates/btvenergy.h"
#include "../../src/contract/templates/btvplayer.h"
#include "../../src/contract/templates/btvcommon.h"
#include "../../src/contract/templates/buildthevoid.h"
#include "../sdktestsuite.hpp"
#include "contract/templates/btvproposals.h"


namespace TBuildTheVoid {
  TEST_CASE("Build The Void Tests", "[contract][buildthevoid]") {
    SECTION("BuildTheVoid Test Energy/Player/Proposals") {
      std::unique_ptr<Options> options = nullptr;
      Address playerAddress = Address();
      Address energyAddress = Address();
      Address proposalAddress = Address();
      Address transferDest1 = Address(Utils::randBytes(20));
      Address transferDest2 = Address(Utils::randBytes(20));
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

        // Try getting the player list of the chain owner
        auto playerListOfChainOwner = sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerTokens, sdk.getChainOwnerAccount().address);
        REQUIRE(playerListOfChainOwner.size() == 2);
        REQUIRE(std::find(playerListOfChainOwner.begin(), playerListOfChainOwner.end(), uint64_t(0)) != playerListOfChainOwner.end());
        REQUIRE(std::find(playerListOfChainOwner.begin(), playerListOfChainOwner.end(), uint64_t(1)) != playerListOfChainOwner.end());

        // Transfer the player to another address
        sdk.callFunction(playerAddress, &BTVPlayer::transferFrom, sdk.getChainOwnerAccount().address, transferDest1, uint256_t(0));
        sdk.callFunction(playerAddress, &BTVPlayer::transferFrom, sdk.getChainOwnerAccount().address, transferDest2, uint256_t(1));

        playerListOfChainOwner = sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerTokens, sdk.getChainOwnerAccount().address);
        auto playerListOfTransferDest1 = sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerTokens, transferDest1);
        auto playerListOfTransferDest2 = sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerTokens, transferDest2);
        REQUIRE(playerListOfChainOwner.size() == 0);
        REQUIRE(playerListOfTransferDest1.size() == 1);
        REQUIRE(playerListOfTransferDest2.size() == 1);

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
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::ownerOf, uint256_t(0)) == transferDest1);

      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerEnergy, uint64_t(1)) == uint256_t("0"));
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::playerExists, std::string("Bob")) == true);
      REQUIRE(sdk.callViewFunction(playerAddress, &BTVPlayer::ownerOf, uint256_t(1)) == transferDest2);

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

      auto playerListOfChainOwner = sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerTokens, sdk.getChainOwnerAccount().address);
      auto playerListOfTransferDest1 = sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerTokens, transferDest1);
      auto playerListOfTransferDest2 = sdk.callViewFunction(playerAddress, &BTVPlayer::getPlayerTokens, transferDest2);
      REQUIRE(playerListOfChainOwner.size() == 0);
      REQUIRE(playerListOfTransferDest1.size() == 1);
      REQUIRE(playerListOfTransferDest2.size() == 1);
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

      BTVUtils::Chunk* worldChunk00 = world.getChunk({0, 0});
      // Check if the 10x10 area at Y5 is SURFACE with NO PLACER
      for (int x = 0; x < 10; x++) {
        for (int z = 0; z < 10; z++) {
          REQUIRE(worldChunk00->blocks[x][5][z].type == BTVUtils::BlockType::SURFACE);
          REQUIRE_FALSE(worldChunk00->blocks[x][5][z].placer_.has_value());
        }
      }
      // Serialize and deserialize this specific chunk
      auto serializedChunk00 = worldChunk00->serialize();
      auto deserializedChunk00 = BTVUtils::Chunk::deserialize(serializedChunk00);
      REQUIRE(*worldChunk00 == deserializedChunk00);
      // Check if the 10x10 area at Y5 is SURFACE with NO PLACER
      for (int x = 0; x < 10; x++) {
        for (int z = 0; z < 10; z++) {
          REQUIRE(deserializedChunk00.blocks[x][5][z].type == BTVUtils::BlockType::SURFACE);
          REQUIRE_FALSE(deserializedChunk00.blocks[x][5][z].placer_.has_value());
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

    SECTION("BuildTheVoid Contract Testing") {
      std::unique_ptr<Options> options = nullptr;
      Address energyContract = Address();
      Address playerContract = Address();
      Address proposalContract = Address();
      Address btvContract = Address();
      BTVUtils::Chunk spawnChunk;
      {
        SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("TestBTVContract");
        options = std::make_unique<Options>(sdk.getOptions());
        playerContract = sdk.deployContract<BTVPlayer>(std::string("Players"), std::string("PLYS"));
        energyContract = sdk.deployContract<BTVEnergy>(std::string("Energy"), std::string("NRG"), uint8_t(18));
        proposalContract = sdk.deployContract<BTVProposals>();
        btvContract = sdk.deployContract<BuildTheVoid>();


        // Set proper ownership of the contracts
        sdk.callFunction(energyContract, &BTVEnergy::transferOwnership, btvContract);
        sdk.callFunction(playerContract, &BTVPlayer::setEnergyContract, energyContract);
        sdk.callFunction(playerContract, &BTVPlayer::setProposalContract, proposalContract);
        sdk.callFunction(playerContract, &BTVPlayer::setWorldContract, btvContract);

        sdk.callFunction(proposalContract, &BTVProposals::setEnergyContract, energyContract);
        sdk.callFunction(proposalContract, &BTVProposals::setPlayerContract, playerContract);
        sdk.callFunction(proposalContract, &BTVProposals::setProposalPrice, uint256_t("10000000000000000000")); // 10.00 NRG

        sdk.callFunction(btvContract, &BuildTheVoid::setEnergyContract, energyContract);
        sdk.callFunction(btvContract, &BuildTheVoid::setPlayerContract, playerContract);
        sdk.callFunction(btvContract, &BuildTheVoid::approve);


        // Check if the 0.0 chunk have the initial 10x10 platform
        spawnChunk = BTVUtils::Chunk::deserialize(sdk.callViewFunction(btvContract, &BuildTheVoid::getChunk, 0, 0));
        // Check if the 10x10 area at Y5 is SURFACE with NO PLACER
        for (int x = 0; x < 10; x++) {
          for (int z = 0; z < 10; z++) {
            REQUIRE(spawnChunk.blocks[x][5][z].type == BTVUtils::BlockType::SURFACE);
            REQUIRE_FALSE(spawnChunk.blocks[x][5][z].placer_.has_value());
          }
        }

        auto updateTx = sdk.callFunction(btvContract, &BuildTheVoid::forceUpdate);
        auto energyCreationEvent = sdk.getEventsEmittedByAddress(btvContract, &BuildTheVoid::BlockChanged);
        REQUIRE(energyCreationEvent.size() == 1);
        auto data = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t, BTVUtils::BlockType, uint64_t>(energyCreationEvent[0].getData());
        const auto& [energyPlacerId, energyX, energyY, energyZ, blockType, timestamp] = data;
        REQUIRE(energyPlacerId == std::numeric_limits<uint64_t>::max());
        REQUIRE(energyY == 6);
        // X and Y must be inside the 0...9 range
        REQUIRE(energyX >= 0);
        REQUIRE(energyX < 10);
        REQUIRE(energyZ >= 0);
        REQUIRE(energyZ < 10);
        REQUIRE(blockType == BTVUtils::BlockType::ENERGYCHEST);
        REQUIRE(timestamp == sdk.getLatestBlock()->getTimestamp());

        // Now, lets create (mint) a new player
        sdk.callFunction(playerContract, &BTVPlayer::mintPlayer, std::string("Alice"), sdk.getChainOwnerAccount().address);
        REQUIRE(sdk.callViewFunction(playerContract, &BTVPlayer::ownerOf, uint256_t(0)) == sdk.getChainOwnerAccount().address);
        REQUIRE(sdk.callViewFunction(playerContract, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == 0);
        REQUIRE(sdk.callViewFunction(playerContract, &BTVPlayer::getPlayerName, uint64_t(0)) == "Alice");

        // Try joining with energy (REQUIRE_THROW)
        REQUIRE_THROWS(sdk.callFunction(btvContract, &BuildTheVoid::loginPlayer, uint64_t(0), uint256_t("1000000000000000000")));
        // Try joining WITHOUT energy
        auto loginTx = sdk.callFunction(btvContract, &BuildTheVoid::loginPlayer, uint64_t(0), uint256_t("0"));
        auto loginEvent = sdk.getEventsEmittedByTx(loginTx, &BuildTheVoid::PlayerLogin);
        REQUIRE(loginEvent.size() == 1);
        auto loginData = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t>(loginEvent[0].getData());
        const auto& [loginPlayerId, loginX, loginY, loginZ] = loginData;
        REQUIRE(loginPlayerId == 0);
        REQUIRE(loginX == 4);
        REQUIRE(loginY == 6);
        REQUIRE(loginZ == 4);

        auto activePlayers = sdk.callViewFunction(btvContract, &BuildTheVoid::getActivePlayers);
        REQUIRE(activePlayers.size() == 1);
        const auto& [playerId, playerPoS, playerEnergy, lastUpdate] = activePlayers[0];
        const auto& [playerX, playerY, playerZ] = playerPoS;
        REQUIRE(playerId == 0);
        REQUIRE(playerX == 4);
        REQUIRE(playerY == 6);
        REQUIRE(playerZ == 4);
        REQUIRE(playerEnergy == 0);
        REQUIRE(lastUpdate == sdk.getLatestBlock()->getTimestamp());

        // Walk back to 1,6,1
        {
          auto walkTx = sdk.callFunction(btvContract, &BuildTheVoid::movePlayer, uint64_t(0), int32_t(1), int32_t(6), int32_t(1));
          auto walkEvent = sdk.getEventsEmittedByTx(walkTx, &BuildTheVoid::PlayerMoved);
          REQUIRE(walkEvent.size() == 1);
          auto walkData = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t>(walkEvent[0].getData());
          const auto& [walkPlayerId, walkX, walkY, walkZ] = walkData;
          REQUIRE(walkPlayerId == 0);
          REQUIRE(walkX == 1);
          REQUIRE(walkY == 6);
          REQUIRE(walkZ == 1);
        }

        // Try walking to 9x6x9 (REQUIRE_THROW)
        REQUIRE_THROWS(sdk.callFunction(btvContract, &BuildTheVoid::movePlayer, uint64_t(0), int32_t(9), int32_t(6), int32_t(9)));

        // Walk 30 blocks up.
        for (int i = 0; i < 30; i++) {
          auto walkTx = sdk.callFunction(btvContract, &BuildTheVoid::movePlayer, uint64_t(0), int32_t(1), int32_t(6 + i), int32_t(1));
          auto walkEvent = sdk.getEventsEmittedByTx(walkTx, &BuildTheVoid::PlayerMoved);
          REQUIRE(walkEvent.size() == 1);
          auto walkData = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t>(walkEvent[0].getData());
          const auto& [walkPlayerId, walkX, walkY, walkZ] = walkData;
          REQUIRE(walkPlayerId == 0);
          REQUIRE(walkX == 1);
          REQUIRE(walkY == 6 + i);
          REQUIRE(walkZ == 1);
        }

        // Try claiming the energy chest (REQUIRE_THROW)
        REQUIRE_THROWS(sdk.callFunction(btvContract, &BuildTheVoid::claimEnergy, uint64_t(0), energyX, energyY, energyZ));

        // Now, walk the 30 blocks back down.
        for (int i = 0; i < 30; i++) {
          auto walkTx = sdk.callFunction(btvContract, &BuildTheVoid::movePlayer, uint64_t(0), int32_t(1), int32_t(6 + 30 - i), int32_t(1));
          auto walkEvent = sdk.getEventsEmittedByTx(walkTx, &BuildTheVoid::PlayerMoved);
          REQUIRE(walkEvent.size() == 1);
          auto walkData = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t>(walkEvent[0].getData());
          const auto& [walkPlayerId, walkX, walkY, walkZ] = walkData;
          REQUIRE(walkPlayerId == 0);
          REQUIRE(walkX == 1);
          REQUIRE(walkY == 6 + 30 - i);
          REQUIRE(walkZ == 1);
        }

        // Try placing a block at 1, 8, 1 (REQUIRE_THROW) since user does not have energy
        REQUIRE_THROWS(sdk.callFunction(btvContract, &BuildTheVoid::changeBlock, uint64_t(0), int32_t(1), int32_t(8), int32_t(1), BTVUtils::BlockType::WALL));

        // Now, walk to the middle (4-6-5)
        {
          auto walkTx = sdk.callFunction(btvContract, &BuildTheVoid::movePlayer, uint64_t(0), int32_t(4), int32_t(6), int32_t(5));
          auto walkEvent = sdk.getEventsEmittedByTx(walkTx, &BuildTheVoid::PlayerMoved);
          REQUIRE(walkEvent.size() == 1);
          auto walkData = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t>(walkEvent[0].getData());
          const auto& [walkPlayerId, walkX, walkY, walkZ] = walkData;
          REQUIRE(walkPlayerId == 0);
          REQUIRE(walkX == 4);
          REQUIRE(walkY == 6);
          REQUIRE(walkZ == 5);
        }

        // After walking, claim the energy chest
        uint256_t energyValue;
        {
          auto claimTx = sdk.callFunction(btvContract, &BuildTheVoid::claimEnergy, uint64_t(0), energyX, energyY, energyZ);
          std::cout << "Claimed Energy: " << claimTx.hex().get() << std::endl;
          auto claimEvent = sdk.getEventsEmittedByTx(claimTx, &BuildTheVoid::ClaimedEnergy);
          REQUIRE(claimEvent.size() == 1);
          auto claimData = ABI::Decoder::decodeData<uint64_t, uint256_t>(claimEvent[0].getData());
          const auto& [claimPlayerId, claimValue] = claimData;
          energyValue = claimValue;
          // Get the active players list to check how much energy the player has
          auto activePlayers = sdk.callViewFunction(btvContract, &BuildTheVoid::getActivePlayers);
          REQUIRE(activePlayers.size() == 1);
          const auto& [playerId, playerPoS, playerEnergy, lastUpdate] = activePlayers[0];
          REQUIRE(playerId == 0);
          REQUIRE(playerEnergy == claimValue);
          REQUIRE(lastUpdate == sdk.getLatestBlock()->getTimestamp());

          REQUIRE(sdk.callViewFunction(energyContract, &BTVEnergy::balanceOf, btvContract) == claimValue);
          REQUIRE(sdk.callViewFunction(playerContract, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == 0); // Energy is only transferred when the player logs OUT
        }
        // Now, we should be able to place the block
        {
          auto changeBlockTx = sdk.callFunction(btvContract, &BuildTheVoid::changeBlock, uint64_t(0), int32_t(4), int32_t(8), int32_t(4), BTVUtils::BlockType::WALL);
          auto changeBlockEvent = sdk.getEventsEmittedByTx(changeBlockTx, &BuildTheVoid::BlockChanged);
          REQUIRE(changeBlockEvent.size() == 1);
          auto changeBlockData = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t, BTVUtils::BlockType, uint64_t>(changeBlockEvent[0].getData());
          const auto& [blockPlacerId, blockX, blockY, blockZ, blockType, blockTimestamp] = changeBlockData;
          REQUIRE(blockPlacerId == 0);
          REQUIRE(blockX == 4);
          REQUIRE(blockY == 8);
          REQUIRE(blockZ == 4);
          REQUIRE(blockType == BTVUtils::BlockType::WALL);
          REQUIRE(blockTimestamp == sdk.getLatestBlock()->getTimestamp());
          // Dont forget the check current player list to see if the energy was consumed
          auto activePlayers = sdk.callViewFunction(btvContract, &BuildTheVoid::getActivePlayers);
          REQUIRE(activePlayers.size() == 1);
          const auto& [playerId, playerPoS, playerEnergy, lastUpdate] = activePlayers[0];
          const auto& [playerX, playerY, playerZ] = playerPoS;
          REQUIRE(playerId == 0);
          REQUIRE(playerEnergy == energyValue - uint256_t("1000000000000000000")); // 1.00 NRG
          REQUIRE(lastUpdate == sdk.getLatestBlock()->getTimestamp());
          REQUIRE(sdk.callViewFunction(energyContract, &BTVEnergy::balanceOf, btvContract) == energyValue);
          REQUIRE(playerX == 4);
          REQUIRE(playerY == 6);
          REQUIRE(playerZ == 5);
        }
        {
          // Finally, logout the player
          auto logoutTx = sdk.callFunction(btvContract, &BuildTheVoid::logoutPlayer, uint64_t(0));
          auto logoutEvent = sdk.getEventsEmittedByTx(logoutTx, &BuildTheVoid::PlayerLogout);
          REQUIRE(logoutEvent.size() == 1);
          auto logoutData = ABI::Decoder::decodeData<uint64_t>(logoutEvent[0].getData());
          const auto& [logoutPlayerId] = logoutData;
          REQUIRE(logoutPlayerId == 0);
          // It should also have transfered the energy to the player
          REQUIRE(sdk.callViewFunction(playerContract, &BTVPlayer::getPlayerEnergy, uint64_t(0)) == energyValue - uint256_t("1000000000000000000"));
          REQUIRE(sdk.callViewFunction(energyContract, &BTVEnergy::balanceOf, playerContract) == energyValue - uint256_t("1000000000000000000"));
          // Active players must be empty
          REQUIRE(sdk.callViewFunction(btvContract, &BuildTheVoid::getActivePlayers).size() == 0);
          // We should be placed on the inactivePlayers and with 0 energy
          auto inactivePlayers = sdk.callViewFunction(btvContract, &BuildTheVoid::getInnactivePlayers);
          REQUIRE(inactivePlayers.size() == 1);
          const auto& [inactivePlayerId, inactivePlayerPoS, inactivePlayerEnergy, inactiveLastUpdate] = inactivePlayers[0];
          const auto& [inactivePlayerX, inactivePlayerY, inactivePlayerZ] = inactivePlayerPoS;
          REQUIRE(inactivePlayerId == 0);
          REQUIRE(inactivePlayerX == 4);
          REQUIRE(inactivePlayerY == 6);
          REQUIRE(inactivePlayerZ == 5);
          REQUIRE(inactivePlayerEnergy == 0);
          REQUIRE(inactiveLastUpdate == sdk.getLatestBlock()->getTimestamp());
        }

        // After logged out, moving the player or placing a block MUST THROW
        REQUIRE_THROWS(sdk.callFunction(btvContract, &BuildTheVoid::movePlayer, uint64_t(0), int32_t(1), int32_t(6), int32_t(1)));
        REQUIRE_THROWS(sdk.callFunction(btvContract, &BuildTheVoid::changeBlock, uint64_t(0), int32_t(5), int32_t(8), int32_t(5), BTVUtils::BlockType::WALL));

        spawnChunk = BTVUtils::Chunk::deserialize(sdk.callViewFunction(btvContract, &BuildTheVoid::getChunk, 0, 0));
        std::cout << "Ok" << std::endl;
        sdk.getState().saveToDB();
      }

      SDKTestSuite sdk(*options);

      auto newSpawnChunk = BTVUtils::Chunk::deserialize(sdk.callViewFunction(btvContract, &BuildTheVoid::getChunk, 0, 0));
      // Block 4,8,4 must be a WALL
      REQUIRE(newSpawnChunk.blocks[4][8][4].type == BTVUtils::BlockType::WALL);
      // And a 10x10 SURFACE platform at Y=5
      for (int x = 0; x < 10; x++) {
        for (int z = 0; z < 10; z++) {
          REQUIRE(newSpawnChunk.blocks[x][5][z].type == BTVUtils::BlockType::SURFACE);
          REQUIRE_FALSE(newSpawnChunk.blocks[x][5][z].placer_.has_value());
        }
      }
      REQUIRE (spawnChunk == newSpawnChunk);


    }

    SECTION("BuildTheVoid Entire World Wall Placing + Serialization") {
      BTVUtils::World world;
      // Now, lets place a huge wall (Y: 0 to 63) at X: 0, Z: 0 to X 250 Z 0
      // and a wall at X: 250, Z: 0 to X: 250, Z: 250
      auto timestampInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 251; x++) {
          auto block = world.getBlock(BTVUtils::WorldBlockPos{x, y, 0});
          block->type = BTVUtils::BlockType::WALL;
          block->modificationTimestamp = timestampInMicroseconds;
        }
      }
      for (int y = 0; y < 64; y++) {
        for (int z = 0; z < 251; z++) {
          auto block = world.getBlock(BTVUtils::WorldBlockPos{250, y, z});
          block->type = BTVUtils::BlockType::WALL;
          block->modificationTimestamp = timestampInMicroseconds;
        }
      }

      // Now, we check both walls to see if they were placed correctly
      for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 251; x++) {
          auto block = world.getBlock(BTVUtils::WorldBlockPos{x, y, 0});
          REQUIRE(block->type == BTVUtils::BlockType::WALL);
          REQUIRE(block->modificationTimestamp == timestampInMicroseconds);
          // Calculate the local position
          auto localPos = world.worldToLocal(BTVUtils::WorldBlockPos{x, y, 0});
          REQUIRE(world.getChunk({localPos.cx, localPos.cy})->blocks[localPos.x][localPos.y][localPos.z].type == BTVUtils::BlockType::WALL);
          // Make sure that both pointers are pointing to the same block
          REQUIRE(world.getBlock(BTVUtils::WorldBlockPos{x, y, 0}) == &world.getChunk({localPos.cx, localPos.cy})->blocks[localPos.x][localPos.y][localPos.z]);
        }
      }
      for (int y = 0; y < 64; y++) {
        for (int z = 0; z < 251; z++) {
          auto block = world.getBlock(BTVUtils::WorldBlockPos{250, y, z});
          REQUIRE(block->type == BTVUtils::BlockType::WALL);
          REQUIRE(block->modificationTimestamp == timestampInMicroseconds);
          // Calculate the local position
          auto localPos = world.worldToLocal(BTVUtils::WorldBlockPos{250, y, z});
          REQUIRE(world.getChunk({localPos.cx, localPos.cy})->blocks[localPos.x][localPos.y][localPos.z].type == BTVUtils::BlockType::WALL);
          // Make sure that both pointers are pointing to the same block
          REQUIRE(world.getBlock(BTVUtils::WorldBlockPos{250, y, z}) == &world.getChunk({localPos.cx, localPos.cy})->blocks[localPos.x][localPos.y][localPos.z]);
        }
      }
      // Let's deserialize all the chunks and check if the blocks are the same
      std::unordered_map<BTVUtils::ChunkCoord2D, Bytes, SafeHash> serializedChunks;
      for (const auto& [key, chunk] : world.getChunks()) {
        serializedChunks[key] = chunk.serialize();
      }
      BTVUtils::World deserializedWorld;
      deserializedWorld.getChunks().clear();
      for (const auto& [key, serializedChunk] : serializedChunks) {
        deserializedWorld.getChunks().emplace(key, BTVUtils::Chunk::deserialize(serializedChunk));
      }
      // Now, we check if the chunks are the same
      REQUIRE(world.getChunks().size() == deserializedWorld.getChunks().size());
      REQUIRE(world.getChunks().size() == 4096);
      REQUIRE(world.getChunks() == deserializedWorld.getChunks());
    }
  }
}


