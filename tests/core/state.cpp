#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/rdpos.h"
#include "../../src/core/storage.h"
#include "../../src/core/state.h"
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

// This creates a valid block given the state within the rdPoS class.
// Should not be used during network/thread testing, as it will automatically sign all TxValidator transactions within the block
// And that is not the purpose of network/thread testing.
// Definition from state.cpp, when linking, the compiler should find the function.
Block createValidBlock(std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage, const std::vector<TxBlock>& txs = {});

// We initialize the blockchain database
// To make sure that if the genesis is changed within the main source code
// The tests will still work, as tests uses own genesis block.
void initialize(std::unique_ptr<DB>& db,
                std::unique_ptr<Storage>& storage,
                std::unique_ptr<P2P::ManagerNormal>& p2p,
                std::unique_ptr<rdPoS>& rdpos,
                std::unique_ptr<State>& state,
                PrivKey validatorKey,
                uint64_t serverPort,
                bool clearDb = true,
                std::string dbPrefix = "") {
  std::string dbName = dbPrefix + "stateTests";
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
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")),1678887538000000);
    db->put("latest", genesis.serializeBlock(), DBPrefix::blocks);
    db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    db->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);

    // Populate rdPoS DB with unique validators, not default.
    for (uint64_t i = 0; i < validatorPrivKeys.size(); ++i) {
      db->put(Utils::uint64ToBytes(i), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[i]))).get(),
              DBPrefix::validators);
    }
    // Populate State DB with one address.
    /// Initialize with 0x00dead00665771855a34155f5e7405489df2c3c6 with nonce 0.
    Address dev1(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true);
    /// See ~State for encoding
    uint256_t desiredBalance("1000000000000000000000");
    std::string value = Utils::uintToBytes(Utils::bytesRequired(desiredBalance)) + Utils::uintToBytes(desiredBalance) + '\x00';
    db->put(dev1.get(), value, DBPrefix::nativeAccounts);
  }

  storage = std::make_unique<Storage>(db);
  p2p = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), serverPort, rdpos);
  rdpos = std::make_unique<rdPoS>(db, 8080, storage, p2p, validatorKey);
  state = std::make_unique<State>(db, storage, rdpos, p2p);
}

namespace TState {
  TEST_CASE("State Class", "[core][state]") {
    SECTION("State Class Constructor/Destructor", "[state]") {
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, true, "stateConstructorTest");
        REQUIRE(state->getNativeBalance(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true)) ==
                uint256_t("1000000000000000000000"));
      }
      // Wait a little until everyone has been destructed.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      //// Check if opening the state loads successfully from DB.
      initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, false, "stateConstructorTest");
      REQUIRE(state->getNativeBalance(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true)) ==
              uint256_t("1000000000000000000000"));
      REQUIRE(state->getNativeNonce(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true)) == 0);
    }

    SECTION("State Class addBalance to random Addresses") {
      std::vector<std::pair<Address, uint256_t>> addresses;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, true, "stateAddBalanceTest");

        for (uint64_t i = 0; i < 1024; ++i) {
          std::pair<Address, uint256_t> randomAddress = std::make_pair(Address(Utils::randBytes(20), true),
                                                                       uint256_t("1000000000000000000000"));
          state->addBalance(randomAddress.first);
          addresses.push_back(randomAddress);
        }

        for (const auto &[address, expectedBalance]: addresses) {
          REQUIRE(state->getNativeBalance(address) == expectedBalance);
          REQUIRE(state->getNativeNonce(address) == 0);
        }
      }
      // Wait until destructors are called.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // Load everything back from DB.
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, false, "stateAddBalanceTest");
      for (const auto &[address, expectedBalance]: addresses) {
        REQUIRE(state->getNativeBalance(address) == expectedBalance);
        REQUIRE(state->getNativeNonce(address) == 0);
      }
    }

    SECTION("Test Simple block on State (No Transactions only rdPoS") {
      std::unique_ptr<Block> latestBlock = nullptr;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, true, "stateSimpleBlockTest");

        auto newBlock = createValidBlock(rdpos, storage);
        REQUIRE(state->validateNextBlock(newBlock));
        state->processNextBlock(std::move(newBlock));
        latestBlock = std::make_unique<Block>(*storage->latest().get());
      }
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, false, "stateSimpleBlockTest");

      REQUIRE(latestBlock->hash() == storage->latest()->hash());
    }

    SECTION("Test Block with Transactions on State") {
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 500; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20), true);
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, true, "stateSimpleBlockTest");

        /// Add balance to the random Accounts and create random transactions
        std::vector<TxBlock> transactions;
        for (auto &[privkey, val]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          state->addBalance(me);
          transactions.emplace_back(
            targetOfTransactions,
            me,
            "",
            8080,
            state->getNativeNonce(me),
            1000000000000000000,
            21000,
            1000000000,
            privkey
          );

          /// Take note of expected balance and nonce
          val.first = state->getNativeBalance(me) - (transactions.back().getGasPrice() * transactions.back().getGas()) -
                      transactions.back().getValue();
          val.second = state->getNativeNonce(me) + 1;
        }

        auto newBestBlock = createValidBlock(rdpos, storage, transactions);
        REQUIRE(state->validateNextBlock(newBestBlock));

        state->processNextBlock(std::move(newBestBlock));

        for (const auto &[privkey, val]: randomAccounts) {
          auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          REQUIRE(state->getNativeBalance(me) == val.first);
          REQUIRE(state->getNativeNonce(me) == val.second);
        }
      }
    }
    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, no transactions") {
      // Initialize 8 different node instances, with different ports and DBs.
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<State> state1;
      initialize(db1, storage1, p2p1, rdpos1, state1, validatorPrivKeys[0], 8080, true, "node1");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<State> state2;
      initialize(db2, storage2, p2p2, rdpos2, state2, validatorPrivKeys[1], 8081, true, "node2");

      std::unique_ptr<DB> db3;
      std::unique_ptr<Storage> storage3;
      std::unique_ptr<P2P::ManagerNormal> p2p3;
      PrivKey validatorKey3 = PrivKey();
      std::unique_ptr<rdPoS> rdpos3;
      std::unique_ptr<State> state3;
      initialize(db3, storage3, p2p3, rdpos3, state3, validatorPrivKeys[2], 8082, true, "node3");

      std::unique_ptr<DB> db4;
      std::unique_ptr<Storage> storage4;
      std::unique_ptr<P2P::ManagerNormal> p2p4;
      PrivKey validatorKey4 = PrivKey();
      std::unique_ptr<rdPoS> rdpos4;
      std::unique_ptr<State> state4;
      initialize(db4, storage4, p2p4, rdpos4, state4, validatorPrivKeys[3], 8083, true, "node4");

      std::unique_ptr<DB> db5;
      std::unique_ptr<Storage> storage5;
      std::unique_ptr<P2P::ManagerNormal> p2p5;
      PrivKey validatorKey5 = PrivKey();
      std::unique_ptr<rdPoS> rdpos5;
      std::unique_ptr<State> state5;
      initialize(db5, storage5, p2p5, rdpos5, state5, validatorPrivKeys[4], 8084, true, "node5");

      std::unique_ptr<DB> db6;
      std::unique_ptr<Storage> storage6;
      std::unique_ptr<P2P::ManagerNormal> p2p6;
      PrivKey validatorKey6 = PrivKey();
      std::unique_ptr<rdPoS> rdpos6;
      std::unique_ptr<State> state6;
      initialize(db6, storage6, p2p6, rdpos6, state6, validatorPrivKeys[5], 8085, true, "node6");

      std::unique_ptr<DB> db7;
      std::unique_ptr<Storage> storage7;
      std::unique_ptr<P2P::ManagerNormal> p2p7;
      PrivKey validatorKey7 = PrivKey();
      std::unique_ptr<rdPoS> rdpos7;
      std::unique_ptr<State> state7;
      initialize(db7, storage7, p2p7, rdpos7, state7, validatorPrivKeys[6], 8086, true, "node7");

      std::unique_ptr<DB> db8;
      std::unique_ptr<Storage> storage8;
      std::unique_ptr<P2P::ManagerNormal> p2p8;
      PrivKey validatorKey8 = PrivKey();
      std::unique_ptr<rdPoS> rdpos8;
      std::unique_ptr<State> state8;
      initialize(db8, storage8, p2p8, rdpos8, state8, validatorPrivKeys[7], 8087, true, "node8");

      // Initialize the discovery node.
      std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery = std::make_unique<P2P::ManagerDiscovery>(
        boost::asio::ip::address::from_string("127.0.0.1"), 8090);

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
      p2pDiscovery->startServer();
      p2p1->startServer();
      p2p2->startServer();
      p2p3->startServer();
      p2p4->startServer();
      p2p5->startServer();
      p2p6->startServer();
      p2p7->startServer();
      p2p8->startServer();

      // Connect nodes to the discovery node.
      p2p1->connectToServer("127.0.0.1", 8090);
      p2p2->connectToServer("127.0.0.1", 8090);
      p2p3->connectToServer("127.0.0.1", 8090);
      p2p4->connectToServer("127.0.0.1", 8090);
      p2p5->connectToServer("127.0.0.1", 8090);
      p2p6->connectToServer("127.0.0.1", 8090);
      p2p7->connectToServer("127.0.0.1", 8090);
      p2p8->connectToServer("127.0.0.1", 8090);

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      while (p2pDiscovery->getSessionsIDs().size() != 8) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      // Sleep an extra second
      std::this_thread::sleep_for(std::chrono::seconds(1));

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);

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
        while (rdpos1->getMempool().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

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
                  if (tx.getData().substr(0, 4) == Hex::toBytes("0xcfffe746")) {
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
                  if (tx.getData().substr(0, 4) == Hex::toBytes("0x6fc5a2d6")) {
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
            REQUIRE(state1->validateNextBlock(block));
            REQUIRE(state2->validateNextBlock(block));
            REQUIRE(state3->validateNextBlock(block));
            REQUIRE(state4->validateNextBlock(block));
            REQUIRE(state5->validateNextBlock(block));
            REQUIRE(state6->validateNextBlock(block));
            REQUIRE(state7->validateNextBlock(block));
            REQUIRE(state8->validateNextBlock(block));

            state1->processNextBlock(Block(block)); // Create copy.
            state2->processNextBlock(Block(block)); // Create copy.
            state3->processNextBlock(Block(block)); // Create copy.
            state4->processNextBlock(Block(block)); // Create copy.
            state5->processNextBlock(Block(block)); // Create copy.
            state6->processNextBlock(Block(block)); // Create copy.
            state7->processNextBlock(Block(block)); // Create copy.
            state8->processNextBlock(Block(block)); // Create copy.

            ++blocks;
            break;
          }
        }
      }
      // Sleep so it can conclude the last operations.
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}