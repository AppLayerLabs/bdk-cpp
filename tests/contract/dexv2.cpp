/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/contractmanager.h"
#include "../../src/contract/templates/dexv2/dexv2pair.h"
#include "../../src/contract/templates/dexv2/dexv2factory.h"
#include "../../src/contract/templates/dexv2/dexv2router02.h"
#include "../../src/contract/templates/nativewrapper.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/core/rdpos.h"
#include "../../src/core/state.h"

#include "../sdktestsuite.hpp"

#include <utility>

// TODO: test events if/when implemented

namespace TDEXV2 {
  TEST_CASE("DEXV2 Test", "[contract][dexv2]") {
    SECTION("Deploy DEXV2Router/Factory with a single pair") {
      SDKTestSuite sdk("testDEXV2SinglePair");
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;
      for (const auto& contract : sdk.getState()->getContracts()) {
        if (contract.first == "NativeWrapper") REQUIRE(contract.second == wrapped);
        if (contract.first == "DEXV2Factory")  REQUIRE(contract.second == factory);
        if (contract.first == "DEXV2Router02") REQUIRE(contract.second == router);
      }
    }

    SECTION("Deploy DEXV2 and add liquidity to token/token pair") {
      SDKTestSuite sdk("testDEXV2LiqTokenTokenPair");
      Address tokenA = sdk.deployContract<ERC20>(std::string("TokenA"), std::string("TKNA"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address tokenB = sdk.deployContract<ERC20>(std::string("TokenB"), std::string("TKNB"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;
      for (const auto& contract : sdk.getState()->getContracts()) {
        if (contract.first == "NativeWrapper") REQUIRE(contract.second == wrapped);
        if (contract.first == "DEXV2Factory")  REQUIRE(contract.second == factory);
        if (contract.first == "DEXV2Router02") REQUIRE(contract.second == router);
      }

      // Approve "router" so it can spend up to 10000 tokens from both sides
      // on behalf of "owner" (which already has the tokens)
      Hash approveATx = sdk.callFunction(tokenA, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      Hash approveBTx = sdk.callFunction(tokenB, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenA, &ERC20::allowance, owner, router) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenB, &ERC20::allowance, owner, router) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenB, &ERC20::balanceOf, owner) == uint256_t("10000000000000000000000"));

      // Add liquidity of 100 from A and 250 from B
      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;  // 60 seconds
      // tokenA, tokenB, amountADesired, amountBDesired, amountAMin, amountBMin, to, deadline
      Hash addLiqudityTx = sdk.callFunction(router, &DEXV2Router02::addLiquidity,
        tokenA, tokenB, uint256_t("100000000000000000000"), uint256_t("250000000000000000000"),
        uint256_t(0), uint256_t(0), owner, deadline
      );

      // Check if operation worked sucessfully
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, tokenB);
      uint256_t ownerTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner);
      uint256_t ownerTknB = sdk.callViewFunction(tokenB, &ERC20::balanceOf, owner);
      uint256_t pairTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, pair);
      uint256_t pairTknB = sdk.callViewFunction(tokenB, &ERC20::balanceOf, pair);
      REQUIRE(ownerTknA == uint256_t("9900000000000000000000"));
      REQUIRE(ownerTknB == uint256_t("9750000000000000000000"));
      REQUIRE(pairTknA == uint256_t("100000000000000000000"));
      REQUIRE(pairTknB == uint256_t("250000000000000000000"));
    }

    SECTION("Deploy DEXV2 and add liquidity to token/native pair") {
      SDKTestSuite sdk("testDEXV2LiqTokenNativePair");
      Address tokenA = sdk.deployContract<ERC20>(std::string("TokenA"), std::string("TKNA"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;
      for (const auto& contract : sdk.getState()->getContracts()) {
        if (contract.first == "NativeWrapper") REQUIRE(contract.second == wrapped);
        if (contract.first == "DEXV2Factory")  REQUIRE(contract.second == factory);
        if (contract.first == "DEXV2Router02") REQUIRE(contract.second == router);
      }

      // Approve "router" so it can spend up to 10000 TKNA on behalf of "owner"
      Hash approveATx = sdk.callFunction(tokenA, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenA, &ERC20::allowance, owner, router) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner) == uint256_t("10000000000000000000000"));

      uint256_t ownerNativeBeforeAddLiq = sdk.getNativeBalance(owner);
      // Add liquidity of 100 WSPARQ and 100 TKNA
      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;  // 60 seconds
      // token, amountTokenDesired, amountTokenMin, amountNativeMin, to, deadline
      Hash addLiqudityTx = sdk.callFunction(
        router, uint256_t("100000000000000000000"), &DEXV2Router02::addLiquidityNative,
        tokenA, uint256_t("100000000000000000000"), uint256_t("100000000000000000000"),
        uint256_t("100000000000000000000"), owner, deadline
      );

      // Check if operation worked successfully
      // SDKTestSuite::createNewTx() is adding a tx fee of 21000 gwei, thus * 2
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, wrapped);
      uint256_t ownerTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner);
      uint256_t ownerNative = sdk.getNativeBalance(owner);
      uint256_t pairTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, pair);
      uint256_t pairNative = sdk.getNativeBalance(wrapped);
      REQUIRE(ownerTknA == uint256_t("9900000000000000000000"));
      REQUIRE(ownerNative == ownerNativeBeforeAddLiq - uint256_t("100000000000000000000") - (uint256_t(1000000000) * 21000));
      REQUIRE(pairTknA == uint256_t("100000000000000000000"));
      REQUIRE(pairNative == uint256_t("100000000000000000000"));
    }
  }
}

