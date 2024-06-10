/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "sdktestsuite.hpp"

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/templates/simplecontract.h"
#include "../../src/contract/templates/testThrowVars.h"
#include "../../src/utils/clargs.h"
#include <tuple>

// Initialize static listen port generator parameters
int SDKTestSuite::p2pListenPortMin_ = 20000;
int SDKTestSuite::p2pListenPortMax_ = 29999;
int SDKTestSuite::p2pListenPortGen_ = SDKTestSuite::p2pListenPortMin_;

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
  if (opt.netThreads == -1) {
    // The default P2P IO worker thread count for unit tests is 1, which facilitates debugging of
    //   networked unit tests with multiple P2P engines in the same process, and reduces threading
    //   overhead on the test machine.
    opt.netThreads = 1;
  }
  if (!applyProcessOptions(opt)) return 1;

  // Avoid ManagerBase::instanceIdGen_ == 0, which produces log logical location string ""
  //   (for production nodes that only instantiate one ManagerBase, ever, and don't need
  //    the logical location consuming space in the log file)
  P2P::ManagerBase::setTesting();

  Utils::safePrintTest("Running Catch2...");
  int result = Catch::Session().run(catchArgs.size(), catchArgs.data());
  return result;
}

namespace TSDKTestSuite {
  TEST_CASE("SDK Test Suite", "[sdktestsuite]") {
    SECTION("SDK Test Suite Constructor") {
      Address destinationOfTransfer = Address(Utils::randBytes(20));
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuitConstructor");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
    }

    SECTION("SDK Test Suite advanceChain") {
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteAdvanceChain");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
      sdkTestSuite.advanceChain();
      latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 1);
    }

    SECTION("SDK Test Suite Simple Transfer") {
      Address destinationOfTransfer = Address(Utils::randBytes(20));
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteSimpleTransfer");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
      sdkTestSuite.transfer(sdkTestSuite.getChainOwnerAccount(), destinationOfTransfer, 1000000000000000000);
      latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 1);
      REQUIRE(latestBlock->getTxs().size() == 1);
      REQUIRE(sdkTestSuite.getNativeBalance(destinationOfTransfer) == 1000000000000000000);
      REQUIRE(sdkTestSuite.getNativeBalance(sdkTestSuite.getChainOwnerAccount().address) < uint256_t("999000000000000000000")); // 1000 - 1 - fees
    }

    SECTION("SDK Test Suite Deploy Throwing Contract") {
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteDeployThrowingContract");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
      REQUIRE_THROWS(sdkTestSuite.deployContract<TestThrowVars>(std::string("var1"), std::string("var2"), std::string("var3")));
    }

    SECTION("SDK Test Suite Deploy ERC20 Contract") {
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteDeployERC20Contract");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
      Address newContract = sdkTestSuite.deployContract<ERC20>(std::string("ERC20"), std::string("ERC20"), uint8_t(18), uint256_t("1000000000000000000"));
      latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 1);
      REQUIRE(latestBlock->getTxs().size() == 1);
      REQUIRE(newContract != Address());
    }

    SECTION("SDK Test Suite Deploy and Call ERC20 Contract") {
      Address destinationOfTransfer = Address(Utils::randBytes(20));
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteDeployAndCallERC20Contract");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
      Address newContract = sdkTestSuite.deployContract<ERC20>(std::string("ERC20"), std::string("ERC20"), uint8_t(18), uint256_t("1000000000000000000"));
      latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 1);
      REQUIRE(latestBlock->getTxs().size() == 1);
      REQUIRE(newContract != Address());
      REQUIRE(sdkTestSuite.callViewFunction(newContract, &ERC20::balanceOf, sdkTestSuite.getChainOwnerAccount().address) == uint256_t("1000000000000000000"));
      Hash transferTx = sdkTestSuite.callFunction(newContract, &ERC20::transfer, destinationOfTransfer, uint256_t("10000000000000000"));
      latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 2);
      REQUIRE(latestBlock->getTxs().size() == 1);
      REQUIRE(sdkTestSuite.callViewFunction(newContract, &ERC20::balanceOf, destinationOfTransfer) == uint256_t("10000000000000000"));
      REQUIRE(sdkTestSuite.callViewFunction(newContract, &ERC20::balanceOf, sdkTestSuite.getChainOwnerAccount().address) == uint256_t("990000000000000000"));
    }

    SECTION("SDK Test Suite SimpleContract Get Events") {
      printf("1\n");
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteSimpleContractGetEvents");
      auto simpleContractAddress = sdkTestSuite.deployContract<SimpleContract>(
        std::string("Hello World!"), uint256_t(10), std::make_tuple(std::string("From Inside"), uint256_t(5000))
      );
      printf("2\n");
      auto changeNameTx = sdkTestSuite.callFunction(simpleContractAddress, &SimpleContract::setName, std::string("Hello World 2!"));
      printf("2.5\n");
      auto events = sdkTestSuite.getEventsEmittedByTx(changeNameTx, &SimpleContract::nameChanged);
      REQUIRE(events.size() == 1);
      printf("3\n");
      auto filteredEvents = sdkTestSuite.getEventsEmittedByTx(
        changeNameTx, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("Hello World 2!"))
      );
      REQUIRE(filteredEvents.size() == 1);
      printf("4\n");
      auto filteredEvents2 = sdkTestSuite.getEventsEmittedByTx(
        changeNameTx, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("Hello World 3!"))
      );
      REQUIRE(filteredEvents2.size() == 0);
      printf("5\n");
      auto filteredEvents3 = sdkTestSuite.getEventsEmittedByAddress(
        simpleContractAddress, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("Hello World 2!"))
      );
      REQUIRE(filteredEvents3.size() == 1);
      printf("6\n");

      auto changeNumberTx = sdkTestSuite.callFunction(simpleContractAddress, &SimpleContract::setNumber, uint256_t(20));
      auto tupleVec = sdkTestSuite.getEventsEmittedByTxTup(changeNumberTx, &SimpleContract::numberChanged);
      REQUIRE(tupleVec.size() == 1);
      printf("7\n");
      for (auto& tuple : tupleVec) REQUIRE(std::get<0>(tuple) == uint256_t(20));

      auto changeTupleTx = sdkTestSuite.callFunction(simpleContractAddress, &SimpleContract::setTuple,
        std::make_tuple(std::string("Now Outside"), uint256_t(10000))
      );
      std::vector<Event> tupleRet = sdkTestSuite.getEventsEmittedByTx(changeTupleTx, &SimpleContract::tupleChanged);
      REQUIRE(tupleRet.size() == 1);
      printf("8\n");
    }
  }
}


