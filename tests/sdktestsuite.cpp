/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/utils/clargs.h" // ProcessOptions

#include "../../src/core/comet.h"

#include "sdktestsuite.hpp"

// ----------------------------------------------------------------------
// Initialize static listen port generator parameters
// ----------------------------------------------------------------------

int SDKTestSuite::p2pListenPortMin_ = 20000;
int SDKTestSuite::p2pListenPortMax_ = 29999;
int SDKTestSuite::p2pListenPortGen_ = SDKTestSuite::p2pListenPortMin_;

// ----------------------------------------------------------------------
// Catch2 globals
// ----------------------------------------------------------------------

/**
 * Custom logging listener for Catch2
 */
class LoggingListener : public Catch::EventListenerBase {
public:
  using EventListenerBase::EventListenerBase;

  std::string testCaseName = "NONE";

  // Called when a test run is starting
  void testRunStarting(Catch::TestRunInfo const& testRunInfo) override {
    GLOGINFO("Starting test run: " + testRunInfo.name);
  }

  // Called when a test case is starting
  void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
    GLOGINFO("Starting TEST_CASE: " + testInfo.name);
    testCaseName = testInfo.name;
  }

  // Called when a section is starting
  void sectionStarting(Catch::SectionInfo const& sectionInfo) override {
    GLOGINFO("[" + testCaseName + "]: Starting SECTION: " + sectionInfo.name);
  }

  void sectionEnded(Catch::SectionStats const& sectionStats) override {
    GLOGINFO("[" + testCaseName + "]: Finished SECTION: " + sectionStats.sectionInfo.name);
  }

  // Called when a test case has ended
  void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
    GLOGINFO("Finished TEST_CASE: " + testCaseStats.testInfo->name);
    testCaseName = "NONE";
  }

  // Called when a test run has ended
  void testRunEnded(Catch::TestRunStats const& testRunStats) override {
    GLOGINFO("Finished test run: " + std::to_string(testRunStats.totals.testCases.total()) + " test cases run.");
  }
};

CATCH_REGISTER_LISTENER(LoggingListener)

/**
 * Custom main function for Catch2.
 * We can define our own main() here because CMakeLists.txt is
 * defining CATCH_AMALGAMATED_CUSTOM_MAIN.
 */
int main(int argc, char* argv[]) {
  Utils::safePrintTest("bdkd-tests: Blockchain Development Kit unit test suite");
  Utils::safePrintTest("Any arguments before -- are sent to Catch2");
  Utils::safePrintTest("Any arguments after -- are sent to the BDK args parser");

  std::vector<char*> bdkArgs;
  std::vector<char*> catchArgs;
  bdkArgs.push_back(argv[0]);
  catchArgs.push_back(argv[0]);

  bool bdkArgsStarted = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--") == 0) {
      bdkArgsStarted = true;
      continue;
    }
    if (bdkArgsStarted) {
      bdkArgs.push_back(argv[i]);
    } else {
      catchArgs.push_back(argv[i]);
    }
  }

  // Even if there are no BDK args supplied, run this to apply the default debug level we want
  Utils::safePrintTest("Processing BDK args and defaults...");
  ProcessOptions opt = parseCommandLineArgs(bdkArgs.size(), bdkArgs.data(), BDKTool::UNIT_TEST_SUITE);
  if (opt.logLevel == "") opt.logLevel = "DEBUG";
  if (!applyProcessOptions(opt)) return 1;

  // Check cometbft engine
  Comet::checkCometBFT();

  // FIXME: send this to Blockchain instance instead
  // Avoid ManagerBase::instanceIdGen_ == 0, which produces log logical location string ""
  //   (for production nodes that only instantiate one ManagerBase, ever, and don't need
  //    the logical location consuming space in the log file)
  //P2P::ManagerBase::setTesting();

  Utils::safePrintTest("Running Catch2...");
  int result = Catch::Session().run(catchArgs.size(), catchArgs.data());
  return result;
}

// ----------------------------------------------------------------------
// SDKTestSuite impl
// ----------------------------------------------------------------------

SDKTestSuite SDKTestSuite::createNewEnvironment(
  const std::string& sdkPath,
  const std::vector<TestAccount>& accounts,
  const Options* const options,
  const std::string& instanceId
) {
  if (std::filesystem::exists(sdkPath)) std::filesystem::remove_all(sdkPath);

  /*
    TODO:
    Default ("genesis") state such as accounts should either:
    1 - be injected directly into the State.
    2 - be encoded in a serialized snapshot that is included in the cometBFT
        genesis data support.
    Since 1 is easier, we'll do that first during integration since that's
    mostly for testing. When serialization and deserialization of machine state
    is added, we can upgrade it to 2.
    For production we can just hard-code genesis state (i.e. the initial machine
    state, such as pre-existing accounts and balances, pre-existing deployed
    contracts at height 0, etc) in the binary -- that is, a hard-coded protocol
    rule that is implicit.
    In fact, since the State object and the machine have no support, currently,
    to be anything other than genesis state (while there's no flat-file
    serialization and deserialization implemented), then every use case such as
    tests must inject the starting State on boot since it's always starting
    from genesis height 0 on node boot (testcases that load or save state from/to
    the old stateDb are just deleted for now).
  */

  // Create a default options if none is provided.
  std::unique_ptr<Options> options_;
  if (options == nullptr) {
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

    // Genesis block and validators is entirely on the comet side (comet genesis file)

    // Discovery nodes will be implemented using cometbft seed-nodes

    // TODO: accounts, contracts, etc. is to be injected directly into the State here since
    //       the State IS genesis state by definition while we don't do load/save from
    //       snapshot flat files.
    //       AND, when we DO implement load/save, then in that case the genesis-seeded State
    //       is just dropped in favor of whatever is in the file you loaded, so it never hurts
    //       to init State to whatever the production node or testcase thinks genesis state is.
    // NOTE: Now that SDKTestSuite is a subclass of Blockchain, so we would be modifying
    //       its state_, db_, etc. before returning it from this static factory method.

    // Create a genesis block with a timestamp of 1678887538000000 (2023-02-12 00:45:38 UTC)
    //uint64_t genesisTimestamp = 1678887538000000;
    //PrivKey genesisSigner(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8"));
    //FinalizedBlock genesis = FinalizedBlock::createNewValidBlock({}, {}, Hash(), genesisTimestamp, 0, genesisSigner);
    //std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
    //std::vector<std::pair<Address,uint256_t>> genesisBalances;
    // Add the chain owner account to the genesis balances.
    //const uint256_t desiredBalance("1000000000000000000000");
    //genesisBalances.emplace_back(chainOwnerAccount().address, desiredBalance);
    // Add the remaining accounts to the genesis balances.
    //for (const TestAccount& account : accounts) {
    //  genesisBalances.emplace_back(account.address, desiredBalance);
    //}
    //std::vector<Address> genesisValidators;
    //for (const auto& privKey : validatorPrivKeys()) {
    //  genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
    //}

    options_ = std::make_unique<Options>(
      sdkPath,
      "BDK/cpp/linux_x86-64/0.2.0",
      1,
      8080,
      Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
      //LOCALHOST,
      //SDKTestSuite::getTestPort(),
      SDKTestSuite::getTestPort(), //9999, // CHANGED: the HTTPPort (RPC port) needs to be unique as well
      //11,
      //11,
      //200,
      //50,
      2000,
      10000,
      1000,
      //4,
      //discoveryNodes,
      //genesis,
      //genesisTimestamp,
      //genesisSigner,
      //genesisBalances,
      //genesisValidators,
      IndexingMode::RPC_TRACE,
      defaultCometBFTOptions
    );
  } else {
    options_ = std::make_unique<Options>(*options);
  }
  return SDKTestSuite(*options_, instanceId);
}
