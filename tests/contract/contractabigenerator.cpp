/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/jsonabi.h"
#include <filesystem>

TEST_CASE("ContractABIGenerator helper", "[contract][contractabigenerator]") {
  SECTION("ContractABIGenerator writeContractsToJson") {
    REQUIRE(JsonAbi::writeContractsToJson<ERC20, ERC20Wrapper, NativeWrapper>() == 0);
    REQUIRE(std::filesystem::exists("ABI/ERC20.json"));
    REQUIRE(std::filesystem::exists("ABI/ERC20Wrapper.json"));
    REQUIRE(std::filesystem::exists("ABI/NativeWrapper.json"));
    REQUIRE(std::filesystem::exists("ABI/ContractManager.json"));
  }

  SECTION("ContractABIGenerator check file content ERC20") {
    json j;
    std::ifstream i("ABI/ERC20.json");
    i >> j;
    REQUIRE(j.size() == 10);
    REQUIRE(j[0]["name"] == "");
    REQUIRE(j[0]["stateMutability"] == "nonpayable");
    REQUIRE(j[0]["inputs"][0]["internalType"] == "string");
    REQUIRE(j[0]["inputs"][0]["name"] == "erc20name");
    REQUIRE(j[0]["inputs"][0]["type"] == "string");
    REQUIRE(j[0]["inputs"][1]["internalType"] == "string");
    REQUIRE(j[0]["inputs"][1]["name"] == "erc20symbol");
    REQUIRE(j[0]["inputs"][1]["type"] == "string");
    REQUIRE(j[0]["inputs"][2]["internalType"] == "uint8");
    REQUIRE(j[0]["inputs"][2]["name"] == "erc20decimals");
    REQUIRE(j[0]["inputs"][2]["type"] == "uint8");
    REQUIRE(j[0]["inputs"][3]["internalType"] == "uint256");
    REQUIRE(j[0]["inputs"][3]["name"] == "mintValue");
    REQUIRE(j[0]["inputs"][3]["type"] == "uint256");
    REQUIRE(j[1]["name"] == "name");
    REQUIRE(j[1]["stateMutability"] == "view");
    REQUIRE(j[1]["type"] == "function");
    REQUIRE(j[1]["outputs"][0]["internalType"] == "string");
    REQUIRE(j[1]["outputs"][0]["name"] == "");
    REQUIRE(j[1]["outputs"][0]["type"] == "string");
    REQUIRE(j[2]["name"] == "symbol");
    REQUIRE(j[2]["stateMutability"] == "view");
    REQUIRE(j[2]["type"] == "function");
    REQUIRE(j[2]["outputs"][0]["internalType"] == "string");
    REQUIRE(j[2]["outputs"][0]["name"] == "");
    REQUIRE(j[2]["outputs"][0]["type"] == "string");
    REQUIRE(j[3]["name"] == "decimals");
    REQUIRE(j[3]["stateMutability"] == "view");
    REQUIRE(j[3]["type"] == "function");
    REQUIRE(j[3]["outputs"][0]["internalType"] == "uint8");
    REQUIRE(j[3]["outputs"][0]["name"] == "");
    REQUIRE(j[3]["outputs"][0]["type"] == "uint8");
    REQUIRE(j[4]["name"] == "totalSupply");
    REQUIRE(j[4]["stateMutability"] == "view");
    REQUIRE(j[4]["type"] == "function");
    REQUIRE(j[4]["outputs"][0]["internalType"] == "uint256");
    REQUIRE(j[4]["outputs"][0]["name"] == "");
    REQUIRE(j[4]["outputs"][0]["type"] == "uint256");
    REQUIRE(j[5]["name"] == "balanceOf");
    REQUIRE(j[5]["stateMutability"] == "view");
    REQUIRE(j[5]["type"] == "function");
    REQUIRE(j[5]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[5]["inputs"][0]["name"] == "owner");
    REQUIRE(j[5]["inputs"][0]["type"] == "address");
    REQUIRE(j[5]["outputs"][0]["internalType"] == "uint256");
    REQUIRE(j[5]["outputs"][0]["name"] == "");
    REQUIRE(j[5]["outputs"][0]["type"] == "uint256");
    REQUIRE(j[6]["name"] == "transfer");
    REQUIRE(j[6]["stateMutability"] == "nonpayable");
    REQUIRE(j[6]["type"] == "function");
    REQUIRE(j[6]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[6]["inputs"][0]["name"] == "to");
    REQUIRE(j[6]["inputs"][0]["type"] == "address");
    REQUIRE(j[6]["inputs"][1]["internalType"] == "uint256");
    REQUIRE(j[6]["inputs"][1]["name"] == "value");
    REQUIRE(j[6]["inputs"][1]["type"] == "uint256");
    REQUIRE(j[7]["name"] == "approve");
    REQUIRE(j[7]["stateMutability"] == "nonpayable");
    REQUIRE(j[7]["type"] == "function");
    REQUIRE(j[7]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[7]["inputs"][0]["name"] == "spender");
    REQUIRE(j[7]["inputs"][0]["type"] == "address");
    REQUIRE(j[7]["inputs"][1]["internalType"] == "uint256");
    REQUIRE(j[7]["inputs"][1]["name"] == "value");
    REQUIRE(j[7]["inputs"][1]["type"] == "uint256");
    REQUIRE(j[8]["name"] == "allowance");
    REQUIRE(j[8]["stateMutability"] == "view");
    REQUIRE(j[8]["type"] == "function");
    REQUIRE(j[8]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[8]["inputs"][0]["name"] == "owner");
    REQUIRE(j[8]["inputs"][0]["type"] == "address");
    REQUIRE(j[8]["inputs"][1]["internalType"] == "address");
    REQUIRE(j[8]["inputs"][1]["name"] == "spender");
    REQUIRE(j[8]["inputs"][1]["type"] == "address");
    REQUIRE(j[8]["outputs"][0]["internalType"] == "uint256");
    REQUIRE(j[8]["outputs"][0]["name"] == "");
    REQUIRE(j[8]["outputs"][0]["type"] == "uint256");
    REQUIRE(j[9]["name"] == "transferFrom");
    REQUIRE(j[9]["stateMutability"] == "nonpayable");
    REQUIRE(j[9]["type"] == "function");
    REQUIRE(j[9]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[9]["inputs"][0]["name"] == "from");
    REQUIRE(j[9]["inputs"][0]["type"] == "address");
    REQUIRE(j[9]["inputs"][1]["internalType"] == "address");
    REQUIRE(j[9]["inputs"][1]["name"] == "to");
    REQUIRE(j[9]["inputs"][1]["type"] == "address");
    REQUIRE(j[9]["inputs"][2]["internalType"] == "uint256");
    REQUIRE(j[9]["inputs"][2]["name"] == "value");
    REQUIRE(j[9]["inputs"][2]["type"] == "uint256");
  }

  SECTION("ContractABIGenerator check file content ERC20Wrapper") {
    json j;
    std::ifstream i("ABI/ERC20Wrapper.json");
    i >> j;
    REQUIRE(j.size() == 6);
    REQUIRE(j[0]["name"] == "");
    REQUIRE(j[0]["type"] == "constructor");
    REQUIRE(j[0]["stateMutability"] == "nonpayable");
    REQUIRE(j[1]["name"] == "getContractBalance");
    REQUIRE(j[1]["stateMutability"] == "view");
    REQUIRE(j[1]["type"] == "function");
    REQUIRE(j[1]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[1]["inputs"][0]["name"] == "token");
    REQUIRE(j[1]["inputs"][0]["type"] == "address");
    REQUIRE(j[1]["outputs"][0]["internalType"] == "uint256");
    REQUIRE(j[1]["outputs"][0]["name"] == "");
    REQUIRE(j[1]["outputs"][0]["type"] == "uint256");
    REQUIRE(j[2]["name"] == "getUserBalance");
    REQUIRE(j[2]["stateMutability"] == "view");
    REQUIRE(j[2]["type"] == "function");
    REQUIRE(j[2]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[2]["inputs"][0]["name"] == "token");
    REQUIRE(j[2]["inputs"][0]["type"] == "address");
    REQUIRE(j[2]["inputs"][1]["internalType"] == "address");
    REQUIRE(j[2]["inputs"][1]["name"] == "user");
    REQUIRE(j[2]["inputs"][1]["type"] == "address");
    REQUIRE(j[2]["outputs"][0]["internalType"] == "uint256");
    REQUIRE(j[2]["outputs"][0]["name"] == "");
    REQUIRE(j[2]["outputs"][0]["type"] == "uint256");
    REQUIRE(j[3]["name"] == "withdraw");
    REQUIRE(j[3]["stateMutability"] == "nonpayable");
    REQUIRE(j[3]["type"] == "function");
    REQUIRE(j[3]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[3]["inputs"][0]["name"] == "token");
    REQUIRE(j[3]["inputs"][0]["type"] == "address");
    REQUIRE(j[3]["inputs"][1]["internalType"] == "uint256");
    REQUIRE(j[3]["inputs"][1]["name"] == "value");
    REQUIRE(j[3]["inputs"][1]["type"] == "uint256");
    REQUIRE(j[4]["name"] == "transferTo");
    REQUIRE(j[4]["stateMutability"] == "nonpayable");
    REQUIRE(j[4]["type"] == "function");
    REQUIRE(j[4]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[4]["inputs"][0]["name"] == "token");
    REQUIRE(j[4]["inputs"][0]["type"] == "address");
    REQUIRE(j[4]["inputs"][1]["internalType"] == "address");
    REQUIRE(j[4]["inputs"][1]["name"] == "to");
    REQUIRE(j[4]["inputs"][1]["type"] == "address");
    REQUIRE(j[4]["inputs"][2]["internalType"] == "uint256");
    REQUIRE(j[4]["inputs"][2]["name"] == "value");
    REQUIRE(j[4]["inputs"][2]["type"] == "uint256");
    REQUIRE(j[5]["name"] == "deposit");
    REQUIRE(j[5]["stateMutability"] == "nonpayable");
    REQUIRE(j[5]["type"] == "function");
    REQUIRE(j[5]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[5]["inputs"][0]["name"] == "token");
    REQUIRE(j[5]["inputs"][0]["type"] == "address");
    REQUIRE(j[5]["inputs"][1]["internalType"] == "uint256");
    REQUIRE(j[5]["inputs"][1]["name"] == "value");
    REQUIRE(j[5]["inputs"][1]["type"] == "uint256");
}

  SECTION("ContractABIGenerator check file content NativeWrapper") {
    json j;
    std::ifstream i("ABI/NativeWrapper.json");
    i >> j;
    REQUIRE(j.size() == 3);
    REQUIRE(j[0]["name"] == "");
    REQUIRE(j[0]["type"] == "constructor");
    REQUIRE(j[0]["stateMutability"] == "nonpayable");
    REQUIRE(j[0]["inputs"][0]["internalType"] == "string");
    REQUIRE(j[0]["inputs"][0]["name"] == "erc20_name");
    REQUIRE(j[0]["inputs"][0]["type"] == "string");
    REQUIRE(j[0]["inputs"][1]["internalType"] == "string");
    REQUIRE(j[0]["inputs"][1]["name"] == "erc20_symbol");
    REQUIRE(j[0]["inputs"][1]["type"] == "string");
    REQUIRE(j[0]["inputs"][2]["internalType"] == "uint8");
    REQUIRE(j[0]["inputs"][2]["name"] == "erc20_decimals");
    REQUIRE(j[0]["inputs"][2]["type"] == "uint8");
    REQUIRE(j[1]["name"] == "deposit");
    REQUIRE(j[1]["stateMutability"] == "payable");
    REQUIRE(j[1]["type"] == "function");
    REQUIRE(j[2]["name"] == "withdraw");
    REQUIRE(j[2]["stateMutability"] == "payable");
    REQUIRE(j[2]["type"] == "function");
    REQUIRE(j[2]["inputs"][0]["internalType"] == "uint256");
    REQUIRE(j[2]["inputs"][0]["name"] == "value");
    REQUIRE(j[2]["inputs"][0]["type"] == "uint256");
  }

  SECTION("ContractABIGenerator check file content ContractManager") {
    json j;
    std::ifstream i("ABI/ContractManager.json");
    i >> j;

    REQUIRE(j.size() == 12); // 11 contracts functions + getDeployedContracts

    REQUIRE(j[0]["name"] == "createNewERC20Contract");
    REQUIRE(j[0]["stateMutability"] == "nonpayable");
    REQUIRE(j[0]["type"] == "function");
    REQUIRE(j[0]["inputs"][0]["internalType"] == "string");
    REQUIRE(j[0]["inputs"][0]["name"] == "erc20name");
    REQUIRE(j[0]["inputs"][0]["type"] == "string");
    REQUIRE(j[0]["inputs"][1]["internalType"] == "string");
    REQUIRE(j[0]["inputs"][1]["name"] == "erc20symbol");
    REQUIRE(j[0]["inputs"][1]["type"] == "string");
    REQUIRE(j[0]["inputs"][2]["internalType"] == "uint8");
    REQUIRE(j[0]["inputs"][2]["name"] == "erc20decimals");
    REQUIRE(j[0]["inputs"][2]["type"] == "uint8");
    REQUIRE(j[0]["inputs"][3]["internalType"] == "uint256");
    REQUIRE(j[0]["inputs"][3]["name"] == "mintValue");
    REQUIRE(j[0]["inputs"][3]["type"] == "uint256");

    REQUIRE(j[1]["name"] == "createNewERC20WrapperContract");
    REQUIRE(j[1]["stateMutability"] == "nonpayable");
    REQUIRE(j[1]["type"] == "function");

    REQUIRE(j[2]["name"] == "createNewNativeWrapperContract");
    REQUIRE(j[2]["stateMutability"] == "nonpayable");
    REQUIRE(j[2]["type"] == "function");
    REQUIRE(j[2]["inputs"][0]["internalType"] == "string");
    REQUIRE(j[2]["inputs"][0]["name"] == "erc20_name");
    REQUIRE(j[2]["inputs"][0]["type"] == "string");
    REQUIRE(j[2]["inputs"][1]["internalType"] == "string");
    REQUIRE(j[2]["inputs"][1]["name"] == "erc20_symbol");
    REQUIRE(j[2]["inputs"][1]["type"] == "string");
    REQUIRE(j[2]["inputs"][2]["internalType"] == "uint8");
    REQUIRE(j[2]["inputs"][2]["name"] == "erc20_decimals");
    REQUIRE(j[2]["inputs"][2]["type"] == "uint8");

    REQUIRE(j[3]["name"] == "createNewSimpleContractContract");
    REQUIRE(j[3]["stateMutability"] == "nonpayable");
    REQUIRE(j[3]["type"] == "function");
    REQUIRE(j[3]["inputs"][0]["internalType"] == "string");
    REQUIRE(j[3]["inputs"][0]["name"] == "name_");
    REQUIRE(j[3]["inputs"][0]["type"] == "string");
    REQUIRE(j[3]["inputs"][1]["internalType"] == "uint256");
    REQUIRE(j[3]["inputs"][1]["name"] == "value_");
    REQUIRE(j[3]["inputs"][1]["type"] == "uint256");

    REQUIRE(j[4]["name"] == "createNewDEXV2PairContract");
    REQUIRE(j[4]["stateMutability"] == "nonpayable");
    REQUIRE(j[4]["type"] == "function");

    REQUIRE(j[5]["name"] == "createNewDEXV2FactoryContract");
    REQUIRE(j[5]["stateMutability"] == "nonpayable");
    REQUIRE(j[5]["type"] == "function");
    REQUIRE(j[5]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[5]["inputs"][0]["name"] == "_feeToSetter");
    REQUIRE(j[5]["inputs"][0]["type"] == "address");

    REQUIRE(j[6]["name"] == "createNewDEXV2Router02Contract");
    REQUIRE(j[6]["stateMutability"] == "nonpayable");
    REQUIRE(j[6]["type"] == "function");
    REQUIRE(j[6]["inputs"][0]["internalType"] == "address");
    REQUIRE(j[6]["inputs"][0]["name"] == "factory");
    REQUIRE(j[6]["inputs"][0]["type"] == "address");
    REQUIRE(j[6]["inputs"][1]["internalType"] == "address");
    REQUIRE(j[6]["inputs"][1]["name"] == "wrappedNative");
    REQUIRE(j[6]["inputs"][1]["type"] == "address");

    REQUIRE(j[7]["name"] == "createNewERC721Contract");
    REQUIRE(j[7]["stateMutability"] == "nonpayable");
    REQUIRE(j[7]["type"] == "function");
    REQUIRE(j[7]["inputs"][0]["internalType"] == "string");
    REQUIRE(j[7]["inputs"][0]["name"] == "erc721name");
    REQUIRE(j[7]["inputs"][0]["type"] == "string");
    REQUIRE(j[7]["inputs"][1]["internalType"] == "string");
    REQUIRE(j[7]["inputs"][1]["name"] == "erc721symbol");
    REQUIRE(j[7]["inputs"][1]["type"] == "string");

    REQUIRE(j[8]["name"] == "createNewThrowTestAContract");
    REQUIRE(j[8]["stateMutability"] == "nonpayable");
    REQUIRE(j[8]["type"] == "function");

    REQUIRE(j[9]["name"] == "createNewThrowTestBContract");
    REQUIRE(j[9]["stateMutability"] == "nonpayable");
    REQUIRE(j[9]["type"] == "function");

    REQUIRE(j[10]["name"] == "createNewThrowTestCContract");
    REQUIRE(j[10]["stateMutability"] == "nonpayable");
    REQUIRE(j[10]["type"] == "function");

    REQUIRE(j[11]["name"] == "getDeployedContracts");
    REQUIRE(j[11]["outputs"][0]["internalType"] == "string[]");
    REQUIRE(j[11]["outputs"][1]["internalType"] == "address[]");
    REQUIRE(j[11]["stateMutability"] == "view");
    REQUIRE(j[11]["type"] == "function");
  }
}