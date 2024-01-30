/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <expected>

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/jsonabi.h"
#include "expectedABI.cpp" /// yeah lmao include a .cpp wtf
#include <filesystem>

TEST_CASE("ContractABIGenerator helper", "[contract][contractabigenerator]") {
  SECTION("ContractABIGenerator writeContractsToJson") {
    REQUIRE(JsonAbi::writeContractsToJson<ERC20, ERC20Wrapper, NativeWrapper, SimpleContract>() == 0);
    REQUIRE(std::filesystem::exists("ABI/ERC20.json"));
    REQUIRE(std::filesystem::exists("ABI/ERC20Wrapper.json"));
    REQUIRE(std::filesystem::exists("ABI/NativeWrapper.json"));
    REQUIRE(std::filesystem::exists("ABI/ContractManager.json"));
    REQUIRE(std::filesystem::exists("ABI/SimpleContract.json"));
  }

  SECTION("ContractABIGenerator check file content ERC20") {
    json j;
    std::ifstream i("ABI/ERC20.json");
    i >> j;
    REQUIRE(j.size() == 9);

    auto findTransferFrom = std::find(j.begin(), j.end(), EXPECTED::ERC20::transferFrom);
    REQUIRE(findTransferFrom != j.end());
    auto findApprove = std::find(j.begin(), j.end(), EXPECTED::ERC20::approve);
    REQUIRE(findApprove != j.end());
    auto findBalanceOf = std::find(j.begin(), j.end(), EXPECTED::ERC20::balanceOf);
    REQUIRE(findBalanceOf != j.end());
    auto findTotalSupply = std::find(j.begin(), j.end(), EXPECTED::ERC20::totalSupply);
    REQUIRE(findTotalSupply != j.end());
    auto findTransfer = std::find(j.begin(), j.end(), EXPECTED::ERC20::transfer);
    REQUIRE(findTransfer != j.end());
    auto findAllowance = std::find(j.begin(), j.end(), EXPECTED::ERC20::allowance);
    REQUIRE(findAllowance != j.end());
    auto findDecimals = std::find(j.begin(), j.end(), EXPECTED::ERC20::decimals);
    REQUIRE(findDecimals != j.end());
    auto findSymbol = std::find(j.begin(), j.end(), EXPECTED::ERC20::symbol);
    REQUIRE(findSymbol != j.end());
    auto findName = std::find(j.begin(), j.end(), EXPECTED::ERC20::name);
    REQUIRE(findName != j.end());
  }

  SECTION("ContractABIGenerator check file content ERC20Wrapper") {
    json j;
    std::ifstream i("ABI/ERC20Wrapper.json");
    i >> j;
    REQUIRE(j.size() == 5);

    auto findDeposit = std::find(j.begin(), j.end(), EXPECTED::ERC20Wrapper::deposit);
    REQUIRE(findDeposit != j.end());
    auto findTransferTo = std::find(j.begin(), j.end(), EXPECTED::ERC20Wrapper::transferTo);
    REQUIRE(findTransferTo != j.end());
    auto findWithdraw = std::find(j.begin(), j.end(), EXPECTED::ERC20Wrapper::withdraw);
    REQUIRE(findWithdraw != j.end());
    auto findGetUserBalance = std::find(j.begin(), j.end(), EXPECTED::ERC20Wrapper::getUserBalance);
    REQUIRE(findGetUserBalance != j.end());
    auto findGetContractBalance = std::find(j.begin(), j.end(), EXPECTED::ERC20Wrapper::getContractBalance);
    REQUIRE(findGetContractBalance != j.end());
}

  SECTION("ContractABIGenerator check file content NativeWrapper") {
    json j;
    std::ifstream i("ABI/NativeWrapper.json");
    i >> j;
    REQUIRE(j.size() == 2);

    auto findDeposit = std::find(j.begin(), j.end(), EXPECTED::NativeWrapper::deposit);
    REQUIRE(findDeposit != j.end());
    auto findWithdraw = std::find(j.begin(), j.end(), EXPECTED::NativeWrapper::withdraw);
    REQUIRE(findWithdraw != j.end());
  }

  SECTION("ContractABIGenerator check file content ContractManager") {
    json j;
    std::ifstream i("ABI/ContractManager.json");
    i >> j;
    REQUIRE(j.size() == std::tuple_size<ContractTypes>() + 1);

    auto findCreateNewERC20Contract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewERC20Contract);
    REQUIRE(findCreateNewERC20Contract != j.end());
    auto findCreateNewERC20WrapperContract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewERC20WrapperContract);
    REQUIRE(findCreateNewERC20WrapperContract != j.end());
    auto findCreateNewNativeWrapperContract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewNativeWrapperContract);
    REQUIRE(findCreateNewNativeWrapperContract != j.end());
    auto findCreateNewSimpleContract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewSimpleContract);
    REQUIRE(findCreateNewSimpleContract != j.end());
    auto findCreateNewDEXV2PairContract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewDEXV2PairContract);
    REQUIRE(findCreateNewDEXV2PairContract != j.end());
    auto findCreateNewDEXV2Router02Contract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewDEXV2Router02Contract);
    REQUIRE(findCreateNewDEXV2Router02Contract != j.end());
    auto findCreateNewDEXV2FactoryContract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewDEXV2FactoryContract);
    REQUIRE(findCreateNewDEXV2FactoryContract != j.end());
    auto findCreateNewERC721Contract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewERC721Contract);
    REQUIRE(findCreateNewERC721Contract != j.end());
    auto findCreateNewThrowTestAContract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewThrowTestAContract);
    REQUIRE(findCreateNewThrowTestAContract != j.end());
    auto findCreateNewThrowTestBContract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewThrowTestBContract);
    REQUIRE(findCreateNewThrowTestBContract != j.end());
    auto findCreateNewThrowTestCContract = std::find(j.begin(), j.end(), EXPECTED::ContractManager::createNewThrowTestCContract);
    REQUIRE(findCreateNewThrowTestCContract != j.end());
    auto findGetDeployedContracts = std::find(j.begin(), j.end(), EXPECTED::ContractManager::getDeployedContracts);
    REQUIRE(findGetDeployedContracts != j.end());
  }

  SECTION("ContractABIGenerator check file content SimpleContract") {
    json j;
    std::ifstream i("ABI/SimpleContract.json");
    i >> j;

    REQUIRE(j.size() == 23);
    auto findSetName = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::setName);
    REQUIRE(findSetName != j.end());
    auto findSetNames = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::setNames);
    REQUIRE(findSetNames != j.end());
    auto findSetValue = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::setValue);
    REQUIRE(findSetValue != j.end());
    auto findSetValues = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::setValues);
    REQUIRE(findSetValues != j.end());
    auto findSetNamesAndValues = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::setNamesAndValues);
    REQUIRE(findSetNamesAndValues != j.end());
    auto findSetNamesAndValuesInTuple = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::setNamesAndValuesInTuple);
    REQUIRE(findSetNamesAndValuesInTuple != j.end());
    auto findSetNamesAndValuesInArrayOfArrays = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::setNamesAndValuesInArrayOfArrays);
    REQUIRE(findSetNamesAndValuesInArrayOfArrays != j.end());
    auto findSetTuple = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::setTuple);
    REQUIRE(findSetTuple != j.end());
    auto findGetName = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getName);
    REQUIRE(findGetName != j.end());
    auto findGetNames = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getNames);
    REQUIRE(findGetNames != j.end());
    auto findGetValue = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getValue);
    REQUIRE(findGetValue != j.end());
    auto findGetValues = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getValues);
    REQUIRE(findGetValues != j.end());
    auto findGetNameAndValue = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getNameAndValue);
    REQUIRE(findGetNameAndValue != j.end());
    auto findGetNamesAndValues = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getNamesAndValues);
    REQUIRE(findGetNamesAndValues != j.end());
    auto findGetNamesAndValuesInTuple = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getNamesAndValuesInTuple);
    REQUIRE(findGetNamesAndValuesInTuple != j.end());
    auto findGetNamesAndValuesInArrayOfArrays = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getNamesAndValuesInArrayOfArrays);
    REQUIRE(findGetNamesAndValuesInArrayOfArrays != j.end());
    auto findGetTuple = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getTuple);
    REQUIRE(findGetTuple != j.end());
    auto findGetValueOverload = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::getValueOverload);
    REQUIRE(findGetValueOverload != j.end());
    auto findNameAndValueTupleChanged = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::nameAndValueTupleChanged);
    REQUIRE(findNameAndValueTupleChanged != j.end());
    auto findNameAndValueChanged = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::nameAndValueChanged);
    REQUIRE(findNameAndValueChanged != j.end());
    auto findValueChanged = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::valueChanged);
    REQUIRE(findValueChanged != j.end());
    auto findNameChanged = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::nameChanged);
    REQUIRE(findNameChanged != j.end());
    auto findTupleChanged = std::find(j.begin(), j.end(), EXPECTED::SimpleContract::tupleChanged);
    REQUIRE(findTupleChanged != j.end());
  }

  SECTION("ContractABIGenerator getFunctionName") {
    REQUIRE(ContractReflectionInterface::getFunctionName(&NativeWrapper::deposit) == "deposit");
    REQUIRE(ContractReflectionInterface::getFunctionName(&NativeWrapper::withdraw) == "withdraw");
    REQUIRE(ContractReflectionInterface::getFunctionName(&NativeWrapper::transfer) == "transfer");
    REQUIRE(ContractReflectionInterface::getFunctionName(&ERC20::transfer) == "transfer");
  }
}
