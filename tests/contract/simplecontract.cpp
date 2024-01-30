/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/core/rdpos.h"
#include "../../src/contract/abi.h"
#include "../../src/contract/contractmanager.h"
#include "../../src/contract/templates/simplecontract.h"

#include "../sdktestsuite.hpp"

#include <filesystem>

namespace TSimpleContract {
  TEST_CASE("SimpleContract class", "[contract][simplecontract]") {
    SECTION("SimpleContract creation") {
      SDKTestSuite sdk("testSimpleContractCreation");
      Address simpleContract = sdk.deployContract<SimpleContract>(
        std::string("TestName"), uint256_t(19283187581),
        std::make_tuple(std::string("TupleName"), uint256_t(987654321))
      );
      std::string name = sdk.callViewFunction(simpleContract, &SimpleContract::getName);
      uint256_t value = sdk.callViewFunction(simpleContract, &SimpleContract::getValue);
      std::tuple<std::string, uint256_t> tuple = sdk.callViewFunction(simpleContract, &SimpleContract::getTuple);
      REQUIRE(name == "TestName");
      REQUIRE(value == 19283187581);
      REQUIRE(std::get<0>(tuple) == "TupleName");
      REQUIRE(std::get<1>(tuple) == 987654321);
    }

    SECTION("SimpleContract setName, setValue and setTuple") {
      SDKTestSuite sdk("testSimpleContractSetNameValueAndTuple");
      Address simpleContract = sdk.deployContract<SimpleContract>(
        std::string("TestName"), uint256_t(19283187581),
        std::make_tuple(std::string("TupleName"), uint256_t(987654321))
      );

      std::string name = sdk.callViewFunction(simpleContract, &SimpleContract::getName);
      uint256_t value = sdk.callViewFunction(simpleContract, &SimpleContract::getValue);
      std::tuple<std::string, uint256_t> tuple = sdk.callViewFunction(simpleContract, &SimpleContract::getTuple);
      REQUIRE(name == "TestName");
      REQUIRE(value == 19283187581);
      REQUIRE(std::get<0>(tuple) == "TupleName");
      REQUIRE(std::get<1>(tuple) == 987654321);

      Hash nameTx = sdk.callFunction(simpleContract, &SimpleContract::setName, std::string("TryThisName"));
      Hash valueTx = sdk.callFunction(simpleContract, &SimpleContract::setValue, uint256_t("918258172319061203818967178162134821351"));
      Hash tupleTx = sdk.callFunction(simpleContract, &SimpleContract::setTuple, std::make_tuple(std::string("AnotherName"), uint256_t(999999999)));
      name = sdk.callViewFunction(simpleContract, &SimpleContract::getName);
      value = sdk.callViewFunction(simpleContract, &SimpleContract::getValue);
      tuple = sdk.callViewFunction(simpleContract, &SimpleContract::getTuple);
      REQUIRE(name == "TryThisName");
      REQUIRE(value == uint256_t("918258172319061203818967178162134821351"));
      REQUIRE(std::get<0>(tuple) == "AnotherName");
      REQUIRE(std::get<1>(tuple) == 999999999);

      auto nameEvent = sdk.getEventsEmittedByTx(nameTx, &SimpleContract::nameChanged,
        std::make_tuple(EventParam<std::string, true>("TryThisName"))
      );
      auto valueEvent = sdk.getEventsEmittedByTx(valueTx, &SimpleContract::valueChanged,
        std::make_tuple(EventParam<uint256_t, false>(uint256_t("918258172319061203818967178162134821351")))
      );
      auto tupleEvent = sdk.getEventsEmittedByTx(tupleTx, &SimpleContract::tupleChanged,
        std::make_tuple(EventParam<std::tuple<std::string, uint256_t>, true>(std::make_tuple("AnotherName", uint256_t(999999999))))
      );
      REQUIRE(nameEvent.size() == 1);
      REQUIRE(valueEvent.size() == 1);
      REQUIRE(tupleEvent.size() == 1);
    }
  }
}

