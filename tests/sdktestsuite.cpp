#include "sdktestsuite.hpp"


#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/templates/simplecontract.h"

namespace TSDKTestSuite {
  TEST_CASE("SDK Test Suite", "[sdktestsuite]") {
    SECTION("SDK Test Suite Constructor") {
      Address destinationOfTransfer = Address(Utils::randBytes(20));
      SDKTestSuite sdkTestSuite("testSuitConstructor");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
    }

    SECTION("SDK Test Suite advanceChain") {
      SDKTestSuite sdkTestSuite("testSuiteAdvanceChain");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
      sdkTestSuite.advanceChain();
      latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 1);
    }

    SECTION("SDK Test Suite Simple Transfer") {
      Address destinationOfTransfer = Address(Utils::randBytes(20));
      SDKTestSuite sdkTestSuite("testSuiteSimpleTransfer");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
      sdkTestSuite.transfer(sdkTestSuite.getChainOwnerAccount(), destinationOfTransfer, 1000000000000000000);
      latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 1);
      REQUIRE(latestBlock->getTxs().size() == 1);
      REQUIRE(sdkTestSuite.getNativeBalance(destinationOfTransfer) == 1000000000000000000);
      REQUIRE(sdkTestSuite.getNativeBalance(sdkTestSuite.getChainOwnerAccount().address) < uint256_t("999000000000000000000")); // 1000 - 1 - fees
    }

    SECTION("SDK Test Suite Deploy Contract") {
      SDKTestSuite sdkTestSuite("testSuiteDeployContract");
      auto latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 0); // Genesis
      Address newContract = sdkTestSuite.deployContract<ERC20>(std::string("ERC20"), std::string("ERC20"), uint8_t(18), uint256_t("1000000000000000000"));
      latestBlock = sdkTestSuite.getLatestBlock();
      REQUIRE(latestBlock->getNHeight() == 1);
      REQUIRE(latestBlock->getTxs().size() == 1);
      REQUIRE(newContract != Address());
    }

    SECTION("SDK Test Suite Deploy and Call Contract")
    {
      Address destinationOfTransfer = Address(Utils::randBytes(20));
      SDKTestSuite sdkTestSuite("testSuiteDeployAndCall");
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

    SECTION("SDK Test Suite getEvents") {
      SDKTestSuite sdkTestSuite("testSuiteGetEvents");
      auto simpleContractAddress = sdkTestSuite.deployContract<SimpleContract>(std::string("Hello World!"), uint256_t(10));

      auto changeNameAndValueTx = sdkTestSuite.callFunction(simpleContractAddress, &SimpleContract::setName, std::string("Hello World 2!"));

      auto events = sdkTestSuite.getEventsEmittedByTx(changeNameAndValueTx, &SimpleContract::nameChanged);
      REQUIRE(events.size() == 1);

      auto filteredEvents = sdkTestSuite.getEventsEmittedByTx(changeNameAndValueTx, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("Hello World 2!")));
      REQUIRE(filteredEvents.size() == 1);

      auto filteredEvents2 = sdkTestSuite.getEventsEmittedByTx(changeNameAndValueTx, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("Hello World 3!")));
      REQUIRE(filteredEvents2.size() == 0);

    }
  }
}