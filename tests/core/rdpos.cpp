/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/net/p2p/managerdiscovery.h"

#include "../blockchainwrapper.hpp" // blockchain.h -> consensus.h -> state.h -> (rdpos.h -> net/p2p/managernormal.h), dump.h -> (storage.h -> utils/options.h), utils/db.h
#include "../sdktestsuite.hpp"

const std::vector<Hash> validatorPrivKeysRdpos {
  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

namespace TRdPoS {
  // Simple rdPoS execution, does not test network functionality neither validator execution (rdPoSWorker)
  TEST_CASE("rdPoS Class", "[core][rdpos]") {
    PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    Address chainOwnerAddress = Secp256k1::toAddress(Secp256k1::toUPub(chainOwnerPrivKey));
    std::string testDumpPath = Utils::getTestDumpPath();
    SECTION("rdPoS class Startup") {
      std::set<Validator> validatorsList;

      int p2pServerPort = SDKTestSuite::getTestPort();
      {
        PrivKey validatorKey = PrivKey();
        auto blockchainWrapper = initialize(validatorPrivKeysRdpos, validatorKey, p2pServerPort, true, testDumpPath + "/rdPoSStartup");

        auto validators = blockchainWrapper.state.rdposGetValidators();
        REQUIRE(blockchainWrapper.state.rdposGetValidators().size() == 8);
        REQUIRE(validators.contains(Address(Hex::toBytes("1531bfdf7d48555a0034e4647fa46d5a04c002c3"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("e3dff2cc3f367df7d0254c834a0c177064d7c7f5"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("24e10d8ebe80abd3d3fddd89a26f08f3888d1380"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("b5f7152a2589c6cc2535c5facedfc853194d60a5"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("098ff62812043f5106db718e5c4349111de3b6b4"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("50d2ce9815e0e2354de7834f6fdd4d6946442a24"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("7c2b2a0a75e10b49e652d99bba8afee3a6bc78dd"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("6e67067edc1b4837b67c0b1def689eddee257521"))));
        REQUIRE(blockchainWrapper.state.rdposGetBestRandomSeed() == Hash()); // Genesis blocks randomness is 0.

        auto randomList = blockchainWrapper.state.rdposGetRandomList();
        validatorsList = blockchainWrapper.state.rdposGetValidators();
        REQUIRE(randomList.size() == 8);
        for (const auto& i : randomList) {
          REQUIRE(validatorsList.contains(i));
        }

        // Check ordering of random list. deterministic.
        REQUIRE(randomList[0] == Address(Hex::toBytes("50d2ce9815e0e2354de7834f6fdd4d6946442a24")));
        REQUIRE(randomList[1] == Address(Hex::toBytes("6e67067edc1b4837b67c0b1def689eddee257521")));
        REQUIRE(randomList[2] == Address(Hex::toBytes("24e10d8ebe80abd3d3fddd89a26f08f3888d1380")));
        REQUIRE(randomList[3] == Address(Hex::toBytes("7c2b2a0a75e10b49e652d99bba8afee3a6bc78dd")));
        REQUIRE(randomList[4] == Address(Hex::toBytes("1531bfdf7d48555a0034e4647fa46d5a04c002c3")));
        REQUIRE(randomList[5] == Address(Hex::toBytes("b5f7152a2589c6cc2535c5facedfc853194d60a5")));
        REQUIRE(randomList[6] == Address(Hex::toBytes("e3dff2cc3f367df7d0254c834a0c177064d7c7f5")));
        REQUIRE(randomList[7] == Address(Hex::toBytes("098ff62812043f5106db718e5c4349111de3b6b4")));
      }

      PrivKey validatorKey = PrivKey();
      auto blockchainWrapper = initialize(validatorPrivKeysRdpos, validatorKey, p2pServerPort, false, testDumpPath + "/rdPoSStartup");

      auto validators = blockchainWrapper.state.rdposGetValidators();
      REQUIRE(validators == validatorsList);
    }

    SECTION ("rdPoS validateBlock(), one block from genesis") {
      PrivKey validatorKey = PrivKey();
      auto blockchainWrapper = initialize(validatorPrivKeysRdpos, validatorKey, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSValidateBlockOneBlock");

      auto block = createValidBlock(validatorPrivKeysRdpos, blockchainWrapper.state, blockchainWrapper.storage);
      // Validate the block on rdPoS
      REQUIRE(blockchainWrapper.state.rdposValidateBlock(block));
    }

    SECTION ("rdPoS validateBlock(), ten block from genesis") {
      Hash expectedRandomnessFromBestBlock;
      std::vector<Validator> expectedRandomList;
      int p2pServerPort = SDKTestSuite::getTestPort();
      {
        PrivKey validatorKey = PrivKey();
        auto blockchainWrapper = initialize(validatorPrivKeysRdpos, validatorKey, p2pServerPort, true, testDumpPath + "/rdPoSValidateBlockTenBlocks");

        for (uint64_t i = 0; i < 10; ++i) {
          // Create a valid block, with the correct rdPoS transactions
          auto block = createValidBlock(validatorPrivKeysRdpos, blockchainWrapper.state, blockchainWrapper.storage);

          // Validate the block on rdPoS
          REQUIRE(blockchainWrapper.state.rdposValidateBlock(block));

          // Process block on rdPoS.
          blockchainWrapper.state.rdposProcessBlock(block);

          // Add the block to the storage.
          blockchainWrapper.storage.pushBlock(std::move(block));
        }

        // We expect to have moved 10 blocks forward.
        auto latestBlock = blockchainWrapper.storage.latest();
        REQUIRE(latestBlock->getNHeight() == 10);
        REQUIRE(latestBlock->getBlockRandomness() == blockchainWrapper.state.rdposGetBestRandomSeed());

        expectedRandomList = blockchainWrapper.state.rdposGetRandomList();
        expectedRandomnessFromBestBlock = blockchainWrapper.state.rdposGetBestRandomSeed();
      }

      PrivKey validatorKey = PrivKey();
      // Initialize same DB and storage as before.
      auto blockchainWrapper = initialize(validatorPrivKeysRdpos, validatorKey, p2pServerPort, false, testDumpPath + "/rdPoSValidateBlockTenBlocks");

      REQUIRE(blockchainWrapper.state.rdposGetBestRandomSeed() == expectedRandomnessFromBestBlock);
      REQUIRE(blockchainWrapper.state.rdposGetRandomList() == expectedRandomList);
    }

    SECTION("rdPoS validateBlock Errors (Coverage)") {
      PrivKey validatorKey = PrivKey();
      auto blockchainWrapper = initialize(validatorPrivKeysRdpos, validatorKey, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSValidateBlockCoverage");

      // Wrong signature (not randomList_[0])
      FinalizedBlock b1(Signature(), UPubKey(), bytes::random(), bytes::random(), bytes::random(), bytes::random(), 0, 0, {}, {}, bytes::random(), 1);
      REQUIRE_FALSE(blockchainWrapper.state.rdposValidateBlock(b1));

      // TODO: this should be covered further, but faking block contents is too much hassle as it is (same goes for addValidatorTx and maybe getTxValidatorFunction which is not exposed by State)
    }
  }

  TEST_CASE("rdPoS Class With Network Functionality", "[core][rdpos][net]") {
    PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    Address chainOwnerAddress = Secp256k1::toAddress(Secp256k1::toUPub(chainOwnerPrivKey));
    SECTION("Two Nodes instances, simple transaction broadcast") {
      // Initialize two different node instances, with different ports and DBs.
      std::string testDumpPath = Utils::getTestDumpPath();
      PrivKey validatorKey1 = PrivKey();
      auto blockchainWrapper1 = initialize(validatorPrivKeysRdpos, validatorKey1, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPosBasicNetworkNode1");

      PrivKey validatorKey2 = PrivKey();
      auto blockchainWrapper2 = initialize(validatorPrivKeysRdpos, validatorKey2, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPosBasicNetworkNode2");


      // Start respective p2p servers, and connect each other.
      blockchainWrapper1.p2p.start();
      blockchainWrapper2.p2p.start();
      blockchainWrapper1.p2p.connectToServer(LOCALHOST, blockchainWrapper2.p2p.serverPort());
      auto futureConnect = std::async(std::launch::async, [&]() {
        while (blockchainWrapper1.p2p.getSessionsIDs().size() != 1 ||
               blockchainWrapper2.p2p.getSessionsIDs().size() != 1) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });
      REQUIRE(futureConnect.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);
      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 1);

      // Create valid TxValidator transactions (8 in total), append them to node 1's storage.
      // After appending to node 1's storage, broadcast them to all nodes.
      auto validators = blockchainWrapper1.state.rdposGetValidators();
      auto randomList = blockchainWrapper1.state.rdposGetRandomList();

      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the rdPoS in the order of the random list, limited to rdPoS' minValidators.
      orderedPrivKeys.reserve(4);
      for (const auto& privKey : validatorPrivKeysRdpos) {
        if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
          blockSignerPrivKey = privKey;
          break;
        }
      }

      for (uint64_t i = 1; i < blockchainWrapper1.state.rdposGetMinValidators() + 1; i++) {
        for (const auto& privKey : validatorPrivKeysRdpos) {
          if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[i]) {
            orderedPrivKeys.push_back(privKey);
            break;
          }
        }
      }

      // By now we should have randomList[0] privKey in blockSignerPrivKey and the rest in orderedPrivKeys, ordered by the random list.
      // We can proceed with creating the block, transactions have to be **ordered** by the random list.

      // Create a block with 8 TxValidator transactions, 2 for each validator, in order (randomHash and random)
      uint64_t newBlocknHeight = blockchainWrapper1.storage.latest()->getNHeight() + 1;
      std::vector<TxValidator> txValidators;

      std::vector<Hash> randomSeeds(orderedPrivKeys.size(), bytes::random());
      for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
        Bytes hashTxData = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(hashTxData, Utils::sha3(randomSeeds[i]));
        Bytes randomTxData = Hex::toBytes("0x6fc5a2d6");
        Utils::appendBytes(randomTxData, randomSeeds[i]);
        txValidators.emplace_back(
          validatorAddress,
          hashTxData,
          8080,
          newBlocknHeight,
          orderedPrivKeys[i]
        );
        txValidators.emplace_back(
          validatorAddress,
          randomTxData,
          8080,
          newBlocknHeight,
          orderedPrivKeys[i]
        );
      }
      // Append the transactions to the block.
      for (const auto& tx : txValidators) {
        REQUIRE(isTxStatusValid(blockchainWrapper1.state.rdposAddValidatorTx(tx)));
      }

      // Broadcast the transactions
      for (const auto& tx : txValidators) {
        blockchainWrapper1.p2p.getBroadcaster().broadcastTxValidator(tx);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(250));

      auto node1Mempool = blockchainWrapper1.state.rdposGetMempool();
      auto node2Mempool = blockchainWrapper2.state.rdposGetMempool();

      // As transactions were broadcasted, they should be included in both nodes.
      REQUIRE(node1Mempool == node2Mempool);

      // Clear mempool from node 1.
      blockchainWrapper1.state.rdposClearMempool();

      // Request the transactions from node 1 to node 2
      std::vector<P2P::NodeID> nodesIds = blockchainWrapper1.p2p.getSessionsIDs();
      REQUIRE(nodesIds.size() == 1);
      auto transactionList = blockchainWrapper1.p2p.requestValidatorTxs(nodesIds[0]);

      REQUIRE(transactionList.size() == 14);

      // Append transactions back to node 1 mempool.
      for (const auto& tx : transactionList) {
        REQUIRE(isTxStatusValid(blockchainWrapper1.state.rdposAddValidatorTx(tx)));
      }

      // Check that the mempool is the same as before.
      node1Mempool = blockchainWrapper1.state.rdposGetMempool();
      REQUIRE(node1Mempool == node2Mempool);
    }

    SECTION("Ten NormalNodes and one DiscoveryNode, test broadcast") {
      // Initialize ten different node instances, with different ports and DBs.
      std::string testDumpPath = Utils::getTestDumpPath();
      PrivKey validatorKey1 = PrivKey();
      auto blockchainWrapper1 = initialize(validatorPrivKeysRdpos, validatorKey1, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode1");

      PrivKey validatorKey2 = PrivKey();
      auto blockchainWrapper2 = initialize(validatorPrivKeysRdpos, validatorKey2, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode2");

      PrivKey validatorKey3 = PrivKey();
      auto blockchainWrapper3 = initialize(validatorPrivKeysRdpos, validatorKey3, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode3");

      PrivKey validatorKey4 = PrivKey();
      auto blockchainWrapper4 = initialize(validatorPrivKeysRdpos, validatorKey4, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode4");

      PrivKey validatorKey5 = PrivKey();
      auto blockchainWrapper5 = initialize(validatorPrivKeysRdpos, validatorKey5, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode5");

      PrivKey validatorKey6 = PrivKey();
      auto blockchainWrapper6 = initialize(validatorPrivKeysRdpos, validatorKey6, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode6");

      PrivKey validatorKey7 = PrivKey();
      auto blockchainWrapper7 = initialize(validatorPrivKeysRdpos, validatorKey7, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode7");

      PrivKey validatorKey8 = PrivKey();
      auto blockchainWrapper8 = initialize(validatorPrivKeysRdpos, validatorKey8, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode8");

      PrivKey validatorKey9 = PrivKey();
      auto blockchainWrapper9 = initialize(validatorPrivKeysRdpos, validatorKey9, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode9");

      PrivKey validatorKey10 = PrivKey();
      auto blockchainWrapper10 = initialize(validatorPrivKeysRdpos, validatorKey10, SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestBroadcastNode10");

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> peers;
      PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      uint64_t genesisTimestamp = 1678887538000000;
      FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{}, Hash(), genesisTimestamp, 0, genesisPrivKey);
      std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
      std::vector<Address> genesisValidators;
      for (const auto& privKey : validatorPrivKeysRdpos) {
        genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
      }
      Options discoveryOptions(
          testDumpPath + "/rdPoSdiscoveryNodeTestBroadcast",
          "BDK/cpp/linux_x86-64/0.2.0",
          1,
          8080,
          Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
          LOCALHOST,
          SDKTestSuite::getTestPort(),
          9999,
          11,
          11,
          200,
          50,
          2000,
          10000,
          1000,
          4,
          peers,
          genesis,
          genesisTimestamp,
          genesisPrivKey,
          genesisBalances,
          genesisValidators,
          IndexingMode::RPC
        );
      P2P::ManagerDiscovery p2pDiscovery(LOCALHOST, discoveryOptions);

      // Start servers
      p2pDiscovery.start();
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

      // Connect nodes to the discovery node.
      blockchainWrapper1.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper2.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper3.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper4.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper5.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper6.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper7.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper8.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper9.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
      blockchainWrapper10.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());

      // Wait for connection towards discovery node.
      auto discoveryFuture = std::async (std::launch::async, [&] {
        while(p2pDiscovery.getSessionsIDs().size() != 10)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });

      REQUIRE(discoveryFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      REQUIRE(p2pDiscovery.getSessionsIDs().size() == 10);
      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper4.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper5.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper6.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper7.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper8.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper9.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper10.p2p.getSessionsIDs().size() == 1);

      p2pDiscovery.startDiscovery();
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


      auto connectionsFuture = std::async(std::launch::async, [&]() {
        while(blockchainWrapper1.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper2.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper3.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper4.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper5.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper6.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper7.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper8.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper9.p2p.getSessionsIDs().size() != 10 ||
              blockchainWrapper10.p2p.getSessionsIDs().size() != 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });

      REQUIRE(connectionsFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      // Stop discovery after all nodes have connected to each other.
      // Making so that the broadcast down this line takes too long to complete
      blockchainWrapper1.p2p.stopDiscovery();
      blockchainWrapper2.p2p.stopDiscovery();
      blockchainWrapper3.p2p.stopDiscovery();
      blockchainWrapper4.p2p.stopDiscovery();
      blockchainWrapper5.p2p.stopDiscovery();
      blockchainWrapper6.p2p.stopDiscovery();
      blockchainWrapper7.p2p.stopDiscovery();
      blockchainWrapper8.p2p.stopDiscovery();
      blockchainWrapper9.p2p.stopDiscovery();
      blockchainWrapper10.p2p.stopDiscovery();
      p2pDiscovery.stopDiscovery();

      // Create valid TxValidator transactions (8 in total), append them to node 1's storage.
      // After appending to node 1's storage, broadcast them to all nodes.
      auto validators = blockchainWrapper1.state.rdposGetValidators();
      auto randomList = blockchainWrapper1.state.rdposGetRandomList();

      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the rdPoS in the order of the random list, limited to rdPoS' minValidators.
      orderedPrivKeys.reserve(4);
      for (const auto& privKey : validatorPrivKeysRdpos) {
        if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
          blockSignerPrivKey = privKey;
          break;
        }
      }

      for (uint64_t i = 1; i < blockchainWrapper1.state.rdposGetMinValidators() + 1; i++) {
        for (const auto& privKey : validatorPrivKeysRdpos) {
          if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[i]) {
            orderedPrivKeys.push_back(privKey);
            break;
          }
        }
      }

      // By now we should have randomList[0] privKey in blockSignerPrivKey and the rest in orderedPrivKeys, ordered by the random list.
      // We can proceed with creating the block, transactions have to be **ordered** by the random list.

      // Create a block with 8 TxValidator transactions, 2 for each validator, in order (randomHash and random)
      uint64_t newBlocknHeight = blockchainWrapper1.storage.latest()->getNHeight() + 1;
      std::vector<TxValidator> txValidators;
      std::vector<Hash> randomSeeds(orderedPrivKeys.size(), bytes::random());
      for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
        Bytes hashTxData = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(hashTxData, Utils::sha3(randomSeeds[i]));
        Bytes randomTxData = Hex::toBytes("0x6fc5a2d6");
        Utils::appendBytes(randomTxData, randomSeeds[i]);
        txValidators.emplace_back(
          validatorAddress,
          hashTxData,
          8080,
          newBlocknHeight,
          orderedPrivKeys[i]
        );
        txValidators.emplace_back(
          validatorAddress,
          randomTxData,
          8080,
          newBlocknHeight,
          orderedPrivKeys[i]
        );
      }
      // Append the transactions to the block.
      for (const auto& tx : txValidators) {
        REQUIRE(isTxStatusValid(blockchainWrapper1.state.rdposAddValidatorTx(tx)));
      }

      // Broadcast transactions to all nodes.
      for (const auto& tx : txValidators) {
        blockchainWrapper1.p2p.getBroadcaster().broadcastTxValidator(tx);
      }

      /// Wait till transactions are broadcasted
      auto finalMempool = blockchainWrapper1.state.rdposGetMempool();

      auto broadcastFuture = std::async(std::launch::async, [&]() {
        while(blockchainWrapper2.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool() ||
            blockchainWrapper3.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool() ||
            blockchainWrapper4.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool() ||
            blockchainWrapper5.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool() ||
            blockchainWrapper6.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool() ||
            blockchainWrapper7.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool() ||
            blockchainWrapper8.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool() ||
            blockchainWrapper9.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool() ||
            blockchainWrapper10.state.rdposGetMempool() != blockchainWrapper1.state.rdposGetMempool()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });

      REQUIRE(broadcastFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      // Check if all mempools matchs
      auto node1Mempool = blockchainWrapper1.state.rdposGetMempool();
      auto node2Mempool = blockchainWrapper2.state.rdposGetMempool();
      auto node3Mempool = blockchainWrapper3.state.rdposGetMempool();
      auto node4Mempool = blockchainWrapper4.state.rdposGetMempool();
      auto node5Mempool = blockchainWrapper5.state.rdposGetMempool();
      auto node6Mempool = blockchainWrapper6.state.rdposGetMempool();
      auto node7Mempool = blockchainWrapper7.state.rdposGetMempool();
      auto node8Mempool = blockchainWrapper8.state.rdposGetMempool();
      auto node9Mempool = blockchainWrapper9.state.rdposGetMempool();
      auto node10Mempool = blockchainWrapper10.state.rdposGetMempool();

      REQUIRE(node1Mempool == node2Mempool);
      REQUIRE(node2Mempool == node3Mempool);
      REQUIRE(node3Mempool == node4Mempool);
      REQUIRE(node4Mempool == node5Mempool);
      REQUIRE(node5Mempool == node6Mempool);
      REQUIRE(node6Mempool == node7Mempool);
      REQUIRE(node7Mempool == node8Mempool);
      REQUIRE(node8Mempool == node9Mempool);
      REQUIRE(node9Mempool == node10Mempool);
    }
  }

  TEST_CASE("rdPoS class with Network and rdPoSWorker Functionality, move 10 blocks forward", "[core][rdpos][net][heavy]") {
    PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    Address chainOwnerAddress = Secp256k1::toAddress(Secp256k1::toUPub(chainOwnerPrivKey));
    // Initialize 8 different node instances, with different ports and DBs.
    std::string testDumpPath = Utils::getTestDumpPath();
    auto blockchainWrapper1 = initialize(validatorPrivKeysRdpos, validatorPrivKeysRdpos[0], SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestMove10BlocksNode1");

    auto blockchainWrapper2 = initialize(validatorPrivKeysRdpos, validatorPrivKeysRdpos[1], SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestMove10BlocksNode2");

    auto blockchainWrapper3 = initialize(validatorPrivKeysRdpos, validatorPrivKeysRdpos[2], SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestMove10BlocksNode3");

    auto blockchainWrapper4 = initialize(validatorPrivKeysRdpos, validatorPrivKeysRdpos[3], SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestMove10BlocksNode4");

    auto blockchainWrapper5 = initialize(validatorPrivKeysRdpos, validatorPrivKeysRdpos[4], SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestMove10BlocksNode5");

    auto blockchainWrapper6 = initialize(validatorPrivKeysRdpos, validatorPrivKeysRdpos[5], SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestMove10BlocksNode6");

    auto blockchainWrapper7 = initialize(validatorPrivKeysRdpos, validatorPrivKeysRdpos[6], SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestMove10BlocksNode7");

    auto blockchainWrapper8 = initialize(validatorPrivKeysRdpos, validatorPrivKeysRdpos[7], SDKTestSuite::getTestPort(), true, testDumpPath + "/rdPoSdiscoveryNodeTestMove10BlocksNode8");

    // Initialize the discovery node.
    std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
    PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    uint64_t genesisTimestamp = 1678887538000000;
    FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{}, Hash(), genesisTimestamp, 0, genesisPrivKey);
    std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
    std::vector<Address> genesisValidators;
    for (const auto& privKey : validatorPrivKeysRdpos) {
      genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
    }
    Options discoveryOptions(
      testDumpPath + "/rdPoSdiscoveryNodeTestMove10Blocks",
      "BDK/cpp/linux_x86-64/0.2.0",
      1,
      8080,
      Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
      LOCALHOST,
      SDKTestSuite::getTestPort(),
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
      genesisValidators,
      IndexingMode::RPC
    );
    P2P::ManagerDiscovery p2pDiscovery(LOCALHOST, discoveryOptions);

    // Start servers
    p2pDiscovery.start();
    blockchainWrapper1.p2p.start();
    blockchainWrapper2.p2p.start();
    blockchainWrapper3.p2p.start();
    blockchainWrapper4.p2p.start();
    blockchainWrapper5.p2p.start();
    blockchainWrapper6.p2p.start();
    blockchainWrapper7.p2p.start();
    blockchainWrapper8.p2p.start();

    // Connect nodes to the discovery node.
    blockchainWrapper1.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
    blockchainWrapper2.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
    blockchainWrapper3.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
    blockchainWrapper4.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
    blockchainWrapper5.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
    blockchainWrapper6.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
    blockchainWrapper7.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());
    blockchainWrapper8.p2p.connectToServer(LOCALHOST, p2pDiscovery.serverPort());

    // Wait for connection towards discovery node.
    auto discoveryFuture = std::async(std::launch::async, [&]() {
      while (p2pDiscovery.getSessionsIDs().size() != 8) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    REQUIRE(discoveryFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

    REQUIRE(p2pDiscovery.getSessionsIDs().size() == 8);
    REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 1);
    REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 1);
    REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 1);
    REQUIRE(blockchainWrapper4.p2p.getSessionsIDs().size() == 1);
    REQUIRE(blockchainWrapper5.p2p.getSessionsIDs().size() == 1);
    REQUIRE(blockchainWrapper6.p2p.getSessionsIDs().size() == 1);
    REQUIRE(blockchainWrapper7.p2p.getSessionsIDs().size() == 1);
    REQUIRE(blockchainWrapper8.p2p.getSessionsIDs().size() == 1);

    p2pDiscovery.startDiscovery();
    blockchainWrapper1.p2p.startDiscovery();
    blockchainWrapper2.p2p.startDiscovery();
    blockchainWrapper3.p2p.startDiscovery();
    blockchainWrapper4.p2p.startDiscovery();
    blockchainWrapper5.p2p.startDiscovery();
    blockchainWrapper6.p2p.startDiscovery();
    blockchainWrapper7.p2p.startDiscovery();
    blockchainWrapper8.p2p.startDiscovery();

    auto connectionFuture = std::async(std::launch::async, [&]() {
      while(p2pDiscovery.getSessionsIDs().size() != 8 ||
            blockchainWrapper1.p2p.getSessionsIDs().size() != 8 ||
            blockchainWrapper2.p2p.getSessionsIDs().size() != 8 ||
            blockchainWrapper3.p2p.getSessionsIDs().size() != 8 ||
            blockchainWrapper4.p2p.getSessionsIDs().size() != 8 ||
            blockchainWrapper5.p2p.getSessionsIDs().size() != 8 ||
            blockchainWrapper6.p2p.getSessionsIDs().size() != 8 ||
            blockchainWrapper7.p2p.getSessionsIDs().size() != 8 ||
            blockchainWrapper8.p2p.getSessionsIDs().size() != 8) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });

    REQUIRE(connectionFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

    // Stop discovery after all nodes have connected to each other.
    // Making so that the broadcast down this line takes too long to complete
    blockchainWrapper1.p2p.stopDiscovery();
    blockchainWrapper2.p2p.stopDiscovery();
    blockchainWrapper3.p2p.stopDiscovery();
    blockchainWrapper4.p2p.stopDiscovery();
    blockchainWrapper5.p2p.stopDiscovery();
    blockchainWrapper6.p2p.stopDiscovery();
    blockchainWrapper7.p2p.stopDiscovery();
    blockchainWrapper8.p2p.stopDiscovery();
    p2pDiscovery.stopDiscovery();

    // After a while, the discovery thread should have found all the nodes and connected between each other.
    REQUIRE(blockchainWrapper1.state.rdposGetIsValidator());
    REQUIRE(blockchainWrapper2.state.rdposGetIsValidator());
    REQUIRE(blockchainWrapper3.state.rdposGetIsValidator());
    REQUIRE(blockchainWrapper4.state.rdposGetIsValidator());
    REQUIRE(blockchainWrapper5.state.rdposGetIsValidator());
    REQUIRE(blockchainWrapper6.state.rdposGetIsValidator());
    REQUIRE(blockchainWrapper7.state.rdposGetIsValidator());
    REQUIRE(blockchainWrapper8.state.rdposGetIsValidator());

    blockchainWrapper1.consensus.start();
    blockchainWrapper2.consensus.start();
    blockchainWrapper3.consensus.start();
    blockchainWrapper4.consensus.start();
    blockchainWrapper5.consensus.start();
    blockchainWrapper6.consensus.start();
    blockchainWrapper7.consensus.start();
    blockchainWrapper8.consensus.start();

    // When consensus is running, we can just wait for the blocks to be created.
    int timeoutSecs = 60;
    auto now = Utils::getCurrentTimeMillisSinceEpoch();
    auto rdPoSBlockFuture = std::async(std::launch::async, [&]() {
      auto start = Utils::getCurrentTimeMillisSinceEpoch();
      int timeoutFutureThreadMillis = (timeoutSecs + 5) * 1000; // +5s than main test thread to make sure
      uint64_t targetLatestHeight = 1;
      while (blockchainWrapper1.storage.latest()->getNHeight() != 10) {
        if (Utils::getCurrentTimeMillisSinceEpoch() - start > timeoutFutureThreadMillis) { Utils::safePrintTest("Future thread timeout."); return; }
        // We need to forcefully make a transaction and broadcast in the network so the consensus can create a block
        // otherwise it will sleep forever
        Address targetOfTransactions(Utils::randBytes(20));
        TxBlock tx (targetOfTransactions,
              chainOwnerAddress,
              Bytes(),
              8080,
              blockchainWrapper1.state.getNativeNonce(chainOwnerAddress),
              1000000000000000000,
              21000,
              1000000000,
              1000000000,
              chainOwnerPrivKey);
        TxStatus txStatus = blockchainWrapper1.state.addTx(std::move(tx));
        REQUIRE(isTxStatusValid(txStatus));
        blockchainWrapper1.p2p.getBroadcaster().broadcastTxBlock(tx);
        // Block height has to advance in lockstep across all nodes before issuing the next transaction.
        while
        (
          blockchainWrapper1.storage.latest()->getNHeight() != targetLatestHeight ||
          blockchainWrapper2.storage.latest()->getNHeight() != targetLatestHeight ||
          blockchainWrapper3.storage.latest()->getNHeight() != targetLatestHeight ||
          blockchainWrapper4.storage.latest()->getNHeight() != targetLatestHeight ||
          blockchainWrapper5.storage.latest()->getNHeight() != targetLatestHeight ||
          blockchainWrapper6.storage.latest()->getNHeight() != targetLatestHeight ||
          blockchainWrapper7.storage.latest()->getNHeight() != targetLatestHeight ||
          blockchainWrapper8.storage.latest()->getNHeight() != targetLatestHeight
        )
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          if (Utils::getCurrentTimeMillisSinceEpoch() - start > timeoutFutureThreadMillis) { Utils::safePrintTest("Future thread timeout."); return; }
        }
        ++targetLatestHeight;
      }
    });

    REQUIRE(rdPoSBlockFuture.wait_for(std::chrono::seconds(timeoutSecs)) != std::future_status::timeout);
    auto after = Utils::getCurrentTimeMillisSinceEpoch();
    std::cout << "Took: " << (after - now) << "ms to create 10 blocks." << std::endl;
  }
};
