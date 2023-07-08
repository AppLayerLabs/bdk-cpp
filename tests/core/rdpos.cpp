#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/rdpos.h"
#include "../../src/core/storage.h"
#include "../../src/core/state.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/net/p2p/managernormal.h"
#include "../../src/net/p2p/managerdiscovery.h"

#include <filesystem>
#include <utility>

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

// We initialize the blockchain database
// To make sure that if the genesis is changed within the main source code
// The tests will still work, as tests uses own genesis block.
void initialize(std::unique_ptr<DB>& db, 
                std::unique_ptr<Storage>& storage, 
                std::unique_ptr<P2P::ManagerNormal>& p2p,
                PrivKey validatorKey,
                std::unique_ptr<rdPoS>& rdpos,
                std::unique_ptr<Options>& options,
                std::unique_ptr<State>& state,
                uint64_t serverPort,
                bool clearDb,
                std::string folderName) {
  std::string dbName = folderName + "/db";
  if (clearDb) {
    if (std::filesystem::exists(dbName)) {
      std::filesystem::remove_all(dbName);
    }
  }
  db = std::make_unique<DB>(dbName);
  if (clearDb) {
    Block genesis(Hash(Utils::uint256ToBytes(0)), 1678887537000000, 0);
                                                  
    // Genesis Keys:
    // Private: 0xe89ef6409c467285bcae9f80ab1cfeb348  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),7cfe61ab28fb7d36443e1daa0c2867
    // Address: 0x00dead00665771855a34155f5e7405489df2c3c6
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")), 1678887538000000);
    db->put(Utils::stringToBytes("latest"), genesis.serializeBlock(), DBPrefix::blocks);
    db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    db->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);

    // Populate rdPoS DB with unique rdPoS, not default.
    for (uint64_t i = 0; i < validatorPrivKeys.size(); ++i) {
      db->put(Utils::uint64ToBytes(i), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[i]))).get(),
              DBPrefix::rdPoS);
    }
  }
  std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
  if (!validatorKey) {
    options = std::make_unique<Options>(
        folderName,
        "OrbiterSDK/cpp/linux_x86-64/0.0.3",
        1,
        8080,
        serverPort,
        9999,
        discoveryNodes
      );
  } else {
    options = std::make_unique<Options>(
      folderName,
      "OrbiterSDK/cpp/linux_x86-64/0.0.3",
      1,
      8080,
      serverPort,
      9999,
      discoveryNodes,
      validatorKey
    );
  }

  storage = std::make_unique<Storage>(db, options);
  p2p = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), rdpos, options, storage, state);
  rdpos = std::make_unique<rdPoS>(db, storage, p2p, options, state);
  state = std::make_unique<State>(db, storage, rdpos, p2p, options);
}

/*    Options(const std::string& rootPath,
            const std::string& web3clientVersion,
            const uint64_t& version,
            const uint64_t& chainID,
            const uint16_t& wsPort,
            const uint16_t& httpPort); */
// This creates a valid block given the state within the rdPoS class.
// Should not be used during network/thread testing, as it will automatically sign all TxValidator transactions within the block
// And that is not the purpose of network/thread testing.
Block createValidBlock(std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage, const std::vector<TxBlock>& txs = {}) {
  auto validators = rdpos->getValidators();
  auto randomList = rdpos->getRandomList();

  Hash blockSignerPrivKey;           // Private key for the block signer.
  std::vector<Hash> orderedPrivKeys; // Private keys for the rdPoS in the order of the random list, limited to rdPoS::minValidators.
  orderedPrivKeys.reserve(4);
  for (const auto& privKey : validatorPrivKeys) {
    if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
      blockSignerPrivKey = privKey;
      break;
    }
  }

  for (uint64_t i = 1; i < rdPoS::minValidators + 1; i++) {
    for (const auto& privKey : validatorPrivKeys) {
      if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[i]) {
        orderedPrivKeys.push_back(privKey);
        break;
      }
    }
  }

  // By now we should have randomList[0] privKey in blockSignerPrivKey and the rest in orderedPrivKeys, ordered by the random list.
  // We can proceed with creating the block, transactions have to be **ordered** by the random list.

  // Create a block with 8 TxValidator transactions, 2 for each validator, in order (randomHash and random)
  uint64_t newBlocknHeight = storage->latest()->getNHeight() + 1;
  uint64_t newBlockTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  Hash newBlockPrevHash = storage->latest()->hash();
  Block block(newBlockPrevHash, newBlockTimestamp, newBlocknHeight);
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomTxs;

  std::vector<Hash> randomSeeds(orderedPrivKeys.size(), Hash::random());
  for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
    Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
    Bytes hashTxData = Hex::toBytes("0xcfffe746");
    Utils::appendBytes(hashTxData, Utils::sha3(randomSeeds[i].get()));
    Bytes randomTxData = Hex::toBytes("0x6fc5a2d6");
    Utils::appendBytes(randomTxData, randomSeeds[i].get());
    randomHashTxs.emplace_back(
      validatorAddress,
      hashTxData,
      8080,
      newBlocknHeight,
      orderedPrivKeys[i]
    );
    randomTxs.emplace_back(
      validatorAddress,
      randomTxData,
      8080,
      newBlocknHeight,
      orderedPrivKeys[i]
    );
  }
  // Append the transactions to the block.
  for (const auto& tx : randomHashTxs) {
    rdpos->addValidatorTx(tx);
    block.appendTxValidator(tx);
  }
  for (const auto& tx : randomTxs) {
    rdpos->addValidatorTx(tx);
    block.appendTxValidator(tx);
  }
  for (const auto& tx : txs) {
    block.appendTx(tx);
  }

  // Check rdPoS mempool.
  auto rdPoSmempool = rdpos->getMempool();
  REQUIRE(rdpos->getMempool().size() == 8);
  for (const auto& tx : randomHashTxs) {
    REQUIRE(rdPoSmempool.contains(tx.hash()));
  }
  for (const auto& tx : randomTxs) {
    REQUIRE(rdPoSmempool.contains(tx.hash()));
  }
      
  // Finalize the block
  block.finalize(blockSignerPrivKey, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
  return block;
}

namespace TRdPoS {
  // Simple rdPoS execution, does not test network functionality neither validator execution (rdPoSWorker)
  TEST_CASE("rdPoS Class", "[core][rdpos]") {
    SECTION("rdPoS class Startup") {
      std::set<Validator> validatorsList;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        PrivKey validatorKey = PrivKey();
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<Options> options;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, validatorKey, rdpos, options, state, 8080, true, "rdPoSStartup");

        auto validators = rdpos->getValidators();
        REQUIRE(rdpos->getValidators().size() == 8);
        REQUIRE(validators.contains(Address(Hex::toBytes("1531bfdf7d48555a0034e4647fa46d5a04c002c3"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("e3dff2cc3f367df7d0254c834a0c177064d7c7f5"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("24e10d8ebe80abd3d3fddd89a26f08f3888d1380"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("b5f7152a2589c6cc2535c5facedfc853194d60a5"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("098ff62812043f5106db718e5c4349111de3b6b4"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("50d2ce9815e0e2354de7834f6fdd4d6946442a24"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("7c2b2a0a75e10b49e652d99bba8afee3a6bc78dd"))));
        REQUIRE(validators.contains(Address(Hex::toBytes("6e67067edc1b4837b67c0b1def689eddee257521"))));
        REQUIRE(rdpos->getBestRandomSeed() == Hash()); // Genesis blocks randomness is 0.

        auto randomList = rdpos->getRandomList();
        validatorsList = rdpos->getValidators();
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
      
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      PrivKey validatorKey = PrivKey();
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<Options> options;
      std::unique_ptr<State> state;
      initialize(db, storage, p2p, validatorKey, rdpos, options, state, 8080, false, "rdPoSStartup");

      auto validators = rdpos->getValidators();
      REQUIRE(validators == validatorsList);
    }

    SECTION ("rdPoS validateBlock(), one block from genesis") {
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      PrivKey validatorKey = PrivKey();
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<Options> options;
      std::unique_ptr<State> state;
      initialize(db, storage, p2p, validatorKey, rdpos, options, state, 8080, true, "rdPoSValidateBlock");

      auto block = createValidBlock(rdpos, storage);
      // Validate the block on rdPoS
      REQUIRE(rdpos->validateBlock(block));
    }

    SECTION ("rdPoS validateBlock(), ten block from genesis") {
      Hash expectedRandomnessFromBestBlock;
      std::vector<Validator> expectedRandomList;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        PrivKey validatorKey = PrivKey();
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<Options> options;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, validatorKey, rdpos, options, state, 8080, true, "rdPoSValidateBlockTenBlocks");

        for (uint64_t i = 0; i < 10; ++i) {
          // Create a valid block, with the correct rdPoS transactions
          auto block = createValidBlock(rdpos, storage);

          // Validate the block on rdPoS
          REQUIRE(rdpos->validateBlock(block));
          
          // Process block on rdPoS.
          rdpos->processBlock(block);

          // Add the block to the storage.
          storage->pushBack(std::move(block));
        }

        // We expect to have moved 10 blocks forward.
        auto latestBlock = storage->latest();
        REQUIRE(latestBlock->getNHeight() == 10);
        REQUIRE(latestBlock->getBlockRandomness() == rdpos->getBestRandomSeed());

        expectedRandomList = rdpos->getRandomList();
        expectedRandomnessFromBestBlock = rdpos->getBestRandomSeed();
      }

      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      PrivKey validatorKey = PrivKey();
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<Options> options;
      std::unique_ptr<State> state;
      // Initialize same DB and storage as before.
      initialize(db, storage, p2p, validatorKey, rdpos, options, state, 8080, false, "rdPoSValidateBlockTenBlocks");

      REQUIRE(rdpos->getBestRandomSeed() == expectedRandomnessFromBestBlock);
      REQUIRE(rdpos->getRandomList() == expectedRandomList);
    }
  }

  TEST_CASE("rdPoS Class With Network Functionality", "[core][rdpos][net]") {
    SECTION("Two Nodes instances, simple transaction broadcast") {
      // Initialize two different node instances, with different ports and DBs.
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<Options> options1;
      std::unique_ptr<State> state1;
      initialize(db1, storage1, p2p1, validatorKey1, rdpos1, options1, state1, 8080, true, "rdPosBasicNetworkNode1");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<Options> options2;
      std::unique_ptr<State> state2;
      initialize(db2, storage2, p2p2, validatorKey2, rdpos2, options2, state2, 8081, true, "rdPosBasicNetworkNode2");


      // Start respective p2p servers, and connect each other.
      p2p1->start();
      p2p2->start();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      p2p1->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8081);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      REQUIRE(p2p1->getSessionsIDs().size() == 1);

      // Create valid TxValidator transactions (8 in total), append them to node 1's storage.
      // After appending to node 1's storage, broadcast them to all nodes.
      auto validators = rdpos1->getValidators();
      auto randomList = rdpos1->getRandomList();

      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the rdPoS in the order of the random list, limited to rdPoS::minValidators.
      orderedPrivKeys.reserve(4);
      for (const auto& privKey : validatorPrivKeys) {
        if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
          blockSignerPrivKey = privKey;
          break;
        }
      }

      for (uint64_t i = 1; i < rdPoS::minValidators + 1; i++) {
        for (const auto& privKey : validatorPrivKeys) {
          if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[i]) {
            orderedPrivKeys.push_back(privKey);
            break;
          }
        }
      }

      // By now we should have randomList[0] privKey in blockSignerPrivKey and the rest in orderedPrivKeys, ordered by the random list.
      // We can proceed with creating the block, transactions have to be **ordered** by the random list.

      // Create a block with 8 TxValidator transactions, 2 for each validator, in order (randomHash and random)
      uint64_t newBlocknHeight = storage1->latest()->getNHeight() + 1;
      std::vector<TxValidator> txValidators;

      std::vector<Hash> randomSeeds(orderedPrivKeys.size(), Hash::random());
      for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
        Bytes hashTxData = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(hashTxData, Utils::sha3(randomSeeds[i].get()));
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
        REQUIRE(rdpos1->addValidatorTx(tx));
      }

      // Broadcast the transactions
      for (const auto& tx : txValidators) {
        p2p1->broadcastTxValidator(tx);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(250));

      auto node1Mempool = rdpos1->getMempool();
      auto node2Mempool = rdpos2->getMempool();

      // As transactions were broadcasted, they should be included in both nodes.
      REQUIRE(node1Mempool == node2Mempool);

      // Clear mempool from node 1.
      rdpos1->clearMempool();

      // Request the transactions from node 1 to node 2
      std::vector<P2P::NodeID> nodesIds = p2p1->getSessionsIDs();
      REQUIRE(nodesIds.size() == 1);
      auto transactionList = p2p1->requestValidatorTxs(nodesIds[0]);

      REQUIRE(transactionList.size() == 8);

      // Append transactions back to node 1 mempool.
      for (const auto& tx : transactionList) {
        REQUIRE(rdpos1->addValidatorTx(tx));
      }

      // Check that the mempool is the same as before.
      node1Mempool = rdpos1->getMempool();
      REQUIRE(node1Mempool == node2Mempool);
    }

    SECTION("Ten NormalNodes and one DiscoveryNode, test broadcast") {
      // Initialize ten different node instances, with different ports and DBs.
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<Options> options1;
      std::unique_ptr<State> state1;
      initialize(db1, storage1, p2p1, validatorKey1, rdpos1, options1, state1, 8080, true, "rdPoSdiscoveryNodeTestBroadcastNode1");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<Options> options2;
      std::unique_ptr<State> state2;
      initialize(db2, storage2, p2p2, validatorKey2, rdpos2, options2, state2, 8081, true, "rdPoSdiscoveryNodeTestBroadcastNode2");

      std::unique_ptr<DB> db3;
      std::unique_ptr<Storage> storage3;
      std::unique_ptr<P2P::ManagerNormal> p2p3;
      PrivKey validatorKey3 = PrivKey();
      std::unique_ptr<rdPoS> rdpos3;
      std::unique_ptr<Options> options3;
      std::unique_ptr<State> state3;
      initialize(db3, storage3, p2p3, validatorKey3, rdpos3, options3, state3, 8082, true, "rdPoSdiscoveryNodeTestBroadcastNode3");

      std::unique_ptr<DB> db4;
      std::unique_ptr<Storage> storage4;
      std::unique_ptr<P2P::ManagerNormal> p2p4;
      PrivKey validatorKey4 = PrivKey();
      std::unique_ptr<rdPoS> rdpos4;
      std::unique_ptr<Options> options4;
      std::unique_ptr<State> state4;
      initialize(db4, storage4, p2p4, validatorKey4, rdpos4, options4, state4, 8083, true, "rdPoSdiscoveryNodeTestBroadcastNode4");

      std::unique_ptr<DB> db5;
      std::unique_ptr<Storage> storage5;
      std::unique_ptr<P2P::ManagerNormal> p2p5;
      PrivKey validatorKey5 = PrivKey();
      std::unique_ptr<rdPoS> rdpos5;
      std::unique_ptr<Options> options5;
      std::unique_ptr<State> state5;
      initialize(db5, storage5, p2p5, validatorKey5, rdpos5, options5, state5, 8084, true, "rdPoSdiscoveryNodeTestBroadcastNode5");

      std::unique_ptr<DB> db6;
      std::unique_ptr<Storage> storage6;
      std::unique_ptr<P2P::ManagerNormal> p2p6;
      PrivKey validatorKey6 = PrivKey();
      std::unique_ptr<rdPoS> rdpos6;
      std::unique_ptr<Options> options6;
      std::unique_ptr<State> state6;
      initialize(db6, storage6, p2p6, validatorKey6, rdpos6, options6, state6, 8085, true, "rdPoSdiscoveryNodeTestBroadcastNode6");

      std::unique_ptr<DB> db7;
      std::unique_ptr<Storage> storage7;
      std::unique_ptr<P2P::ManagerNormal> p2p7;
      PrivKey validatorKey7 = PrivKey();
      std::unique_ptr<rdPoS> rdpos7;
      std::unique_ptr<Options> options7;
      std::unique_ptr<State> state7;
      initialize(db7, storage7, p2p7, validatorKey7, rdpos7, options7, state7, 8086, true, "rdPoSdiscoveryNodeTestBroadcastNode7");

      std::unique_ptr<DB> db8;
      std::unique_ptr<Storage> storage8;
      std::unique_ptr<P2P::ManagerNormal> p2p8;
      PrivKey validatorKey8 = PrivKey();
      std::unique_ptr<rdPoS> rdpos8;
      std::unique_ptr<Options> options8;
      std::unique_ptr<State> state8;
      initialize(db8, storage8, p2p8, validatorKey8, rdpos8, options8, state8, 8087, true, "rdPoSdiscoveryNodeTestBroadcastNode8");

      std::unique_ptr<DB> db9;
      std::unique_ptr<Storage> storage9;
      std::unique_ptr<P2P::ManagerNormal> p2p9;
      PrivKey validatorKey9 = PrivKey();
      std::unique_ptr<rdPoS> rdpos9;
      std::unique_ptr<Options> options9;
      std::unique_ptr<State> state9;
      initialize(db9, storage9, p2p9, validatorKey9, rdpos9, options9, state9, 8088, true, "rdPoSdiscoveryNodeTestBroadcastNode9");

      std::unique_ptr<DB> db10;
      std::unique_ptr<Storage> storage10;
      std::unique_ptr<P2P::ManagerNormal> p2p10;
      PrivKey validatorKey10 = PrivKey();
      std::unique_ptr<rdPoS> rdpos10;
      std::unique_ptr<Options> options10;
      std::unique_ptr<State> state10;
      initialize(db10, storage10, p2p10, validatorKey10, rdpos10, options10, state10, 8089, true, "rdPoSdiscoveryNodeTestBroadcastNode10");

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> peers;
      std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
          "rdPoSdiscoveryNodeTestBroadcast",
          "OrbiterSDK/cpp/linux_x86-64/0.0.3",
          1,
          8080,
          8090,
          9999,
          peers
        );
      std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery  = std::make_unique<P2P::ManagerDiscovery>(boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

      // Start servers
      p2pDiscovery->start();
      p2p1->start();
      p2p2->start();
      p2p3->start();
      p2p4->start();
      p2p5->start();
      p2p6->start();
      p2p7->start();
      p2p8->start();
      p2p9->start();
      p2p10->start();

      // Connect nodes to the discovery node.
      p2p1->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p2->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p3->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p4->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p5->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p6->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p7->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p8->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p9->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      p2p10->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

      // Wait for connection towards discovery node.
      auto discoveryFuture = std::async (std::launch::async, [&] {
        while(p2pDiscovery->getSessionsIDs().size() != 10)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });

      REQUIRE(discoveryFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 10);
      REQUIRE(p2p1->getSessionsIDs().size() == 1);
      REQUIRE(p2p2->getSessionsIDs().size() == 1);
      REQUIRE(p2p3->getSessionsIDs().size() == 1);
      REQUIRE(p2p4->getSessionsIDs().size() == 1);
      REQUIRE(p2p5->getSessionsIDs().size() == 1);
      REQUIRE(p2p6->getSessionsIDs().size() == 1);
      REQUIRE(p2p7->getSessionsIDs().size() == 1);
      REQUIRE(p2p8->getSessionsIDs().size() == 1);
      REQUIRE(p2p9->getSessionsIDs().size() == 1);
      REQUIRE(p2p10->getSessionsIDs().size() == 1);

      p2pDiscovery->startDiscovery();
      p2p1->startDiscovery();
      p2p2->startDiscovery();
      p2p3->startDiscovery();
      p2p4->startDiscovery();
      p2p5->startDiscovery();
      p2p6->startDiscovery();
      p2p7->startDiscovery();
      p2p8->startDiscovery();
      p2p9->startDiscovery();
      p2p10->startDiscovery();


      auto connectionsFuture = std::async(std::launch::async, [&]() {
        while(p2p1->getSessionsIDs().size() != 10 ||
              p2p2->getSessionsIDs().size() != 10 ||
              p2p3->getSessionsIDs().size() != 10 ||
              p2p4->getSessionsIDs().size() != 10 ||
              p2p5->getSessionsIDs().size() != 10 ||
              p2p6->getSessionsIDs().size() != 10 ||
              p2p7->getSessionsIDs().size() != 10 ||
              p2p8->getSessionsIDs().size() != 10 ||
              p2p9->getSessionsIDs().size() != 10 ||
              p2p10->getSessionsIDs().size() != 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });

      REQUIRE(connectionsFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      // Stop discovery after all nodes have connected to each other.
      // Making so that the broadcast down this line takes too long to complete
      p2p1->stopDiscovery();
      p2p2->stopDiscovery();
      p2p3->stopDiscovery();
      p2p4->stopDiscovery();
      p2p5->stopDiscovery();
      p2p6->stopDiscovery();
      p2p7->stopDiscovery();
      p2p8->stopDiscovery();
      p2p9->stopDiscovery();
      p2p10->stopDiscovery();
      p2pDiscovery->stopDiscovery();

      // Create valid TxValidator transactions (8 in total), append them to node 1's storage.
      // After appending to node 1's storage, broadcast them to all nodes.
      auto validators = rdpos1->getValidators();
      auto randomList = rdpos1->getRandomList();

      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the rdPoS in the order of the random list, limited to rdPoS::minValidators.
      orderedPrivKeys.reserve(4);
      for (const auto& privKey : validatorPrivKeys) {
        if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
          blockSignerPrivKey = privKey;
          break;
        }
      }

      for (uint64_t i = 1; i < rdPoS::minValidators + 1; i++) {
        for (const auto& privKey : validatorPrivKeys) {
          if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[i]) {
            orderedPrivKeys.push_back(privKey);
            break;
          }
        }
      }

      // By now we should have randomList[0] privKey in blockSignerPrivKey and the rest in orderedPrivKeys, ordered by the random list.
      // We can proceed with creating the block, transactions have to be **ordered** by the random list.

      // Create a block with 8 TxValidator transactions, 2 for each validator, in order (randomHash and random)
      uint64_t newBlocknHeight = storage1->latest()->getNHeight() + 1;
      std::vector<TxValidator> txValidators;
      std::vector<Hash> randomSeeds(orderedPrivKeys.size(), Hash::random());
      for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
        Bytes hashTxData = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(hashTxData, Utils::sha3(randomSeeds[i].get()));
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
        REQUIRE(rdpos1->addValidatorTx(tx));
      }

      // Broadcast transactions to all nodes.
      for (const auto& tx : txValidators) {
        p2p1->broadcastTxValidator(tx);
      }

      /// Wait till transactions are broadcasted
      auto finalMempool = rdpos1->getMempool();

      auto broadcastFuture = std::async(std::launch::async, [&]() {
        while(rdpos2->getMempool() != rdpos1->getMempool() ||
              rdpos3->getMempool() != rdpos1->getMempool() ||
            rdpos4->getMempool() != rdpos1->getMempool() ||
            rdpos5->getMempool() != rdpos1->getMempool() ||
            rdpos6->getMempool() != rdpos1->getMempool() ||
            rdpos7->getMempool() != rdpos1->getMempool() ||
            rdpos8->getMempool() != rdpos1->getMempool() ||
            rdpos9->getMempool() != rdpos1->getMempool() ||
            rdpos10->getMempool() != rdpos1->getMempool()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });

      REQUIRE(broadcastFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      // Check if all mempools matchs
      auto node1Mempool = rdpos1->getMempool();
      auto node2Mempool = rdpos2->getMempool();
      auto node3Mempool = rdpos3->getMempool();
      auto node4Mempool = rdpos4->getMempool();
      auto node5Mempool = rdpos5->getMempool();
      auto node6Mempool = rdpos6->getMempool();
      auto node7Mempool = rdpos7->getMempool();
      auto node8Mempool = rdpos8->getMempool();
      auto node9Mempool = rdpos9->getMempool();
      auto node10Mempool = rdpos10->getMempool();

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
    // Initialize 8 different node instances, with different ports and DBs.
    std::unique_ptr<DB> db1;
    std::unique_ptr<Storage> storage1;
    std::unique_ptr<P2P::ManagerNormal> p2p1;
    PrivKey validatorKey1 = PrivKey();
    std::unique_ptr<rdPoS> rdpos1;
    std::unique_ptr<Options> options1;
    std::unique_ptr<State> state1;
    initialize(db1, storage1, p2p1, validatorPrivKeys[0], rdpos1, options1, state1, 8080, true, "rdPoSdiscoveryNodeTestMove10BlocksNode1");

    std::unique_ptr<DB> db2;
    std::unique_ptr<Storage> storage2;
    std::unique_ptr<P2P::ManagerNormal> p2p2;
    PrivKey validatorKey2 = PrivKey();
    std::unique_ptr<rdPoS> rdpos2;
    std::unique_ptr<Options> options2;
    std::unique_ptr<State> state2;
    initialize(db2, storage2, p2p2, validatorPrivKeys[1], rdpos2, options2, state2, 8081, true, "rdPoSdiscoveryNodeTestMove10BlocksNode2");

    std::unique_ptr<DB> db3;
    std::unique_ptr<Storage> storage3;
    std::unique_ptr<P2P::ManagerNormal> p2p3;
    PrivKey validatorKey3 = PrivKey();
    std::unique_ptr<rdPoS> rdpos3;
    std::unique_ptr<Options> options3;
    std::unique_ptr<State> state3;
    initialize(db3, storage3, p2p3, validatorPrivKeys[2], rdpos3, options3, state3, 8082, true, "rdPoSdiscoveryNodeTestMove10BlocksNode3");

    std::unique_ptr<DB> db4;
    std::unique_ptr<Storage> storage4;
    std::unique_ptr<P2P::ManagerNormal> p2p4;
    PrivKey validatorKey4 = PrivKey();
    std::unique_ptr<rdPoS> rdpos4;
    std::unique_ptr<Options> options4;
    std::unique_ptr<State> state4;
    initialize(db4, storage4, p2p4, validatorPrivKeys[3], rdpos4, options4, state4, 8083, true, "rdPoSdiscoveryNodeTestMove10BlocksNode4");

    std::unique_ptr<DB> db5;
    std::unique_ptr<Storage> storage5;
    std::unique_ptr<P2P::ManagerNormal> p2p5;
    PrivKey validatorKey5 = PrivKey();
    std::unique_ptr<rdPoS> rdpos5;
    std::unique_ptr<Options> options5;
    std::unique_ptr<State> state5;
    initialize(db5, storage5, p2p5, validatorPrivKeys[4], rdpos5, options5, state5, 8084, true, "rdPoSdiscoveryNodeTestMove10BlocksNode5");

    std::unique_ptr<DB> db6;
    std::unique_ptr<Storage> storage6;
    std::unique_ptr<P2P::ManagerNormal> p2p6;
    PrivKey validatorKey6 = PrivKey();
    std::unique_ptr<rdPoS> rdpos6;
    std::unique_ptr<Options> options6;
    std::unique_ptr<State> state6;
    initialize(db6, storage6, p2p6, validatorPrivKeys[5], rdpos6, options6, state6, 8085, true, "rdPoSdiscoveryNodeTestMove10BlocksNode6");

    std::unique_ptr<DB> db7;
    std::unique_ptr<Storage> storage7;
    std::unique_ptr<P2P::ManagerNormal> p2p7;
    PrivKey validatorKey7 = PrivKey();
    std::unique_ptr<rdPoS> rdpos7;
    std::unique_ptr<Options> options7;
    std::unique_ptr<State> state7;
    initialize(db7, storage7, p2p7, validatorPrivKeys[6], rdpos7, options7, state7, 8086, true, "rdPoSdiscoveryNodeTestMove10BlocksNode7");

    std::unique_ptr<DB> db8;
    std::unique_ptr<Storage> storage8;
    std::unique_ptr<P2P::ManagerNormal> p2p8;
    PrivKey validatorKey8 = PrivKey();
    std::unique_ptr<rdPoS> rdpos8;
    std::unique_ptr<Options> options8;
    std::unique_ptr<State> state8;
    initialize(db8, storage8, p2p8, validatorPrivKeys[7], rdpos8, options8, state8, 8087, true, "rdPoSdiscoveryNodeTestMove10BlocksNode8");

    // Initialize the discovery node.
    std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
    std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
      "rdPoSdiscoveryNodeTestMove10Blocks",
      "OrbiterSDK/cpp/linux_x86-64/0.0.3",
      1,
      8080,
      8090,
      9999,
      discoveryNodes
    );
    std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery  = std::make_unique<P2P::ManagerDiscovery>(boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

    // References for the rdPoS workers vector.
    std::vector<std::reference_wrapper<std::unique_ptr<rdPoS>>> rdPoSreferences;
    rdPoSreferences.emplace_back(rdpos1);
    rdPoSreferences.emplace_back(rdpos2);
    rdPoSreferences.emplace_back(rdpos3);
    rdPoSreferences.emplace_back(rdpos4);
    rdPoSreferences.emplace_back(rdpos5);
    rdPoSreferences.emplace_back(rdpos6);
    rdPoSreferences.emplace_back(rdpos7);
    rdPoSreferences.emplace_back(rdpos8);

    // Start servers
    p2pDiscovery->start();
    p2p1->start();
    p2p2->start();
    p2p3->start();
    p2p4->start();
    p2p5->start();
    p2p6->start();
    p2p7->start();
    p2p8->start();

    // Connect nodes to the discovery node.
    p2p1->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
    p2p2->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
    p2p3->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
    p2p4->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
    p2p5->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
    p2p6->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
    p2p7->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
    p2p8->connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

    // Wait for connection towards discovery node.
    auto discoveryFuture = std::async(std::launch::async, [&]() {
      while(p2pDiscovery->getSessionsIDs().size() != 8)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });

    REQUIRE(discoveryFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);


    REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
    REQUIRE(p2p1->getSessionsIDs().size() == 1);
    REQUIRE(p2p2->getSessionsIDs().size() == 1);
    REQUIRE(p2p3->getSessionsIDs().size() == 1);
    REQUIRE(p2p4->getSessionsIDs().size() == 1);
    REQUIRE(p2p5->getSessionsIDs().size() == 1);
    REQUIRE(p2p6->getSessionsIDs().size() == 1);
    REQUIRE(p2p7->getSessionsIDs().size() == 1);
    REQUIRE(p2p8->getSessionsIDs().size() == 1);

    p2pDiscovery->startDiscovery();
    p2p1->startDiscovery();
    p2p2->startDiscovery();
    p2p3->startDiscovery();
    p2p4->startDiscovery();
    p2p5->startDiscovery();
    p2p6->startDiscovery();
    p2p7->startDiscovery();
    p2p8->startDiscovery();


    auto connectionFuture = std::async(std::launch::async, [&]() {
      while(p2pDiscovery->getSessionsIDs().size() != 8 ||
            p2p1->getSessionsIDs().size() != 8 ||
            p2p2->getSessionsIDs().size() != 8 ||
            p2p3->getSessionsIDs().size() != 8 ||
            p2p4->getSessionsIDs().size() != 8 ||
            p2p5->getSessionsIDs().size() != 8 ||
            p2p6->getSessionsIDs().size() != 8 ||
            p2p7->getSessionsIDs().size() != 8 ||
            p2p8->getSessionsIDs().size() != 8) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });

    REQUIRE(connectionFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

    // Stop discovery after all nodes have connected to each other.
    // Making so that the broadcast down this line takes too long to complete
    p2p1->stopDiscovery();
    p2p2->stopDiscovery();
    p2p3->stopDiscovery();
    p2p4->stopDiscovery();
    p2p5->stopDiscovery();
    p2p6->stopDiscovery();
    p2p7->stopDiscovery();
    p2p8->stopDiscovery();
    p2pDiscovery->stopDiscovery();


    // After a while, the discovery thread should have found all the nodes and connected between each other.

    REQUIRE(rdpos1->getIsValidator());
    REQUIRE(rdpos2->getIsValidator());
    REQUIRE(rdpos3->getIsValidator());
    REQUIRE(rdpos4->getIsValidator());
    REQUIRE(rdpos5->getIsValidator());
    REQUIRE(rdpos6->getIsValidator());
    REQUIRE(rdpos7->getIsValidator());
    REQUIRE(rdpos8->getIsValidator());

    rdpos1->startrdPoSWorker();
    rdpos2->startrdPoSWorker();
    rdpos3->startrdPoSWorker();
    rdpos4->startrdPoSWorker();
    rdpos5->startrdPoSWorker();
    rdpos6->startrdPoSWorker();
    rdpos7->startrdPoSWorker();
    rdpos8->startrdPoSWorker();

    // Loop for block creation.
    uint64_t blocks = 0;
    while (blocks < 10) {
      auto rdPoSmempoolFuture = std::async(std::launch::async, [&]() {
        while (rdpos1->getMempool().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });

      REQUIRE(rdPoSmempoolFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      for (auto &blockCreator: rdPoSreferences) {
        if (blockCreator.get()->canCreateBlock()) {
          // Create the block.
          auto mempool = blockCreator.get()->getMempool();
          auto randomList = blockCreator.get()->getRandomList();
          // Order the transactions in the proper manner.
          std::vector<TxValidator> randomHashTxs;
          std::vector<TxValidator> randomnessTxs;
          uint64_t i = 1;
          while (randomHashTxs.size() != rdPoS::minValidators) {
            for (const auto [txHash, tx]: mempool) {
              if (tx.getFrom() == randomList[i]) {
                if (Bytes(tx.getData().begin(), tx.getData().begin() + 4) == Hex::toBytes("0xcfffe746")) {
                  randomHashTxs.emplace_back(tx);
                  ++i;
                  break;
                }
              }
            }
          }
          i = 1;
          while (randomnessTxs.size() != rdPoS::minValidators) {
            for (const auto [txHash, tx]: mempool) {
              if (tx.getFrom() == randomList[i]) {
                if (Bytes(tx.getData().begin(), tx.getData().begin() + 4) == Hex::toBytes("0x6fc5a2d6")) {
                  randomnessTxs.emplace_back(tx);
                  ++i;
                  break;
                }
              }
            }
          }

          // Create the block and append to all chains, we can use any storage for latestblock
          auto latestBlock = storage1->latest();
          Block block(latestBlock->hash(), latestBlock->getTimestamp(), latestBlock->getNHeight() + 1);
          // Append transactions towards block.
          for (const auto &tx: randomHashTxs) {
            block.appendTxValidator(tx);
          }
          for (const auto &tx: randomnessTxs) {
            block.appendTxValidator(tx);
          }

          blockCreator.get()->signBlock(block);
          // Validate the block.
          REQUIRE(rdpos2->validateBlock(block));
          REQUIRE(rdpos3->validateBlock(block));
          REQUIRE(rdpos4->validateBlock(block));
          REQUIRE(rdpos5->validateBlock(block));
          REQUIRE(rdpos1->validateBlock(block));
          REQUIRE(rdpos8->validateBlock(block));
          REQUIRE(rdpos6->validateBlock(block));
          REQUIRE(rdpos7->validateBlock(block));

          rdpos1->processBlock(block);
          storage1->pushBack(Block(block));

          rdpos2->processBlock(block);
          storage2->pushBack(Block(block));

          rdpos3->processBlock(block);
          storage3->pushBack(Block(block));

          rdpos4->processBlock(block);
          storage4->pushBack(Block(block));

          rdpos5->processBlock(block);
          storage5->pushBack(Block(block));

          rdpos6->processBlock(block);
          storage6->pushBack(Block(block));

          rdpos7->processBlock(block);
          storage7->pushBack(Block(block));

          rdpos8->processBlock(block);
          storage8->pushBack(Block(block));
          ++blocks;
          break;
        }
      }
    }

    rdpos1->stoprdPoSWorker();
    rdpos2->stoprdPoSWorker();
    rdpos3->stoprdPoSWorker();
    rdpos4->stoprdPoSWorker();
    rdpos5->stoprdPoSWorker();
    rdpos6->stoprdPoSWorker();
    rdpos7->stoprdPoSWorker();
    rdpos8->stoprdPoSWorker();
    // Sleep so it can conclude the last operations.
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
};