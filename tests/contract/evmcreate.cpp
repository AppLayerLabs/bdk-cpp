/*
Copyright (c) [2023-2024] [AppLayer Developers]
  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#include "../sdktestsuite.hpp"
#include "../../src/libs/catch2/catch_amalgamated.hpp"

/**
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

// First contract: SimpleContract with a constant and a getter function.
contract SimpleContract {
uint256 public constant X = 10;

// Returns the constant value X.
function getNumber() public pure returns (uint256) {
return X;
}
}

// Second contract: Factory contract with two deployment methods.
contract Factory {
event Deployed(address addr);

// createNormal() uses the legacy method to deploy a new instance.
function createNormal() external returns (address) {
SimpleContract instance = new SimpleContract();
emit Deployed(address(instance));
return address(instance);
}

// createCreate2() uses the new create2 method with a fixed salt.
function createCreate2() external returns (address) {
// Fixed salt for demonstration purposes.
bytes32 salt = bytes32(0);

// Deploy using create2. Note: Solidity 0.8.x allows using {salt: salt} syntax.
SimpleContract instance = new SimpleContract{salt: salt}();
emit Deployed(address(instance));
return address(instance);
}
}
*/


namespace TEVMCreate {
  class SolFactory {
    public:
      Address createNormal() { return Address(); }
      Address createCreate2() { return Address(); }
      void static registerContract() {
        ContractReflectionInterface::registerContractMethods<
          SolFactory
        >(
          std::vector<std::string>{},
          std::make_tuple("createNormal", &SolFactory::createNormal, FunctionTypes::NonPayable, std::vector<std::string>{}),
          std::make_tuple("createCreate2", &SolFactory::createCreate2, FunctionTypes::NonPayable, std::vector<std::string>{})
        );
    }
  };


  Bytes contractBytecode(Hex::toBytes("0x6080604052348015600e575f80fd5b506102dc8061001c5f395ff3fe608060405234801561000f575f80fd5b5060043610610034575f3560e01c80630f4910dd14610038578063159d107114610056575b5f80fd5b610040610074565b60405161004d919061019b565b60405180910390f35b61005e6100e8565b60405161006b919061019b565b60405180910390f35b5f805f801b90505f8160405161008990610150565b8190604051809103905ff59050801580156100a6573d5f803e3d5ffd5b5090507ff40fcec21964ffb566044d083b4073f29f7f7929110ea19e1b3ebe375d89055e816040516100d8919061019b565b60405180910390a1809250505090565b5f806040516100f690610150565b604051809103905ff08015801561010f573d5f803e3d5ffd5b5090507ff40fcec21964ffb566044d083b4073f29f7f7929110ea19e1b3ebe375d89055e81604051610141919061019b565b60405180910390a18091505090565b60f2806101b583390190565b5f73ffffffffffffffffffffffffffffffffffffffff82169050919050565b5f6101858261015c565b9050919050565b6101958161017b565b82525050565b5f6020820190506101ae5f83018461018c565b9291505056fe6080604052348015600e575f80fd5b5060d880601a5f395ff3fe6080604052348015600e575f80fd5b50600436106030575f3560e01c8063c1599bd9146034578063f2c9ecd814604e575b5f80fd5b603a6068565b60405160459190608b565b60405180910390f35b6054606d565b604051605f9190608b565b60405180910390f35b600a81565b5f600a905090565b5f819050919050565b6085816075565b82525050565b5f602082019050609c5f830184607e565b9291505056fea264697066735822122060b40cdbef8c669a4526ed65c2b8a80519355b77c8874d19d20df94faa202e1664736f6c634300081a0033a264697066735822122093190d797d6da853a7d23096512703640018fe121ed71aa9ffe020bebc7d4fab64736f6c634300081a0033"));

  TEST_CASE("EVM Create Test", "[contract][evm][create]") {
    SECTION("Legacy Create + Create2.") {
      std::unique_ptr<Options> options = nullptr;
      Address contractAddress = Address();
      {
        SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testCreationEVM");
        contractAddress = sdk.deployBytecode(contractBytecode);

        auto contractSize = sdk.getState().getEvmContracts().size();

        sdk.callFunction(contractAddress, &SolFactory::createNormal);

        auto newContractSize = sdk.getState().getEvmContracts().size();
        REQUIRE(newContractSize == contractSize + 1);
        contractSize = newContractSize;

        sdk.callFunction(contractAddress, &SolFactory::createCreate2);

        newContractSize = sdk.getState().getEvmContracts().size();
        REQUIRE (newContractSize == contractSize + 1);
        contractSize = newContractSize;

        sdk.callFunction(contractAddress, &SolFactory::createNormal);

        newContractSize = sdk.getState().getEvmContracts().size();
        REQUIRE (newContractSize == contractSize + 1);

        sdk.callFunction(contractAddress, &SolFactory::createCreate2);
      }
    }
  }

}