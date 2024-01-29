/*
Copyright (c) [2023] [Sparq Network]

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
        std::string("TestName"), uint256_t(19283187581)
      );
      std::string name = sdk.callViewFunction(simpleContract, &SimpleContract::getName);
      uint256_t value = sdk.callViewFunction(simpleContract, &SimpleContract::getValue);
      REQUIRE(name == "TestName");
      REQUIRE(value == 19283187581);
    }

    SECTION("SimpleContract setName and setValue") {
      SDKTestSuite sdk("testSimpleContractSetNameAndSetValue");
      Address simpleContract = sdk.deployContract<SimpleContract>(
        std::string("TestName"), uint256_t(19283187581)
      );

      std::string name = sdk.callViewFunction(simpleContract, &SimpleContract::getName);
      uint256_t value = sdk.callViewFunction(simpleContract, &SimpleContract::getValue);
      REQUIRE(name == "TestName");
      REQUIRE(value == 19283187581);

      Hash nameTx = sdk.callFunction(simpleContract, &SimpleContract::setName, std::string("TryThisName"));
      Hash valueTx = sdk.callFunction(simpleContract, &SimpleContract::setValue, uint256_t("918258172319061203818967178162134821351"));
      name = sdk.callViewFunction(simpleContract, &SimpleContract::getName);
      value = sdk.callViewFunction(simpleContract, &SimpleContract::getValue);
      REQUIRE(name == "TryThisName");
      REQUIRE(value == uint256_t("918258172319061203818967178162134821351"));

      auto nameEvent = sdk.getEventsEmittedByTx(nameTx, &SimpleContract::nameChanged, std::make_tuple(EventParam<std::string, true>("TryThisName")));
      auto valueEvent = sdk.getEventsEmittedByTx(valueTx, &SimpleContract::valueChanged, std::make_tuple(EventParam<uint256_t, false>(uint256_t("918258172319061203818967178162134821351"))));
      REQUIRE(nameEvent.size() == 1);
      REQUIRE(valueEvent.size() == 1);

      // TODO: maybe test the rest of the events?
    }
  }
}

