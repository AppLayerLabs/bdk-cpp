#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/rdpos.h"
#include "../../src/core/storage.h"
#include "../../src/utils/db.h"
#include "../../src/net/p2p/p2pmanagernormal.h"
#include "../../src/net/p2p/p2pmanagerdiscovery.h"

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
                uint64_t serverPort,
                bool clearDb = true,
                std::string dbPrefix = "") {
  std::string dbName = dbPrefix + "rdPoStests";
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
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")));
    db->put("latest", genesis.serializeBlock(), DBPrefix::blocks);
    db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    db->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);

    // Populate rdPoS DB with unique validators, not default.
    // PrivateKey: 0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8
    db->put(Utils::uint64ToBytes(0), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[0]))).get(), DBPrefix::validators);
    // PrivateKey: b254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66
    db->put(Utils::uint64ToBytes(1), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[1]))).get(), DBPrefix::validators);
    // PrivateKey: 8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da
    db->put(Utils::uint64ToBytes(2), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[2]))).get(), DBPrefix::validators);
    // PrivateKey: 9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745
    db->put(Utils::uint64ToBytes(3), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[3]))).get(), DBPrefix::validators);
    // PrivateKey: 0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739
    db->put(Utils::uint64ToBytes(4), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[4]))).get(), DBPrefix::validators);
    // PrivateKey: a69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7
    db->put(Utils::uint64ToBytes(5), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[5]))).get(), DBPrefix::validators);
    // PrivateKey: d9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5
    db->put(Utils::uint64ToBytes(6), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[6]))).get(), DBPrefix::validators);
    // PrivateKey: 426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f
    db->put(Utils::uint64ToBytes(7), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[7]))).get(), DBPrefix::validators);
  }

  storage = std::make_unique<Storage>(db);
  p2p = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), serverPort, rdpos);
  rdpos = std::make_unique<rdPoS>(db, 8080, storage, p2p, validatorKey);
}


// This creates a valid block given the state within the rdPoS class.
// Should not be used during network/thread testing, as it will automatically sign all TxValidator transactions within the block
// And that is not the purpose of network/thread testing.
Block createValidBlock(std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage) {
  auto validators = rdpos->getValidators();
  auto randomList = rdpos->getRandomList();

  Hash blockSignerPrivKey;           // Private key for the block signer.
  std::vector<Hash> orderedPrivKeys; // Private keys for the validators in the order of the random list, limited to rdPoS::minValidators.
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
  uint64_t newBlockTimestamp = storage->latest()->getTimestamp() + 100000;
  Hash newBlockPrevHash = storage->latest()->hash();
  Block block(newBlockPrevHash, newBlockTimestamp, newBlocknHeight);
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomTxs;

  std::vector<Hash> randomSeeds(orderedPrivKeys.size(), Hash::random());
  for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
    Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
    std::string hashTxData = Hex::toBytes("0xcfffe746") + Utils::sha3(randomSeeds[i].get()).get();
    std::string randomTxData = Hex::toBytes("0x6fc5a2d6") + randomSeeds[i].get();
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
  block.finalize(blockSignerPrivKey);
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
        initialize(db, storage, p2p, validatorKey, rdpos, 8080);

        auto validators = rdpos->getValidators();
        REQUIRE(rdpos->getValidators().size() == 8);
        REQUIRE(validators.contains(Address(Hex::toBytes("1531bfdf7d48555a0034e4647fa46d5a04c002c3"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("e3dff2cc3f367df7d0254c834a0c177064d7c7f5"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("24e10d8ebe80abd3d3fddd89a26f08f3888d1380"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("b5f7152a2589c6cc2535c5facedfc853194d60a5"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("098ff62812043f5106db718e5c4349111de3b6b4"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("50d2ce9815e0e2354de7834f6fdd4d6946442a24"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("7c2b2a0a75e10b49e652d99bba8afee3a6bc78dd"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("6e67067edc1b4837b67c0b1def689eddee257521"), true)));
        REQUIRE(rdpos->getBestRandomSeed() == Hash()); // Genesis blocks randomness is 0.

        auto randomList = rdpos->getRandomList();
        validatorsList = rdpos->getValidators();
        REQUIRE(randomList.size() == 8);
        for (const auto& i : randomList) {
          REQUIRE(validatorsList.contains(i));
        }

        // Check ordering of random list. deterministic.
        REQUIRE(randomList[0] == Address(Hex::toBytes("50d2ce9815e0e2354de7834f6fdd4d6946442a24"), true));
        REQUIRE(randomList[1] == Address(Hex::toBytes("6e67067edc1b4837b67c0b1def689eddee257521"), true));
        REQUIRE(randomList[2] == Address(Hex::toBytes("24e10d8ebe80abd3d3fddd89a26f08f3888d1380"), true));
        REQUIRE(randomList[3] == Address(Hex::toBytes("7c2b2a0a75e10b49e652d99bba8afee3a6bc78dd"), true));
        REQUIRE(randomList[4] == Address(Hex::toBytes("1531bfdf7d48555a0034e4647fa46d5a04c002c3"), true));
        REQUIRE(randomList[5] == Address(Hex::toBytes("b5f7152a2589c6cc2535c5facedfc853194d60a5"), true));
        REQUIRE(randomList[6] == Address(Hex::toBytes("e3dff2cc3f367df7d0254c834a0c177064d7c7f5"), true));
        REQUIRE(randomList[7] == Address(Hex::toBytes("098ff62812043f5106db718e5c4349111de3b6b4"), true));
      }
      
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      PrivKey validatorKey = PrivKey();
      std::unique_ptr<rdPoS> rdpos;
      initialize(db, storage, p2p, validatorKey, rdpos, 8080, false);

      auto validators = rdpos->getValidators();
      REQUIRE(validators == validatorsList);
    }

    SECTION ("rdPoS validateBlock(), one block from genesis") {
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      PrivKey validatorKey = PrivKey();
      std::unique_ptr<rdPoS> rdpos;
      initialize(db, storage, p2p, validatorKey, rdpos, 8080);

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
        initialize(db, storage, p2p, validatorKey, rdpos, 8080);

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
      // Initialize same DB and storage as before.
      initialize(db, storage, p2p, validatorKey, rdpos, 8080, false);

      REQUIRE(rdpos->getBestRandomSeed() == expectedRandomnessFromBestBlock);
      REQUIRE(rdpos->getRandomList() == expectedRandomList);
    }
  }

  TEST_CASE("rdPoS Class With Network Functionality", "[core][rdpos][net][p2p]") {
    SECTION("Two Nodes instances, simple transaction broadcast") {
      // Initialize two different node instances, with different ports and DBs.
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      initialize(db1, storage1, p2p1, validatorKey1, rdpos1, 8080, true, "node1");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      initialize(db2, storage2, p2p2, validatorKey2, rdpos2, 8081, true, "node2");


      // Start respective p2p servers, and connect each other.
      p2p1->startServer();
      p2p2->startServer();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      p2p1->connectToServer("127.0.0.1", 8081);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      REQUIRE(p2p1->getSessionsIDs().size() == 1);

      // Create valid TxValidator transactions (8 in total), append them to node 1's storage.
      // After appending to node 1's storage, broadcast them to all nodes.
      auto validators = rdpos1->getValidators();
      auto randomList = rdpos1->getRandomList();

      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the validators in the order of the random list, limited to rdPoS::minValidators.
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
        std::string hashTxData = Hex::toBytes("0xcfffe746") + Utils::sha3(randomSeeds[i].get()).get();
        std::string randomTxData = Hex::toBytes("0x6fc5a2d6") + randomSeeds[i].get();
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
      std::vector<Hash> nodesIds = p2p1->getSessionsIDs();
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
      initialize(db1, storage1, p2p1, validatorKey1, rdpos1, 8080, true, "node1");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      initialize(db2, storage2, p2p2, validatorKey2, rdpos2, 8081, true, "node2");

      std::unique_ptr<DB> db3;
      std::unique_ptr<Storage> storage3;
      std::unique_ptr<P2P::ManagerNormal> p2p3;
      PrivKey validatorKey3 = PrivKey();
      std::unique_ptr<rdPoS> rdpos3;
      initialize(db3, storage3, p2p3, validatorKey3, rdpos3, 8082, true, "node3");

      std::unique_ptr<DB> db4;
      std::unique_ptr<Storage> storage4;
      std::unique_ptr<P2P::ManagerNormal> p2p4;
      PrivKey validatorKey4 = PrivKey();
      std::unique_ptr<rdPoS> rdpos4;
      initialize(db4, storage4, p2p4, validatorKey4, rdpos4, 8083, true, "node4");

      std::unique_ptr<DB> db5;
      std::unique_ptr<Storage> storage5;
      std::unique_ptr<P2P::ManagerNormal> p2p5;
      PrivKey validatorKey5 = PrivKey();
      std::unique_ptr<rdPoS> rdpos5;
      initialize(db5, storage5, p2p5, validatorKey5, rdpos5, 8084, true, "node5");

      std::unique_ptr<DB> db6;
      std::unique_ptr<Storage> storage6;
      std::unique_ptr<P2P::ManagerNormal> p2p6;
      PrivKey validatorKey6 = PrivKey();
      std::unique_ptr<rdPoS> rdpos6;
      initialize(db6, storage6, p2p6, validatorKey6, rdpos6, 8085, true, "node6");

      std::unique_ptr<DB> db7;
      std::unique_ptr<Storage> storage7;
      std::unique_ptr<P2P::ManagerNormal> p2p7;
      PrivKey validatorKey7 = PrivKey();
      std::unique_ptr<rdPoS> rdpos7;
      initialize(db7, storage7, p2p7, validatorKey7, rdpos7, 8086, true, "node7");

      std::unique_ptr<DB> db8;
      std::unique_ptr<Storage> storage8;
      std::unique_ptr<P2P::ManagerNormal> p2p8;
      PrivKey validatorKey8 = PrivKey();
      std::unique_ptr<rdPoS> rdpos8;
      initialize(db8, storage8, p2p8, validatorKey8, rdpos8, 8087, true, "node8");

      std::unique_ptr<DB> db9;
      std::unique_ptr<Storage> storage9;
      std::unique_ptr<P2P::ManagerNormal> p2p9;
      PrivKey validatorKey9 = PrivKey();
      std::unique_ptr<rdPoS> rdpos9;
      initialize(db9, storage9, p2p9, validatorKey9, rdpos9, 8088, true, "node9");

      std::unique_ptr<DB> db10;
      std::unique_ptr<Storage> storage10;
      std::unique_ptr<P2P::ManagerNormal> p2p10;
      PrivKey validatorKey10 = PrivKey();
      std::unique_ptr<rdPoS> rdpos10;
      initialize(db10, storage10, p2p10, validatorKey10, rdpos10, 8089, true, "node10");

      // Initialize the discovery node.
      std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery  = std::make_unique<P2P::ManagerDiscovery>(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

      // Start servers
      p2pDiscovery->startServer();
      p2p1->startServer();
      p2p2->startServer();
      p2p3->startServer();
      p2p4->startServer();
      p2p5->startServer();
      p2p6->startServer();
      p2p7->startServer();
      p2p8->startServer();
      p2p9->startServer();
      p2p10->startServer();

      // Connect nodes to the discovery node.
      p2p1->connectToServer("127.0.0.1", 8090);
      p2p2->connectToServer("127.0.0.1", 8090);
      p2p3->connectToServer("127.0.0.1", 8090);
      p2p4->connectToServer("127.0.0.1", 8090);
      p2p5->connectToServer("127.0.0.1", 8090);
      p2p6->connectToServer("127.0.0.1", 8090);
      p2p7->connectToServer("127.0.0.1", 8090);
      p2p8->connectToServer("127.0.0.1", 8090);
      p2p9->connectToServer("127.0.0.1", 8090);
      p2p10->connectToServer("127.0.0.1", 8090);

			// After a while, the discovery thread should have found all the nodes and connected between each other.
			std::this_thread::sleep_for(std::chrono::seconds(10));

      REQUIRE(p2pDiscovery->getSessionsIDs().size());
      
      // Create valid TxValidator transactions (8 in total), append them to node 1's storage.
      // After appending to node 1's storage, broadcast them to all nodes.
      auto validators = rdpos1->getValidators();
      auto randomList = rdpos1->getRandomList();

      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the validators in the order of the random list, limited to rdPoS::minValidators.
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
        std::string hashTxData = Hex::toBytes("0xcfffe746") + Utils::sha3(randomSeeds[i].get()).get();
        std::string randomTxData = Hex::toBytes("0x6fc5a2d6") + randomSeeds[i].get();
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

      std::this_thread::sleep_for(std::chrono::seconds(1));

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

  TEST_CASE("rdPoS class with Network and rdPoSWorker Functionality", "[core][rdpos][net][p2p]") {

  }
};