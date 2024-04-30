/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/net/p2p/managernormal.h"
#include "../../src/net/p2p/managerdiscovery.h"
#include "../../src/core/rdpos.h"
#include "../../src/core/storage.h"
#include "../../src/core/state.h"
#include "../../src/utils/db.h"
#include "../../blockchainwrapper.hpp"

using Catch::Matchers::Equals;

// Blockchain wrapper initializer for testing purposes.
// Defined in rdpos.cpp
TestBlockchainWrapper initialize(const std::vector<Hash>& validatorPrivKeys,
                                 const PrivKey& validatorKey,
                                 const uint64_t& serverPort,
                                 bool clearDb,
                                 const std::string& folderName);

// This creates a valid block given the state within the rdPoS class.
// Should not be used during network/thread testing, as it will automatically sign all TxValidator transactions within the block
// And that is not the purpose of network/thread testing.
// Definition from state.cpp, when linking, the compiler should find the function.
FinalizedBlock createValidBlock(const std::vector<Hash>& validatorPrivKeys, State& state, Storage& storage, const std::vector<TxBlock>& txs = {});

namespace TP2P {

  const std::vector<Hash> validatorPrivKeysP2P {
    Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
    Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
    Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
    Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
    Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
    Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
    Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
    Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
  };

  std::string testDumpPath = Utils::getTestDumpPath();

  TEST_CASE("P2P Manager", "[p2p]") {

    SECTION("2 Node Network, Syncer") {

      /// Make blockchainWrapper be 10 blocks ahead
      auto blockchainWrapper = initialize(validatorPrivKeysP2P, validatorPrivKeysP2P[0], 8080, true, testDumpPath + "/p2pRequestBlockNode1");
      for (uint64_t index = 0; index < 10; ++index) {
        std::vector<TxBlock> txs;
        auto newBestBlock = createValidBlock(validatorPrivKeysP2P, blockchainWrapper.state, blockchainWrapper.storage, txs);
        REQUIRE_NOTHROW(blockchainWrapper.state.processNextBlock(std::move(newBestBlock))); // Throws if block is invalid
      }
      REQUIRE(blockchainWrapper.storage.latest()->getNHeight() == 10);

      /// Create a blockchaiNWrapper2 with zero blocks
      auto blockchainWrapper2 = initialize(validatorPrivKeysP2P, PrivKey(), 8081, true, testDumpPath + "/p2pRequestBlockNode2");

      /// Start the servers and connect them
      blockchainWrapper.p2p.start();
      blockchainWrapper2.p2p.start();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      blockchainWrapper.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8081);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      REQUIRE(blockchainWrapper.p2p.getSessionsIDs().size() == 1);

      /// Run blockchainWrapper2's Syncer
      REQUIRE(blockchainWrapper2.syncer.sync(1)); // Abort on first download failure (which should never happen normally)
      REQUIRE(blockchainWrapper2.storage.latest()->getNHeight() == 10);
    }

    SECTION ("P2P::Manager Simple 3 node network") {

      auto blockchainWrapper1 = initialize(validatorPrivKeysP2P, PrivKey(), 8080, true, testDumpPath + "/testP2PManagerSimpleNetworkNode1");
      auto blockchainWrapper2 = initialize(validatorPrivKeysP2P, PrivKey(), 8081, true, testDumpPath + "/testP2PManagerSimpleNetworkNode2");
      auto blockchainWrapper3 = initialize(validatorPrivKeysP2P, PrivKey(), 8082, true, testDumpPath + "/testP2PManagerSimpleNetworkNode3");

      P2P::NodeID node1Id = { boost::asio::ip::address::from_string("127.0.0.1"), 8080 };
      P2P::NodeID node2Id = { boost::asio::ip::address::from_string("127.0.0.1"), 8081 };
      P2P::NodeID node3Id = { boost::asio::ip::address::from_string("127.0.0.1"), 8082 };

      blockchainWrapper1.p2p.start();
      blockchainWrapper2.p2p.start();
      blockchainWrapper3.p2p.start();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      REQUIRE(blockchainWrapper1.p2p.isServerRunning() == true);
      REQUIRE(blockchainWrapper2.p2p.isServerRunning() == true);
      REQUIRE(blockchainWrapper3.p2p.isServerRunning() == true);

      blockchainWrapper1.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8081);
      blockchainWrapper1.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8082);
      blockchainWrapper2.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8082);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Start discovery
      blockchainWrapper1.p2p.startDiscovery();
      blockchainWrapper2.p2p.startDiscovery();
      blockchainWrapper3.p2p.startDiscovery();

      auto node1SessionsIDs = blockchainWrapper1.p2p.getSessionsIDs();
      auto node2SessionsIDs = blockchainWrapper2.p2p.getSessionsIDs();
      auto node3SessionsIDs = blockchainWrapper3.p2p.getSessionsIDs();

      REQUIRE(node1SessionsIDs.size() == 2);
      REQUIRE(node2SessionsIDs.size() == 2);
      REQUIRE(node3SessionsIDs.size() == 2);

      // Try pinging each other
      for (auto session : node1SessionsIDs) {
        blockchainWrapper1.p2p.ping(session);
      }

      for (auto session : node2SessionsIDs) {
        blockchainWrapper2.p2p.ping(session);
      }

      for (auto session : node3SessionsIDs) {
        blockchainWrapper3.p2p.ping(session);
      }

      // Stop discovery on nodes, disconnect and check.
      blockchainWrapper1.p2p.stopDiscovery();
      blockchainWrapper2.p2p.stopDiscovery();
      blockchainWrapper3.p2p.stopDiscovery();
      blockchainWrapper1.p2p.disconnectSession(node2Id);


      auto futureSessionNode1 = std::async(std::launch::async, [&]() {
        while (blockchainWrapper1.p2p.getSessionsIDs().size() != 1) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
      REQUIRE(futureSessionNode1.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      auto futureSessionNode2 = std::async(std::launch::async, [&]() {
        while (blockchainWrapper2.p2p.getSessionsIDs().size() != 1) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
      REQUIRE(futureSessionNode2.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);


      node1SessionsIDs = blockchainWrapper1.p2p.getSessionsIDs();
      node2SessionsIDs = blockchainWrapper2.p2p.getSessionsIDs();
      node3SessionsIDs = blockchainWrapper3.p2p.getSessionsIDs();

      REQUIRE(node1SessionsIDs.size() == 1);
      REQUIRE(node2SessionsIDs.size() == 1);
      REQUIRE(node3SessionsIDs.size() == 2);

      // Request Nodes from Node 3.
      auto nodesFromNode1 = blockchainWrapper3.p2p.requestNodes(node1Id);
      auto nodesFromNode2 = blockchainWrapper3.p2p.requestNodes(node2Id);

      REQUIRE(nodesFromNode1 == nodesFromNode2); // Node 1 and Node 2 should have the same nodes (only connected to the same node 3)

      // Start discovery, should recover the lost connection
      blockchainWrapper1.p2p.startDiscovery();
      blockchainWrapper2.p2p.startDiscovery();
      blockchainWrapper3.p2p.startDiscovery();

      auto futureSessionNode1AfterDiscovery = std::async(std::launch::async, [&]() {
        while (blockchainWrapper1.p2p.getSessionsIDs().size() != 2) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
      REQUIRE(futureSessionNode1AfterDiscovery.wait_for(std::chrono::seconds(10)) != std::future_status::timeout);

      auto futureSessionNode2AfterDiscovery = std::async(std::launch::async, [&]() {
        while (blockchainWrapper2.p2p.getSessionsIDs().size() != 2) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
      REQUIRE(futureSessionNode2AfterDiscovery.wait_for(std::chrono::seconds(10)) != std::future_status::timeout);

      node1SessionsIDs = blockchainWrapper1.p2p.getSessionsIDs();
      node2SessionsIDs = blockchainWrapper2.p2p.getSessionsIDs();
      node3SessionsIDs = blockchainWrapper3.p2p.getSessionsIDs();

      REQUIRE(node1SessionsIDs.size() == 2);
      REQUIRE(node2SessionsIDs.size() == 2);
      REQUIRE(node3SessionsIDs.size() == 2);

      // Try pinging again each other again.
      for (auto session : node1SessionsIDs) {
        blockchainWrapper1.p2p.ping(session);
      }

      for (auto session : node2SessionsIDs) {
        blockchainWrapper2.p2p.ping(session);
      }

      for (auto session : node3SessionsIDs) {
        blockchainWrapper3.p2p.ping(session);
      }
      // Stop the servers
      blockchainWrapper1.p2p.stop();
      blockchainWrapper2.p2p.stop();
      blockchainWrapper3.p2p.stop();


      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 0);

      REQUIRE(blockchainWrapper1.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper2.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper3.p2p.isServerRunning() == false);
    }

    SECTION("2 Node Network, request info") {
      auto blockchainWrapper1 = initialize(validatorPrivKeysP2P, PrivKey(), 8080, true, testDumpPath + "/p2pRequestInfoNode1");

      auto blockchainWrapper2 = initialize(validatorPrivKeysP2P, PrivKey(), 8081, true, testDumpPath + "/p2pRequestInfoNode2");

      /// Start the servers
      blockchainWrapper1.p2p.start();
      blockchainWrapper2.p2p.start();

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      /// Connect to each other
      blockchainWrapper1.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8081);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 1);

      auto p2p2NodeId = blockchainWrapper1.p2p.getSessionsIDs()[0];

      auto p2p2NodeInfo = blockchainWrapper1.p2p.requestNodeInfo(p2p2NodeId);

      REQUIRE(p2p2NodeInfo.nodeVersion() == blockchainWrapper2.options.getVersion());
      REQUIRE(p2p2NodeInfo.latestBlockHeight() == blockchainWrapper2.storage.latest()->getNHeight());
      REQUIRE(p2p2NodeInfo.latestBlockHash() == blockchainWrapper2.storage.latest()->getHash());
    }

    SECTION("10 P2P::ManagerNormal 1 P2P::ManagerDiscovery") {
      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      uint64_t genesisTimestamp = 1678887538000000;
      MutableBlock genesisMutable(Hash(), 0, 0);
      FinalizedBlock genesis = genesisMutable.finalize(genesisPrivKey, genesisTimestamp);
      std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
      std::vector<Address> genesisValidators;
      for (const auto& privKey : validatorPrivKeysP2P) {
        genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
      }
      Options discoveryOptions(
        testDumpPath + "/stateDiscoveryNodeNetworkCapabilities",
        "BDK/cpp/linux_x86-64/0.2.0",
        1,
        8080,
        Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
        boost::asio::ip::address::from_string("127.0.0.1"),
        8090,
        9999,
        11,
        11,
        200,
        50,
        2000,
        10000,
        1000,
        4,
        discoveryNodes,
        genesis,
        genesisTimestamp,
        genesisPrivKey,
        genesisBalances,
        genesisValidators
      );

      P2P::ManagerDiscovery p2pDiscoveryNode(boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);
      auto blockchainWrapper1 = initialize(validatorPrivKeysP2P, PrivKey(), 8080, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode1");
      auto blockchainWrapper2 = initialize(validatorPrivKeysP2P, PrivKey(), 8081, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode2");
      auto blockchainWrapper3 = initialize(validatorPrivKeysP2P, PrivKey(), 8082, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode3");
      auto blockchainWrapper4 = initialize(validatorPrivKeysP2P, PrivKey(), 8083, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode4");
      auto blockchainWrapper5 = initialize(validatorPrivKeysP2P, PrivKey(), 8084, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode5");
      auto blockchainWrapper6 = initialize(validatorPrivKeysP2P, PrivKey(), 8085, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode6");
      auto blockchainWrapper7 = initialize(validatorPrivKeysP2P, PrivKey(), 8086, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode7");
      auto blockchainWrapper8 = initialize(validatorPrivKeysP2P, PrivKey(), 8087, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode8");
      auto blockchainWrapper9 = initialize(validatorPrivKeysP2P, PrivKey(), 8088, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode9");
      auto blockchainWrapper10 = initialize(validatorPrivKeysP2P, PrivKey(), 8089, true, testDumpPath + "/testP2PManagerDiscoveryNetworkNode10");

      p2pDiscoveryNode.start();
      blockchainWrapper1.p2p.start();
      blockchainWrapper2.p2p.start();
      blockchainWrapper3.p2p.start();
      blockchainWrapper4.p2p.start();
      blockchainWrapper5.p2p.start();
      blockchainWrapper6.p2p.start();
      blockchainWrapper7.p2p.start();
      blockchainWrapper8.p2p.start();
      blockchainWrapper9.p2p.start();
      blockchainWrapper10.p2p.start();

      blockchainWrapper1.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper2.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper3.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper4.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper5.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper6.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper7.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper8.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper9.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper10.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

      // Wait until all peers are connected to the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Start discovery
      p2pDiscoveryNode.startDiscovery();
      blockchainWrapper1.p2p.startDiscovery();
      blockchainWrapper2.p2p.startDiscovery();
      blockchainWrapper3.p2p.startDiscovery();
      blockchainWrapper4.p2p.startDiscovery();
      blockchainWrapper5.p2p.startDiscovery();
      blockchainWrapper6.p2p.startDiscovery();
      blockchainWrapper7.p2p.startDiscovery();
      blockchainWrapper8.p2p.startDiscovery();
      blockchainWrapper9.p2p.startDiscovery();
      blockchainWrapper10.p2p.startDiscovery();

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      auto futureWaitAllNodesConnected = std::async(std::launch::async, [&]() {
        while(p2pDiscoveryNode.getSessionsIDs().size() != 10 ||
              blockchainWrapper1.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper2.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper3.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper4.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper5.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper6.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper7.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper8.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper9.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper10.p2p.getSessionsIDs().size() != 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
      });

      REQUIRE(futureWaitAllNodesConnected.wait_for(std::chrono::seconds(10)) != std::future_status::timeout);

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      auto nodeDiscoverySessionsIDs = p2pDiscoveryNode.getSessionsIDs();
      auto node1SessionsIDs = blockchainWrapper1.p2p.getSessionsIDs();
      auto node2SessionsIDs = blockchainWrapper2.p2p.getSessionsIDs();
      auto node3SessionsIDs = blockchainWrapper3.p2p.getSessionsIDs();
      auto node4SessionsIDs = blockchainWrapper4.p2p.getSessionsIDs();
      auto node5SessionsIDs = blockchainWrapper5.p2p.getSessionsIDs();
      auto node6SessionsIDs = blockchainWrapper6.p2p.getSessionsIDs();
      auto node7SessionsIDs = blockchainWrapper7.p2p.getSessionsIDs();
      auto node8SessionsIDs = blockchainWrapper8.p2p.getSessionsIDs();
      auto node9SessionsIDs = blockchainWrapper9.p2p.getSessionsIDs();
      auto node10SessionsIDs = blockchainWrapper10.p2p.getSessionsIDs();

      REQUIRE(nodeDiscoverySessionsIDs.size() == 10);
      REQUIRE(node1SessionsIDs.size() == 10);
      REQUIRE(node2SessionsIDs.size() == 10);
      REQUIRE(node3SessionsIDs.size() == 10);
      REQUIRE(node4SessionsIDs.size() == 10);
      REQUIRE(node5SessionsIDs.size() == 10);
      REQUIRE(node6SessionsIDs.size() == 10);
      REQUIRE(node7SessionsIDs.size() == 10);
      REQUIRE(node8SessionsIDs.size() == 10);
      REQUIRE(node9SessionsIDs.size() == 10);
      REQUIRE(node10SessionsIDs.size() == 10);
      // Try pinging each other.

      for (auto session : nodeDiscoverySessionsIDs) {
        p2pDiscoveryNode.ping(session);
      }

      for (auto session : node1SessionsIDs) {
        blockchainWrapper1.p2p.ping(session);
      }

      for (auto session : node2SessionsIDs) {
        blockchainWrapper2.p2p.ping(session);
      }

      for (auto session : node3SessionsIDs) {
        blockchainWrapper3.p2p.ping(session);
      }

      for (auto session : node4SessionsIDs) {
        blockchainWrapper4.p2p.ping(session);
      }

      for (auto session : node5SessionsIDs) {
        blockchainWrapper5.p2p.ping(session);
      }

      for (auto session : node6SessionsIDs) {
        blockchainWrapper6.p2p.ping(session);
      }

      for (auto session : node7SessionsIDs) {
        blockchainWrapper7.p2p.ping(session);
      }

      for (auto session : node8SessionsIDs) {
        blockchainWrapper8.p2p.ping(session);
      }

      for (auto session : node9SessionsIDs) {
        blockchainWrapper9.p2p.ping(session);
      }

      for (auto session : node10SessionsIDs) {
        blockchainWrapper10.p2p.ping(session);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Close all the nodes.
      p2pDiscoveryNode.stop();
      blockchainWrapper1.p2p.stop();
      blockchainWrapper2.p2p.stop();
      blockchainWrapper3.p2p.stop();
      blockchainWrapper4.p2p.stop();
      blockchainWrapper5.p2p.stop();
      blockchainWrapper6.p2p.stop();
      blockchainWrapper7.p2p.stop();
      blockchainWrapper8.p2p.stop();
      blockchainWrapper9.p2p.stop();
      blockchainWrapper10.p2p.stop();

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      REQUIRE(p2pDiscoveryNode.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper4.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper5.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper6.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper7.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper8.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper9.p2p.getSessionsIDs().size() == 0);
      REQUIRE(blockchainWrapper10.p2p.getSessionsIDs().size() == 0);

      REQUIRE(p2pDiscoveryNode.isServerRunning() == false);
      REQUIRE(blockchainWrapper1.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper2.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper3.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper4.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper5.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper6.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper7.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper8.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper9.p2p.isServerRunning() == false);
      REQUIRE(blockchainWrapper10.p2p.isServerRunning() == false);
    }
  }
};
