/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "sdktestsuite.hpp"

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/templates/simplecontract.h"
#include <tuple>

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

    SECTION("SDK Test Suite Deploy Contract") {
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteDeployContract");
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
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteDeployAndCall");
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
      SDKTestSuite sdkTestSuite = SDKTestSuite::createNewEnvironment("testSuiteGetEvents");
      auto simpleContractAddress = sdkTestSuite.deployContract<SimpleContract>(
        std::string("Hello World!"), uint256_t(10), std::make_tuple(std::string("From Inside"), uint256_t(5000))
      );
      auto changeNameAndNumberTx = sdkTestSuite.callFunction(simpleContractAddress, &SimpleContract::setName, std::string("Hello World 2!"));
      auto events = sdkTestSuite.getEventsEmittedByTx(changeNameAndNumberTx, &SimpleContract::nameChanged);
      REQUIRE(events.size() == 1);
      auto filteredEvents = sdkTestSuite.getEventsEmittedByTx(
        changeNameAndNumberTx, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("Hello World 2!"))
      );
      REQUIRE(filteredEvents.size() == 1);
      auto filteredEvents2 = sdkTestSuite.getEventsEmittedByTx(
        changeNameAndNumberTx, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("Hello World 3!"))
      );
      REQUIRE(filteredEvents2.size() == 0);
      auto filteredEvents3 = sdkTestSuite.getEventsEmittedByAddress(
        simpleContractAddress, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("Hello World 2!"))
      );
      REQUIRE(filteredEvents3.size() == 1);

      auto changeNumberTx = sdkTestSuite.callFunction(simpleContractAddress, &SimpleContract::setNumber, uint256_t(20));
      auto tupleVec = sdkTestSuite.getEventsEmittedByTxTup(changeNumberTx, &SimpleContract::numberChanged);
      REQUIRE(tupleVec.size() == 1);
      for (auto& tuple : tupleVec) REQUIRE(std::get<0>(tuple) == uint256_t(20));

      auto changeTupleTx = sdkTestSuite.callFunction(simpleContractAddress, &SimpleContract::setTuple,
        std::make_tuple(std::string("Now Outside"), uint256_t(10000))
      );
      std::vector<Event> tupleRet = sdkTestSuite.getEventsEmittedByTx(changeTupleTx, &SimpleContract::tupleChanged);
      REQUIRE(tupleRet.size() == 1);
    }
  }
}


