/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20.h"
#include "../../src/contract/templates/nativewrapper.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/core/storage.h"
#include "../../src/core/state.h"
#include "../../src/core/rdpos.h"

#include "../sdktestsuite.hpp"

// TODO: test events if/when implemented

namespace TNativeWrapper {
  TEST_CASE("NativeWrapper tests", "[contract][nativewrapper]") {
    SECTION("NativeWrapper creation") {
      SDKTestSuite sdk("testNativeWrapperCreation");
      Address nativeWrapper = sdk.deployContract<NativeWrapper>(
        std::string("WrappedToken"), std::string("WTKN"), uint8_t(18)
      );
      Address owner = sdk.getChainOwnerAccount().address;
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::name) == "WrappedToken");
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::symbol) == "WTKN");
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::decimals) == 18);
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::totalSupply) == 0);
    }

    SECTION("NativeWrapper deposit() and withdraw()") {
      SDKTestSuite sdk("testNativeWrapperDepositAndWithdraw");
      Address nativeWrapper = sdk.deployContract<NativeWrapper>(
        std::string("WrappedToken"), std::string("WTKN"), uint8_t(18)
      );
      Address owner = sdk.getChainOwnerAccount().address;
      uint256_t amountToTransfer = uint256_t("192838158112259");
      uint256_t amountToWithdraw = amountToTransfer / 3;
      Hash depositTx = sdk.callFunction(nativeWrapper, &NativeWrapper::deposit, amountToTransfer);
      REQUIRE(sdk.getNativeBalance(nativeWrapper) == amountToTransfer);
      // SDKTestSuite::createNewTx() is adding a tx fee of 21000 gwei, thus * 2 (or * 3 in the test below)
      REQUIRE(sdk.getNativeBalance(owner) == uint256_t("1000000000000000000000") - amountToTransfer - (uint256_t(1000000000) * 21000 * 2));
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::balanceOf, owner) == amountToTransfer);
      Hash withdrawTx = sdk.callFunction(nativeWrapper, &NativeWrapper::withdraw, amountToWithdraw);
      REQUIRE(sdk.getNativeBalance(nativeWrapper) == amountToTransfer - amountToWithdraw);
      REQUIRE(sdk.getNativeBalance(owner) == uint256_t("1000000000000000000000") - amountToTransfer + amountToWithdraw - (uint256_t(1000000000) * 21000 * 3));
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::balanceOf, owner) == amountToTransfer - amountToWithdraw);
    }
  }
}

