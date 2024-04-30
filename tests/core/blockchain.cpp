/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/blockchain.h"
#include "../../src/net/p2p/managerdiscovery.h"
#include <filesystem>

/// Forward decleration from tests/net/http/httpjsonrpc.cpp
/// For usage within sending transactions
std::string makeHTTPRequest(
    const std::string& reqBody, const std::string& host, const std::string& port,
    const std::string& target, const std::string& requestType, const std::string& contentType
);


void initialize(const std::string& blockchainPath,
                const std::string& web3clientVersion,
                const uint64_t& version,
                const uint64_t& chainId,
                const uint64_t& wsPort,
                const uint64_t& httpPort,
                const PrivKey& privKey,
                const std::pair<std::string,uint64_t>& discoveryNodes,
                std::unique_ptr<Blockchain>& blockchain,
                bool clearFolder = true) {
  if (clearFolder) {
    if (std::filesystem::exists(blockchainPath)) {
      std::filesystem::remove_all(blockchainPath);
    }
    std::filesystem::create_directories(blockchainPath);

    json optionsJson;
    optionsJson["rootPath"] = blockchainPath;
    optionsJson["web3clientVersion"] = web3clientVersion;
    optionsJson["version"] = version;
    optionsJson["chainID"] = chainId;
    optionsJson["wsPort"] = wsPort;
    optionsJson["httpPort"] = httpPort;
    if (privKey) {
      optionsJson["privKey"] = privKey.hex();
    }
    optionsJson["discoveryNodes"][0]["address"] = discoveryNodes.first;
    optionsJson["discoveryNodes"][0]["port"] = discoveryNodes.second;
    std::ofstream optionsFile(blockchainPath + "/options.json");
    optionsFile << optionsJson.dump(2);
    optionsFile.close();
  }
  blockchain = std::make_unique<Blockchain>(blockchainPath);
}


namespace TBlockchain {
  TEST_CASE("Blockchain Class", "[core][blockchain]") {
    std::string testDumpPath = Utils::getTestDumpPath();
    SECTION("Initialize Blockchain Multiple Nodes") {
      std::shared_ptr<const Block> bestBlock;
      {
        /// Initialize ManagerDiscovery
        std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
        std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
            testDumpPath + "/statedDiscoveryNodeNetworkCapabilitiesWithTxBlockBroadcast",
            "BDK/cpp/linux_x86-64/0.2.0",
            1,
            8080,
            8100,
            9999,
            discoveryNodes
        );
        std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery = std::make_unique<P2P::ManagerDiscovery>(
            boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

        /// Initialize multiple blockchain nodes.
        std::unique_ptr<Blockchain> blockchain1;
        initialize("blockchainInitializeTestNode1", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8080, 8101,
                   PrivKey(), {"127.0.0.1", 8100}, blockchain1);

        std::unique_ptr<Blockchain> blockchain2;
        initialize("blockchainInitializeTestNode2", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8081, 8102,
                   PrivKey(), {"127.0.0.1", 8100}, blockchain2);

        std::unique_ptr<Blockchain> blockchain3;
        initialize("blockchainInitializeTestNode3", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8082, 8103,
                   PrivKey(), {"127.0.0.1", 8100}, blockchain3);

        std::unique_ptr<Blockchain> blockchain4;
        initialize("blockchainInitializeTestNode4", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8083, 8104,
                   PrivKey(), {"127.0.0.1", 8100}, blockchain4);

        /// Start the blockchain nodes.
        p2pDiscovery->start();
        p2pDiscovery->startDiscovery();
        blockchain1->start();
        blockchain2->start();
        blockchain3->start();
        blockchain4->start();

        REQUIRE(!blockchain1->getOptions()->getIsValidator());
        REQUIRE(!blockchain2->getOptions()->getIsValidator());
        REQUIRE(!blockchain3->getOptions()->getIsValidator());
        REQUIRE(!blockchain4->getOptions()->getIsValidator());

        /// Wait everyone to connect.
        auto connectFuture = std::async(std::launch::async, [&]() {
          while (blockchain1->getP2P()->getSessionsIDs().size() != 4 ||
                 blockchain2->getP2P()->getSessionsIDs().size() != 4 ||
                 blockchain3->getP2P()->getSessionsIDs().size() != 4 ||
                 blockchain4->getP2P()->getSessionsIDs().size() != 4) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
        });

        REQUIRE(connectFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

        REQUIRE(blockchain1->getP2P()->getSessionsIDs().size() == 4);
        REQUIRE(blockchain2->getP2P()->getSessionsIDs().size() == 4);
        REQUIRE(blockchain3->getP2P()->getSessionsIDs().size() == 4);
        REQUIRE(blockchain4->getP2P()->getSessionsIDs().size() == 4);

        bestBlock = blockchain1->getStorage()->latest();
        REQUIRE(blockchain1->getStorage()->latest()->hash() == blockchain2->getStorage()->latest()->hash());
        REQUIRE(blockchain1->getStorage()->latest()->hash() == blockchain3->getStorage()->latest()->hash());
        REQUIRE(blockchain1->getStorage()->latest()->hash() == blockchain4->getStorage()->latest()->hash());

        /// Stop the blockchain nodes.
        p2pDiscovery->stop();
        blockchain1->stop();
        blockchain2->stop();
        blockchain3->stop();
        blockchain4->stop();
      }
      std::unique_ptr<Blockchain> blockchain1;
      initialize("blockchainInitializeTestNode1", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8080, 8101,
                 PrivKey(), {"127.0.0.1", 8100}, blockchain1, false);

      REQUIRE(blockchain1->getStorage()->latest()->hash() == bestBlock->hash());
    }

    SECTION("Blockchain multiple nodes, move 10 blocks forward 1 tx per block.") {
      PrivKey privKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address me = Secp256k1::toAddress(Secp256k1::toUPub(privKey));
      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetBalance = 0;
      uint256_t myBalance("1000000000000000000000");
      std::shared_ptr<const Block> bestBlock;

      /// Create the discovery node.
      {
        std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
        std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
            testDumpPath + "/statedDiscoveryNodeNetworkCapabilitiesWithTxBlockBroadcast",
            "BDK/cpp/linux_x86-64/0.2.0",
            1,
            8080,
            8100,
            9999,
            discoveryNodes
        );
        std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery = std::make_unique<P2P::ManagerDiscovery>(
            boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

        /// Create the validator nodes (5 in total)
        std::unique_ptr<Blockchain> blockchainValidator1;
        initialize("blockchainMove10BlocksTestValidator1", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8080, 8101,
                   PrivKey(Hex::toBytes("0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759")),
                   {"127.0.0.1", 8100}, blockchainValidator1);

        std::unique_ptr<Blockchain> blockchainValidator2;
        initialize("blockchainMove10BlocksTestValidator2", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8081, 8102,
                   PrivKey(Hex::toBytes("0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4")),
                   {"127.0.0.1", 8100}, blockchainValidator2);

        std::unique_ptr<Blockchain> blockchainValidator3;
        initialize("blockchainMove10BlocksTestValidator3", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8082, 8103,
                   PrivKey(Hex::toBytes("0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751")),
                   {"127.0.0.1", 8100}, blockchainValidator3);

        std::unique_ptr<Blockchain> blockchainValidator4;
        initialize("blockchainMove10BlocksTestValidator4", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8083, 8104,
                   PrivKey(Hex::toBytes("0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7")),
                   {"127.0.0.1", 8100}, blockchainValidator4);

        std::unique_ptr<Blockchain> blockchainValidator5;
        initialize("blockchainMove10BlocksTestValidator5", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8084, 8105,
                   PrivKey(Hex::toBytes("0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6")),
                   {"127.0.0.1", 8100}, blockchainValidator5);

        /// Create the normal nodes (6 in total)
        std::unique_ptr<Blockchain> blockchainNode1;
        initialize("blockchainMove10BlocksTestNode1", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8085, 8106,
                   PrivKey(), {"127.0.0.1", 8100}, blockchainNode1);

        std::unique_ptr<Blockchain> blockchainNode2;
        initialize("blockchainMove10BlocksTestNode2", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8086, 8107,
                   PrivKey(), {"127.0.0.1", 8100}, blockchainNode2);

        std::unique_ptr<Blockchain> blockchainNode3;
        initialize("blockchainMove10BlocksTestNode3", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8087, 8108,
                   PrivKey(), {"127.0.0.1", 8100}, blockchainNode3);

        std::unique_ptr<Blockchain> blockchainNode4;
        initialize("blockchainMove10BlocksTestNode4", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8088, 8109,
                   PrivKey(), {"127.0.0.1", 8100}, blockchainNode4);

        std::unique_ptr<Blockchain> blockchainNode5;
        initialize("blockchainMove10BlocksTestNode5", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8089, 8110,
                   PrivKey(), {"127.0.0.1", 8100}, blockchainNode5);

        std::unique_ptr<Blockchain> blockchainNode6;
        initialize("blockchainMove10BlocksTestNode6", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8090, 8111,
                   PrivKey(), {"127.0.0.1", 8100}, blockchainNode6);

        /// Start the discovery node.
        p2pDiscovery->start();
        p2pDiscovery->startDiscovery();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        /// Start the validator nodes.
        blockchainValidator1->start();
        blockchainValidator2->start();
        blockchainValidator3->start();
        blockchainValidator4->start();
        blockchainValidator5->start();

        /// Start all nodes
        blockchainNode1->start();
        blockchainNode2->start();
        blockchainNode3->start();
        blockchainNode4->start();
        blockchainNode5->start();
        blockchainNode6->start();

        /// Wait everyone to sync
        auto syncFuture = std::async(std::launch::async, [&]() {
          while (!blockchainValidator1->isSynced() ||
                 !blockchainValidator2->isSynced() ||
                 !blockchainValidator3->isSynced() ||
                 !blockchainValidator4->isSynced() ||
                 !blockchainValidator5->isSynced() ||
                 !blockchainNode1->isSynced() ||
                 !blockchainNode2->isSynced() ||
                 !blockchainNode3->isSynced() ||
                 !blockchainNode4->isSynced() ||
                 !blockchainNode5->isSynced() ||
                 !blockchainNode6->isSynced()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
        });

        REQUIRE(syncFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

        uint64_t blocks = 0;
        while (blocks < 10) {
          TxBlock tx(
              targetOfTransactions,
              me,
              Bytes(),
              8080,
              blockchainValidator1->getState()->getNativeNonce(me),
              1000000000000000000,
              21000,
              1000000000,
              1000000000,
              privKey
          );

          /// Commment this out and IT WILL NOT WORK.
          /// Waiting for rdPoSWorker rework.
          auto rdPoSmempoolFuture = std::async(std::launch::async, [&]() {
            while (blockchainValidator1->getState().rdposGetMempool().size() != 8 ||
                   blockchainValidator2->getState().rdposGetMempool().size() != 8 ||
                   blockchainValidator3->getState().rdposGetMempool().size() != 8 ||
                   blockchainValidator4->getState().rdposGetMempool().size() != 8 ||
                   blockchainValidator5->getState().rdposGetMempool().size() != 8 ||
                   blockchainNode1->getState().rdposGetMempool().size() != 8 ||
                   blockchainNode2->getState().rdposGetMempool().size() != 8 ||
                   blockchainNode3->getState().rdposGetMempool().size() != 8 ||
                   blockchainNode4->getState().rdposGetMempool().size() != 8 ||
                   blockchainNode5->getState().rdposGetMempool().size() != 8 ||
                   blockchainNode6->getState().rdposGetMempool().size() != 8) {
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
          });

          REQUIRE(rdPoSmempoolFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

          myBalance -= tx.getValue() + (tx.getMaxFeePerGas() * tx.getGasLimit());
          targetBalance += tx.getValue();
          /// Send the transactions through HTTP
          auto sendRawTxJson = json({
                                    {"jsonrpc", "2.0"},
                                    {"id", 1},
                                    {"method", "eth_sendRawTransaction"},
                                    {"params", json::array({Hex::fromBytes(tx.rlpSerialize(), true).forRPC()})}});

          /// Send the transaction to the first validator.
          auto sendRawTxResponse = json::parse(makeHTTPRequest(sendRawTxJson.dump(),
                                               "127.0.0.1",
                                               std::to_string(8101),
                                               "/",
                                               "POST",
                                               "application/json"));


          REQUIRE(sendRawTxResponse["result"].get<std::string>() == tx.hash().hex(true).get());

          /// Wait for the new best block to be broadcasted and accepted by all nodes
          ++blocks;
          auto blockFuture = std::async(std::launch::async, [&]() {
            while (blocks != blockchainNode1->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainNode2->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainNode3->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainNode4->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainNode5->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainNode6->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainValidator1->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainValidator2->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainValidator3->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainValidator4->getStorage()->latest()->getNHeight() ||
                   blocks != blockchainValidator5->getStorage()->latest()->getNHeight()) {
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
          });

          std::cout << "Before: " << std::endl;
          std::cout << blockchainNode1->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode2->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode3->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode4->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode5->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode6->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator1->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator2->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator3->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator4->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator5->getStorage()->latest()->getNHeight() << std::endl;

          std::cout << "P2P Connections Count: " << std::endl;
          std::cout << blockchainNode1->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode2->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode3->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode4->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode5->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode6->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator1->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator2->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator3->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator4->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator5->getP2P()->getSessionsIDs().size() << std::endl;

          auto wait = blockFuture.wait_for(std::chrono::seconds(10));
          std::cout << "After: " << std::endl;

          std::cout << blockchainNode1->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode2->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode3->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode4->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode5->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainNode6->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator1->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator2->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator3->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator4->getStorage()->latest()->getNHeight() << std::endl;
          std::cout << blockchainValidator5->getStorage()->latest()->getNHeight() << std::endl;

          std::cout << "P2P Connections Count: " << std::endl;
          std::cout << blockchainNode1->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode2->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode3->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode4->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode5->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainNode6->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator1->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator2->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator3->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator4->getP2P()->getSessionsIDs().size() << std::endl;
          std::cout << blockchainValidator5->getP2P()->getSessionsIDs().size() << std::endl;

          REQUIRE(wait != std::future_status::timeout);

          /// Everyone should be on the same block
          REQUIRE(blockchainValidator1->getStorage()->latest()->getNHeight() == blocks);
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainValidator2->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainValidator3->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainValidator4->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainValidator5->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainNode1->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainNode2->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainNode3->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainNode4->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainNode5->getStorage()->latest()->hash());
          REQUIRE(blockchainValidator1->getStorage()->latest()->hash() == blockchainNode6->getStorage()->latest()->hash());
        }
        bestBlock = blockchainValidator1->getStorage()->latest();
        /// Stop the nodes
        p2pDiscovery->stop();
        blockchainValidator1->stop();
        blockchainValidator2->stop();
        blockchainValidator3->stop();
        blockchainValidator4->stop();
        blockchainValidator5->stop();
        blockchainNode1->stop();
        blockchainNode2->stop();
        blockchainNode3->stop();
        blockchainNode4->stop();
        blockchainNode5->stop();
        blockchainNode6->stop();
      }

      std::unique_ptr<Blockchain> blockchainNode1;
      initialize("blockchainMove10BlocksTestNode1", "BDK/cpp/linux_x86-64/0.2.0", 8080, 8080, 8085, 8106,
                 PrivKey(), {"127.0.0.1", 8100}, blockchainNode1, false);

      REQUIRE(blockchainNode1->getStorage()->latest()->hash() == bestBlock->hash());
    }
  }
}
