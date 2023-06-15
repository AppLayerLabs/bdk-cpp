#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/rdpos.h"
#include "../../src/core/storage.h"
#include "../../src/core/state.h"
#include "../../src/utils/db.h"
#include "../../src/net/p2p/p2pmanagernormal.h"
#include "../../src/net/p2p/p2pmanagerdiscovery.h"
#include "../../src/contract/abi.h"

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

// Forward declaration from contractmanager.cpp
ethCallInfoAllocated buildCallInfo(const Address& addressToCall, const Functor& function, const Bytes& dataToCall);

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
    // Populate State DB with one address.
    /// Initialize with 0x00dead00665771855a34155f5e7405489df2c3c6 with nonce 0.
    Address dev1(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"));
    /// See ~State for encoding
    uint256_t desiredBalance("1000000000000000000000");
    Bytes value = Utils::uintToBytes(Utils::bytesRequired(desiredBalance));
    Utils::appendBytes(value, Utils::uintToBytes(desiredBalance));
    value.insert(value.end(), 0x00);
    db->put(dev1.get(), value, DBPrefix::nativeAccounts);
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

namespace TState {
  TEST_CASE("State Class", "[core][state]") {
    SECTION("State Class Constructor/Destructor", "[state]") {
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, "stateConstructorTest");
        REQUIRE(state->getNativeBalance(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"))) ==
                uint256_t("1000000000000000000000"));
      }
      // Wait a little until everyone has been destructed.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      std::unique_ptr<Options> options;
      //// Check if opening the state loads successfully from DB.
      initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, false, "stateConstructorTest");
      REQUIRE(state->getNativeBalance(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"))) ==
              uint256_t("1000000000000000000000"));
      REQUIRE(state->getNativeNonce(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"))) == 0);
    }

    SECTION("State Class addBalance to random Addresses") {
      std::vector<std::pair<Address, uint256_t>> addresses;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, "stateAddBalanceTest");

        for (uint64_t i = 0; i < 1024; ++i) {
          std::pair<Address, uint256_t> randomAddress = std::make_pair(Address(Utils::randBytes(20)),
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
      std::unique_ptr<Options> options;
      initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, false, "stateAddBalanceTest");
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
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, "stateSimpleBlockTest");

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
      std::unique_ptr<Options> options;
      initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, false, "stateSimpleBlockTest");

      REQUIRE(latestBlock->hash() == storage->latest()->hash());
    }

    SECTION("Test Block with Transactions on State") {
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 500; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, "stateSimpleBlockTest");

        /// Add balance to the random Accounts and create random transactions
        std::vector<TxBlock> transactions;
        for (auto &[privkey, val]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          state->addBalance(me);
          transactions.emplace_back(
            targetOfTransactions,
            me,
            Bytes(),
            8080,
            state->getNativeNonce(me),
            1000000000000000000,
            21000,
            1000000000,
            1000000000,
            privkey
          );

          /// Take note of expected balance and nonce
          val.first =
            state->getNativeBalance(me) - (transactions.back().getMaxFeePerGas() * transactions.back().getGasLimit()) -
            transactions.back().getValue();
          val.second = state->getNativeNonce(me) + 1;
          targetExpectedValue += transactions.back().getValue();
        }

        auto newBestBlock = createValidBlock(rdpos, storage, transactions);
        REQUIRE(state->validateNextBlock(newBestBlock));

        state->processNextBlock(std::move(newBestBlock));

        for (const auto &[privkey, val]: randomAccounts) {
          auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          REQUIRE(state->getNativeBalance(me) == val.first);
          REQUIRE(state->getNativeNonce(me) == val.second);
        }
        REQUIRE(state->getNativeBalance(targetOfTransactions) == targetExpectedValue);
      }
    }

    SECTION("Test State mempool") {
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 500; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, "stateSimpleBlockTest");

        /// Add balance to the random Accounts and add tx's to directly to mempool.
        for (auto &[privkey, val]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          state->addBalance(me);
          TxBlock tx(
            targetOfTransactions,
            me,
            Bytes(),
            8080,
            state->getNativeNonce(me),
            1000000000000000000,
            21000,
            1000000000,
            1000000000,
            privkey
          );

          /// Take note of expected balance and nonce
          val.first = state->getNativeBalance(me) - (tx.getMaxFeePerGas() * tx.getGasLimit()) - tx.getValue();
          val.second = state->getNativeNonce(me) + 1;
          targetExpectedValue += tx.getValue();
          state->addTx(std::move(tx));
        }

        auto mempoolCopy = state->getMempool();
        REQUIRE(mempoolCopy.size() == 500);
        std::vector<TxBlock> txCopy;
        for (const auto &[key, value]: mempoolCopy) {
          txCopy.emplace_back(value);
        }

        auto newBestBlock = createValidBlock(rdpos, storage, txCopy);
        REQUIRE(state->validateNextBlock(newBestBlock));

        state->processNextBlock(std::move(newBestBlock));

        for (const auto &[privkey, val]: randomAccounts) {
          auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          REQUIRE(state->getNativeBalance(me) == val.first);
          REQUIRE(state->getNativeNonce(me) == val.second);
        }
        REQUIRE(state->getNativeBalance(targetOfTransactions) == targetExpectedValue);
      }
    }

    SECTION("Test State mempool refresh") {
      /// The block included will only have transactions where the address starts with \x08 or lower
      /// where the mempool will have 500 transactions, including the \x08 addresses txs.
      /// Test if the mempool is refreshed correctly.
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 500; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, "stateSimpleBlockTest");

        /// Add balance to the random Accounts and add tx's to directly to mempool.
        std::vector<TxBlock> txs;
        std::vector<TxBlock> notOnBlock;
        for (auto &[privkey, val]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          state->addBalance(me);
          TxBlock tx(
            targetOfTransactions,
            me,
            Bytes(),
            8080,
            state->getNativeNonce(me),
            1000000000000000000,
            21000,
            1000000000,
            1000000000,
            privkey
          );

          if (me[0] <= 0x08) {
            txs.emplace_back(tx);
            /// Take note of expected balance and nonce
            val.first = state->getNativeBalance(me) - (tx.getMaxFeePerGas() * tx.getGasLimit()) - tx.getValue();
            val.second = state->getNativeNonce(me) + 1;
            targetExpectedValue += tx.getValue();
          } else {
            val.first = state->getNativeBalance(me);
            val.second = state->getNativeNonce(me);
            notOnBlock.emplace_back(tx);
          }
          state->addTx(std::move(tx));
        }

        auto newBestBlock = createValidBlock(rdpos, storage, txs);
        REQUIRE(state->validateNextBlock(newBestBlock));

        state->processNextBlock(std::move(newBestBlock));

        REQUIRE(state->getMempool().size() == notOnBlock.size());

        auto mempoolCopy = state->getMempool();
        for (const auto &tx: notOnBlock) {
          REQUIRE(mempoolCopy.contains(tx.hash()));
        }

        for (const auto &[privkey, val]: randomAccounts) {
          auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          REQUIRE(state->getNativeBalance(me) == val.first);
          REQUIRE(state->getNativeNonce(me) == val.second);
        }
        REQUIRE(state->getNativeBalance(targetOfTransactions) == targetExpectedValue);
      }

    }

    SECTION("Test 10 blocks forward on State (100 Transactions per block)") {
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      std::unique_ptr<Block> latestBlock = nullptr;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        std::unique_ptr<Options> options;
        initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, true, "state10BlocksTest");
        /// Add balance to the given addresses
        for (const auto &[privkey, account]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          state->addBalance(me);
        }

        for (uint64_t index = 0; index < 10; ++index) {
          /// Create random transactions
          std::vector<TxBlock> txs;
          for (auto &[privkey, account]: randomAccounts) {
            Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
            txs.emplace_back(
              targetOfTransactions,
              me,
              Bytes(),
              8080,
              state->getNativeNonce(me),
              1000000000000000000,
              21000,
              1000000000,
              1000000000,
              privkey
            );
            /// Take note of expected balance and nonce
            account.first = state->getNativeBalance(me) - (txs.back().getMaxFeePerGas() * txs.back().getGasLimit()) -
                            txs.back().getValue();
            account.second = state->getNativeNonce(me) + 1;
            targetExpectedValue += txs.back().getValue();
          }

          // Create the new block
          auto newBestBlock = createValidBlock(rdpos, storage, txs);
          REQUIRE(state->validateNextBlock(newBestBlock));

          state->processNextBlock(std::move(newBestBlock));
          for (const auto &[privkey, val]: randomAccounts) {
            auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
            REQUIRE(state->getNativeBalance(me) == val.first);
            REQUIRE(state->getNativeNonce(me) == val.second);
          }
          REQUIRE(state->getNativeBalance(targetOfTransactions) == targetExpectedValue);
        }

        latestBlock = std::make_unique<Block>(*storage->latest().get());
      }
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      std::unique_ptr<Options> options;
      initialize(db, storage, p2p, rdpos, state, options, validatorPrivKeys[0], 8080, false, "state10BlocksTest");

      REQUIRE(latestBlock->hash() == storage->latest()->hash());
      REQUIRE(storage->latest()->getNHeight() == 10);
      for (const auto &[privkey, val]: randomAccounts) {
        auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        REQUIRE(state->getNativeBalance(me) == val.first);
        REQUIRE(state->getNativeNonce(me) == val.second);
      }
      REQUIRE(state->getNativeBalance(targetOfTransactions) == targetExpectedValue);
    }

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, test Tx Broadcast") {
      // Initialize 8 different node instances, with different ports and DBs.
      std::vector<PrivKey> randomAccounts;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAccounts.emplace_back(PrivKey(Utils::randBytes(32)));
      }

      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<State> state1;
      std::unique_ptr<Options> options1;
      initialize(db1, storage1, p2p1, rdpos1, state1, options1, validatorPrivKeys[0], 8080, true,
                 "stateNode1NetworkCapabilities");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<State> state2;
      std::unique_ptr<Options> options2;
      initialize(db2, storage2, p2p2, rdpos2, state2, options2, validatorPrivKeys[1], 8081, true,
                 "stateNode2NetworkCapabilities");

      std::unique_ptr<DB> db3;
      std::unique_ptr<Storage> storage3;
      std::unique_ptr<P2P::ManagerNormal> p2p3;
      PrivKey validatorKey3 = PrivKey();
      std::unique_ptr<rdPoS> rdpos3;
      std::unique_ptr<State> state3;
      std::unique_ptr<Options> options3;
      initialize(db3, storage3, p2p3, rdpos3, state3, options3, validatorPrivKeys[2], 8082, true,
                 "stateNode3NetworkCapabilities");

      std::unique_ptr<DB> db4;
      std::unique_ptr<Storage> storage4;
      std::unique_ptr<P2P::ManagerNormal> p2p4;
      PrivKey validatorKey4 = PrivKey();
      std::unique_ptr<rdPoS> rdpos4;
      std::unique_ptr<State> state4;
      std::unique_ptr<Options> options4;
      initialize(db4, storage4, p2p4, rdpos4, state4, options4, validatorPrivKeys[3], 8083, true,
                 "stateNode4NetworkCapabilities");

      std::unique_ptr<DB> db5;
      std::unique_ptr<Storage> storage5;
      std::unique_ptr<P2P::ManagerNormal> p2p5;
      PrivKey validatorKey5 = PrivKey();
      std::unique_ptr<rdPoS> rdpos5;
      std::unique_ptr<State> state5;
      std::unique_ptr<Options> options5;
      initialize(db5, storage5, p2p5, rdpos5, state5, options5, validatorPrivKeys[4], 8084, true,
                 "stateNode5NetworkCapabilities");

      std::unique_ptr<DB> db6;
      std::unique_ptr<Storage> storage6;
      std::unique_ptr<P2P::ManagerNormal> p2p6;
      PrivKey validatorKey6 = PrivKey();
      std::unique_ptr<rdPoS> rdpos6;
      std::unique_ptr<State> state6;
      std::unique_ptr<Options> options6;
      initialize(db6, storage6, p2p6, rdpos6, state6, options6, validatorPrivKeys[5], 8085, true,
                 "stateNode6NetworkCapabilities");

      std::unique_ptr<DB> db7;
      std::unique_ptr<Storage> storage7;
      std::unique_ptr<P2P::ManagerNormal> p2p7;
      PrivKey validatorKey7 = PrivKey();
      std::unique_ptr<rdPoS> rdpos7;
      std::unique_ptr<State> state7;
      std::unique_ptr<Options> options7;
      initialize(db7, storage7, p2p7, rdpos7, state7, options7, validatorPrivKeys[6], 8086, true,
                 "stateNode7NetworkCapabilities");

      std::unique_ptr<DB> db8;
      std::unique_ptr<Storage> storage8;
      std::unique_ptr<P2P::ManagerNormal> p2p8;
      PrivKey validatorKey8 = PrivKey();
      std::unique_ptr<rdPoS> rdpos8;
      std::unique_ptr<State> state8;
      std::unique_ptr<Options> options8;
      initialize(db8, storage8, p2p8, rdpos8, state8, options8, validatorPrivKeys[7], 8087, true,
                 "stateNode8NetworkCapabilities");

      // Initialize state with all balances
      for (const auto &privkey: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        state1->addBalance(me);
        state2->addBalance(me);
        state3->addBalance(me);
        state4->addBalance(me);
        state5->addBalance(me);
        state6->addBalance(me);
        state7->addBalance(me);
        state8->addBalance(me);
      }

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
        "stateDiscoveryNodeNetworkCapabilities",
        "OrbiterSDK/cpp/linux_x86-64/0.0.3",
        1,
        8080,
        8090,
        9999,
        discoveryNodes
      );
      std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery = std::make_unique<P2P::ManagerDiscovery>(
        boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

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

      // Wait everyone be connected with the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      while (p2pDiscovery->getSessionsIDs().size() != 8) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 1);
      REQUIRE(p2p2->getSessionsIDs().size() == 1);
      REQUIRE(p2p3->getSessionsIDs().size() == 1);
      REQUIRE(p2p4->getSessionsIDs().size() == 1);
      REQUIRE(p2p5->getSessionsIDs().size() == 1);
      REQUIRE(p2p6->getSessionsIDs().size() == 1);
      REQUIRE(p2p7->getSessionsIDs().size() == 1);
      REQUIRE(p2p8->getSessionsIDs().size() == 1);

      // Start discovery
      p2pDiscovery->startDiscovery();
      p2p1->startDiscovery();
      p2p2->startDiscovery();
      p2p3->startDiscovery();
      p2p4->startDiscovery();
      p2p5->startDiscovery();
      p2p6->startDiscovery();
      p2p7->startDiscovery();
      p2p8->startDiscovery();

      // Wait for discovery to take effect
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Wait for nodes to connect.
      while (p2pDiscovery->getSessionsIDs().size() != 8 ||
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

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 8);
      REQUIRE(p2p2->getSessionsIDs().size() == 8);
      REQUIRE(p2p3->getSessionsIDs().size() == 8);
      REQUIRE(p2p4->getSessionsIDs().size() == 8);
      REQUIRE(p2p5->getSessionsIDs().size() == 8);
      REQUIRE(p2p6->getSessionsIDs().size() == 8);
      REQUIRE(p2p7->getSessionsIDs().size() == 8);
      REQUIRE(p2p8->getSessionsIDs().size() == 8);

      REQUIRE(rdpos1->getIsValidator());
      REQUIRE(rdpos2->getIsValidator());
      REQUIRE(rdpos3->getIsValidator());
      REQUIRE(rdpos4->getIsValidator());
      REQUIRE(rdpos5->getIsValidator());
      REQUIRE(rdpos6->getIsValidator());
      REQUIRE(rdpos7->getIsValidator());
      REQUIRE(rdpos8->getIsValidator());

      // Test tx broadcasting
      for (const auto &privkey: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        Address targetOfTransactions = Address(Utils::randBytes(20));
        TxBlock tx(
          targetOfTransactions,
          me,
          Bytes(),
          8080,
          state1->getNativeNonce(me),
          1000000000000000000,
          21000,
          1000000000,
          1000000000,
          privkey
        );
        state1->addTx(TxBlock(tx));
        p2p1->broadcastTxBlock(tx);
      }

      /// Wait for the transactions to be broadcasted.
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      REQUIRE(state1->getMempool().size() == 100);
      REQUIRE(state1->getMempool() == state2->getMempool());
      REQUIRE(state1->getMempool() == state3->getMempool());
      REQUIRE(state1->getMempool() == state4->getMempool());
      REQUIRE(state1->getMempool() == state5->getMempool());
      REQUIRE(state1->getMempool() == state6->getMempool());
      REQUIRE(state1->getMempool() == state7->getMempool());
      REQUIRE(state1->getMempool() == state8->getMempool());

      // Sleep so it can conclude the last operations.
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, no transactions") {
      // Initialize 8 different node instances, with different ports and DBs.
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<State> state1;
      std::unique_ptr<Options> options1;
      initialize(db1, storage1, p2p1, rdpos1, state1, options1, validatorPrivKeys[0], 8080, true,
                 "stateNode1NetworkCapabilities");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<State> state2;
      std::unique_ptr<Options> options2;
      initialize(db2, storage2, p2p2, rdpos2, state2, options2, validatorPrivKeys[1], 8081, true,
                 "stateNode2NetworkCapabilities");

      std::unique_ptr<DB> db3;
      std::unique_ptr<Storage> storage3;
      std::unique_ptr<P2P::ManagerNormal> p2p3;
      PrivKey validatorKey3 = PrivKey();
      std::unique_ptr<rdPoS> rdpos3;
      std::unique_ptr<State> state3;
      std::unique_ptr<Options> options3;
      initialize(db3, storage3, p2p3, rdpos3, state3, options3, validatorPrivKeys[2], 8082, true,
                 "stateNode3NetworkCapabilities");

      std::unique_ptr<DB> db4;
      std::unique_ptr<Storage> storage4;
      std::unique_ptr<P2P::ManagerNormal> p2p4;
      PrivKey validatorKey4 = PrivKey();
      std::unique_ptr<rdPoS> rdpos4;
      std::unique_ptr<State> state4;
      std::unique_ptr<Options> options4;
      initialize(db4, storage4, p2p4, rdpos4, state4, options4, validatorPrivKeys[3], 8083, true,
                 "stateNode4NetworkCapabilities");

      std::unique_ptr<DB> db5;
      std::unique_ptr<Storage> storage5;
      std::unique_ptr<P2P::ManagerNormal> p2p5;
      PrivKey validatorKey5 = PrivKey();
      std::unique_ptr<rdPoS> rdpos5;
      std::unique_ptr<State> state5;
      std::unique_ptr<Options> options5;
      initialize(db5, storage5, p2p5, rdpos5, state5, options5, validatorPrivKeys[4], 8084, true,
                 "stateNode5NetworkCapabilities");

      std::unique_ptr<DB> db6;
      std::unique_ptr<Storage> storage6;
      std::unique_ptr<P2P::ManagerNormal> p2p6;
      PrivKey validatorKey6 = PrivKey();
      std::unique_ptr<rdPoS> rdpos6;
      std::unique_ptr<State> state6;
      std::unique_ptr<Options> options6;
      initialize(db6, storage6, p2p6, rdpos6, state6, options6, validatorPrivKeys[5], 8085, true,
                 "stateNode6NetworkCapabilities");

      std::unique_ptr<DB> db7;
      std::unique_ptr<Storage> storage7;
      std::unique_ptr<P2P::ManagerNormal> p2p7;
      PrivKey validatorKey7 = PrivKey();
      std::unique_ptr<rdPoS> rdpos7;
      std::unique_ptr<State> state7;
      std::unique_ptr<Options> options7;
      initialize(db7, storage7, p2p7, rdpos7, state7, options7, validatorPrivKeys[6], 8086, true,
                 "stateNode7NetworkCapabilities");

      std::unique_ptr<DB> db8;
      std::unique_ptr<Storage> storage8;
      std::unique_ptr<P2P::ManagerNormal> p2p8;
      PrivKey validatorKey8 = PrivKey();
      std::unique_ptr<rdPoS> rdpos8;
      std::unique_ptr<State> state8;
      std::unique_ptr<Options> options8;
      initialize(db8, storage8, p2p8, rdpos8, state8, options8, validatorPrivKeys[7], 8087, true,
                 "stateNode8NetworkCapabilities");

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
        "stateDiscoveryNodeNetworkCapabilities",
        "OrbiterSDK/cpp/linux_x86-64/0.0.3",
        1,
        8080,
        8090,
        9999,
        discoveryNodes
      );
      std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery = std::make_unique<P2P::ManagerDiscovery>(
        boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

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

      // Wait everyone be connected with the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      while (p2pDiscovery->getSessionsIDs().size() != 8) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 1);
      REQUIRE(p2p2->getSessionsIDs().size() == 1);
      REQUIRE(p2p3->getSessionsIDs().size() == 1);
      REQUIRE(p2p4->getSessionsIDs().size() == 1);
      REQUIRE(p2p5->getSessionsIDs().size() == 1);
      REQUIRE(p2p6->getSessionsIDs().size() == 1);
      REQUIRE(p2p7->getSessionsIDs().size() == 1);
      REQUIRE(p2p8->getSessionsIDs().size() == 1);

      // Start discovery
      p2pDiscovery->startDiscovery();
      p2p1->startDiscovery();
      p2p2->startDiscovery();
      p2p3->startDiscovery();
      p2p4->startDiscovery();
      p2p5->startDiscovery();
      p2p6->startDiscovery();
      p2p7->startDiscovery();
      p2p8->startDiscovery();

      // Wait for discovery to take effect
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Wait for nodes to connect.
      while (p2pDiscovery->getSessionsIDs().size() != 8 ||
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

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 8);
      REQUIRE(p2p2->getSessionsIDs().size() == 8);
      REQUIRE(p2p3->getSessionsIDs().size() == 8);
      REQUIRE(p2p4->getSessionsIDs().size() == 8);
      REQUIRE(p2p5->getSessionsIDs().size() == 8);
      REQUIRE(p2p6->getSessionsIDs().size() == 8);
      REQUIRE(p2p7->getSessionsIDs().size() == 8);
      REQUIRE(p2p8->getSessionsIDs().size() == 8);

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
      /// TODO: This is done for the same reason as stopDiscovery.
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

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, 100 transactions per block") {
      // Create random accounts for the transactions.
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      // Initialize 8 different node instances, with different ports and DBs.
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<State> state1;
      std::unique_ptr<Options> options1;
      initialize(db1, storage1, p2p1, rdpos1, state1, options1, validatorPrivKeys[0], 8080, true,
                 "stateNode1NetworkCapabilitiesWithTx");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<State> state2;
      std::unique_ptr<Options> options2;
      initialize(db2, storage2, p2p2, rdpos2, state2, options2, validatorPrivKeys[1], 8081, true,
                 "stateNode2NetworkCapabilitiesWithTx");

      std::unique_ptr<DB> db3;
      std::unique_ptr<Storage> storage3;
      std::unique_ptr<P2P::ManagerNormal> p2p3;
      PrivKey validatorKey3 = PrivKey();
      std::unique_ptr<rdPoS> rdpos3;
      std::unique_ptr<State> state3;
      std::unique_ptr<Options> options3;
      initialize(db3, storage3, p2p3, rdpos3, state3, options3, validatorPrivKeys[2], 8082, true,
                 "stateNode3NetworkCapabilitiesWithTx");

      std::unique_ptr<DB> db4;
      std::unique_ptr<Storage> storage4;
      std::unique_ptr<P2P::ManagerNormal> p2p4;
      PrivKey validatorKey4 = PrivKey();
      std::unique_ptr<rdPoS> rdpos4;
      std::unique_ptr<State> state4;
      std::unique_ptr<Options> options4;
      initialize(db4, storage4, p2p4, rdpos4, state4, options4, validatorPrivKeys[3], 8083, true,
                 "stateNode4NetworkCapabilitiesWithTx");

      std::unique_ptr<DB> db5;
      std::unique_ptr<Storage> storage5;
      std::unique_ptr<P2P::ManagerNormal> p2p5;
      PrivKey validatorKey5 = PrivKey();
      std::unique_ptr<rdPoS> rdpos5;
      std::unique_ptr<State> state5;
      std::unique_ptr<Options> options5;
      initialize(db5, storage5, p2p5, rdpos5, state5, options5, validatorPrivKeys[4], 8084, true,
                 "stateNode5NetworkCapabilitiesWithTx");

      std::unique_ptr<DB> db6;
      std::unique_ptr<Storage> storage6;
      std::unique_ptr<P2P::ManagerNormal> p2p6;
      PrivKey validatorKey6 = PrivKey();
      std::unique_ptr<rdPoS> rdpos6;
      std::unique_ptr<State> state6;
      std::unique_ptr<Options> options6;
      initialize(db6, storage6, p2p6, rdpos6, state6, options6, validatorPrivKeys[5], 8085, true,
                 "stateNode6NetworkCapabilitiesWithTx");

      std::unique_ptr<DB> db7;
      std::unique_ptr<Storage> storage7;
      std::unique_ptr<P2P::ManagerNormal> p2p7;
      PrivKey validatorKey7 = PrivKey();
      std::unique_ptr<rdPoS> rdpos7;
      std::unique_ptr<State> state7;
      std::unique_ptr<Options> options7;
      initialize(db7, storage7, p2p7, rdpos7, state7, options7, validatorPrivKeys[6], 8086, true,
                 "stateNode7NetworkCapabilitiesWithTx");

      std::unique_ptr<DB> db8;
      std::unique_ptr<Storage> storage8;
      std::unique_ptr<P2P::ManagerNormal> p2p8;
      PrivKey validatorKey8 = PrivKey();
      std::unique_ptr<rdPoS> rdpos8;
      std::unique_ptr<State> state8;
      std::unique_ptr<Options> options8;
      initialize(db8, storage8, p2p8, rdpos8, state8, options8, validatorPrivKeys[7], 8087, true,
                 "stateNode8NetworkCapabilitiesWithTx");

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
        "statedDiscoveryNodeNetworkCapabilitiesWithTx",
        "OrbiterSDK/cpp/linux_x86-64/0.0.3",
        1,
        8080,
        8090,
        9999,
        discoveryNodes
      );
      std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery = std::make_unique<P2P::ManagerDiscovery>(
        boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

      // Initialize state with all balances
      for (const auto &[privkey, account]: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        state1->addBalance(me);
        state2->addBalance(me);
        state3->addBalance(me);
        state4->addBalance(me);
        state5->addBalance(me);
        state6->addBalance(me);
        state7->addBalance(me);
        state8->addBalance(me);
      }

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

      // Wait everyone be connected with the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      while (p2pDiscovery->getSessionsIDs().size() != 8) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 1);
      REQUIRE(p2p2->getSessionsIDs().size() == 1);
      REQUIRE(p2p3->getSessionsIDs().size() == 1);
      REQUIRE(p2p4->getSessionsIDs().size() == 1);
      REQUIRE(p2p5->getSessionsIDs().size() == 1);
      REQUIRE(p2p6->getSessionsIDs().size() == 1);
      REQUIRE(p2p7->getSessionsIDs().size() == 1);
      REQUIRE(p2p8->getSessionsIDs().size() == 1);

      // Start discovery
      p2pDiscovery->startDiscovery();
      p2p1->startDiscovery();
      p2p2->startDiscovery();
      p2p3->startDiscovery();
      p2p4->startDiscovery();
      p2p5->startDiscovery();
      p2p6->startDiscovery();
      p2p7->startDiscovery();
      p2p8->startDiscovery();

      // Wait for discovery to take effect
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Wait for nodes to connect.
      while (p2pDiscovery->getSessionsIDs().size() != 8 ||
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

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 8);
      REQUIRE(p2p2->getSessionsIDs().size() == 8);
      REQUIRE(p2p3->getSessionsIDs().size() == 8);
      REQUIRE(p2p4->getSessionsIDs().size() == 8);
      REQUIRE(p2p5->getSessionsIDs().size() == 8);
      REQUIRE(p2p6->getSessionsIDs().size() == 8);
      REQUIRE(p2p7->getSessionsIDs().size() == 8);
      REQUIRE(p2p8->getSessionsIDs().size() == 8);

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

            /// Add balance to the random Accounts and create random transactions
            for (auto &[privkey, val]: randomAccounts) {
              Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
              TxBlock tx(
                targetOfTransactions,
                me,
                Bytes(),
                8080,
                state1->getNativeNonce(me),
                1000000000000000000,
                21000,
                1000000000,
                1000000000,
                privkey
              );

              /// Take note of expected balance and nonce
              val.first = state1->getNativeBalance(me) - (tx.getMaxFeePerGas() * tx.getGasLimit()) - tx.getValue();
              val.second = state1->getNativeNonce(me) + 1;
              targetExpectedValue += tx.getValue();
              block.appendTx(tx);
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

            for (const auto &[privkey, val]: randomAccounts) {
              auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
              REQUIRE(state1->getNativeBalance(me) == val.first);
              REQUIRE(state1->getNativeNonce(me) == val.second);
            }

            REQUIRE(state1->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state2->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state3->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state4->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state5->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state6->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state7->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state8->getNativeBalance(targetOfTransactions) == targetExpectedValue);

            ++blocks;
            break;
          }
        }
      }
      /// TODO: This is done for the same reason as stopDiscovery.
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

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, 1000 transactions per block, broadcast blocks.") {
      // Create random accounts for the transactions.
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 1000; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      // Initialize 8 different node instances, with different ports and DBs.
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<State> state1;
      std::unique_ptr<Options> options1;
      initialize(db1, storage1, p2p1, rdpos1, state1, options1, validatorPrivKeys[0], 8080, true,
                 "stateNode1NetworkCapabilitiesWithTxBlockBroadcast");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<State> state2;
      std::unique_ptr<Options> options2;
      initialize(db2, storage2, p2p2, rdpos2, state2, options2, validatorPrivKeys[1], 8081, true,
                 "stateNode2NetworkCapabilitiesWithTxBlockBroadcast");

      std::unique_ptr<DB> db3;
      std::unique_ptr<Storage> storage3;
      std::unique_ptr<P2P::ManagerNormal> p2p3;
      PrivKey validatorKey3 = PrivKey();
      std::unique_ptr<rdPoS> rdpos3;
      std::unique_ptr<State> state3;
      std::unique_ptr<Options> options3;
      initialize(db3, storage3, p2p3, rdpos3, state3, options3, validatorPrivKeys[2], 8082, true,
                 "stateNode3NetworkCapabilitiesWithTxBlockBroadcast");

      std::unique_ptr<DB> db4;
      std::unique_ptr<Storage> storage4;
      std::unique_ptr<P2P::ManagerNormal> p2p4;
      PrivKey validatorKey4 = PrivKey();
      std::unique_ptr<rdPoS> rdpos4;
      std::unique_ptr<State> state4;
      std::unique_ptr<Options> options4;
      initialize(db4, storage4, p2p4, rdpos4, state4, options4, validatorPrivKeys[3], 8083, true,
                 "stateNode4NetworkCapabilitiesWithTxBlockBroadcast");

      std::unique_ptr<DB> db5;
      std::unique_ptr<Storage> storage5;
      std::unique_ptr<P2P::ManagerNormal> p2p5;
      PrivKey validatorKey5 = PrivKey();
      std::unique_ptr<rdPoS> rdpos5;
      std::unique_ptr<State> state5;
      std::unique_ptr<Options> options5;
      initialize(db5, storage5, p2p5, rdpos5, state5, options5, validatorPrivKeys[4], 8084, true,
                 "stateNode5NetworkCapabilitiesWithTxBlockBroadcast");

      std::unique_ptr<DB> db6;
      std::unique_ptr<Storage> storage6;
      std::unique_ptr<P2P::ManagerNormal> p2p6;
      PrivKey validatorKey6 = PrivKey();
      std::unique_ptr<rdPoS> rdpos6;
      std::unique_ptr<State> state6;
      std::unique_ptr<Options> options6;
      initialize(db6, storage6, p2p6, rdpos6, state6, options6, validatorPrivKeys[5], 8085, true,
                 "stateNode6NetworkCapabilitiesWithTxBlockBroadcast");

      std::unique_ptr<DB> db7;
      std::unique_ptr<Storage> storage7;
      std::unique_ptr<P2P::ManagerNormal> p2p7;
      PrivKey validatorKey7 = PrivKey();
      std::unique_ptr<rdPoS> rdpos7;
      std::unique_ptr<State> state7;
      std::unique_ptr<Options> options7;
      initialize(db7, storage7, p2p7, rdpos7, state7, options7, validatorPrivKeys[6], 8086, true,
                 "stateNode7NetworkCapabilitiesWithTxBlockBroadcast");

      std::unique_ptr<DB> db8;
      std::unique_ptr<Storage> storage8;
      std::unique_ptr<P2P::ManagerNormal> p2p8;
      PrivKey validatorKey8 = PrivKey();
      std::unique_ptr<rdPoS> rdpos8;
      std::unique_ptr<State> state8;
      std::unique_ptr<Options> options8;
      initialize(db8, storage8, p2p8, rdpos8, state8, options8, validatorPrivKeys[7], 8087, true,
                 "stateNode8NetworkCapabilitiesWithTxBlockBroadcast");

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
        "statedDiscoveryNodeNetworkCapabilitiesWithTxBlockBroadcast",
        "OrbiterSDK/cpp/linux_x86-64/0.0.3",
        1,
        8080,
        8090,
        9999,
        discoveryNodes
      );
      std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery = std::make_unique<P2P::ManagerDiscovery>(
        boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

      // Initialize state with all balances
      for (const auto &[privkey, account]: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        state1->addBalance(me);
        state2->addBalance(me);
        state3->addBalance(me);
        state4->addBalance(me);
        state5->addBalance(me);
        state6->addBalance(me);
        state7->addBalance(me);
        state8->addBalance(me);
      }

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

      // Wait everyone be connected with the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      while (p2pDiscovery->getSessionsIDs().size() != 8) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 1);
      REQUIRE(p2p2->getSessionsIDs().size() == 1);
      REQUIRE(p2p3->getSessionsIDs().size() == 1);
      REQUIRE(p2p4->getSessionsIDs().size() == 1);
      REQUIRE(p2p5->getSessionsIDs().size() == 1);
      REQUIRE(p2p6->getSessionsIDs().size() == 1);
      REQUIRE(p2p7->getSessionsIDs().size() == 1);
      REQUIRE(p2p8->getSessionsIDs().size() == 1);

      // Start discovery
      p2pDiscovery->startDiscovery();
      p2p1->startDiscovery();
      p2p2->startDiscovery();
      p2p3->startDiscovery();
      p2p4->startDiscovery();
      p2p5->startDiscovery();
      p2p6->startDiscovery();
      p2p7->startDiscovery();
      p2p8->startDiscovery();

      // Wait for discovery to take effect
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Wait for nodes to connect.
      while (p2pDiscovery->getSessionsIDs().size() != 8 ||
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

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 8);
      REQUIRE(p2p2->getSessionsIDs().size() == 8);
      REQUIRE(p2p3->getSessionsIDs().size() == 8);
      REQUIRE(p2p4->getSessionsIDs().size() == 8);
      REQUIRE(p2p5->getSessionsIDs().size() == 8);
      REQUIRE(p2p6->getSessionsIDs().size() == 8);
      REQUIRE(p2p7->getSessionsIDs().size() == 8);
      REQUIRE(p2p8->getSessionsIDs().size() == 8);

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

            /// Add balance to the random Accounts and create random transactions
            for (auto &[privkey, val]: randomAccounts) {
              Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
              TxBlock tx(
                targetOfTransactions,
                me,
                Bytes(),
                8080,
                state1->getNativeNonce(me),
                1000000000000000000,
                21000,
                1000000000,
                1000000000,
                privkey
              );

              /// Take note of expected balance and nonce
              val.first = state1->getNativeBalance(me) - (tx.getMaxFeePerGas() * tx.getGasLimit()) - tx.getValue();
              val.second = state1->getNativeNonce(me) + 1;
              targetExpectedValue += tx.getValue();
              block.appendTx(tx);
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

            Hash latestBlockHash = block.hash();
            state1->processNextBlock(std::move(block));
            REQUIRE(storage1->latest()->hash() == latestBlockHash);
            // Broadcast the Block!
            p2p1->broadcastBlock(storage1->latest());
            // Sleep for blocks to be broadcasted and accepted.
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));

            // Check if the block was accepted by all nodes.
            REQUIRE(storage1->latest()->hash() == storage2->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage3->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage4->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage5->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage6->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage7->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage8->latest()->hash());

            for (const auto &[privkey, val]: randomAccounts) {
              auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
              REQUIRE(state1->getNativeBalance(me) == val.first);
              REQUIRE(state1->getNativeNonce(me) == val.second);
            }

            REQUIRE(state1->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state2->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state3->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state4->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state5->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state6->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state7->getNativeBalance(targetOfTransactions) == targetExpectedValue);
            REQUIRE(state8->getNativeBalance(targetOfTransactions) == targetExpectedValue);

            ++blocks;
            break;
          }
        }
      }
      /// TODO: This is done for the same reason as stopDiscovery.
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

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, 1 ERC20 transactions per block, broadcast blocks.") {
      // Create random accounts for the transactions.
      PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      // Initialize 8 different node instances, with different ports and DBs.
      std::unique_ptr<DB> db1;
      std::unique_ptr<Storage> storage1;
      std::unique_ptr<P2P::ManagerNormal> p2p1;
      PrivKey validatorKey1 = PrivKey();
      std::unique_ptr<rdPoS> rdpos1;
      std::unique_ptr<State> state1;
      std::unique_ptr<Options> options1;
      initialize(db1, storage1, p2p1, rdpos1, state1, options1, validatorPrivKeys[0], 8080, true, "stateNode1NetworkCapabilitiesWithERC20TxBlockBroadcast");

      std::unique_ptr<DB> db2;
      std::unique_ptr<Storage> storage2;
      std::unique_ptr<P2P::ManagerNormal> p2p2;
      PrivKey validatorKey2 = PrivKey();
      std::unique_ptr<rdPoS> rdpos2;
      std::unique_ptr<State> state2;
      std::unique_ptr<Options> options2;
      initialize(db2, storage2, p2p2, rdpos2, state2, options2, validatorPrivKeys[1], 8081, true, "stateNode2NetworkCapabilitiesWithERC20TxBlockBroadcast");

      std::unique_ptr<DB> db3;
      std::unique_ptr<Storage> storage3;
      std::unique_ptr<P2P::ManagerNormal> p2p3;
      PrivKey validatorKey3 = PrivKey();
      std::unique_ptr<rdPoS> rdpos3;
      std::unique_ptr<State> state3;
      std::unique_ptr<Options> options3;
      initialize(db3, storage3, p2p3, rdpos3, state3, options3, validatorPrivKeys[2], 8082, true, "stateNode3NetworkCapabilitiesWithERC20TxBlockBroadcast");

      std::unique_ptr<DB> db4;
      std::unique_ptr<Storage> storage4;
      std::unique_ptr<P2P::ManagerNormal> p2p4;
      PrivKey validatorKey4 = PrivKey();
      std::unique_ptr<rdPoS> rdpos4;
      std::unique_ptr<State> state4;
      std::unique_ptr<Options> options4;
      initialize(db4, storage4, p2p4, rdpos4, state4, options4, validatorPrivKeys[3], 8083, true, "stateNode4NetworkCapabilitiesWithERC20TxBlockBroadcast");

      std::unique_ptr<DB> db5;
      std::unique_ptr<Storage> storage5;
      std::unique_ptr<P2P::ManagerNormal> p2p5;
      PrivKey validatorKey5 = PrivKey();
      std::unique_ptr<rdPoS> rdpos5;
      std::unique_ptr<State> state5;
      std::unique_ptr<Options> options5;
      initialize(db5, storage5, p2p5, rdpos5, state5, options5, validatorPrivKeys[4], 8084, true, "stateNode5NetworkCapabilitiesWithERC20TxBlockBroadcast");

      std::unique_ptr<DB> db6;
      std::unique_ptr<Storage> storage6;
      std::unique_ptr<P2P::ManagerNormal> p2p6;
      PrivKey validatorKey6 = PrivKey();
      std::unique_ptr<rdPoS> rdpos6;
      std::unique_ptr<State> state6;
      std::unique_ptr<Options> options6;
      initialize(db6, storage6, p2p6, rdpos6, state6, options6, validatorPrivKeys[5], 8085, true, "stateNode6NetworkCapabilitiesWithERC20TxBlockBroadcast");

      std::unique_ptr<DB> db7;
      std::unique_ptr<Storage> storage7;
      std::unique_ptr<P2P::ManagerNormal> p2p7;
      PrivKey validatorKey7 = PrivKey();
      std::unique_ptr<rdPoS> rdpos7;
      std::unique_ptr<State> state7;
      std::unique_ptr<Options> options7;
      initialize(db7, storage7, p2p7, rdpos7, state7, options7, validatorPrivKeys[6], 8086, true, "stateNode7NetworkCapabilitiesWithTxERC20BlockBroadcast");

      std::unique_ptr<DB> db8;
      std::unique_ptr<Storage> storage8;
      std::unique_ptr<P2P::ManagerNormal> p2p8;
      PrivKey validatorKey8 = PrivKey();
      std::unique_ptr<rdPoS> rdpos8;
      std::unique_ptr<State> state8;
      std::unique_ptr<Options> options8;
      initialize(db8, storage8, p2p8, rdpos8, state8, options8, validatorPrivKeys[7], 8087, true, "stateNode8NetworkCapabilitiesWithTxERC20BlockBroadcast");

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      std::unique_ptr<Options> discoveryOptions = std::make_unique<Options>(
        "statedDiscoveryNodeNetworkCapabilitiesWithTxBlockBroadcast",
        "OrbiterSDK/cpp/linux_x86-64/0.0.3",
        1,
        8080,
        8090,
        9999,
        discoveryNodes
      );
      std::unique_ptr<P2P::ManagerDiscovery> p2pDiscovery = std::make_unique<P2P::ManagerDiscovery>(
        boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

      // Initialize state with all balances
      state1->addBalance(owner);
      state2->addBalance(owner);
      state3->addBalance(owner);
      state4->addBalance(owner);
      state5->addBalance(owner);
      state6->addBalance(owner);
      state7->addBalance(owner);
      state8->addBalance(owner);

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

      // Wait everyone be connected with the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      while (p2pDiscovery->getSessionsIDs().size() != 8) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 1);
      REQUIRE(p2p2->getSessionsIDs().size() == 1);
      REQUIRE(p2p3->getSessionsIDs().size() == 1);
      REQUIRE(p2p4->getSessionsIDs().size() == 1);
      REQUIRE(p2p5->getSessionsIDs().size() == 1);
      REQUIRE(p2p6->getSessionsIDs().size() == 1);
      REQUIRE(p2p7->getSessionsIDs().size() == 1);
      REQUIRE(p2p8->getSessionsIDs().size() == 1);

      // Start discovery
      p2pDiscovery->startDiscovery();
      p2p1->startDiscovery();
      p2p2->startDiscovery();
      p2p3->startDiscovery();
      p2p4->startDiscovery();
      p2p5->startDiscovery();
      p2p6->startDiscovery();
      p2p7->startDiscovery();
      p2p8->startDiscovery();

      // Wait for discovery to take effect
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Wait for nodes to connect.
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

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery->getSessionsIDs().size() == 8);
      REQUIRE(p2p1->getSessionsIDs().size() == 8);
      REQUIRE(p2p2->getSessionsIDs().size() == 8);
      REQUIRE(p2p3->getSessionsIDs().size() == 8);
      REQUIRE(p2p4->getSessionsIDs().size() == 8);
      REQUIRE(p2p5->getSessionsIDs().size() == 8);
      REQUIRE(p2p6->getSessionsIDs().size() == 8);
      REQUIRE(p2p7->getSessionsIDs().size() == 8);
      REQUIRE(p2p8->getSessionsIDs().size() == 8);

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

            /// ERC20 Transactions.
            if (blocks == 0) {
              /// We need to firstly create the contract!
              std::string tokenName = "TestToken";
              std::string tokenSymbol = "TT";
              uint256_t tokenDecimals = 18;
              uint256_t tokenSupply = 1000000000000000000;

              ABI::Encoder::EncVar createNewERC20ContractVars;
              createNewERC20ContractVars.push_back(tokenName);
              createNewERC20ContractVars.push_back(tokenSymbol);
              createNewERC20ContractVars.push_back(tokenDecimals);
              createNewERC20ContractVars.push_back(tokenSupply);
              ABI::Encoder createNewERC20ContractEncoder(createNewERC20ContractVars);
              Bytes createNewERC20ContractData = Hex::toBytes("0xb74e5ed5");
              Utils::appendBytes(createNewERC20ContractData, createNewERC20ContractEncoder.getData());

              TxBlock createNewERC2OTx = TxBlock(
                ProtocolContractAddresses.at("ContractManager"),
                owner,
                createNewERC20ContractData,
                8080,
                state1->getNativeNonce(owner),
                0,
                21000,
                1000000000,
                1000000000,
                ownerPrivKey
              );

              block.appendTx(createNewERC2OTx);

            } else {
              const auto ERC20ContractAddress = state1->getContracts()[0].second;

              ABI::Encoder::EncVar transferVars;
              transferVars.push_back(targetOfTransactions);
              transferVars.push_back(10000000000000000);
              ABI::Encoder transferEncoder(transferVars);
              Bytes transferData = Hex::toBytes("0xa9059cbb");
              Utils::appendBytes(transferData, transferEncoder.getData());

              TxBlock transferERC20 = TxBlock(
                ERC20ContractAddress,
                owner,
                transferData,
                8080,
                state1->getNativeNonce(owner),
                0,
                21000,
                1000000000,
                1000000000,
                ownerPrivKey
              );

              targetExpectedValue += 10000000000000000;
              block.appendTx(transferERC20);
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

            Hash latestBlockHash = block.hash();
            state1->processNextBlock(std::move(block));
            REQUIRE(storage1->latest()->hash() == latestBlockHash);
            // Broadcast the Block!
            p2p1->broadcastBlock(storage1->latest());
            // Sleep for blocks to be broadcasted and accepted.
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));

            // Check if the block was accepted by all nodes.
            REQUIRE(storage1->latest()->hash() == storage2->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage3->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage4->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage5->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage6->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage7->latest()->hash());
            REQUIRE(storage1->latest()->hash() == storage8->latest()->hash());


            const auto contractAddress = state1->getContracts()[0].second;
            ABI::Encoder::EncVar getBalanceMeVars;
            getBalanceMeVars.push_back(targetOfTransactions);
            ABI::Encoder getBalanceMeEncoder(getBalanceMeVars, "balanceOf(address)");
            Bytes getBalanceMeNode1Result = state1->ethCall(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
            ABI::Decoder getBalanceMeNode1Decoder({ABI::Types::uint256}, getBalanceMeNode1Result);
            REQUIRE(getBalanceMeNode1Decoder.getData<uint256_t>(0) == targetExpectedValue);

            Bytes getBalanceMeNode2Result = state2->ethCall(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
            ABI::Decoder getBalanceMeNode2Decoder({ABI::Types::uint256}, getBalanceMeNode2Result);
            REQUIRE(getBalanceMeNode2Decoder.getData<uint256_t>(0) == targetExpectedValue);

            Bytes getBalanceMeNode3Result = state3->ethCall(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
            ABI::Decoder getBalanceMeNode3Decoder({ABI::Types::uint256}, getBalanceMeNode3Result);
            REQUIRE(getBalanceMeNode3Decoder.getData<uint256_t>(0) == targetExpectedValue);

            Bytes getBalanceMeNode4Result = state4->ethCall(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
            ABI::Decoder getBalanceMeNode4Decoder({ABI::Types::uint256}, getBalanceMeNode4Result);
            REQUIRE(getBalanceMeNode4Decoder.getData<uint256_t>(0) == targetExpectedValue);

            Bytes getBalanceMeNode5Result = state5->ethCall(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
            ABI::Decoder getBalanceMeNode5Decoder({ABI::Types::uint256}, getBalanceMeNode5Result);
            REQUIRE(getBalanceMeNode5Decoder.getData<uint256_t>(0) == targetExpectedValue);

            Bytes getBalanceMeNode6Result = state6->ethCall(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
            ABI::Decoder getBalanceMeNode6Decoder({ABI::Types::uint256}, getBalanceMeNode6Result);
            REQUIRE(getBalanceMeNode6Decoder.getData<uint256_t>(0) == targetExpectedValue);

            Bytes getBalanceMeNode7Result = state7->ethCall(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
            ABI::Decoder getBalanceMeNode7Decoder({ABI::Types::uint256}, getBalanceMeNode7Result);
            REQUIRE(getBalanceMeNode7Decoder.getData<uint256_t>(0) == targetExpectedValue);

            Bytes getBalanceMeNode8Result = state8->ethCall(buildCallInfo(contractAddress, getBalanceMeEncoder.getFunctor(), getBalanceMeEncoder.getData()));
            ABI::Decoder getBalanceMeNode8Decoder({ABI::Types::uint256}, getBalanceMeNode8Result);
            REQUIRE(getBalanceMeNode8Decoder.getData<uint256_t>(0) == targetExpectedValue);

            ++blocks;
            break;
          }
        }
      }
      /// TODO: This is done for the same reason as stopDiscovery.
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
  }
}