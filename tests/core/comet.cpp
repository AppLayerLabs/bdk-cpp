/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/core/comet.h"

std::string createTestDumpPath(const std::string& testDir) {
  std::string testDumpPath = Utils::getTestDumpPath() + "/" + testDir;
  if (std::filesystem::exists(testDumpPath)) {
    std::filesystem::remove_all(testDumpPath);
  }
  std::filesystem::create_directories(testDumpPath);
  GLOGDEBUG("Test dump path: " + testDumpPath);
  return testDumpPath;
}

/**
 * Get an Options object to test a single Comet instance.
 * @param keyNumber Index of validator key from the predefined test validator key set.
 * @param numKeys Number of validator keys to include in the genesis spec.
 * @return Options object set up for testing a Comet instance.
 */
Options getOptionsForCometTest(const std::string rootPath, int keyNumber = 0, int numKeys = 1) {

  // TODO:
  // - use keyNumber
  // - use numKeys
  // - generate other Options parameters to allow it to test a full Blockchain instance
  //   that has a Comet instance, instead of just a standalone Comet instance

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

  // NOTE: most parameters are unused by the Comet class
  PrivKey genesisPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));
  FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({},{}, Hash(), 0, 0, genesisPrivKey);
  const Options options = Options(
    rootPath,
    "BDK/cpp/linux_x86-64/0.2.0",
    1,
    8080,
    Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
    LOCALHOST,
    12345,
    9999,
    11,
    11,
    200,
    50,
    2000,
    10000,
    1000,
    4,
    {},
    genesis,
    0,
    genesisPrivKey,
    {},
    {},
    IndexingMode::RPC_TRACE,
    defaultCometBFTOptions
  );

  return options;
}

namespace TComet {
  TEST_CASE("Comet ", "[core][comet]") {
    SECTION("CometBootTest") {

      // Very simple test flow that runs a single cometbft node that runs a single-validator blockchain
      //   that can thus advance with a single validator producing blocks.

      std::string testDumpPath = createTestDumpPath("CometBootTest");

      GLOGDEBUG("TEST: Constructing Comet");

      const Options options = getOptionsForCometTest(testDumpPath);

      // Create a simple listener that just records that we got InitChain and what the current height is.
      class TestCometListener : public CometListener {
      public:
        std::atomic<bool> gotInitChain = false;
        std::atomic<uint64_t> finalizedHeight = 0;
        virtual void initChain() {
          gotInitChain = true;
        }
        virtual void incomingBlock(const uint64_t height, const uint64_t syncingToHeight, const std::vector<Bytes>& txs, Bytes& appHash) {
          finalizedHeight = height;
          appHash.clear();
        }
      };
      TestCometListener cometListener;

      // Set up comet with single validator
      Comet comet(&cometListener, "", options);

      // Set pause at configured
      comet.setPauseState(CometState::CONFIGURED);

      GLOGDEBUG("TEST: Starting");

      // Start comet.
      comet.start();

      // --- config check ---

      GLOGDEBUG("TEST: Waiting configuration");

      // Waits for the pause state or error status
      REQUIRE(comet.waitPauseState(10000) == "");

      // --- start ABCI server ---

      comet.setPauseState(CometState::STARTED_ABCI);

      GLOGDEBUG("TEST: Waiting for gRPC server to be successfully started");

      // Waits for the pause state or error status
      REQUIRE(comet.waitPauseState(10000) == "");

      // --- start cometbft ---

      comet.setPauseState(CometState::STARTED_COMET);

      GLOGDEBUG("TEST: Waiting for 'cometbft start' to be successfully started");

      // Waits for the pause state or error status
      REQUIRE(comet.waitPauseState(10000) == "");

      // --- test ABCI connection ---

      // Set pause at tested the comet gRPC connection
      comet.setPauseState(CometState::TESTED_COMET);

      GLOGDEBUG("TEST: Waiting for ABCI connection test");

      // Waits for the pause state or error status
      REQUIRE(comet.waitPauseState(10000) == "");

      // --- Wait for an InitChain ABCI callback ---

      GLOGDEBUG("TEST: Waiting for CometBFT InitChain");

      auto futureInitChain = std::async(std::launch::async, [&]() {
        while (!cometListener.gotInitChain) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });
      REQUIRE(futureInitChain.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);
      REQUIRE(cometListener.gotInitChain);

      // --- Wait for a FinalizeBlock ABCI callback for the first produced block ---

      GLOGDEBUG("TEST: Waiting for CometBFT FinalizeBlock");

      auto futureFinalizeBlock = std::async(std::launch::async, [&]() {
        while (cometListener.finalizedHeight < 1) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });
      REQUIRE(futureFinalizeBlock.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);
      REQUIRE(cometListener.finalizedHeight >= 1);

      // --- stop ---

      GLOGDEBUG("TEST: Stopping...");

      REQUIRE(comet.getStatus()); // no error reported (must check before stop())

      // Stop comet.
      comet.stop();

      GLOGDEBUG("TEST: Stopped");

      REQUIRE(comet.getState() == CometState::STOPPED);

      GLOGDEBUG("TEST: Finished");
    }

    // TODO: stop at block M and restart test with no block replay, 1 validator
    // (snapshotted state at M before shutdown reported by Info on restart)
    // Non-empty transactions and verify that it reaches the same end state

    // TODO: stop at block M and restart test with some block replay, 1 validator
    // (Info on restart reports block N where N<M and thus replays N+1 to M)
    // Non-empty transactions and verify that it reaches the same end state

    // TODO: stop and restart test with full block replay, 1 validator
    // (Info on restart reports block 0, causing InitChain to be called again and replay to M)
    // Non-empty transactions and verify that it reaches the same end state

    // TODO: a test that runs a blockchain/genesis with two validators, that is:
    //       getOptionsForCometTest( path , 0 , 2 );   // instance 1 of 2
    //       getOptionsForCometTest( path , 1 , 2 );   // instance 2 of 2
  }
}
