/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/net/p2p/encoding.h"

// For coverage
namespace TP2PNodeInfo {
  TEST_CASE("P2P NodeInfo", "[p2p][nodeinfo]") {
    SECTION("NodeInfo Constructor") {
      Hash randomBlockHash = Hash::random();
      P2P::NodeID randomId(boost::asio::ip::address::from_string("127.0.0.1"), uint16_t(8000));
      P2P::NodeInfo emptyNode;
      P2P::NodeInfo node(uint64_t(1), uint64_t(15000), uint64_t(30000), uint64_t(5), uint64_t(12345), randomBlockHash, {randomId});
      REQUIRE(emptyNode.nodeVersion() == 0);
      REQUIRE(emptyNode.currentNodeTimestamp() == 0);
      REQUIRE(emptyNode.currentTimestamp() == 0);
      REQUIRE(emptyNode.timeDifference() == 0);
      REQUIRE(emptyNode.latestBlockHeight() == 0);
      REQUIRE(emptyNode.latestBlockHash() == Hash());
      REQUIRE(emptyNode.peers().empty());
      REQUIRE(node.nodeVersion() == 1);
      REQUIRE(node.currentNodeTimestamp() == 15000);
      REQUIRE(node.currentTimestamp() == 30000);
      REQUIRE(node.timeDifference() == 5);
      REQUIRE(node.latestBlockHeight() == 12345);
      REQUIRE(node.latestBlockHash() == randomBlockHash);
      REQUIRE(node.peers()[0] == randomId);
    }

    SECTION("NodeInfo operator==") {
      Hash randomBlockHash = Hash::random();
      P2P::NodeID randomId(boost::asio::ip::address::from_string("127.0.0.1"), uint16_t(8000));
      P2P::NodeID randomId2(boost::asio::ip::address::from_string("127.0.0.2"), uint16_t(8001));
      P2P::NodeInfo node1(uint64_t(1), uint64_t(15000), uint64_t(30000), uint64_t(5), uint64_t(12345), randomBlockHash, {randomId});
      P2P::NodeInfo node2(uint64_t(1), uint64_t(15000), uint64_t(30000), uint64_t(5), uint64_t(12345), randomBlockHash, {randomId});
      P2P::NodeInfo node3(uint64_t(2), uint64_t(1000), uint64_t(3000), uint64_t(4), uint64_t(54321), Hash::random(), {randomId2});
      REQUIRE(node1 == node2);
      REQUIRE_FALSE(node1 == node3);
    }
  }
}

