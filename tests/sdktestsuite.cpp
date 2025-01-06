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

/**
 * TODO (review tests):
 *
 * The assumption behind deployContract(), and probably other bits of unit testing functionality,
 * no longer holds, as cometbft does not really give us an "advanceChain()" RPC call -- there's no
 * trivial implementation of "advanceChain()" to be made.
 *
 * Instead, deployContract() will have to contend with the possibility of block advancement involving
 * multiple blocks. That is, it cannot assume that one block is produced. It will have to use the
 * Comet interface to send a deploy-contract transaction, then synchronously wait for it to be
 * accepted, and only then return from here with the address of the successful deployment.
 *
 * using advancechain to test revert means instead looking at a call transaction and cheching its
 * status afterwards, ultimately (can also run the tx blob and metadata(sig, gas payer balance + gas limit) past
 * basic transaction checking first etc).
 */

// Getter for `chainOwnerAccount_`.
TestAccount SDKTestSuite::getChainOwnerAccount() const {
  return this->chainOwnerAccount();
};

// Get the native balance of a given address.
const uint256_t SDKTestSuite::getNativeBalance(const Address& address) const {
  return this->state_.getNativeBalance(address);
}

// Get the nonce of a given address.
const uint64_t SDKTestSuite::getNativeNonce(const Address& address) const {
  return this->state_.getNativeNonce(address);
}

int64_t SDKTestSuite::estimateGas(const evmc_message& callInfo) {
  return this->state_.estimateGas(callInfo);
}

// Get a block by its hash.
//const std::shared_ptr<const CometBlock> SDKTestSuite::getBlock(const Hash& hash) const {

  // easiest implementation:
  // - fetch block from cometbft store via RPC
  // - use the info we got from cometbft to build a CometBlock object
  //   (the old FinalizedBlock type will cease to exist.. maybe.. probably..) and then return it
  //
  // The goal is that for everything that defines what a block is inside of BDK, we can get it
  // already from cometbft, or we can compute locally after we get the base info from cometbft
  // (and then cache that computation inside e.g. the CometBlock struct, and then cache the
  // CometBlock object itself).

  //return this->storage_.getBlock(hash);
 // return {}; // FIXME/TODO: since it's for testing, this will first get this with a blocking RPC call to cometbft to fetch the block, later, can be cached locally as well
//}

// Get a block by its height.
//const std::shared_ptr<const CometBlock> SDKTestSuite::getBlock(const uint64_t height) const {
  //return this->storage_.getBlock(height);
//  return {}; // FIXME/TODO: since it's for testing, this will first get this with a blocking RPC call to cometbft to fetch the block, later, can be cached locally as well
//}

// Compatibility method for tests that want to advance a cometbft blockchain.
void SDKTestSuite::advanceChain(std::vector<TxBlock>&& txs) {
  // Save the current SDKTestSuite-tracked height which is updated by incomingBlock().
  std::unique_lock<std::mutex> lock(advanceChainMutex_);
  uint64_t startingHeight = advanceChainHeight_;
  lock.unlock();

  // Send all transactions (if any)
  for (const auto& tx : txs) {
    // Validate it first
    if (!state_.validateTransaction(tx)) {
      throw DynamicException("Transaction " + tx.hash().hex().get() + " is invalid");
    }
    // Serialize it
    Bytes txBytes = tx.rlpSerialize();
    // Send it.
    //std::shared_ptr<Hash> ethHash = std::make_shared<Hash>();
    uint64_t tId = comet_.sendTransaction(txBytes/*, &ethHash*/);
    // Take the sha3 of the serialized tx and add it to the set of
    // expected transactions.
    Hash txHash = Utils::sha3(txBytes);
    lock.lock();
    advanceChainPendingTxs_.insert(txHash);
    lock.unlock();
  }

  // Blocking/sync wait until the chain advanced by at least one block and all transactions
  // we sent above are included in blocks we have already received.
  // (height check is so that this method does something even with an empty txs arg)
  while (true) {
    lock.lock();
    if (advanceChainPendingTxs_.empty() && advanceChainHeight_ > startingHeight) {
      break;
    }
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

// Initialize all components of a full blockchain node.
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
        "genesis.json":
        {
          "genesis_time": "2024-09-17T18:26:34.583377166Z",
          "chain_id": "",
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

        "priv_validator_key.json":
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
        },

        "node_key.json": {
          "priv_key": {
            "type": "tendermint/PrivKeyEd25519",
            "value": "DJZS1+kjt1kICsxkgfKuFaBW3OYeefr75gpy1jeTZfsd6MIwWjUKJClUnfC7XZCUApoZ4GpksvGyku5aXdQeAg=="
          }
        },

        "config.toml": {}
      }
    )");

    // P2P/RPC parameters are required, generate them
    int p2pPort = SDKTestSuite::getTestPort();
    int rpcPort = SDKTestSuite::getTestPort();
    defaultCometBFTOptions["config.toml"]["p2p"] = {
      {"laddr", "tcp://0.0.0.0:" + std::to_string(p2pPort)},
      {"allow_duplicate_ip", true},
      {"addr_book_strict", false}
    };
    defaultCometBFTOptions["config.toml"]["rpc"] = {
      {"laddr", "tcp://0.0.0.0:" + std::to_string(rpcPort)},
    };

    // The Chain ID parameter is actually controlled by the Options object, and it is an uint64_t.
    // CometBFT expects a string for it, so we simply set it to the decimal string conversion of
    // the uint64_t value.
    // This is one of the few exceptions where the cometbft genesis file value is discarded and
    // overriden by a value from Options. In general we don't want duplication.
    defaultCometBFTOptions["genesis"]["chain_id"] = std::to_string(DEFAULT_UINT64_TEST_CHAIN_ID);

    // Genesis block and validators is entirely on the comet side (comet genesis file)

    // Discovery nodes will be implemented using cometbft seed-nodes

    // TODO: accounts, contracts, etc. is to be injected directly into the State here since
    //       the State IS genesis state by definition while we don't do load/save from disk.
    //       AND, when we DO implement load/save, then in that case the genesis-seeded State
    //       is just dropped in favor of whatever is in the file you loaded, so it never hurts
    //       to init State to whatever the production node or testcase thinks genesis state is.
    // NOTE: Now that SDKTestSuite is a subclass of Blockchain, so we would be modifying
    //       its state_, db_, etc. before returning it from this static factory method.

    options_ = std::make_unique<Options>(
      sdkPath,
      "BDK/cpp/linux_x86-64/0.2.0",
      1,
      DEFAULT_UINT64_TEST_CHAIN_ID,
      Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
      SDKTestSuite::getTestPort(), // CHANGED: the HTTPPort (RPC port) needs to be unique as well
      2000,
      10000,
      1000,
      IndexingMode::RPC_TRACE,
      defaultCometBFTOptions
    );
  } else {
    options_ = std::make_unique<Options>(*options);
  }

  // Defer initial testing deposits to the SDKTestSuite ctor
  return SDKTestSuite(*options_, instanceId, accounts);
}

// Create a new TxBlock object based on the provided account and given the current state (for nonce).
TxBlock SDKTestSuite::createNewTx(
  const TestAccount& from, const Address& to, const uint256_t& value, Bytes data
) {
  evmc_message callInfo;
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
    callCodeAddress] = callInfo;

  callKind = (to == Address()) ? EVMC_CREATE : EVMC_CALL;
  callFlags = 0;
  callDepth = 1;
  callGas = 1000000000;
  callRecipient = to.toEvmcAddress();
  callSender = from.address.toEvmcAddress();
  callInputData = data.data();
  callInputSize = data.size();
  callValue = EVMCConv::uint256ToEvmcUint256(value);
  callCreate2Salt = {};
  callCodeAddress = {};
  auto usedGas = this->estimateGas(callInfo);
  usedGas += 10000; // Add some extra gas for the transaction itself
  /// Estimate the gas to see how much gaslimit we should give to the tx itself
  return TxBlock(to, from.address, data, this->options_.getChainID(),
    this->getNativeNonce(from.address),
    value,
    1000000000,
    1000000000,
    usedGas,
    from.privKey
  );
}

// ------------------------------------------------------------------
// CometListener
// Here we trap the CometListener calls to implement test suite behavior
//   and then we just forward them to the parent class (Blockchain)
//   to resume regular processing.
// ------------------------------------------------------------------

void SDKTestSuite::initChain(
  const uint64_t genesisTime, const std::string& chainId, const Bytes& initialAppState, const uint64_t initialHeight,
  const std::vector<CometValidatorUpdate>& initialValidators, Bytes& appHash
) {
  Blockchain::initChain(genesisTime, chainId, initialAppState, initialHeight, initialValidators, appHash);
}

void SDKTestSuite::checkTx(const Bytes& tx, int64_t& gasWanted, bool& accept)
{
  Blockchain::checkTx(tx, gasWanted, accept);
}

void SDKTestSuite::incomingBlock(
  const uint64_t syncingToHeight, std::unique_ptr<CometBlock> block, Bytes& appHash,
  std::vector<CometExecTxResult>& txResults, std::vector<CometValidatorUpdate>& validatorUpdates
) {
  // We have to std::move(block) before we update advanceChainPendingTxs_, so compute the tx hashes
  // first and save them for use after Blockchain::incomingBlock() (that is, processing the block).
  std::vector<Hash> txHashes;
  for (const auto& tx : block->txs) {
    txHashes.emplace_back(Utils::sha3(tx));
  }
  uint64_t blockHeight = block->height;

  // First, forward the block to the Blockchain so the txs will modify the State, compute appHash, etc.
  Blockchain::incomingBlock(syncingToHeight, std::move(block), appHash, txResults, validatorUpdates);

  // *After* the block has been processed into the state:
  // Trapping incomingBlock here allows the test suite to figure out what transactions have just been
  // applied to the state machine (in SDKTestSuite::advanceChain(), which is a blocking call).
  std::unique_lock<std::mutex> lock(advanceChainMutex_);
  advanceChainHeight_ = blockHeight;
  for (const auto& txHash : txHashes) {
    advanceChainPendingTxs_.erase(txHash); // In case this tx was pending, then it is no longer pending.
  }
  lock.unlock();
}

void SDKTestSuite::buildBlockProposal(
  const uint64_t maxTxBytes, const CometBlock& block, bool& noChange, std::vector<size_t>& txIds
) {
  Blockchain::buildBlockProposal(maxTxBytes, block, noChange, txIds);
}

void SDKTestSuite::validateBlockProposal(const CometBlock& block, bool& accept) {
  Blockchain::validateBlockProposal(block, accept);
}

void SDKTestSuite::getCurrentState(uint64_t& height, Bytes& appHash, std::string& appSemVer, uint64_t& appVersion) {
  Blockchain::getCurrentState(height, appHash, appSemVer, appVersion);
}

void SDKTestSuite::getBlockRetainHeight(uint64_t& height) {
  Blockchain::getBlockRetainHeight(height);
}

void SDKTestSuite::currentCometBFTHeight(const uint64_t height) {
  Blockchain::currentCometBFTHeight(height);
}

void SDKTestSuite::sendTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash, const Bytes& tx) {
  Blockchain::sendTransactionResult(tId, success, response, txHash, tx);
}

void SDKTestSuite::checkTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash) {
  Blockchain::checkTransactionResult(tId, success, response, txHash);
}

void SDKTestSuite::rpcAsyncCallResult(const uint64_t tId, const bool success, const json& response, const std::string& method, const json& params) {
  Blockchain::rpcAsyncCallResult(tId, success, response, method, params);
}

void SDKTestSuite::cometStateTransition(const CometState newState, const CometState oldState) {
  Blockchain::cometStateTransition(newState, oldState);
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------







