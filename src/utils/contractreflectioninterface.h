/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACTREFLECTIONINTERFACE_H
#define CONTRACTREFLECTIONINTERFACE_H

#include "contract/abi.h"
#include <unordered_set>

/**
 * Namespace for the reflection interface used for registering contract classes.
 * Only the following functions are used in normal operation:
 * - registerContract() -> By the derived DynamicContract class, to register contract methods, arguments, etc.
 * - getConstructorArgumentTypesString<TContract>() -> By ContractFactory and ContractManager, to get the list of ctor arg types (e.g "uint256,uint256")
 * - isContractRegistered<TContract>() -> By ContractFactory and ContractManager, to check if the contract is registered
 * The remaining functions and mappings are accessed for JSON ABI purposes only
 * TODO: Add support for overloaded methods! This will require a change in the mappings and templates...
 */
namespace ContractReflectionInterface {
  // All declared in the cpp file.
  extern std::unordered_map<std::string, bool> contractsMap;
  extern std::unordered_map<std::string, std::vector<std::string>> ctorArgNamesMap;
  extern std::unordered_map<std::string, std::unordered_multimap<std::string, ABI::MethodDescription>> methodDescsMap;
  extern std::unordered_map<std::string,
    std::pair<bool, std::vector<std::tuple<std::string, std::string, bool>>>
  > eventsMap;

  /**
   * Helper struct to extract arguments from a function pointer.
   * We need multiple helpers because the function can have no arguments at all.
   * A function can also be <Args... const> or <Args...>, and the function itself can be const.
   * In total there are 6 combinations.
   * Getter functions are static because it is a struct.
   */
  template <typename T> struct populateMethodTypesMapHelper;

  /**
   * Specialization of populateMethodTypesMapHelper for non-const functions without args.
   * @tparam TContract The contract class type.
   * @tparam R The return type.
   */
  template <typename TContract, typename R>
  struct populateMethodTypesMapHelper<R(TContract::*)()> {
    using ReturnType = R; ///< Return type.
    using ClassType = TContract; ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() { return {}; }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
  };

  /**
   * Specialization of populateMethodTypesMapHelper for const functions without args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   */
  template <typename TContract, typename R>
  struct populateMethodTypesMapHelper<R(TContract::*)() const> {
    using ReturnType = R; ///< Return type.
    using ClassType = TContract; ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() { return {}; }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
  };

  /**
   * Specialization of populateMethodTypesMapHelper for non-const functions with non-const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   */
  template <typename TContract, typename R, typename... Args>
  struct populateMethodTypesMapHelper<R(TContract::*)(Args...)> {
    using ReturnType = R; ///< Return type.
    using ClassType = TContract; ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() {
      return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
    }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
  };

  /**
   * Specialization of populateMethodTypesMapHelper for const functions with non-const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   */
  template <typename TContract, typename R, typename... Args>
  struct populateMethodTypesMapHelper<R(TContract::*)(Args...) const> {
    using ReturnType = R; ///< Return type.
    using ClassType = TContract; ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() {
      return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
    }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
  };

  /**
   * Specialization of populateMethodTypesMapHelper for non-const functions with const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   */
  template <typename TContract, typename R, typename... Args>
  struct populateMethodTypesMapHelper<R(TContract::*)(const Args&...)> {
    using ReturnType = R; ///< Return type.
    using ClassType = TContract; ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() {
      return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
    }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
  };

  /**
   * Specialization of populateMethodTypesMapHelper for const functions with const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   */
  template <typename TContract, typename R, typename... Args>
  struct populateMethodTypesMapHelper<R(TContract::*)(const Args&...) const> {
    using ReturnType = R; ///< Return type.
    using ClassType = TContract; ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() {
      return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
    }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
  };

  /**
   * Populate the constructor argument names map.
   * @tparam TContract The contract type.
   * @param name The method's name.
   * @param mut The method's mutability.
   * @param args The method's arguments.
   * @param argsNames The method's argument names.
   * @param rets The method's return types.
   */
  template <typename TContract> void inline populateMethodTypesMap(
    const std::string& name,
    const FunctionTypes& mut,
    const std::vector<std::string>& args,
    const std::vector<std::string>& argsNames,
    const std::vector<std::string>& rets
  ) {
    ABI::MethodDescription desc;
    desc.name = name;
    for (uint64_t i = 0; i < args.size(); i++) {
      std::pair<std::string, std::string> argDesc;
      argDesc.first = args[i];
      argDesc.second = (argsNames.size() > i) ? argsNames[i] : "";
      desc.inputs.push_back(argDesc);
    }
    desc.outputs = rets;
    desc.stateMutability = mut;
    desc.type = "function";
    methodDescsMap[Utils::getRealTypeName<TContract>()].insert(std::make_pair(name, desc));
  }

  /**
   * Check if a contract is already registered in the map.
   * @tparam TContract The contract to check.
   * @return `true` if the contract exists in the map, `false` otherwise.
   */
  template <typename TContract> bool isContractRegistered() {
    // TODO: shouldn't we be checking contractsMap.second here? (the bool that says the contract is actually registered)
    return contractsMap.contains(Utils::getRealTypeName<TContract>());
  }

  /**
   * Register a contract class.
   * methods is a std::tuple<std::string, FunctionPointer, std::string, std::vector<std::string>>, where:
   * - std::get<0>(methods) = Method name
   * - std::get<1>(methods) = Method pointer
   * - std::get<2>(methods) = Method mutability
   * - std::get<3>(methods) = Method argument names
   * @tparam TContract The contract class to register.
   * @tparam Args The constructor argument types.
   * @tparam Methods The methods to register.
   * @param ctorArgs The constructor argument names.
   * @param methods The methods to register.
   */
  template <typename TContract, typename... Args, typename... Methods>
  void inline registerContract(const std::vector<std::string>& ctorArgs, Methods&&... methods) {
    if (isContractRegistered<TContract>()) return; // Skip if contract is already registered
    ctorArgNamesMap[Utils::getRealTypeName<TContract>()] = ctorArgs; // Store ctor arg names in ctorArgNamesMap
    // Register the methods
    ((populateMethodTypesMap<TContract>(
      std::get<0>(methods),
      std::get<2>(methods),
      populateMethodTypesMapHelper<std::decay_t<decltype(std::get<1>(methods))>>::getFunctionArgs(),
      std::get<3>(methods),
      populateMethodTypesMapHelper<std::decay_t<decltype(std::get<1>(methods))>>::getFunctionReturnTypes()
    )), ...);
    contractsMap[Utils::getRealTypeName<TContract>()] = true;
  }

  /**
   * Register a contract's events.
   * @tparam TContract The contract class to register events from.
   * @tparam Events The events to register.
   * @param events The events to register.
   */
  template <typename TContract, typename... Events>
  void inline registerContractEvents(Events&&... events) {
    std::string contractName = typeid(TContract).name();
    for (const std::tuple<std::string, bool, std::vector<
      std::tuple<std::string, std::string, bool>
    >> t : {events...}) {
      // Get details for each event and register in the map - 0 = name, 1 = anonymous, 2 = args
      eventsMap[contractName + "." + std::get<0>(t)] = std::make_pair(std::get<1>(t), std::get<2>(t));
    }
  }

  /**
   * Get the list of constructor argument types of a contract.
   * @tparam Contract The contract to get the constructor argument types of.
   * @return The list of constructor arguments in string format (same as ABI::Functor::listArgumentTypes).
   */
  template <typename TContract> std::string inline getConstructorArgumentTypesString() {
    using ConstructorArgs = typename TContract::ConstructorArguments;
    std::string ret = ABI::FunctorEncoder::listArgumentTypes<ConstructorArgs>();
    // TContract::ConstructorArguments is a tuple, so we need to remove the "(...)" from the string
    ret.erase(0,1);
    ret.pop_back();
    return ret;
  }

  /**
   * Get the constructor ABI data structure of a contract.
   * @tparam Contract The contract to get the constructor argument names of.
   * @return The constructor ABI data structure.
   */
  template <typename Contract> ABI::MethodDescription inline getConstructorDataStructure() {
    if (!isContractRegistered<Contract>()) throw std::runtime_error(
      "Contract " + Utils::getRealTypeName<Contract>() + " not registered"
    );
    // Derive from Contract::ConstructorArguments to get the constructor
    auto ctorArgs = ABI::FunctorEncoder::listArgumentTypesVFromTuple<typename Contract::ConstructorArguments>();
    auto ctorArgsNames = ctorArgNamesMap[Utils::getRealTypeName<Contract>()];
    if (ctorArgs.size() != ctorArgsNames.size()) throw std::runtime_error(
      "Contract " + Utils::getRealTypeName<Contract>() + " constructor argument names not registered, wanted: " +
      std::to_string(ctorArgs.size()) + " got: " + std::to_string(ctorArgsNames.size())
    );
    std::vector<std::pair<std::string,std::string>> ctorArgsDesc;
    for (uint64_t i = 0; i < ctorArgs.size(); i++) {
      ctorArgsDesc.push_back({ctorArgs[i], ctorArgsNames[i]});
    }
    // Create the description
    ABI::MethodDescription ctorDesc;
    ctorDesc.name = "createNew" + Utils::getRealTypeName<Contract>() + "Contract";
    ctorDesc.inputs = ctorArgsDesc;
    ctorDesc.outputs = {};
    ctorDesc.stateMutability = FunctionTypes::NonPayable;
    ctorDesc.type = "function";
    return ctorDesc;
  }

  /**
   * Get the functions ABI data structure of a contract.
   * @tparam Contract The contract to get the function data structure of.
   * @return The function ABI data structure.
   */
  template <typename Contract> std::vector<ABI::MethodDescription> inline getFunctionsDataStructure() {
    if (!isContractRegistered<Contract>()) throw std::runtime_error(
      "Contract " + Utils::getRealTypeName<Contract>() + " not registered"
    );
    std::vector<ABI::MethodDescription> descriptions;
    for (const auto& [name, desc] : methodDescsMap[Utils::getRealTypeName<Contract>()]) {
      descriptions.push_back(desc);
    }
    return descriptions;
  }

  /**
   * Get the events ABI data structure of the contract.
   * @tparam Contract The contract to get the event data structure of.
   * @return The event ABI data structure.
   */
  template <typename Contract>
  std::vector<ABI::EventDescription> inline getEventsDataStructure() {
    if (!isContractRegistered<Contract>()) throw std::runtime_error(
      "Contract " + Utils::getRealTypeName<Contract>() + " not registered"
    );
    std::vector<ABI::EventDescription> ret;
    for (const auto& it : eventsMap) {
      ABI::EventDescription eventDesc;
      std::string contractName = it.first.substr(0, it.first.find("."));
      if (contractName == typeid(Contract).name()) {
        eventDesc.name = it.first.substr(it.first.find(".") + 1); // Remove "."
        eventDesc.anonymous = it.second.first;
        eventDesc.args = it.second.second;
        ret.push_back(eventDesc);
      }
    }
    return ret;
  }
} // namespace ContractReflectionInterface

#endif // CONTRACTREFLECTIONINTERFACE_H
