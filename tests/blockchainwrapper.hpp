/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAINWRAPPER_H
#define BLOCKCHAINWRAPPER_H

#include "../src/core/blockchain.h" // net/http/httpserver.h, consensus.h -> state.h -> (rdpos.h -> net/p2p/managernormal.h), dump.h -> (storage.h -> utils/options.h), utils/db.h -> utils.h

/**
 * Simple wrapper struct for management of all blockchain related objects.
 * Initializes them properly in the constructor, allowing direct access to the
 * underlying objects, but does not apply any logic or checks.
 */
struct TestBlockchainWrapper {
  const Options options;  ///< Options singleton.
  P2P::ManagerNormal p2p; ///< P2P connection manager. NOTE: p2p needs to be constructed first due to getLogicalLocation().
  DB db;                  ///< Database.
  Storage storage;        ///< Blockchain storage.
  State state;            ///< Blockchain state.
  HTTPServer http;        ///< HTTP server.
  Syncer syncer;          ///< Blockchain syncer.
  Consensus consensus;    ///< Block and transaction processing.

  // TODO: Integrating comet with the existing execution engine to be tested will
  //       be done after all tests with the mock execution environment are working.
  //Comet comet;           ///< External consensus engine manager.

  /**
   * Constructor.
   * @param options_ Reference to the Options singleton.
   */

  explicit TestBlockchainWrapper(const Options& options_) :
    options(options_),
    p2p(LOCALHOST, options, storage, state),
    db(std::get<0>(DumpManager::getBestStateDBPath(options))),
    storage(p2p.getLogicalLocation(), options),
    state(db, storage, p2p, std::get<1>(DumpManager::getBestStateDBPath(options)), options),
    http(state, storage, p2p, options),
    syncer(p2p, storage, state),
    consensus(state, p2p, storage, options)

    // TODO: Integrating comet with the existing execution engine to be tested will
    //       be done after all tests with the mock execution environment are working.
    //comet(p2p.getLogicalLocation(), options)
    {};

  /// Destructor.
  ~TestBlockchainWrapper() {
    state.dumpStopWorker();
    // TODO: Integrating comet with the existing execution engine to be tested will
    //       be done after all tests with the mock execution environment are working.
    //comet.stop();
    consensus.stop();
    p2p.stopDiscovery();
    p2p.stop();
    http.stop();
  }
};

// We initialize the blockchain database
// To make sure that if the genesis is changed within the main source code
// The tests will still work, as tests uses own genesis block.
inline TestBlockchainWrapper initialize(const std::vector<Hash>& validatorPrivKeys,
                                 const PrivKey& validatorKey,
                                 const uint64_t& serverPort,
                                 bool clearDb,
                                 const std::string& folderName) {
  if (clearDb) {
    if (std::filesystem::exists(folderName)) {
      std::filesystem::remove_all(folderName);
    }
  }
  std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
  PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  uint64_t genesisTimestamp = 1656356646000000;
  FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{}, Hash(), genesisTimestamp, 0, genesisPrivKey);
  std::vector<std::pair<Address,uint256_t>> genesisBalances = {{Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")), uint256_t("1000000000000000000000")}};
  std::vector<Address> genesisValidators;
  for (const auto& privKey : validatorPrivKeys) {
    genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
  }
  // Use a default cometbft genesis and validator private key for testing
  json defaultCometBFTOptions = json::parse(R"(
    {
      "genesis":
      {
        "genesis_time": "2024-09-17T18:26:34.583377166Z",
        "chain_id": "test-chain-Q1JYzM",
        "initial_height": "0",
        "consensus_params": {
          "block": {
            "max_bytes": "22020096",
            "max_gas": "-1"
          },
          "evidence": {
            "max_age_num_blocks": "100000",
            "max_age_duration": "172800000000000",
            "max_bytes": "1048576"
          },
          "validator": {
            "pub_key_types": [
              "ed25519"
            ]
          },
          "version": {
            "app": "0"
          },
          "abci": {
            "vote_extensions_enable_height": "0"
          }
        },
        "validators": [
          {
            "address": "4C1C6CF20843997082D7F7EF302A05DD6A757B99",
            "pub_key": {
              "type": "tendermint/PubKeyEd25519",
              "value": "c9lrxwblmJz23RhnNZtoab0UlL6wtEjbsm+a7olOShI="
            },
            "power": "10",
            "name": ""
          }
        ],
        "app_hash": ""
      },

      "privValidatorKey":
      {
        "address": "4C1C6CF20843997082D7F7EF302A05DD6A757B99",
        "pub_key": {
          "type": "tendermint/PubKeyEd25519",
          "value": "c9lrxwblmJz23RhnNZtoab0UlL6wtEjbsm+a7olOShI="
        },
        "priv_key": {
          "type": "tendermint/PrivKeyEd25519",
          "value": "u754POzgx4Tc4JBZvVbt4MVk+EhN0GePq1RcMmXj7BJz2WvHBuWYnPbdGGc1m2hpvRSUvrC0SNuyb5ruiU5KEg=="
        }
      }

    }
  )");
  if (!validatorKey) {
    return TestBlockchainWrapper(Options(
        folderName,
        "BDK/cpp/linux_x86-64/0.2.0",
        1,
        8080,
        Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
        LOCALHOST,
        serverPort,
        9999,
        11,
        11,
        200,
        50,
        2000,
        10000,
        1000,
        7,
        discoveryNodes,
        genesis,
        genesisTimestamp,
        genesisPrivKey,
        genesisBalances,
        genesisValidators,
        IndexingMode::RPC_TRACE,
        defaultCometBFTOptions
      ));
  } else {
    return TestBlockchainWrapper(Options(
      folderName,
      "BDK/cpp/linux_x86-64/0.2.0",
      1,
      8080,
      Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
      LOCALHOST,
      serverPort,
      9999,
      11,
      11,
      200,
      50,
      2000,
      10000,
      1000,
      7,
      discoveryNodes,
      genesis,
      genesisTimestamp,
      genesisPrivKey,
      genesisBalances,
      genesisValidators,
      validatorKey,
      IndexingMode::RPC,
      defaultCometBFTOptions
    ));
  }
}

// This creates a valid block given the state within the rdPoS class.
// Should not be used during network/thread testing, as it will automatically sign all TxValidator transactions within the block
// And that is not the purpose of network/thread testing.
inline FinalizedBlock createValidBlock(const std::vector<Hash>& validatorPrivKeys,
                                State& state, Storage& storage,
                                std::vector<TxBlock>&& txs = {}) {
  auto validators = state.rdposGetValidators();
  auto randomList = state.rdposGetRandomList();

  Hash blockSignerPrivKey;           // Private key for the block signer.
  std::vector<Hash> orderedPrivKeys; // Private keys for the rdPoS in the order of the random list, limited to rdPoS' minValidators.
  orderedPrivKeys.reserve(4);
  for (const auto& privKey : validatorPrivKeys) {
    if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
      blockSignerPrivKey = privKey;
      break;
    }
  }

  for (uint64_t i = 1; i < state.rdposGetMinValidators() + 1; i++) {
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
  uint64_t newBlocknHeight = storage.latest()->getNHeight() + 1;
  uint64_t newBlockTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  Hash newBlockPrevHash = storage.latest()->getHash();
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomTxs;

  std::vector<Hash> randomSeeds(orderedPrivKeys.size(), Hash::random());
  for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
    Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
    Bytes hashTxData = Hex::toBytes("0xcfffe746");
    Utils::appendBytes(hashTxData, Utils::sha3(randomSeeds[i]));
    Bytes randomTxData = Hex::toBytes("0x6fc5a2d6");
    Utils::appendBytes(randomTxData, randomSeeds[i]);
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
  std::vector<TxValidator> txsValidator;
  for (const auto& tx : randomHashTxs) {
    state.rdposAddValidatorTx(tx);
    txsValidator.emplace_back(tx);
  }
  for (const auto& tx : randomTxs) {
    state.rdposAddValidatorTx(tx);
    txsValidator.emplace_back(tx);
  }

  // Check rdPoS mempool.
  auto rdPoSmempool = state.rdposGetMempool();
  REQUIRE(state.rdposGetMempool().size() == 14);
  for (const auto& tx : randomHashTxs) {
    REQUIRE(rdPoSmempool.contains(tx.hash()));
  }
  for (const auto& tx : randomTxs) {
    REQUIRE(rdPoSmempool.contains(tx.hash()));
  }

  // Finalize the block
  FinalizedBlock finalized = FinalizedBlock::createNewValidBlock(std::move(txs),
                                                                 std::move(txsValidator),
                                                                 newBlockPrevHash,
                                                                 newBlockTimestamp,
                                                                 newBlocknHeight,
                                                                 blockSignerPrivKey);
  return finalized;
}

/**
 * Soft time limit check that can be placed inside test macros like REQUIRE().
 * TEST_CHECK_TIME: prints only if the limit is exceeded.
 * TEST_CHECK_TIME_VERBOSE: always prints the time elapsed.
 */
template<typename Func>
bool testCheckTime(const char* file, int line, Func&& func, int timeLimitSeconds, bool printInfo) {
  std::filesystem::path filePath(file);
  std::string fileName = filePath.filename().string();
  auto start = std::chrono::high_resolution_clock::now();
  bool result = func();
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  auto timeLimit = std::chrono::milliseconds(timeLimitSeconds * 1000);
  bool warn = duration > timeLimit;
  if (printInfo || warn) {
    std::string msg = warn ? "WARNING" : "INFO";
    msg += " [TIME]: " + std::to_string(duration.count()) + "/" + std::to_string(timeLimit.count())
           + " ms (" + fileName + ":" + std::to_string(line) + ")";
    Utils::safePrintTest(msg);
  }
  return result;
}

#define TEST_CHECK_TIME(func, timeLimitSeconds) \
    testCheckTime(__FILE__, __LINE__, [&]() { return (func); }, timeLimitSeconds, false)

#define TEST_CHECK_TIME_VERBOSE(func, timeLimitSeconds) \
    testCheckTime(__FILE__, __LINE__, [&]() { return (func); }, timeLimitSeconds, true)

/**
 * Helper class for temporarily changing the log level in the scope of unit tests.
 */
class TempLogLevel {
  LogType old_;
public:
  TempLogLevel(LogType tmp) {
    old_ = Logger::getLogLevel();
    Logger::setLogLevel(tmp);
  }
  ~TempLogLevel() {
    Logger::setLogLevel(old_);
  }
};

/**
 * Helper class for temporarily enabling echoing to stdout in the scope of unit tests.
 */
class TempEchoToCout {
public:
  TempEchoToCout() { Logger::setEchoToCout(true); }
  ~TempEchoToCout() { Logger::setEchoToCout(false); }
};

#endif // BLOCKCHAINWRAPPER_H
