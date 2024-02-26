/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/templates/erc20wrapper.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/rdpos.h"

#include "../sdktestsuite.hpp"

#include <filesystem>
#include <string>

// TODO: test events if/when implemented

namespace TERC20Wrapper {
  TEST_CASE("ERC20Wrapper Class", "[contract][erc20wrapper]") {
    SECTION("ERC20Wrapper creation") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20Creation");
      Address erc20 = sdk.deployContract<ERC20>(
        std::string("TestToken"), std::string("TST"), uint8_t(18), uint256_t("1000000000000000000")
      );
      Address erc20Wrapper = sdk.deployContract<ERC20Wrapper>();
      Address owner = sdk.getChainOwnerAccount().address;
      for (const auto& [name, address] : sdk.getState().getContracts()) {
        if (name == "ERC20") REQUIRE(address == erc20);
        if (name == "ERC20Wrapper") REQUIRE(address == erc20Wrapper);
      }
    }

    SECTION("ERC20Wrapper deposit() and withdraw()") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20DepositAndWithdraw");
      Address erc20 = sdk.deployContract<ERC20>(
        std::string("TestToken"), std::string("TST"), uint8_t(18), uint256_t("1000000000000000000")
      );
      Address erc20Wrapper = sdk.deployContract<ERC20Wrapper>();
      Address owner = sdk.getChainOwnerAccount().address;
      for (const auto& [name, address] : sdk.getState().getContracts()) {
        if (name == "ERC20") REQUIRE(address == erc20);
        if (name == "ERC20Wrapper") REQUIRE(address == erc20Wrapper);
      }

      // Try depositing without allowance first
      uint256_t allowance = sdk.callViewFunction(erc20, &ERC20::allowance, owner, erc20Wrapper);
      REQUIRE(allowance == 0);  // "erc20Wrapper" is not allowed (yet) to spend anything on behalf of "owner"
      REQUIRE_THROWS(sdk.callFunction(erc20Wrapper, &ERC20Wrapper::deposit, erc20, uint256_t("500000000000000000")));
      REQUIRE(sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getUserBalance, erc20, owner) == 0);

      // Allow "erc20Wrapper" and make a deposit of 0.5 TST
      Hash approveTx = sdk.callFunction(erc20, &ERC20::approve, erc20Wrapper, uint256_t("500000000000000000"));
      allowance = sdk.callViewFunction(erc20, &ERC20::allowance, owner, erc20Wrapper);
      REQUIRE(allowance == uint256_t("500000000000000000"));  // "erc20Wrapper" is allowed to spend 0.5 TST on behalf of "owner"
      Hash depositTx = sdk.callFunction(erc20Wrapper, &ERC20Wrapper::deposit, erc20, uint256_t("500000000000000000"));
      uint256_t userBal = sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getUserBalance, erc20, owner);
      uint256_t contractBal = sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getContractBalance, erc20);
      uint256_t erc20Bal = sdk.callViewFunction(erc20, &ERC20::balanceOf, owner);
      uint256_t wrapperBal = sdk.callViewFunction(erc20, &ERC20::balanceOf, erc20Wrapper);
      REQUIRE(userBal == uint256_t("500000000000000000"));
      REQUIRE(contractBal == uint256_t("500000000000000000"));
      REQUIRE(erc20Bal == uint256_t("500000000000000000"));
      REQUIRE(wrapperBal == uint256_t("500000000000000000"));

      // Withdraw 0.25 TST
      Hash withdrawTx = sdk.callFunction(erc20Wrapper, &ERC20Wrapper::withdraw, erc20, uint256_t("250000000000000000"));
      userBal = sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getUserBalance, erc20, owner);
      contractBal = sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getContractBalance, erc20);
      erc20Bal = sdk.callViewFunction(erc20, &ERC20::balanceOf, owner);
      wrapperBal = sdk.callViewFunction(erc20, &ERC20::balanceOf, erc20Wrapper);
      REQUIRE(userBal == uint256_t("250000000000000000"));
      REQUIRE(contractBal == uint256_t("250000000000000000"));
      REQUIRE(erc20Bal == uint256_t("750000000000000000"));
      REQUIRE(wrapperBal == uint256_t("250000000000000000"));
    }

    SECTION("ERC20Wrapper transferTo()") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20TransferTo");
      Address erc20 = sdk.deployContract<ERC20>(
        std::string("TestToken"), std::string("TST"), uint8_t(18), uint256_t("1000000000000000000")
      );
      Address erc20Wrapper = sdk.deployContract<ERC20Wrapper>();
      Address owner = sdk.getChainOwnerAccount().address;
      Address dest(Utils::randBytes(20));
      for (const auto& [name, address] : sdk.getState().getContracts()) {
        if (name == "ERC20") REQUIRE(address == erc20);
        if (name == "ERC20Wrapper") REQUIRE(address == erc20Wrapper);
      }

      // Try depositing without allowance first
      uint256_t allowance = sdk.callViewFunction(erc20, &ERC20::allowance, owner, erc20Wrapper);
      REQUIRE(allowance == 0);  // "erc20Wrapper" is not allowed (yet) to spend anything on behalf of "owner"
      REQUIRE_THROWS(sdk.callFunction(erc20Wrapper, &ERC20Wrapper::deposit, erc20, uint256_t("500000000000000000")));
      REQUIRE(sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getUserBalance, erc20, owner) == 0);

      // Allow "erc20Wrapper" and make a deposit of 0.5 TST
      Hash approveTx = sdk.callFunction(erc20, &ERC20::approve, erc20Wrapper, uint256_t("500000000000000000"));
      allowance = sdk.callViewFunction(erc20, &ERC20::allowance, owner, erc20Wrapper);
      REQUIRE(allowance == uint256_t("500000000000000000"));  // "erc20Wrapper" is allowed to spend 0.5 TST on behalf of "owner"
      Hash depositTx = sdk.callFunction(erc20Wrapper, &ERC20Wrapper::deposit, erc20, uint256_t("500000000000000000"));
      uint256_t userBal = sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getUserBalance, erc20, owner);
      uint256_t contractBal = sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getContractBalance, erc20);
      uint256_t erc20Bal = sdk.callViewFunction(erc20, &ERC20::balanceOf, owner);
      uint256_t wrapperBal = sdk.callViewFunction(erc20, &ERC20::balanceOf, erc20Wrapper);
      uint256_t destBal = sdk.callViewFunction(erc20, &ERC20::balanceOf, dest);
      REQUIRE(userBal == uint256_t("500000000000000000"));
      REQUIRE(contractBal == uint256_t("500000000000000000"));
      REQUIRE(erc20Bal == uint256_t("500000000000000000"));
      REQUIRE(wrapperBal == uint256_t("500000000000000000"));
      REQUIRE(destBal == 0);

      // Transfer 0.25 TST to "dest"
      Hash transferTx = sdk.callFunction(erc20Wrapper, &ERC20Wrapper::transferTo, erc20, dest, uint256_t("250000000000000000"));
      userBal = sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getUserBalance, erc20, owner);
      contractBal = sdk.callViewFunction(erc20Wrapper, &ERC20Wrapper::getContractBalance, erc20);
      erc20Bal = sdk.callViewFunction(erc20, &ERC20::balanceOf, owner);
      wrapperBal = sdk.callViewFunction(erc20, &ERC20::balanceOf, erc20Wrapper);
      destBal = sdk.callViewFunction(erc20, &ERC20::balanceOf, dest);
      REQUIRE(userBal == uint256_t("250000000000000000"));
      REQUIRE(contractBal == uint256_t("250000000000000000"));
      REQUIRE(erc20Bal == uint256_t("500000000000000000"));
      REQUIRE(wrapperBal == uint256_t("250000000000000000"));
      REQUIRE(destBal == uint256_t("250000000000000000"));
    }
  }
}

