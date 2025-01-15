/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONABI_H
#define JSONABI_H

#include "../contract/customcontracts.h" // contract templates -> dynamiccontract.h -> contracthost.h -> contractreflectioninterface.h -> contract/abi.h -> utils.h, libs/json.hpp

/// Namespace for managing and converting contract ABI data to JSON format.
namespace JsonAbi {
  /**
   * Check if a type contained within the string input is a tuple.
   * @param type The type to check.
   * @return `true` if type is a tuple, `false` otherwise.
   */
  bool isTuple(const std::string& type);

  /**
   * Check if a type contained within the string input is an array.
   * @param type The type to check.
   * @return `true` if type is an array, `false` otherwise.
   */
  bool isArray(const std::string& type);

  /**
   * Count how many arrays there are in a tuple type from the given string input.
   * @param type The type to count from.
   * @return The number of arrays inside the given tuple.
   */
  uint64_t countTupleArrays(const std::string& type);

  /**
   * Extract the types of a tuple contained within the string input.
   * @param type The type to extract the tuple types from.
   * @return A vector containing the types of the tuple.
   */
  std::vector<std::string> getTupleTypes(const std::string &type);

  /**
   * Create the JSON "components" object for a tuple type.
   * @param tupleTypes The types of the tuple.
   * @return A JSON array containing the components of the tuple.
   */
  json handleTupleComponents(const std::vector<std::string>& tupleTypes);

  /**
   * Parse a given method input to a JSON object.
   * @param inputDesc The input description of the method (std::pair<type,name>).
   *                  Be aware that tuple types are concatenated into the string itself.
   * @return A JSON object containing the inputs of the method.
   */
  json parseMethodInput(const std::vector<std::pair<std::string,std::string>>& inputDesc);

  /**
   * Parse a given method output to a JSON object.
   * @param outputDesc The output description of the method (std::string).
   *                   Be aware that tuple types are concatenated into the string itself.
   * @return A JSON object containing the outputs of the method.
   */
  json parseMethodOutput(const std::vector<std::string>& outputDesc);

  /**
   * Parse a given event's args to a JSON object.
   * @param args The args description of the event (std::tuple<type,name,indexed>).
   *             Be aware that tuple types are concatenated into the string itself.
   * @return A JSON object containing the args of the event.
   */
  json parseEventArgs(const std::vector<std::tuple<std::string, std::string, bool>>& args);

  /**
   * Convert a MethodDescription object to JSON format.
   * @param desc The MethodDescription object to convert.
   * @return The JSON object.
   */
  json methodToJSON(const ABI::MethodDescription& desc);

  /**
   * Convert an EventDescription object to JSON format.
   * @param desc The EventDescription object to convert.
   * @return A JSON object containing the event data.
   */
  json eventToJSON(const ABI::EventDescription& desc);

  /**
   * Register a contract and get its data in JSON format.
   * @tparam Contract The contract to register.
   * @param contractData The JSON object to store the contract data in.
   */
  template <typename Contract> void registerContractAndGetData(json& contractData) {
    Contract::registerContract();
    std::vector<ABI::MethodDescription> funcData;
    std::vector<ABI::EventDescription> eventData;
    funcData = ContractReflectionInterface::getFunctionsDataStructure<Contract>();
    eventData = ContractReflectionInterface::getEventsDataStructure<Contract>();
    for (const auto& func : funcData) contractData.push_back(methodToJSON(func));
    for (const auto& event : eventData) contractData.push_back(eventToJSON(event));
  }

  /**
   * Write contract data to a JSON file. Output filename is the contract's class name.
   * @tparam Contract The contract to write the data of.
   */
  template <typename Contract> void writeContractToJson() {
    json contractData;
    registerContractAndGetData<Contract>(contractData);
    std::string fileName = Utils::getRealTypeName<Contract>();
    if (!fileName.substr(fileName.find_last_of(".") + 1).ends_with("json")) fileName += ".json";
    if (!std::filesystem::exists("ABI")) std::filesystem::create_directory("ABI");
    std::ofstream jsonFile("ABI/" + fileName);
    jsonFile << std::setw(2) << contractData << std::endl;
  }

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
   * Write the data of several contracts to a JSON file.
   * @tparam ContractTuple The list of contracts to write the data of.
   * @tparam N The number of contracts in the list.
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
   * Struct for writing contracts to JSON.
   * @tparam T The contract type.
   */
  template <typename T> struct ContractWriter;

  /**
   * Specialization for a list of contracts.
   * @tparam Contracts The list of contracts to write.
   */
  template <typename... Contracts> struct ContractWriter<std::tuple<Contracts...>> {
    /// Write the data of a contract to a JSON file.
    static void write() {
      writeContractsToJsonImpl<std::tuple<Contracts...>, sizeof...(Contracts) - 1>();
    }
  };

  /**
   * Specialization for a single contract.
   * @tparam Contract The contract to write.
   */
  template <typename Contract> struct ContractWriter {
    /// Write the data of a contract to a JSON file.
    static void write() { writeContractToJson<Contract>(); }
  };

  /**
   * Builder function for creating the ContractManager ABI functions.
   * @tparam Contract The contract to write.
   * @return An array of JSON objects containing the ABI functions.
   */
  template <typename Contract> json getConstructorABI() {
    return JsonAbi::methodToJSON(ContractReflectionInterface::getConstructorDataStructure<Contract>());
  }

  /**
   * Get the ABI of all constructors in a tuple of contracts.
   * @tparam ContractTuple The tuple of contracts to get the constructors of.
   * @tparam N The number of contracts in the tuple.
   * @param abis The array of JSON objects to store the ABI functions in.
   */
  template <typename ContractTuple, std::size_t N>
  requires (N < std::tuple_size<ContractTuple>::value)
  void getConstructorsABI(json& abis) {
    abis.push_back(getConstructorABI<std::tuple_element_t<N, ContractTuple>>());
    if constexpr (N + 1 < std::tuple_size<ContractTuple>::value) {
      getConstructorsABI<ContractTuple, N + 1>(abis);
    }
  }

  /**
   * Base case for getConstructorsABI recursion (do nothing).
   * @tparam ContractTuple The tuple of contracts to get the constructors of.
   * @tparam N The number of contracts in the tuple.
   */
  template <typename ContractTuple, std::size_t N>
  requires (N == std::tuple_size<ContractTuple>::value)
  void getConstructorsABI(json&) {
    // Do nothing by default on recursion
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
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "address"}, {"type", "address"} }
          } }, {"type", "tuple[]"}
        }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    });
    managerABI.push_back({
        {"inputs", {
              { {"internalType", "address"}, {"name", "creator"}, {"type", "address"} }
        }},
      {"name", "getDeployedContractsForCreator"},
      {"outputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "address"}, {"type", "address"} }
          } }, {"type", "tuple[]"}
        }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    });

    std::ofstream jsonFile("ABI/ContractManager.json");
    jsonFile << std::setw(2) << managerABI << std::endl;
  }

  /**
   * Write all contract ABIs to JSON files.
   * @tparam Contracts The list of contracts to write.
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
