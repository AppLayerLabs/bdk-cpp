/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/rdpos.h"
#include "../../src/core/storage.h"
#include "../../src/core/state.h"
#include "../../src/utils/db.h"
#include "../../src/net/p2p/managernormal.h"
#include "../../src/net/p2p/managerdiscovery.h"
#include "../../src/contract/abi.h"
#include "../blockchainwrapper.hpp"

#include <filesystem>
#include <utility>

#include "contract/contracthost.h"

const std::vector<Hash> validatorPrivKeysState {
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
std::pair<evmc_message, Bytes> buildCallInfo(const Address& addressToCall, const Functor& function, const Bytes& dataToCall) {
  std::pair<evmc_message, Bytes> callInfo;
  Bytes& messageBytes = std::get<1>(callInfo);
  Utils::appendBytes(messageBytes, Utils::uint32ToBytes(function.value));
  Utils::appendBytes(messageBytes, dataToCall);
  auto& [callKind,
    callFlags,
    callDepth,
    callGas,
    callRecipient,
    callSender,
    callInputData,
    callInputSize,
    callValue,
    callCreate2Salt,
    callCodeAddress] = std::get<0>(callInfo);
  callKind = EVMC_CALL;
  callFlags = 0;
  callDepth = 1;
  callGas = 100000000;
  callRecipient = addressToCall.toEvmcAddress();
  callSender = {};
  callInputData = messageBytes.data();
  callInputSize = messageBytes.size();
  callValue = {};
  callCreate2Salt = {};
  callCodeAddress = addressToCall.toEvmcAddress();
  return callInfo;
}

namespace TState {
  std::string testDumpPath = Utils::getTestDumpPath();
  TEST_CASE("State Class", "[core][state]") {
    PrivKey chainOwnerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
    Address chainOwnerAddress = Secp256k1::toAddress(Secp256k1::toUPub(chainOwnerPrivKey));
    SECTION("State Class Constructor/Destructor", "[state]") {
      {
        auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/stateConstructorTest");
        REQUIRE(blockchainWrapper.state.getNativeBalance(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"))) ==
                uint256_t("1000000000000000000000"));
      }
      // Wait a little until everyone has been destructed.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      //// Check if opening the state loads successfully from DB.
      auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, false, testDumpPath + "/stateConstructorTest");
      REQUIRE(blockchainWrapper.state.getNativeBalance(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"))) ==
              uint256_t("1000000000000000000000"));
      REQUIRE(blockchainWrapper.state.getNativeNonce(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"))) == 0);
    }

    SECTION("State Class addBalance to random Addresses") {
      std::vector<std::pair<Address, uint256_t>> addresses;
      {
        auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/stateAddBalanceTest");

        for (uint64_t i = 0; i < 1024; ++i) {
          std::pair<Address, uint256_t> randomAddress = std::make_pair(Address(Utils::randBytes(20)),
                                                                       uint256_t("1000000000000000000000"));
          blockchainWrapper.state.addBalance(randomAddress.first);
          addresses.push_back(randomAddress);
        }

        for (const auto &[address, expectedBalance]: addresses) {
          REQUIRE(blockchainWrapper.state.getNativeBalance(address) == expectedBalance);
          REQUIRE(blockchainWrapper.state.getNativeNonce(address) == 0);
        }
        // We actually need to process the next block otherwise saveToDB() will use "0" as the latest block height
        // to save the state to the DB.
        // "0" is specifically used to say there is no state to load from.
        auto newBlock = createValidBlock(validatorPrivKeysState, blockchainWrapper.state, blockchainWrapper.storage);
        REQUIRE(blockchainWrapper.state.validateNextBlock(newBlock));
        blockchainWrapper.state.processNextBlock(std::move(newBlock));
        blockchainWrapper.state.saveToDB();
      }
      // Wait until destructors are called.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // Load everything back from DB.
      auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, false, testDumpPath + "/stateAddBalanceTest");
      for (const auto &[address, expectedBalance]: addresses) {
        REQUIRE(blockchainWrapper.state.getNativeBalance(address) == expectedBalance);
        REQUIRE(blockchainWrapper.state.getNativeNonce(address) == 0);
      }
    }

    SECTION("Test Simple block on State (No Transactions only rdPoS") {
      std::unique_ptr<FinalizedBlock> latestBlock = nullptr;
      {
        auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/stateSimpleBlockTest");

        auto newBlock = createValidBlock(validatorPrivKeysState, blockchainWrapper.state, blockchainWrapper.storage);
        REQUIRE(blockchainWrapper.state.validateNextBlock(newBlock));
        blockchainWrapper.state.processNextBlock(std::move(newBlock));
        latestBlock = std::make_unique<FinalizedBlock>(*blockchainWrapper.storage.latest().get());
        blockchainWrapper.state.saveToDB();
      }
      auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, false, testDumpPath + "/stateSimpleBlockTest");

      REQUIRE(latestBlock->getHash() == blockchainWrapper.storage.latest()->getHash());
    }

    SECTION("Test Block with Transactions on State") {
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 500; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      {
        auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/stateSimpleBlockTest");

        /// Add balance to the random Accounts and create random transactions
        std::vector<TxBlock> transactions;
        for (auto &[privkey, val]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          blockchainWrapper.state.addBalance(me);
          transactions.emplace_back(
              targetOfTransactions,
              me,
              Bytes(),
              8080,
              blockchainWrapper.state.getNativeNonce(me),
              1000000000000000000,
              1000000000,
              1000000000,
              21000,
              privkey
          );

          /// Take note of expected balance and nonce
          val.first =
              blockchainWrapper.state.getNativeBalance(me) -
              (transactions.back().getMaxFeePerGas() * transactions.back().getGasLimit()) -
              transactions.back().getValue();
          val.second = blockchainWrapper.state.getNativeNonce(me) + 1;
          targetExpectedValue += transactions.back().getValue();
        }

        auto newBestBlock = createValidBlock(validatorPrivKeysState, blockchainWrapper.state, blockchainWrapper.storage, std::move(transactions));
        REQUIRE(blockchainWrapper.state.validateNextBlock(newBestBlock));

        blockchainWrapper.state.processNextBlock(std::move(newBestBlock));

        for (const auto &[privkey, val]: randomAccounts) {
          auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          REQUIRE(blockchainWrapper.state.getNativeBalance(me) == val.first);
          REQUIRE(blockchainWrapper.state.getNativeNonce(me) == val.second);
        }
        REQUIRE(blockchainWrapper.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
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
        auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/stateSimpleBlockTest");

        /// Add balance to the random Accounts and add tx's to directly to mempool.
        for (auto &[privkey, val]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          blockchainWrapper.state.addBalance(me);
          TxBlock tx(
              targetOfTransactions,
              me,
              Bytes(),
              8080,
              blockchainWrapper.state.getNativeNonce(me),
              1000000000000000000,
              1000000000,
              1000000000,
              21000,
              privkey
          );

          /// Take note of expected balance and nonce
          val.first = blockchainWrapper.state.getNativeBalance(me) - (tx.getMaxFeePerGas() * tx.getGasLimit()) - tx.getValue();
          val.second = blockchainWrapper.state.getNativeNonce(me) + 1;
          targetExpectedValue += tx.getValue();
          blockchainWrapper.state.addTx(std::move(tx));
        }

        auto txCopy = blockchainWrapper.state.getMempool();
        REQUIRE(txCopy.size() == 500);

        auto newBestBlock = createValidBlock(validatorPrivKeysState, blockchainWrapper.state, blockchainWrapper.storage, std::move(txCopy));
        REQUIRE(blockchainWrapper.state.validateNextBlock(newBestBlock));

        blockchainWrapper.state.processNextBlock(std::move(newBestBlock));

        for (const auto &[privkey, val]: randomAccounts) {
          auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          REQUIRE(blockchainWrapper.state.getNativeBalance(me) == val.first);
          REQUIRE(blockchainWrapper.state.getNativeNonce(me) == val.second);
        }
        REQUIRE(blockchainWrapper.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
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
        auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/stateSimpleBlockTest");

        /// Add balance to the random Accounts and add tx's to directly to mempool.
        std::vector<TxBlock> txs;
        std::vector<TxBlock> notOnBlock;
        for (auto &[privkey, val]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          blockchainWrapper.state.addBalance(me);
          TxBlock tx(
              targetOfTransactions,
              me,
              Bytes(),
              8080,
              blockchainWrapper.state.getNativeNonce(me),
              1000000000000000000,
              1000000000,
              1000000000,
              21000,
              privkey
          );

          if (me[0] <= 0x08) {
            txs.emplace_back(tx);
            /// Take note of expected balance and nonce
            val.first = blockchainWrapper.state.getNativeBalance(me) - (tx.getMaxFeePerGas() * tx.getGasLimit()) - tx.getValue();
            val.second = blockchainWrapper.state.getNativeNonce(me) + 1;
            targetExpectedValue += tx.getValue();
          } else {
            val.first = blockchainWrapper.state.getNativeBalance(me);
            val.second = blockchainWrapper.state.getNativeNonce(me);
            notOnBlock.emplace_back(tx);
          }
          blockchainWrapper.state.addTx(std::move(tx));
        }

        auto newBestBlock = createValidBlock(validatorPrivKeysState, blockchainWrapper.state, blockchainWrapper.storage, std::move(txs));
        REQUIRE(blockchainWrapper.state.validateNextBlock(newBestBlock));

        blockchainWrapper.state.processNextBlock(std::move(newBestBlock));

        REQUIRE(blockchainWrapper.state.getMempool().size() == notOnBlock.size());

        auto mempoolCopy = blockchainWrapper.state.getMempool();
        for (const auto &tx: notOnBlock) {
          REQUIRE(std::find(mempoolCopy.begin(), mempoolCopy.end(), tx) != mempoolCopy.end());
        }

        for (const auto &[privkey, val]: randomAccounts) {
          auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          REQUIRE(blockchainWrapper.state.getNativeBalance(me) == val.first);
          REQUIRE(blockchainWrapper.state.getNativeNonce(me) == val.second);
        }
        REQUIRE(blockchainWrapper.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
      }

    }

    SECTION("Test 10 blocks forward on State (100 Transactions per block)") {
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      std::unique_ptr<FinalizedBlock> latestBlock = nullptr;
      {
        auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/state10BlocksTest");
        /// Add balance to the given addresses
        for (const auto &[privkey, account]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          blockchainWrapper.state.addBalance(me);
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
                blockchainWrapper.state.getNativeNonce(me),
                1000000000000000000,
                1000000000,
                1000000000,
                21000,
                privkey
            );
            /// Take note of expected balance and nonce
            account.first = blockchainWrapper.state.getNativeBalance(me) - (txs.back().getMaxFeePerGas() * txs.back().getGasLimit()) -
                            txs.back().getValue();
            account.second = blockchainWrapper.state.getNativeNonce(me) + 1;
            targetExpectedValue += txs.back().getValue();
          }

          // Create the new block
          auto newBestBlock = createValidBlock(validatorPrivKeysState, blockchainWrapper.state, blockchainWrapper.storage, std::move(txs));
          REQUIRE(blockchainWrapper.state.validateNextBlock(newBestBlock));

          blockchainWrapper.state.processNextBlock(std::move(newBestBlock));
          for (const auto &[privkey, val]: randomAccounts) {
            auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
            REQUIRE(blockchainWrapper.state.getNativeBalance(me) == val.first);
            REQUIRE(blockchainWrapper.state.getNativeNonce(me) == val.second);
          }
          REQUIRE(blockchainWrapper.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
        }

        latestBlock = std::make_unique<FinalizedBlock>(*blockchainWrapper.storage.latest().get());
        blockchainWrapper.state.saveToDB();
      }
      auto blockchainWrapper = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, false, testDumpPath + "/state10BlocksTest");

      REQUIRE(latestBlock->getHash() == blockchainWrapper.storage.latest()->getHash());
      REQUIRE(blockchainWrapper.storage.latest()->getNHeight() == 10);
      for (const auto &[privkey, val]: randomAccounts) {
        auto me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        REQUIRE(blockchainWrapper.state.getNativeBalance(me) == val.first);
        REQUIRE(blockchainWrapper.state.getNativeNonce(me) == val.second);
      }
      REQUIRE(blockchainWrapper.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
    }

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, test Tx Broadcast") {
      // Initialize 8 different node instances, with different ports and DBs.
      std::vector<PrivKey> randomAccounts;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAccounts.emplace_back(PrivKey(Utils::randBytes(32)));
      }

      auto blockchainWrapper1 = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true,
                 testDumpPath + "/stateNode1NetworkCapabilities");

      auto blockchainWrapper2 = initialize(validatorPrivKeysState, validatorPrivKeysState[1], 8081, true,
                  testDumpPath + "/stateNode2NetworkCapabilities");

      auto blockchainWrapper3 = initialize(validatorPrivKeysState, validatorPrivKeysState[2], 8082, true,
                  testDumpPath + "/stateNode3NetworkCapabilities");

      auto blockchainWrapper4 = initialize(validatorPrivKeysState, validatorPrivKeysState[3], 8083, true,
                  testDumpPath + "/stateNode4NetworkCapabilities");

      auto blockchainWrapper5 = initialize(validatorPrivKeysState, validatorPrivKeysState[4], 8084, true,
                  testDumpPath + "/stateNode5NetworkCapabilities");

      auto blockchainWrapper6 = initialize(validatorPrivKeysState, validatorPrivKeysState[5], 8085, true,
                  testDumpPath + "/stateNode6NetworkCapabilities");

      auto blockchainWrapper7 = initialize(validatorPrivKeysState, validatorPrivKeysState[6], 8086, true,
                  testDumpPath + "/stateNode7NetworkCapabilities");

      auto blockchainWrapper8 = initialize(validatorPrivKeysState, validatorPrivKeysState[7], 8087, true,
                  testDumpPath + "/stateNode8NetworkCapabilities");

      // Initialize state with all balances
      for (const auto &privkey: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        blockchainWrapper1.state.addBalance(me);
        blockchainWrapper2.state.addBalance(me);
        blockchainWrapper3.state.addBalance(me);
        blockchainWrapper4.state.addBalance(me);
        blockchainWrapper5.state.addBalance(me);
        blockchainWrapper6.state.addBalance(me);
        blockchainWrapper7.state.addBalance(me);
        blockchainWrapper8.state.addBalance(me);
      }

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      uint64_t genesisTimestamp = 1678887538000000;
      FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{},Hash(), genesisTimestamp, 0, genesisPrivKey);
      std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
      std::vector<Address> genesisValidators;
      for (const auto& privKey : validatorPrivKeysState) {
        genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
      }
      Options discoveryOptions = Options(
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
      P2P::ManagerDiscovery p2pDiscovery(
          boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

      // Vector of references for the states' rdPoS workers
      // (rdPoS is exclusively owned by State and can't be exposed in any way,
      // so we have to pass the whole State object to access rdPoS functionality
      // via wrapper functions from the State)
      std::vector<std::reference_wrapper<State>> rdPoSreferences;
      rdPoSreferences.emplace_back(blockchainWrapper1.state);
      rdPoSreferences.emplace_back(blockchainWrapper2.state);
      rdPoSreferences.emplace_back(blockchainWrapper3.state);
      rdPoSreferences.emplace_back(blockchainWrapper4.state);
      rdPoSreferences.emplace_back(blockchainWrapper5.state);
      rdPoSreferences.emplace_back(blockchainWrapper6.state);
      rdPoSreferences.emplace_back(blockchainWrapper7.state);
      rdPoSreferences.emplace_back(blockchainWrapper8.state);

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
      blockchainWrapper1.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper2.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper3.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper4.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper5.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper6.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper7.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper8.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      auto discoveryFuture = std::async(std::launch::async, [&]() {
        while (p2pDiscovery.getSessionsIDs().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });

      // TODO: This had a 5s timeout, but this is temporarily increased to avoid random failures.
      REQUIRE(discoveryFuture.wait_for(std::chrono::seconds(15)) != std::future_status::timeout);

      REQUIRE(p2pDiscovery.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper4.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper5.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper6.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper7.p2p.getSessionsIDs().size() == 1);
      REQUIRE(blockchainWrapper8.p2p.getSessionsIDs().size() == 1);

      // Start discovery
      p2pDiscovery.startDiscovery();
      blockchainWrapper1.p2p.startDiscovery();
      blockchainWrapper2.p2p.startDiscovery();
      blockchainWrapper3.p2p.startDiscovery();
      blockchainWrapper4.p2p.startDiscovery();
      blockchainWrapper5.p2p.startDiscovery();
      blockchainWrapper6.p2p.startDiscovery();
      blockchainWrapper7.p2p.startDiscovery();
      blockchainWrapper8.p2p.startDiscovery();

      // Wait for nodes to connect.
      auto connectionsFuture = std::async(std::launch::async, [&]() {
        while (p2pDiscovery.getSessionsIDs().size() != 8 ||
               blockchainWrapper1.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper2.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper3.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper4.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper5.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper6.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper7.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper8.p2p.getSessionsIDs().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });

      REQUIRE(connectionsFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper4.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper5.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper6.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper7.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper8.p2p.getSessionsIDs().size() == 8);

      REQUIRE(blockchainWrapper1.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper2.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper3.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper4.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper5.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper6.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper7.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper8.state.rdposGetIsValidator());

      // Test tx broadcasting
      for (const auto &privkey: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        Address targetOfTransactions = Address(Utils::randBytes(20));
        TxBlock tx(
            targetOfTransactions,
            me,
            Bytes(),
            8080,
            blockchainWrapper1.state.getNativeNonce(me),
            1000000000000000000,
            1000000000,
            1000000000,
            21000,
            privkey
        );
        blockchainWrapper1.state.addTx(TxBlock(tx));
        blockchainWrapper1.p2p.getBroadcaster().broadcastTxBlock(tx);
      }

      REQUIRE(blockchainWrapper1.state.getMempool().size() == 100);
      /// Wait for the transactions to be broadcasted.
      auto broadcastFuture = std::async(std::launch::async, [&]() {
        while (blockchainWrapper1.state.getMempool().size() != 100 ||
               blockchainWrapper2.state.getMempool().size() != 100 ||
               blockchainWrapper3.state.getMempool().size() != 100 ||
               blockchainWrapper4.state.getMempool().size() != 100 ||
               blockchainWrapper5.state.getMempool().size() != 100 ||
               blockchainWrapper6.state.getMempool().size() != 100 ||
               blockchainWrapper7.state.getMempool().size() != 100 ||
               blockchainWrapper8.state.getMempool().size() != 100) {
          std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
      });

      REQUIRE(broadcastFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      REQUIRE(blockchainWrapper1.state.getMempool().size() == blockchainWrapper2.state.getMempool().size());
      REQUIRE(blockchainWrapper1.state.getMempool().size() == blockchainWrapper3.state.getMempool().size());
      REQUIRE(blockchainWrapper1.state.getMempool().size() == blockchainWrapper4.state.getMempool().size());
      REQUIRE(blockchainWrapper1.state.getMempool().size() == blockchainWrapper4.state.getMempool().size());
      REQUIRE(blockchainWrapper1.state.getMempool().size() == blockchainWrapper5.state.getMempool().size());
      REQUIRE(blockchainWrapper1.state.getMempool().size() == blockchainWrapper6.state.getMempool().size());
      REQUIRE(blockchainWrapper1.state.getMempool().size() == blockchainWrapper7.state.getMempool().size());
      REQUIRE(blockchainWrapper1.state.getMempool().size() == blockchainWrapper8.state.getMempool().size());

      // Sleep so it can conclude the last operations.
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, no transactions") {
      // Initialize 8 different node instances, with different ports and DBs.
      auto blockchainWrapper1 = initialize(validatorPrivKeysState, validatorPrivKeysState[0], 8080, true,
                  testDumpPath + "/stateNode1NetworkCapabilities");

      auto blockchainWrapper2 = initialize(validatorPrivKeysState, validatorPrivKeysState[1], 8081, true,
                  testDumpPath + "/stateNode2NetworkCapabilities");

      auto blockchainWrapper3 = initialize(validatorPrivKeysState, validatorPrivKeysState[2], 8082, true,
                  testDumpPath + "/stateNode3NetworkCapabilities");

      auto blockchainWrapper4 = initialize(validatorPrivKeysState, validatorPrivKeysState[3], 8083, true,
                  testDumpPath + "/stateNode4NetworkCapabilities");

      auto blockchainWrapper5 = initialize(validatorPrivKeysState, validatorPrivKeysState[4], 8084, true,
                  testDumpPath + "/stateNode5NetworkCapabilities");

      auto blockchainWrapper6 = initialize(validatorPrivKeysState, validatorPrivKeysState[5], 8085, true,
                  testDumpPath + "/stateNode6NetworkCapabilities");

      auto blockchainWrapper7 = initialize(validatorPrivKeysState, validatorPrivKeysState[6], 8086, true,
                  testDumpPath + "/stateNode7NetworkCapabilities");

      auto blockchainWrapper8 = initialize(validatorPrivKeysState, validatorPrivKeysState[7], 8087, true,
                  testDumpPath + "/stateNode8NetworkCapabilities");

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      uint64_t genesisTimestamp = 1678887538000000;
      FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{},Hash(), genesisTimestamp, 0, genesisPrivKey);
      std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
      std::vector<Address> genesisValidators;
      for (const auto& privKey : validatorPrivKeysState) {
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
      P2P::ManagerDiscovery p2pDiscovery(
          boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

      // Vector of references for the states' rdPoS workers
      // (rdPoS is exclusively owned by State and can't be exposed in any way,
      // so we have to pass the whole State object to access rdPoS functionality
      // via wrapper functions from the State)
      std::vector<std::reference_wrapper<State>> rdPoSreferences;
      rdPoSreferences.emplace_back(blockchainWrapper1.state);
      rdPoSreferences.emplace_back(blockchainWrapper2.state);
      rdPoSreferences.emplace_back(blockchainWrapper3.state);
      rdPoSreferences.emplace_back(blockchainWrapper4.state);
      rdPoSreferences.emplace_back(blockchainWrapper5.state);
      rdPoSreferences.emplace_back(blockchainWrapper6.state);
      rdPoSreferences.emplace_back(blockchainWrapper7.state);
      rdPoSreferences.emplace_back(blockchainWrapper8.state);

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
      blockchainWrapper1.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper2.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper3.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper4.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper5.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper6.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper7.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper8.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      auto discoveryFuture = std::async(std::launch::async, [&]() {
        while (p2pDiscovery.getSessionsIDs().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
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

      // Start discovery
      p2pDiscovery.startDiscovery();
      blockchainWrapper1.p2p.startDiscovery();
      blockchainWrapper2.p2p.startDiscovery();
      blockchainWrapper3.p2p.startDiscovery();
      blockchainWrapper4.p2p.startDiscovery();
      blockchainWrapper5.p2p.startDiscovery();
      blockchainWrapper6.p2p.startDiscovery();
      blockchainWrapper7.p2p.startDiscovery();
      blockchainWrapper8.p2p.startDiscovery();

      // Wait for discovery to take effect
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Wait for nodes to connect.
      auto connectionsFuture = std::async(std::launch::async, [&]() {
        while (p2pDiscovery.getSessionsIDs().size() != 8 ||
               blockchainWrapper1.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper2.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper3.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper4.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper5.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper6.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper7.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper8.p2p.getSessionsIDs().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });

      REQUIRE(connectionsFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper4.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper5.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper6.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper7.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper8.p2p.getSessionsIDs().size() == 8);

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
      // We want to do the following in this test:
      // Wait until we got 10 blocks created.
      // We are not going to create any transactions, just create blocks
      // So we can just wait for the blocks to be created!
      // The Consensus class should be able to deal with the block creation.

      auto stateBlockFuture = std::async(std::launch::async, [&]() {
        uint64_t targetLatestHeight = 1;
        while (blockchainWrapper1.storage.latest()->getNHeight() != 10) {
          // We need to forcefully make a transaction and broadcast in the network so the consensus can create a block
          // otherwise it will sleep forever
          Address targetOfTransactions(Utils::randBytes(20));
          TxBlock tx (targetOfTransactions,
                chainOwnerAddress,
                Bytes(),
                8080,
                blockchainWrapper1.state.getNativeNonce(chainOwnerAddress),
                1000000000000000000,
                1000000000,
                1000000000,
                21000,
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
          }
          ++targetLatestHeight;
        }
      });

      REQUIRE(stateBlockFuture.wait_for(std::chrono::seconds(60)) != std::future_status::timeout);


      // Sanity check: blocks
      uint64_t bestBlockHeight = blockchainWrapper1.storage.latest()->getNHeight();

      for (uint64_t i = 0; i <= bestBlockHeight; ++i)
      {
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper2.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper3.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper4.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper5.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper6.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper7.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper8.storage.getBlock(i)->getHash());
      }


      /// TODO: This is done for the same reason as stopDiscovery.
      blockchainWrapper1.consensus.stop();
      blockchainWrapper2.consensus.stop();
      blockchainWrapper3.consensus.stop();
      blockchainWrapper4.consensus.stop();
      blockchainWrapper5.consensus.stop();
      blockchainWrapper6.consensus.stop();
      blockchainWrapper7.consensus.stop();
      blockchainWrapper8.consensus.stop();
      // Sleep so it can conclude the last operations.
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, broadcast blocks, 1000 transactions per set, 10 sets") {
      // Create random accounts for the transactions.
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 1000; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      // Initialize 8 different node instances, with different ports and DBs.
      auto blockchainWrapper1 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/stateNode1NetworkCapabilitiesWithTx"
      );
      auto blockchainWrapper2 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[1], 8081, true, testDumpPath + "/stateNode2NetworkCapabilitiesWithTx"
      );
      auto blockchainWrapper3 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[2], 8082, true, testDumpPath + "/stateNode3NetworkCapabilitiesWithTx"
      );
      auto blockchainWrapper4 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[3], 8083, true, testDumpPath + "/stateNode4NetworkCapabilitiesWithTx"
      );
      auto blockchainWrapper5 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[4], 8084, true, testDumpPath + "/stateNode5NetworkCapabilitiesWithTx"
      );
      auto blockchainWrapper6 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[5], 8085, true, testDumpPath + "/stateNode6NetworkCapabilitiesWithTx"
      );
      auto blockchainWrapper7 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[6], 8086, true, testDumpPath + "/stateNode7NetworkCapabilitiesWithTx"
      );
      auto blockchainWrapper8 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[7], 8087, true, testDumpPath + "/stateNode8NetworkCapabilitiesWithTx"
      );

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      uint64_t genesisTimestamp = 1678887538000000;
      FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{},Hash(), genesisTimestamp, 0, genesisPrivKey);
      std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
      std::vector<Address> genesisValidators;
      for (const auto& privKey : validatorPrivKeysState) {
        genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
      }
      Options discoveryOptions(
          testDumpPath + "/statedDiscoveryNodeNetworkCapabilitiesWithTx",
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
      P2P::ManagerDiscovery p2pDiscovery(
          boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

      // Initialize state with all balances
      for (const auto &[privkey, account]: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        blockchainWrapper1.state.addBalance(me);
        blockchainWrapper2.state.addBalance(me);
        blockchainWrapper3.state.addBalance(me);
        blockchainWrapper4.state.addBalance(me);
        blockchainWrapper5.state.addBalance(me);
        blockchainWrapper6.state.addBalance(me);
        blockchainWrapper7.state.addBalance(me);
        blockchainWrapper8.state.addBalance(me);
      }

      // Vector of references for the states' rdPoS workers
      // (rdPoS is exclusively owned by State and can't be exposed in any way,
      // so we have to pass the whole State object to access rdPoS functionality
      // via wrapper functions from the State)
      std::vector<std::reference_wrapper<State>> rdPoSreferences;
      rdPoSreferences.emplace_back(blockchainWrapper1.state);
      rdPoSreferences.emplace_back(blockchainWrapper2.state);
      rdPoSreferences.emplace_back(blockchainWrapper3.state);
      rdPoSreferences.emplace_back(blockchainWrapper4.state);
      rdPoSreferences.emplace_back(blockchainWrapper5.state);
      rdPoSreferences.emplace_back(blockchainWrapper6.state);
      rdPoSreferences.emplace_back(blockchainWrapper7.state);
      rdPoSreferences.emplace_back(blockchainWrapper8.state);

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
      blockchainWrapper1.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper2.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper3.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper4.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper5.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper6.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper7.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper8.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

      // Wait everyone be connected with the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      auto discoveryFuture = std::async(std::launch::async, [&]() {
        while (p2pDiscovery.getSessionsIDs().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
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

      // Start discovery
      p2pDiscovery.startDiscovery();
      blockchainWrapper1.p2p.startDiscovery();
      blockchainWrapper2.p2p.startDiscovery();
      blockchainWrapper3.p2p.startDiscovery();
      blockchainWrapper4.p2p.startDiscovery();
      blockchainWrapper5.p2p.startDiscovery();
      blockchainWrapper6.p2p.startDiscovery();
      blockchainWrapper7.p2p.startDiscovery();
      blockchainWrapper8.p2p.startDiscovery();

      // Wait for discovery to take effect
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Wait for nodes to connect.
      std::cout << "Waiting for conns" << std::endl;
      auto connectionsFuture = std::async(std::launch::async, [&]() {
        while (p2pDiscovery.getSessionsIDs().size() != 8 ||
               blockchainWrapper1.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper2.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper3.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper4.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper5.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper6.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper7.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper8.p2p.getSessionsIDs().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });

      REQUIRE(connectionsFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper4.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper5.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper6.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper7.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper8.p2p.getSessionsIDs().size() == 8);

      REQUIRE(blockchainWrapper1.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper2.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper3.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper4.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper5.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper6.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper7.state.rdposGetIsValidator());
      REQUIRE(blockchainWrapper8.state.rdposGetIsValidator());

      std::cout << "Starting consensus" << std::endl;
      blockchainWrapper1.consensus.start();
      blockchainWrapper2.consensus.start();
      blockchainWrapper3.consensus.start();
      blockchainWrapper4.consensus.start();
      blockchainWrapper5.consensus.start();
      blockchainWrapper6.consensus.start();
      blockchainWrapper7.consensus.start();
      blockchainWrapper8.consensus.start();
      std::cout << "Consensus started" << std::endl;

      // For this test we have to create 10x 100 transactions
      // But as the consensus worker is running, we dont actually need to create the blocks
      // Just broadcast the transactions and wait for them to confirm
      // lets first create the set of 10x 100 txs
      std::vector<std::vector<TxBlock>> txs;

      for (uint64_t i = 0; i < 10; ++i) {
        std::vector<TxBlock> txSet;
        /// Add balance to the random Accounts and create random transactions
        for (auto &[privkey, val]: randomAccounts) {
          Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
          TxBlock tx(
              targetOfTransactions,
              me,
              Bytes(),
              8080,
              i,
              1000000000000000000,
              1000000000,
              1000000000,
              21000,
              privkey
          );

          /// Take note of expected balance and nonce
          val.first = blockchainWrapper1.state.getNativeBalance(me) - (tx.getMaxFeePerGas() * tx.getGasLimit()) - tx.getValue();
          val.second = blockchainWrapper1.state.getNativeNonce(me) + 1;
          txSet.push_back(tx);
        }
        txs.emplace_back(std::move(txSet));
      }

      /// For each set of transactions, broadcast them and wait for them to be confirmed
      for (const auto &txSet: txs) {
        std::cout << "Broadcasting txs" << std::endl;
        for (const auto &tx: txSet) {
          auto txStatus = blockchainWrapper1.state.addTx(TxBlock(tx));
          REQUIRE(isTxStatusValid(txStatus));
          blockchainWrapper1.p2p.getBroadcaster().broadcastTxBlock(tx);
          targetExpectedValue += tx.getValue();
        }

        /// Wait for the transactions to be confirmed.
        auto confirmFuture = std::async(std::launch::async, [&]() {
          while (true) {
            bool allConfirmed = true;
            for (const auto &tx: txSet) {
              if (!blockchainWrapper1.storage.txExists(tx.hash()) ||
                  !blockchainWrapper2.storage.txExists(tx.hash()) ||
                  !blockchainWrapper3.storage.txExists(tx.hash()) ||
                  !blockchainWrapper4.storage.txExists(tx.hash()) ||
                  !blockchainWrapper5.storage.txExists(tx.hash()) ||
                  !blockchainWrapper6.storage.txExists(tx.hash()) ||
                  !blockchainWrapper7.storage.txExists(tx.hash()) ||
                  !blockchainWrapper8.storage.txExists(tx.hash())) {
                allConfirmed = false;
              }
            }
            if (allConfirmed) { break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
          }
        });

        // Average time of above test is 3s; need a much larger timeout.
        REQUIRE(confirmFuture.wait_for(std::chrono::seconds(120)) != std::future_status::timeout);

        // Check balances for target
        REQUIRE(blockchainWrapper1.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
        REQUIRE(blockchainWrapper2.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
        REQUIRE(blockchainWrapper3.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
        REQUIRE(blockchainWrapper4.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
        REQUIRE(blockchainWrapper5.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
        REQUIRE(blockchainWrapper6.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
        REQUIRE(blockchainWrapper7.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
        REQUIRE(blockchainWrapper8.state.getNativeBalance(targetOfTransactions) == targetExpectedValue);
      }

      // Sanity check: blocks
      uint64_t bestBlockHeight = blockchainWrapper1.storage.latest()->getNHeight();

      for (uint64_t i = 0; i <= bestBlockHeight; ++i)
      {
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper2.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper3.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper4.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper5.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper6.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper7.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper8.storage.getBlock(i)->getHash());
      }

      /// TODO: This is done for the same reason as stopDiscovery.
      std::cout << "Stopping consensus for wrapper 1" << std::endl;
      blockchainWrapper1.consensus.stop();
      std::cout << "Stopping consensus for wrapper 2" << std::endl;
      blockchainWrapper2.consensus.stop();
      std::cout << "Stopping consensus for wrapper 3" << std::endl;
      blockchainWrapper3.consensus.stop();
      std::cout << "Stopping consensus for wrapper 4" << std::endl;
      blockchainWrapper4.consensus.stop();
      std::cout << "Stopping consensus for wrapper 5" << std::endl;
      blockchainWrapper5.consensus.stop();
      std::cout << "Stopping consensus for wrapper 6" << std::endl;
      blockchainWrapper6.consensus.stop();
      std::cout << "Stopping consensus for wrapper 7" << std::endl;
      blockchainWrapper7.consensus.stop();
      std::cout << "Stopping consensus for wrapper 8" << std::endl;
      blockchainWrapper8.consensus.stop();
      // Sleep so it can conclude the last operations.
      std::cout << "Everything should have succesfully stopped..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    SECTION("State test with networking capabilities, 8 nodes, rdPoS fully active, 1 ERC20 transactions per block, broadcast blocks.") {
      // Create random accounts for the transactions.
      PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      Address owner = Secp256k1::toAddress(Secp256k1::toUPub(ownerPrivKey));

      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;

      // Initialize 8 different node instances, with different ports and DBs.
      auto blockchainWrapper1 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[0], 8080, true, testDumpPath + "/stateNode1NetworkCapabilitiesWithERC20TxBlockBroadcast"
      );
      auto blockchainWrapper2 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[1], 8081, true,  testDumpPath + "/stateNode2NetworkCapabilitiesWithERC20TxBlockBroadcast"
      );
      auto blockchainWrapper3 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[2], 8082, true,  testDumpPath + "/stateNode3NetworkCapabilitiesWithERC20TxBlockBroadcast"
      );
      auto blockchainWrapper4 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[3], 8083, true,  testDumpPath + "/stateNode4NetworkCapabilitiesWithERC20TxBlockBroadcast"
      );
      auto blockchainWrapper5 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[4], 8084, true,  testDumpPath + "/stateNode5NetworkCapabilitiesWithERC20TxBlockBroadcast"
      );
      auto blockchainWrapper6 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[5], 8085, true,  testDumpPath + "/stateNode6NetworkCapabilitiesWithERC20TxBlockBroadcast"
      );
      auto blockchainWrapper7 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[6], 8086, true,  testDumpPath + "/stateNode7NetworkCapabilitiesWithERC20TxBlockBroadcast"
      );
      auto blockchainWrapper8 = initialize(
        validatorPrivKeysState, validatorPrivKeysState[7], 8087, true,  testDumpPath + "/stateNode8NetworkCapabilitiesWithERC20TxBlockBroadcast"
      );

      // Initialize the discovery node.
      std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
      PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
      uint64_t genesisTimestamp = 1678887538000000;
      FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{},Hash(), genesisTimestamp, 0, genesisPrivKey);
      std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
      std::vector<Address> genesisValidators;
      for (const auto& privKey : validatorPrivKeysState) {
        genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
      }
      Options discoveryOptions(
          testDumpPath + "/statedDiscoveryNodeNetworkCapabilitiesWithTxBlockBroadcast",
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
      P2P::ManagerDiscovery p2pDiscovery(
          boost::asio::ip::address::from_string("127.0.0.1"), discoveryOptions);

      // Initialize state with all balances
      blockchainWrapper1.state.addBalance(owner);
      blockchainWrapper2.state.addBalance(owner);
      blockchainWrapper3.state.addBalance(owner);
      blockchainWrapper4.state.addBalance(owner);
      blockchainWrapper5.state.addBalance(owner);
      blockchainWrapper6.state.addBalance(owner);
      blockchainWrapper7.state.addBalance(owner);
      blockchainWrapper8.state.addBalance(owner);

      // Vector of references for the states' rdPoS workers
      // (rdPoS is exclusively owned by State and can't be exposed in any way,
      // so we have to pass the whole State object to access rdPoS functionality
      // via wrapper functions from the State)
      std::vector<std::reference_wrapper<State>> rdPoSreferences;
      rdPoSreferences.emplace_back(blockchainWrapper1.state);
      rdPoSreferences.emplace_back(blockchainWrapper2.state);
      rdPoSreferences.emplace_back(blockchainWrapper3.state);
      rdPoSreferences.emplace_back(blockchainWrapper4.state);
      rdPoSreferences.emplace_back(blockchainWrapper5.state);
      rdPoSreferences.emplace_back(blockchainWrapper6.state);
      rdPoSreferences.emplace_back(blockchainWrapper7.state);
      rdPoSreferences.emplace_back(blockchainWrapper8.state);

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
      blockchainWrapper1.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper2.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper3.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper4.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper5.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper6.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper7.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);
      blockchainWrapper8.p2p.connectToServer(boost::asio::ip::address::from_string("127.0.0.1"), 8090);

      // Wait everyone be connected with the discovery node.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // After a while, the discovery thread should have found all the nodes and connected between each other.
      auto discoveryFuture = std::async(std::launch::async, [&]() {
        while (p2pDiscovery.getSessionsIDs().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
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

      // Start discovery
      p2pDiscovery.startDiscovery();
      blockchainWrapper1.p2p.startDiscovery();
      blockchainWrapper2.p2p.startDiscovery();
      blockchainWrapper3.p2p.startDiscovery();
      blockchainWrapper4.p2p.startDiscovery();
      blockchainWrapper5.p2p.startDiscovery();
      blockchainWrapper6.p2p.startDiscovery();
      blockchainWrapper7.p2p.startDiscovery();
      blockchainWrapper8.p2p.startDiscovery();

      // Wait for nodes to connect.
      auto connectionsFuture = std::async(std::launch::async, [&]() {
        while (p2pDiscovery.getSessionsIDs().size() != 8 ||
               blockchainWrapper1.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper2.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper3.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper4.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper5.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper6.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper7.p2p.getSessionsIDs().size() != 8 ||
               blockchainWrapper8.p2p.getSessionsIDs().size() != 8) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });

      REQUIRE(connectionsFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

      // Stop discovery after all nodes have connected to each other.
      // TODO: this is done because there is a mess of mutexes within broadcast
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

      REQUIRE(p2pDiscovery.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper1.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper2.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper3.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper4.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper5.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper6.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper7.p2p.getSessionsIDs().size() == 8);
      REQUIRE(blockchainWrapper8.p2p.getSessionsIDs().size() == 8);

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

      // As Consensus is running
      // We only need to create the txSets and wait for them to be confirmed
      // We are doing 10 ERC20 txs, one per block
      std::vector<TxBlock> txs;
      Hash creationHash = Hash();
      Address ERC20ContractAddress = ContractHost::deriveContractAddress(blockchainWrapper1.state.getNativeNonce(owner), owner);
      uint64_t nonce = 0;
      for (uint64_t i = 0; i < 10; ++i) {
        if (i == 0) {
          // First Tx **MUST** be contract creation
          std::string tokenName = "TestToken";
          std::string tokenSymbol = "TT";
          uint256_t tokenDecimals = uint256_t(18);
          uint256_t tokenSupply = uint256_t(1000000000000000000);

          Bytes createNewERC20ContractEncoder = ABI::Encoder::encodeData(tokenName, tokenSymbol, tokenDecimals,
                                                                            tokenSupply);
          Bytes createNewERC20ContractData = Hex::toBytes("0xb74e5ed5");
          Utils::appendBytes(createNewERC20ContractData, createNewERC20ContractEncoder);

          TxBlock createNewERC2OTx = TxBlock(
              ProtocolContractAddresses.at("ContractManager"),
              owner,
              createNewERC20ContractData,
              8080,
              nonce,
              0,
              1000000000,
              1000000000,
              21000,
              ownerPrivKey
          );
          creationHash = createNewERC2OTx.hash();
          txs.push_back(createNewERC2OTx);
          ++nonce;
        } else {
          Bytes transferEncoder = ABI::Encoder::encodeData(targetOfTransactions, uint256_t(10000000000000000));
          Bytes transferData = Hex::toBytes("0xa9059cbb");
          Utils::appendBytes(transferData, transferEncoder);

          TxBlock transferERC20 = TxBlock(
              ERC20ContractAddress,
              owner,
              transferData,
              8080,
              nonce,
              0,
              21000,
              1000000000,
              1000000000,
              ownerPrivKey
          );
          ++nonce;
          txs.push_back(transferERC20);
        }
      }

      // Now lets process these transactions and wait for them to be confirmed
      for (const auto &tx: txs) {
        if (tx.hash() != creationHash) {
          targetExpectedValue += 10000000000000000;
        }
        auto txStatus = blockchainWrapper1.state.addTx(TxBlock(tx));
        REQUIRE(isTxStatusValid(txStatus));
        blockchainWrapper1.p2p.getBroadcaster().broadcastTxBlock(tx);
        /// Wait for the transactions to be confirmed.
        ///
        auto confirmFuture = std::async(std::launch::async, [&]() {
          while (true) {
            if (blockchainWrapper1.storage.txExists(tx.hash()) &&
                blockchainWrapper2.storage.txExists(tx.hash()) &&
                blockchainWrapper3.storage.txExists(tx.hash()) &&
                blockchainWrapper4.storage.txExists(tx.hash()) &&
                blockchainWrapper5.storage.txExists(tx.hash()) &&
                blockchainWrapper6.storage.txExists(tx.hash()) &&
                blockchainWrapper7.storage.txExists(tx.hash()) &&
                blockchainWrapper8.storage.txExists(tx.hash())) {
              break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
          }
        });

        REQUIRE(confirmFuture.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

        Bytes getBalanceMeEncoder = ABI::Encoder::encodeData(targetOfTransactions);
        Functor getBalanceMeFunctor = ABI::FunctorEncoder::encode<Address>("balanceOf");
        Bytes getBalanceMeNode1Result = blockchainWrapper1.state.ethCall(
            buildCallInfo(ERC20ContractAddress, getBalanceMeFunctor, getBalanceMeEncoder).first);

        auto getBalanceMeNode1Decoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeNode1Result);
        REQUIRE(std::get<0>(getBalanceMeNode1Decoder) == targetExpectedValue);

        Bytes getBalanceMeNode2Result = blockchainWrapper2.state.ethCall(
            buildCallInfo(ERC20ContractAddress, getBalanceMeFunctor, getBalanceMeEncoder).first);

        auto getBalanceMeNode2Decoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeNode2Result);
        REQUIRE(std::get<0>(getBalanceMeNode2Decoder) == targetExpectedValue);

        Bytes getBalanceMeNode3Result = blockchainWrapper3.state.ethCall(
            buildCallInfo(ERC20ContractAddress, getBalanceMeFunctor, getBalanceMeEncoder).first);

        auto getBalanceMeNode3Decoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeNode3Result);
        REQUIRE(std::get<0>(getBalanceMeNode3Decoder) == targetExpectedValue);

        Bytes getBalanceMeNode4Result = blockchainWrapper4.state.ethCall(
            buildCallInfo(ERC20ContractAddress, getBalanceMeFunctor, getBalanceMeEncoder).first);

        auto getBalanceMeNode4Decoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeNode4Result);
        REQUIRE(std::get<0>(getBalanceMeNode4Decoder) == targetExpectedValue);

        Bytes getBalanceMeNode5Result = blockchainWrapper5.state.ethCall(
            buildCallInfo(ERC20ContractAddress, getBalanceMeFunctor, getBalanceMeEncoder).first);

        auto getBalanceMeNode5Decoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeNode5Result);
        REQUIRE(std::get<0>(getBalanceMeNode5Decoder) == targetExpectedValue);

        Bytes getBalanceMeNode6Result = blockchainWrapper6.state.ethCall(
            buildCallInfo(ERC20ContractAddress, getBalanceMeFunctor, getBalanceMeEncoder).first);

        auto getBalanceMeNode6Decoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeNode6Result);
        REQUIRE(std::get<0>(getBalanceMeNode6Decoder) == targetExpectedValue);

        Bytes getBalanceMeNode7Result = blockchainWrapper7.state.ethCall(
            buildCallInfo(ERC20ContractAddress, getBalanceMeFunctor, getBalanceMeEncoder).first);

        auto getBalanceMeNode7Decoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeNode7Result);
        REQUIRE(std::get<0>(getBalanceMeNode7Decoder) == targetExpectedValue);

        Bytes getBalanceMeNode8Result = blockchainWrapper8.state.ethCall(
            buildCallInfo(ERC20ContractAddress, getBalanceMeFunctor, getBalanceMeEncoder).first);

        auto getBalanceMeNode8Decoder = ABI::Decoder::decodeData<uint256_t>(getBalanceMeNode8Result);
        REQUIRE(std::get<0>(getBalanceMeNode8Decoder) == targetExpectedValue);
      }

      // Sanity check: blocks
      uint64_t bestBlockHeight = blockchainWrapper1.storage.latest()->getNHeight();

      for (uint64_t i = 0; i <= bestBlockHeight; ++i)
      {
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper2.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper3.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper4.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper5.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper6.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper7.storage.getBlock(i)->getHash());
        REQUIRE(blockchainWrapper1.storage.getBlock(i)->getHash() == blockchainWrapper8.storage.getBlock(i)->getHash());
      }

      /// TODO: This is done for the same reason as stopDiscovery.
      blockchainWrapper1.consensus.stop();
      blockchainWrapper2.consensus.stop();
      blockchainWrapper3.consensus.stop();
      blockchainWrapper4.consensus.stop();
      blockchainWrapper5.consensus.stop();
      blockchainWrapper6.consensus.stop();
      blockchainWrapper7.consensus.stop();
      blockchainWrapper8.consensus.stop();
      // Sleep so it can conclude the last operations.
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}
