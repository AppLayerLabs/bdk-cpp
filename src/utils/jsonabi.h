/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONABI_H
#define JSONABI_H

#include "../contract/customcontracts.h"
#include "contractreflectioninterface.h"
#include "utils.h" // nlohmann/json.hpp

/// Namespace for managing and converting contract ABI data to JSON format.
namespace JsonAbi {
  /**
   * Convert a MethodDescription object to JSON format.
   * @param obj The JSON object to convert to.
   * @param desc The MethodDescription object to convert.
   */
  void methodToJson(json& obj, const ABI::MethodDescription& desc);

  /**
   * Convert an EventDescription object to JSON format.
   * @param obj The JSON object to convert to.
   * @param desc The EventDescription object to convert.
   */
  void eventToJson(json& obj, const ABI::EventDescription& desc);

  /**
   * Register a contract and gets the contract data in JSON format.
   * @tparam Contract The contract to register.
   * @param contractData The JSON object to store the contract data in.
   */
  template <typename Contract> void registerContractAndGetData(json& contractData) {
    Contract::registerContract();
    std::vector<ABI::MethodDescription> ctorData;
    std::vector<ABI::MethodDescription> funcData;
    std::vector<ABI::EventDescription> eventData;
    ctorData = ContractReflectionInterface::getConstructorDataStructure<Contract>();
    funcData = ContractReflectionInterface::getFunctionDataStructure<Contract>();
    eventData = ContractReflectionInterface::getEventDataStructure<Contract>();
    for (const auto& ctor : ctorData) {
      json ctorJson;
      methodToJson(ctorJson, ctor);
      contractData.push_back(ctorJson);
    }
    for (const auto& func : funcData) {
      json funcJson;
      methodToJson(funcJson, func);
      contractData.push_back(funcJson);
    }
    for (const auto& event : eventData) {
      json eventJson;
      eventToJson(eventJson, event);
      contractData.push_back(eventJson);
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
    std::string fileName = Utils::getRealTypeName<Contract>();
    if (fileName.substr(fileName.find_last_of(".") + 1) != "json") fileName += ".json";
    if (!std::filesystem::exists("ABI")) std::filesystem::create_directory("ABI");
    std::ofstream jsonFile("ABI/" + fileName);
    jsonFile << std::setw(4) << contractData << std::endl;
  }

  /**
   * This function writes the data of a contract to a JSON file.
   * @tparam ContractTuple The tuple of contracts to write the data of.
   * @tparam N The number of contracts to write.
   */
  template <typename ContractTuple, std::size_t N> void writeContractsToJsonImpl() {
    if constexpr (N > 0) {
      writeContractToJson<std::tuple_element_t<N, ContractTuple>>();
      writeContractsToJsonImpl<ContractTuple, N - 1>();
    } else {
      writeContractToJson<std::tuple_element_t<0, ContractTuple>>();
    }
  }

  /**
   * Base struct for writing contracts to JSON
   * @tparam T The contract type.
   */
  template <typename T> struct ContractWriter;

  /**
   * Writer specialization for tuple of contracts.
   * @tparam Contracts The contracts to write.
   */
  template <typename... Contracts> struct ContractWriter<std::tuple<Contracts...>> {
    /// Write the data of a contract to a JSON file.
    static void write() {
      writeContractsToJsonImpl<std::tuple<Contracts...>, sizeof...(Contracts) - 1>();
    }
  };

  /**
   * Writer specialization for single contract
   * @tparam Contract The contract to write.
   */
  template <typename Contract> struct ContractWriter {
    /// Write the data of a contract to a JSON file.
    static void write() { writeContractToJson<Contract>(); }
  };

  /**
   * Builder function for creating the ContractManager ABI functions
   * @tparam Contract The contract to write.
   * @return An array of JSON objects containing the ABI functions
   */
  template <typename Contract> json getConstructorABI() {
    std::vector<ABI::MethodDescription> ctorData;
    json ctorJsonArray = json::array();
    
    ctorData = ContractReflectionInterface::getConstructorDataStructure<Contract>();
    for (auto &methodDesc : ctorData) {
      methodDesc.name = "createNew" + Utils::getRealTypeName<Contract>() + "Contract";
      methodDesc.type = "function";
      json ctorJson;
      methodToJson(ctorJson, methodDesc);
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
  std::enable_if_t<(N<std::tuple_size<ContractTuple>::value)>
  getConstructorsABI(json& abis) {
    json ctors = getConstructorABI<std::tuple_element_t<N, ContractTuple>>();
    for (json::iterator it = ctors.begin(); it != ctors.end(); it++) abis.push_back(*it);
    if constexpr (N + 1 < std::tuple_size<ContractTuple>::value) {
      getConstructorsABI<ContractTuple, N + 1>(abis);
    }
  }

  /**
   * Base case for getConstructorsABI recursion (does nothing).
   * @tparam ContractTuple The tuple of contracts to get the constructors of.
   * @tparam N The number of contracts in the tuple.
   * @param abis The array of JSON objects to store the ABI functions in.
   */
  template <typename ContractTuple, std::size_t N>
  std::enable_if_t<(N == std::tuple_size<ContractTuple>::value)>
  getConstructorsABI(json &abis) {}

  /**
   * Register a tuple of contracts.
   * @tparam ContractTuple The tuple of contracts to register.
   * @tparam Index The index of the contract to register.
   */
  template <typename ContractTuple, std::size_t Index = 0> void registerContracts() {
    using ContractType = typename std::tuple_element<Index, ContractTuple>::type;
    ContractType::registerContract();
    if constexpr(Index + 1 < std::tuple_size<ContractTuple>::value) {
      registerContracts<ContractTuple, Index + 1>();
    }
  }

  /**
   * Write manager ABI to a JSON file.
   * @tparam ContractTuple The tuple of contracts to get the constructors of.
   */
  template <typename ContractTuple> void writeManagerABI() {
    registerContracts<ContractTuple>();
    json managerABI;
    getConstructorsABI<ContractTuple, 0>(managerABI);
    managerABI.push_back({
      {"inputs", json::array()},
      {"name", "getDeployedContracts"},
      {"outputs", {
        {{"internalType", "string[]"}, {"name", ""}, {"type", "string[]"}},
        {{"internalType", "address[]"}, {"name", ""}, {"type", "address[]"}}
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    });
    std::ofstream jsonFile("ABI/ContractManager.json");
    jsonFile << std::setw(4) << managerABI << std::endl;
  }

  /**
   * Write all contracts ABI to JSON files
   * @tparam Contracts The contracts to write.
   * @tparam ContractTypes Declared in customcontracts.h.
   * @return 0
   */
  template <typename... Contracts> int writeContractsToJson() {
    (ContractWriter<Contracts>::write(), ...);
    writeManagerABI<ContractTypes>();
    return 0;
  }
} // namespace JsonAbi

#endif // JSONABI_H
