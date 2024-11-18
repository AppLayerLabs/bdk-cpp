/*
  Copyright (c) [2023-2024] [AppLayer Developers]

  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/utils/db.h"
#include "../../src/core/rdpos.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/options.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/contract/templates/orderbook/orderbook.h"
#include "../sdktestsuite.hpp"

#include <filesystem>

namespace TORDERBOOK {
std::string testDumpPath = Utils::getTestDumpPath();
TEST_CASE("OrderBook Class", "[contract][orderbook]") {
  SECTION("Orderbook creation") {
    SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(testDumpPath + "/testOrderBookCreation");
    REQUIRE(sdk.getState().getDumpManagerSize() == 3);
    Address AAddr = sdk.deployContract<ERC20>(std::string("A_Token"),
                                              std::string("TST"),
                                              uint8_t(18),
                                              uint256_t("1000000000000000000"));
    REQUIRE(sdk.getState().getDumpManagerSize() == 4);
    REQUIRE(sdk.callViewFunction(AAddr, &ERC20::name) == "A_Token");
    REQUIRE(sdk.callViewFunction(AAddr, &ERC20::decimals) > 8);

    Address BAddr = sdk.deployContract<ERC20>(std::string("B_Token"),
                                              std::string("TST"),
                                              uint8_t(18),
                                              uint256_t("1000000000000000000"));
    REQUIRE(sdk.getState().getDumpManagerSize() == 5);
    REQUIRE(sdk.callViewFunction(BAddr, &ERC20::name) == "B_Token");

    Address orderBook = sdk.deployContract<OrderBook>(AAddr,
                                                      std::string("A_Token"),
                                                      BAddr,
                                                      std::string("B_Token"));
    REQUIRE(sdk.getState().getDumpManagerSize() == 6);
  }

  SECTION("Orderbook add bit limit order") {
    SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(testDumpPath + "/testOrderBookCreation");
    Address AAddr = sdk.deployContract<ERC20>(std::string("A_Token"), std::string("TKN_A"), uint8_t(18), uint256_t("2000000000000000000"));
    Address BAddr = sdk.deployContract<ERC20>(std::string("B_Token"), std::string("TKN_B"), uint8_t(18), uint256_t("2000000000000000000"));
    Address orderBook = sdk.deployContract<OrderBook>(AAddr, std::string("A_Token"), BAddr, std::string("B_Token"));
    Address owner = sdk.getChainOwnerAccount().address;

    // why is being called twice?
    sdk.callFunction(orderBook, &OrderBook::setDecimals);

    // sdk.callFunction(AAddr, &ERC20::approve, orderBook, uint256_t("1000000000000000000"));
    // sdk.callFunction(BAddr, &ERC20::approve, orderBook, uint256_t("1000000000000000000"));

    // sdk.callFunction(AAddr, &ERC20::approve, BAddr, uint256_t("1000000000000000000"));
    // sdk.callFunction(BAddr, &ERC20::approve, AAddr, uint256_t("1000000000000000000"));

    //
    sdk.callFunction(AAddr, &ERC20::approve, orderBook, uint256_t("2000000000000000000"));
    sdk.callFunction(BAddr, &ERC20::approve, orderBook, uint256_t("2000000000000000000"));

    sdk.callFunction(orderBook, &OrderBook::addAskLimitOrder, uint256_t("100"), uint256_t("10"));
    sdk.callFunction(orderBook, &OrderBook::addBidLimitOrder, uint256_t("100"), uint256_t("10"));

    // sdk.callFunction(orderBook, &OrderBook::addAskLimitOrder, uint256_t("10000"), uint256_t("10"));
    // sdk.callFunction(orderBook, &OrderBook::addAskLimitOrder, uint256_t("10000"), uint256_t("10"));
    // sdk.callFunction(orderBook, &OrderBook::addAskLimitOrder, uint256_t("10000"), uint256_t("10"));
    // sdk.callFunction(orderBook, &OrderBook::addBidLimitOrder, uint256_t("10000"), uint256_t("10"));
  }
}
}
