/*
Copyright (c) [2023] [Sparq Network]

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

using Catch::Matchers::Equals;

namespace TP2P {

  const std::vector<Hash> validatorPrivKeys {
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
  void initializeOptions(std::unique_ptr<Options>& options, std::string folderPath, uint64_t serverPort) {
    std::vector<std::pair<boost::asio::ip::address, uint64_t>> peers;
    PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    uint64_t genesisTimestamp = 1678887538000000;
    Block genesis(Hash(), 0, 0);
    genesis.finalize(genesisPrivKey, genesisTimestamp);
    std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
    std::vector<Address> genesisValidators;
    for (const auto& privKey : validatorPrivKeys) {
      genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
    }
    options = std::make_unique<Options>(
      folderPath,
      "OrbiterSDK/cpp/linux_x86-64/0.1.2",
      1,
      8080,
      Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
      serverPort,
      9999,
      2000,
      10000,
      peers,
      genesis,
      genesisTimestamp,
      genesisPrivKey,
      genesisBalances,
      genesisValidators
    );
  }

  // We initialize the blockchain database
  // To make sure that if the genesis is changed within the main source code
  // The tests will still work, as tests uses own genesis block.
  void initializeFullChain(std::unique_ptr<DB>& db,
                           std::unique_ptr<Storage>& storage,
                           std::unique_ptr<P2P::ManagerNormal>& p2p,
                           std::unique_ptr<rdPoS>& rdpos,
                           std::unique_ptr<State>& state,
                           std::unique_ptr<Options>& options,
                           PrivKey validatorKey,
                           uint64_t serverPort,
                           bool clearDb,
                           std::string folderName) {
    std::string dbName = folderName + "/db";
    if (clearDb) {
      if (std::filesystem::exists(dbName)) {
        std::filesystem::remove_all(dbName);
      }
      if(std::filesystem::exists(dbName + "/options.json")) {
        std::filesystem::remove(dbName + "/options.json");
      }
    }
    db = std::make_unique<DB>(dbName);
    std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
    PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    uint64_t genesisTimestamp = 1678887538000000;
    Block genesis(Hash(), 0, 0);
    genesis.finalize(genesisPrivKey, genesisTimestamp);
    std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
    std::vector<Address> genesisValidators;
    for (const auto& privKey : validatorPrivKeys) {
      genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
    }
    if (!validatorKey) {
      options = std::make_unique<Options>(
        folderName,
        "OrbiterSDK/cpp/linux_x86-64/0.1.2",
        1,
        8080,
        Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
        serverPort,
        9999,
        2000,
        10000,
        discoveryNodes,
        genesis,
        genesisTimestamp,
        genesisPrivKey,
        genesisBalances,
        genesisValidators
      );
    } else {
      options = std::make_unique<Options>(
        folderName,
        "OrbiterSDK/cpp/linux_x86-64/0.1.2",
        1,
        8080,
        Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
        serverPort,
        9999,
        2000,
        10000,
        discoveryNodes,
        genesis,
        genesisTimestamp,
        genesisPrivKey,
        genesisBalances,
        genesisValidators,
        validatorKey
      );
    }

    storage = std::make_unique<Storage>(db, options);
    p2p = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), rdpos, options, storage, state);
    rdpos = std::make_unique<rdPoS>(db, storage, p2p, options, state);
    state = std::make_unique<State>(db, storage, rdpos, p2p, options);
  }

  TEST_CASE("P2P Manager", "[p2p]") {
    SECTION ("P2P::Manager Simple 3 node network") {
      std::unique_ptr<Options> options1;
      std::unique_ptr<Options> options2;
      std::unique_ptr<Options> options3;
      initializeOptions(options1, testDumpPath + "/testP2PManagerSimpleNetworkNode1", 8080);
      initializeOptions(options2, testDumpPath + "/testP2PManagerSimpleNetworkNode2", 8081);
      initializeOptions(options3, testDumpPath + "/testP2PManagerSimpleNetworkNode3", 8082);
      P2P::ManagerNormal p2pNode1(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options1, nullptr, nullptr);
      P2P::ManagerNormal p2pNode2(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options2, nullptr, nullptr);
      P2P::ManagerNormal p2pNode3(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options3, nullptr, nullptr);

      P2P::NodeID node1Id = { boost::asio::ip::address::from_string("127.0.0.1"), 8080 };
      P2P::NodeID node2Id = { boost::asio::ip::address::from_string("127.0.0.1"), 8081 };
      P2P::NodeID node3Id = { boost::asio::ip::address::from_string("127.0.0.1"), 8082 };

      p2pNode1.start();
      p2pNode2.start();
      p2pNode3.start();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      REQUIRE(p2pNode1.isServerRunning() == true);
      REQUIRE(p2pNode2.isServerRunning() == true);
      REQUIRE(p2pNode3.isServerRunning() == true);

      p2pNode1.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8081);
      p2pNode1.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8082);
      p2pNode2.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8082);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Start discovery
      p2pNode1.startDiscovery();
      p2pNode2.startDiscovery();
      p2pNode3.startDiscovery();

      auto node1SessionsIDs = p2pNode1.getSessionsIDs();
      auto node2SessionsIDs = p2pNode2.getSessionsIDs();
      auto node3SessionsIDs = p2pNode3.getSessionsIDs();

      REQUIRE(node1SessionsIDs.size() == 2);
      REQUIRE(node2SessionsIDs.size() == 2);
      REQUIRE(node3SessionsIDs.size() == 2);

      // Try pinging each other
      for (auto session : node1SessionsIDs) {
        p2pNode1.ping(session);
      }

      for (auto session : node2SessionsIDs) {
        p2pNode2.ping(session);
      }

      for (auto session : node3SessionsIDs) {
        p2pNode3.ping(session);
      }

      // Stop discovery on nodes, disconnect and check.
      p2pNode1.stopDiscovery();
      p2pNode2.stopDiscovery();
      p2pNode3.stopDiscovery();
      p2pNode1.disconnectSession(node2Id);


      auto futureSessionNode1 = std::async(std::launch::async, [&]() {
        while (p2pNode1.getSessionsIDs().size() != 1) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
      REQUIRE(futureSessionNode1.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      auto futureSessionNode2 = std::async(std::launch::async, [&]() {
        while (p2pNode2.getSessionsIDs().size() != 1) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
      REQUIRE(futureSessionNode2.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);


      node1SessionsIDs = p2pNode1.getSessionsIDs();
      node2SessionsIDs = p2pNode2.getSessionsIDs();
      node3SessionsIDs = p2pNode3.getSessionsIDs();

      REQUIRE(node1SessionsIDs.size() == 1);
      REQUIRE(node2SessionsIDs.size() == 1);
      REQUIRE(node3SessionsIDs.size() == 2);

      // Request Nodes from Node 3.
      auto nodesFromNode1 = p2pNode3.requestNodes(node1Id);
      auto nodesFromNode2 = p2pNode3.requestNodes(node2Id);

      REQUIRE(nodesFromNode1 == nodesFromNode2); // Node 1 and Node 2 should have the same nodes (only connected to the same node 3)

      // Start discovery, should recover the lost connection
      p2pNode1.startDiscovery();
      p2pNode2.startDiscovery();
      p2pNode3.startDiscovery();

      auto futureSessionNode1AfterDiscovery = std::async(std::launch::async, [&]() {
        while (p2pNode1.getSessionsIDs().size() != 2) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
      REQUIRE(futureSessionNode1AfterDiscovery.wait_for(std::chrono::seconds(10)) != std::future_status::timeout);

      auto futureSessionNode2AfterDiscovery = std::async(std::launch::async, [&]() {
        while (p2pNode2.getSessionsIDs().size() != 2) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
      REQUIRE(futureSessionNode2AfterDiscovery.wait_for(std::chrono::seconds(10)) != std::future_status::timeout);

      node1SessionsIDs = p2pNode1.getSessionsIDs();
      node2SessionsIDs = p2pNode2.getSessionsIDs();
      node3SessionsIDs = p2pNode3.getSessionsIDs();

      REQUIRE(node1SessionsIDs.size() == 2);
      REQUIRE(node2SessionsIDs.size() == 2);
      REQUIRE(node3SessionsIDs.size() == 2);

      // Try pinging again each other again.
      for (auto session : node1SessionsIDs) {
        p2pNode1.ping(session);
      }

      for (auto session : node2SessionsIDs) {
        p2pNode2.ping(session);
      }

      for (auto session : node3SessionsIDs) {
        p2pNode3.ping(session);
      }
      // Stop the servers
      p2pNode1.stop();
      p2pNode2.stop();
      p2pNode3.stop();


      REQUIRE(p2pNode1.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode2.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode3.getSessionsIDs().size() == 0);

      REQUIRE(p2pNode1.isServerRunning() == false);
      REQUIRE(p2pNode2.isServerRunning() == false);
      REQUIRE(p2pNode3.isServerRunning() == false);
    }

    SECTION("2 Node Network, request info") {
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<State> state1;
      std::unique_ptr<Options> options1;
      initializeFullChain(db1, storage1, p2p1, rdpos1, state1, options1, PrivKey(), 8080, true, testDumpPath + "/p2pRequestInfoNode1");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<State> state2;
      std::unique_ptr<Options> options2;
      initializeFullChain(db2, storage2, p2p2, rdpos2, state2, options2, PrivKey(), 8081, true, testDumpPath + "/p2pRequestInfoNode2");

      /// Start the servers
      p2p1->start();
      p2p2->start();

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      /// Connect to each other
      p2p1->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8081);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      REQUIRE(p2p1->getSessionsIDs().size() == 1);

      auto p2p2NodeId = p2p1->getSessionsIDs()[0];

      auto p2p2NodeInfo = p2p1->requestNodeInfo(p2p2NodeId);

      REQUIRE(p2p2NodeInfo.nodeVersion == options2->getVersion());
      REQUIRE(p2p2NodeInfo.latestBlockHeight == storage2->latest()->getNHeight());
      REQUIRE(p2p2NodeInfo.latestBlockHash == storage2->latest()->hash());
    }

    SECTION("10 P2P::ManagerNormal 1 P2P::ManagerDiscovery") {
      std::unique_ptr<Options> options1;
      std::unique_ptr<Options> options2;
      std::unique_ptr<Options> options3;
      std::unique_ptr<Options> options4;
      std::unique_ptr<Options> options5;
      std::unique_ptr<Options> options6;
      std::unique_ptr<Options> options7;
      std::unique_ptr<Options> options8;
      std::unique_ptr<Options> options9;
      std::unique_ptr<Options> options10;
      std::unique_ptr<Options> optionsDiscovery;
      initializeOptions(optionsDiscovery, testDumpPath + "/testP2PManagerDiscoveryNetworkNodeDiscovery", 8090);
      initializeOptions(options1, testDumpPath + "/testP2PManagerDiscoveryNetworkNode1", 8080);
      initializeOptions(options2, testDumpPath + "/testP2PManagerDiscoveryNetworkNode2", 8081);
      initializeOptions(options3, testDumpPath + "/testP2PManagerDiscoveryNetworkNode3", 8082);
      initializeOptions(options4, testDumpPath + "/testP2PManagerDiscoveryNetworkNode4", 8083);
      initializeOptions(options5, testDumpPath + "/testP2PManagerDiscoveryNetworkNode5", 8084);
      initializeOptions(options6, testDumpPath + "/testP2PManagerDiscoveryNetworkNode6", 8085);
      initializeOptions(options7, testDumpPath + "/testP2PManagerDiscoveryNetworkNode7", 8086);
      initializeOptions(options8, testDumpPath + "/testP2PManagerDiscoveryNetworkNode8", 8087);
      initializeOptions(options9, testDumpPath + "/testP2PManagerDiscoveryNetworkNode9", 8088);
      initializeOptions(options10, testDumpPath + "/testP2PManagerDiscoveryNetworkNode10", 8089);

      P2P::ManagerDiscovery p2pDiscoveryNode(boost::asio::ip::address::from_string("127.0.0.1"), optionsDiscovery);
      P2P::ManagerNormal p2pNode1(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options1, nullptr, nullptr);
      P2P::ManagerNormal p2pNode2(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options2, nullptr, nullptr);
      P2P::ManagerNormal p2pNode3(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options3, nullptr, nullptr);
      P2P::ManagerNormal p2pNode4(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options4, nullptr, nullptr);
      P2P::ManagerNormal p2pNode5(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options5, nullptr, nullptr);
      P2P::ManagerNormal p2pNode6(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options6, nullptr, nullptr);
      P2P::ManagerNormal p2pNode7(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options7, nullptr, nullptr);
      P2P::ManagerNormal p2pNode8(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options8, nullptr, nullptr);
      P2P::ManagerNormal p2pNode9(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options9, nullptr, nullptr);
      P2P::ManagerNormal p2pNode10(boost::asio::ip::address::from_string("127.0.0.1"), nullptr, options10, nullptr, nullptr);


      p2pDiscoveryNode.start();
      p2pNode1.start();
      p2pNode2.start();
      p2pNode3.start();
      p2pNode4.start();
      p2pNode5.start();
      p2pNode6.start();
      p2pNode7.start();
      p2pNode8.start();
      p2pNode9.start();
      p2pNode10.start();


      p2pNode1.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode2.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode3.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode4.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode5.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode6.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode7.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode8.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode9.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2pNode10.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

      // Wait until all peers are connected to the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Start discovery
      p2pDiscoveryNode.startDiscovery();
      p2pNode1.startDiscovery();
      p2pNode2.startDiscovery();
      p2pNode3.startDiscovery();
      p2pNode4.startDiscovery();
      p2pNode5.startDiscovery();
      p2pNode6.startDiscovery();
      p2pNode7.startDiscovery();
      p2pNode8.startDiscovery();
      p2pNode9.startDiscovery();
      p2pNode10.startDiscovery();

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      auto futureWaitAllNodesConnected = std::async(std::launch::async, [&]() {
        while(p2pDiscoveryNode.getSessionsIDs().size() != 10 ||
              p2pNode1.getSessionsIDs().size() != 10 ||
              p2pNode2.getSessionsIDs().size() != 10 ||
              p2pNode3.getSessionsIDs().size() != 10 ||
              p2pNode4.getSessionsIDs().size() != 10 ||
              p2pNode5.getSessionsIDs().size() != 10 ||
              p2pNode6.getSessionsIDs().size() != 10 ||
              p2pNode7.getSessionsIDs().size() != 10 ||
              p2pNode8.getSessionsIDs().size() != 10 ||
              p2pNode9.getSessionsIDs().size() != 10 ||
              p2pNode10.getSessionsIDs().size() != 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
      });

      REQUIRE(futureWaitAllNodesConnected.wait_for(std::chrono::seconds(10)) != std::future_status::timeout);

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      auto nodeDiscoverySessionsIDs = p2pDiscoveryNode.getSessionsIDs();
      auto node1SessionsIDs = p2pNode1.getSessionsIDs();
      auto node2SessionsIDs = p2pNode2.getSessionsIDs();
      auto node3SessionsIDs = p2pNode3.getSessionsIDs();
      auto node4SessionsIDs = p2pNode4.getSessionsIDs();
      auto node5SessionsIDs = p2pNode5.getSessionsIDs();
      auto node6SessionsIDs = p2pNode6.getSessionsIDs();
      auto node7SessionsIDs = p2pNode7.getSessionsIDs();
      auto node8SessionsIDs = p2pNode8.getSessionsIDs();
      auto node9SessionsIDs = p2pNode9.getSessionsIDs();
      auto node10SessionsIDs = p2pNode10.getSessionsIDs();

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
        p2pNode1.ping(session);
      }

      for (auto session : node2SessionsIDs) {
        p2pNode2.ping(session);
      }

      for (auto session : node3SessionsIDs) {
        p2pNode3.ping(session);
      }

      for (auto session : node4SessionsIDs) {
        p2pNode4.ping(session);
      }

      for (auto session : node5SessionsIDs) {
        p2pNode5.ping(session);
      }

      for (auto session : node6SessionsIDs) {
        p2pNode6.ping(session);
      }

      for (auto session : node7SessionsIDs) {
        p2pNode7.ping(session);
      }

      for (auto session : node8SessionsIDs) {
        p2pNode8.ping(session);
      }

      for (auto session : node9SessionsIDs) {
        p2pNode9.ping(session);
      }

      for (auto session : node10SessionsIDs) {
        p2pNode10.ping(session);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Close all the nodes.
      p2pDiscoveryNode.stop();
      p2pNode1.stop();
      p2pNode2.stop();
      p2pNode3.stop();
      p2pNode4.stop();
      p2pNode5.stop();
      p2pNode6.stop();
      p2pNode7.stop();
      p2pNode8.stop();
      p2pNode9.stop();
      p2pNode10.stop();

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      REQUIRE(p2pDiscoveryNode.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode1.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode2.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode3.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode4.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode5.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode6.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode7.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode8.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode9.getSessionsIDs().size() == 0);
      REQUIRE(p2pNode10.getSessionsIDs().size() == 0);

      REQUIRE(p2pDiscoveryNode.isServerRunning() == false);
      REQUIRE(p2pNode1.isServerRunning() == false);
      REQUIRE(p2pNode2.isServerRunning() == false);
      REQUIRE(p2pNode3.isServerRunning() == false);
      REQUIRE(p2pNode4.isServerRunning() == false);
      REQUIRE(p2pNode5.isServerRunning() == false);
      REQUIRE(p2pNode6.isServerRunning() == false);
      REQUIRE(p2pNode7.isServerRunning() == false);
      REQUIRE(p2pNode8.isServerRunning() == false);
      REQUIRE(p2pNode9.isServerRunning() == false);
      REQUIRE(p2pNode10.isServerRunning() == false);
    }
  }
};
