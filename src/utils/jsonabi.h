/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONABI_H
#define JSONABI_H

#include "../contract/customcontracts.h"
#include "contractreflectioninterface.h"
#include "utils.h" //nlohmann/json.hpp

/// Namespace for managing and converting contract ABI data to JSON format.
namespace JsonAbi {

/**
 * This function converts a MethodDescription object to JSON format.
 * @param jsonObject The JSON object to convert to.
 * @param description The MethodDescription object to convert.
 */
void to_json(json &jsonObject,
                    const ABI::MethodDescription &description);

/**
 * This function registers a contract and gets the contract data in JSON format.
 * @tparam Contract The contract to register.
 * @param contractData The JSON object to store the contract data in.
 */
template <typename Contract>
void registerContractAndGetData(json &contractData) {
  Contract::registerContract();
  auto constructorData =
      ContractReflectionInterface::getConstructorDataStructure<Contract>();
  auto functionData =
      ContractReflectionInterface::getFunctionDataStructure<Contract>();

  for (const auto &constructor : constructorData) {
    json ctorJson;
    to_json(ctorJson, constructor);
    contractData.push_back(ctorJson);
  }
  for (const auto &function : functionData) {
    json funcJson;
    to_json(funcJson, function);
    contractData.push_back(funcJson);
  }
}

/**
 * This function writes the contract data of a contract to a JSON file.
 * @tparam Contract The contract to write the data of.
 * @param outputFilename The name of the output file.
 */
template <typename Contract> void writeContractToJson() {
  json contractData;
  registerContractAndGetData<Contract>(contractData);

  std::string outputFileName = Utils::getRealTypeName<Contract>();
  if (outputFileName.substr(outputFileName.find_last_of(".") + 1) != "json") {
    outputFileName += ".json";
  }

  const std::string dirName = "ABI";
  if (!std::filesystem::exists(dirName)) {
    std::filesystem::create_directory(dirName);
  }

  const std::string fullOutputFileName = dirName + "/" + outputFileName;

  std::ofstream jsonFile(fullOutputFileName);
  jsonFile << std::setw(4) << contractData << std::endl;
}

/**
 * This function writes the contract data of a contract to a JSON file.
 * @tparam Contract The contract to write the data of.
 * @tparam N The number of contracts to write.
 */
// For handling a tuple of contracts
template <typename ContractTuple, std::size_t N>
void writeContractsToJsonImpl() {
  if constexpr (N > 0) {
    writeContractToJson<std::tuple_element_t<N, ContractTuple>>();
    writeContractsToJsonImpl<ContractTuple, N - 1>();
  } else {
    writeContractToJson<std::tuple_element_t<0, ContractTuple>>();
  }
}

/**
 * Base struct for writing contracts to JSON
 */
template <typename T> struct ContractWriter;

/**
 * Writer specialization for tuple of contracts
 * @tparam Contracts The contracts to write.
 */
template <typename... Contracts>
struct ContractWriter<std::tuple<Contracts...>> {

  /**
  * This function writes the contract data of a contract to a JSON file.
  */
  static void write() {
    writeContractsToJsonImpl<std::tuple<Contracts...>,
                             sizeof...(Contracts) - 1>();
  }
};

/**
 * Writer specialization for single contract
 * @tparam Contract The contract to write.
 */
template <typename Contract> struct ContractWriter {

  /**
  * This function writes the contract data of a contract to a JSON file.
  */
  static void write() { writeContractToJson<Contract>(); }
};

/**
 * Builder function for creating the ContractManager ABI functions
 * @tparam Contract The contract to write.
 * @return An array of JSON objects containing the ABI functions
 */
template <typename Contract> json getConstructorABI() {
  auto constructorData =
      ContractReflectionInterface::getConstructorDataStructure<Contract>();
  json ctorJsonArray = json::array();
  for (auto &methodDescription : constructorData) {
    methodDescription.name =
        "createNew" + Utils::getRealTypeName<Contract>() + "Contract";
    methodDescription.type = "function";
    json ctorJson;
    to_json(ctorJson, methodDescription);
    ctorJson["outputs"] = json::array();
    ctorJsonArray.push_back(ctorJson);
  }
  return ctorJsonArray;
}

/**
 * Get ABI of all constructors in a tuple of contracts
 * @tparam ContractTuple The tuple of contracts to get the constructors of.
 * @tparam N The number of contracts in the tuple.
 * @param abis The array of JSON objects to store the ABI functions in.
 */
template <typename ContractTuple, std::size_t N>
std::enable_if_t<(N < std::tuple_size<ContractTuple>::value)>
getConstructorsABI(json &abis) {
  json ctors = getConstructorABI<std::tuple_element_t<N, ContractTuple>>();
  for (json::iterator it = ctors.begin(); it != ctors.end(); ++it) {
    abis.push_back(*it);
  }
  if constexpr (N + 1 < std::tuple_size<ContractTuple>::value) {
    getConstructorsABI<ContractTuple, N + 1>(abis);
  }
}

/**
 * Base case for getConstructorsABI recursion
 * @tparam ContractTuple The tuple of contracts to get the constructors of.
 * @tparam N The number of contracts in the tuple.
 * @param abis The array of JSON objects to store the ABI functions in.
 */
template <typename ContractTuple, std::size_t N>
std::enable_if_t<(N == std::tuple_size<ContractTuple>::value)>
getConstructorsABI(json &abis) { /* do nothing */
}

/**
* Register a tuple of contracts
* @tparam Tuple The tuple of contracts to register.
* @tparam Index The index of the contract to register.
*/
template <typename Tuple, std::size_t Index = 0>
void registerContracts()
{
    using ContractType = typename std::tuple_element<Index, Tuple>::type;
    ContractType::registerContract();
    
    if constexpr(Index + 1 < std::tuple_size<Tuple>::value)
    {
        registerContracts<Tuple, Index + 1>();
    }
}

/**
 * Write manager ABI to a JSON file
 * @tparam ContractTuple The tuple of contracts to get the constructors of.
 */
template <typename ContractTuple> void writeManagerABI() {
  registerContracts<ContractTuple>();
  json managerABI;
  getConstructorsABI<ContractTuple, 0>(managerABI);
  managerABI.push_back(
      {{"inputs", json::array()},
       {"name", "getDeployedContracts"},
       {"outputs",
        {{{"internalType", "string[]"}, {"name", ""}, {"type", "string[]"}},
         {{"internalType", "address[]"}, {"name", ""}, {"type", "address[]"}}}},
       {"stateMutability", "view"},
       {"type", "function"}});
  std::ofstream jsonFile("ABI/ContractManager.json");
  jsonFile << std::setw(4) << managerABI << std::endl;
}

/**
 * Write all contracts ABI to JSON files
 * @tparam ContractTypes The contracts to write.
 * @return 0
 */
template <typename... Contracts> int writeContractsToJson() {
  (ContractWriter<Contracts>::write(), ...);
  writeManagerABI<ContractTypes>();
  return 0;
}

} // namespace JsonAbi

#endif // JSONABI_H
