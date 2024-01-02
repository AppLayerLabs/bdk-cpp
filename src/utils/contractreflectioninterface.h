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
 * This namespace contains the reflection interface for the contract
 * classes.
 * Observations about ContractReflectionsInterface
 * Only the following functions are used in normal operation
 * registerContract() -> By the derived DynamicContract class, to register the contract class methods, arguments, etc
 * getConstructorArgumentTypesString<TContract>() -> By ContractFactory and ContractManager, to get the list of constructor argument types .e.g "uint256,uint256"
 * getMethodMutability<TContract>(methodName) -> By the DynamicContract* class, after derived class calling registerMemberFunction(), to get the mutability of the method
 * isContractRegistered<TContract>() -> By ContractFactory and ContractManager, to check if the contract is registered
 * The remaining functions and mapping are accessed for JSON ABI purposes only
 * TODO: Add support for overloaded methods! This will require a change in the mappings and templates...
 */

namespace ContractReflectionInterface {

/// Key (ClassName) -> Value (boolean) (true if registered, false otherwise)
extern std::unordered_map<std::string, bool> registeredContractsMap; ///< Map of registered contracts.
/// Key (ClassName) -> Value (std::vector<std::string>) (ConstructorArgumentNames)
extern std::unordered_map<std::string, std::vector<std::string>> constructorArgumentNamesMap; /// Map to store constructor argument names
/// Key (ClassName) -> Value (std::unordered_set<ABI::MethodDescription>) (MethodDescriptions) methodDescriptionsMap has all the information needed to generate the JSON ABI
extern std::unordered_map<std::string, std::unordered_map<std::string, ABI::MethodDescription>> methodDescriptionsMap; ///< Map to store method descriptions.

/** Helper struct to extract the Args... from a function pointer
 * We need multiple helper functions because the function can have no arguments at all
 * A function can also be <Args... const> or <Args...>
 * And the function itself can be const.
 * In total there are 6 combinations.
 */

template <typename T>
struct populateMethodTypesMapHelper;

/**
 * Helper function to extract the Args... and R into a solidity ABI type list from a function pointers
 * Specialization for non-const functions without args
 * @tparam TContract The contract type.
 * @tparam R The return type.
 */
template <typename TContract, typename R>
struct populateMethodTypesMapHelper<R(TContract::*)()> {
  /// The return type.
  using ReturnType = R;
  /// The class type.
  using ClassType = TContract;
  /// Static function because it is a struct, to get the function arguments
  static std::vector<std::string> getFunctionArgs() {
    return {};
  }
  /// Static function because it is a struct, to get the function return types
  static std::vector<std::string> getFunctionRets() {
    return ABI::FunctorEncoder::listArgumentTypesV<R>();
  }
};

/**
 * Helper function to extract the Args... and R into a solidity ABI type list from a function pointers
 * Specialization for const functions without args
 * @tparam TContract The contract type.
 * @tparam R The return type.
 */
template <typename TContract, typename R>
struct populateMethodTypesMapHelper<R(TContract::*)() const> {
  /// The return type.
  using ReturnType = R;
  /// The class type.
  using ClassType = TContract;
  /// Static function because it is a struct, to get the function arguments
  static std::vector<std::string> getFunctionArgs() {
    return {};
  }
  /// Static function because it is a struct, to get the function return types
  static std::vector<std::string> getFunctionRets() {
    return ABI::FunctorEncoder::listArgumentTypesV<R>();
  }
};

/**
 * Helper function to extract the Args... and R into a solidity ABI type list from a function pointers
 * Specialization for non-const functions with args
 * @tparam TContract The contract type.
 * @tparam R The return type.
 * @tparam Args The argument types.
 */
template <typename TContract, typename R, typename... Args>
struct populateMethodTypesMapHelper<R(TContract::*)(Args...)> {
  /// The return type.
  using ReturnType = R;
  /// The class type.
  using ClassType = TContract;
  /// Static function because it is a struct, to get the function arguments
  static std::vector<std::string> getFunctionArgs() {
    return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
  }
  /// Static function because it is a struct, to get the function return types
  static std::vector<std::string> getFunctionRets() {
    return ABI::FunctorEncoder::listArgumentTypesV<R>();
  }
};

/**
 * Helper function to extract the Args... and R into a solidity ABI type list from a function pointers
 * Specialization for const functions with args
 * @tparam TContract The contract type.
 * @tparam R The return type.
 * @tparam Args The argument types.
 */
template <typename TContract, typename R, typename... Args>
struct populateMethodTypesMapHelper<R(TContract::*)(Args...) const> {
  /// The return type.
  using ReturnType = R;
  /// The class type.
  using ClassType = TContract;
  /// Static function because it is a struct, to get the function arguments
  static std::vector<std::string> getFunctionArgs() {
    return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
  }
  /// Static function because it is a struct, to get the function return types
  static std::vector<std::string> getFunctionRets() {
    return ABI::FunctorEncoder::listArgumentTypesV<R>();
  }
};

/**
 * Helper function to extract the Args... and R into a solidity ABI type list from a function pointers
 * Specialization for non-const functions with const args
 * @tparam TContract The contract type.
 * @tparam R The return type.
 * @tparam Args The argument types.
 */
template <typename TContract, typename R, typename... Args>
struct populateMethodTypesMapHelper<R(TContract::*)(const Args&...)> {
  /// The return type.
  using ReturnType = R;
  /// The class type.
  using ClassType = TContract;
  /// Static function because it is a struct, to get the function arguments
  static std::vector<std::string> getFunctionArgs() {
    return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
  }
  /// Static function because it is a struct, to get the function return types
  static std::vector<std::string> getFunctionRets() {
    return ABI::FunctorEncoder::listArgumentTypesV<R>();
  }
};

/**
 * Helper function to extract the Args... and R into a solidity ABI type list from a function pointers
 * Specialization for const functions with const args
 * @tparam TContract The contract type.
 * @tparam R The return type.
 * @tparam Args The argument types.
 */
template <typename TContract, typename R, typename... Args>
struct populateMethodTypesMapHelper<R(TContract::*)(const Args&...) const> {
  /// The return type.
  using ReturnType = R;
  /// The class type.
  using ClassType = TContract;
  /// Static function because it is a struct, to get the function arguments
  static std::vector<std::string> getFunctionArgs() {
    return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
  }
  /// Static function because it is a struct, to get the function return types
  static std::vector<std::string> getFunctionRets() {
    return ABI::FunctorEncoder::listArgumentTypesV<R>();
  }
};

/**
 * This function populates the constructor argument names map.
 * @tparam TContract The contract type.
 */
template <typename TContract>
void inline populateMethodTypesMap(const std::string& functionName,
                                   const std::string& methodMutability,
                                   const std::vector<std::string>& functionArgs,
                                   const std::vector<std::string>& functionArgsNames,
                                   const std::vector<std::string>& funcRets) {
  std::string contractName = Utils::getRealTypeName<TContract>();
  ABI::MethodDescription methodDescription;
  methodDescription.name = functionName;
  for (uint64_t i = 0; i < functionArgs.size(); i++) {
    std::pair<std::string, std::string> argumentDescription;
    argumentDescription.first = functionArgs[i];
    argumentDescription.second = (functionArgsNames.size() > i) ? functionArgsNames[i] : "";
    methodDescription.inputs.push_back(argumentDescription);
  }
  methodDescription.outputs = funcRets;
  methodDescription.stateMutability = methodMutability;
  methodDescription.type = "function";
  methodDescriptionsMap[contractName][functionName] = methodDescription;
}

/**
 * Template function to get the constructor data structure of a contract.
 * @tparam TContract The contract to get the constructor data structure of.
 * @return The constructor data structure in ABI format.
 */
template <typename TContract> bool isContractRegistered() {
  return registeredContractsMap.contains(Utils::getRealTypeName<TContract>());
}

/**
 * Template function to register a contract class.
 * @tparam TContract The contract class to register.
 * @tparam Args The constructor argument types.
 * @tparam Methods The methods to register.
 * @param ctorArgs The constructor argument names.
 * @param methods The methods to register.
 * methods = std::tuple<std::string, FunctionPointer, std::string, std::vector<std::string>>
 * std::get<0>(methods) = Method name
 * std::get<1>(methods) = Method pointer
 * std::get<2>(methods) = Method mutability
 * std::get<3>(methods) = Method argument names
 */
template <typename TContract, typename... Args, typename... Methods>
void inline registerContract(const std::vector<std::string> &ctorArgs,
                             Methods &&...methods) {
  if (isContractRegistered<TContract>()) {
    /// Already registered, do nothing
    return;
  }
  std::string contractName = Utils::getRealTypeName<TContract>();
  // Store constructor argument names in the constructorArgumenqtNamesMap
  constructorArgumentNamesMap[contractName] = ctorArgs;


  // Register the methods
  ((populateMethodTypesMap<TContract>(std::get<0>(methods),
    std::get<2>(methods),
    populateMethodTypesMapHelper<std::decay_t<decltype(std::get<1>(methods))>>::getFunctionArgs(),
    std::get<3>(methods),
    populateMethodTypesMapHelper<std::decay_t<decltype(std::get<1>(methods))>>::getFunctionRets())), ...);

  registeredContractsMap[Utils::getRealTypeName<TContract>()] = true;
}

/**
* Template function to get the list of constructor argument types of a
* contract.
* @tparam Contract The contract to get the constructor argument types of.
* @return The list of constructor argument in string format. (same as ABI::Functor::listArgumentTypes)
*/
template <typename TContract>
std::string inline getConstructorArgumentTypesString() {
  using ConstructorArgs = typename TContract::ConstructorArguments;
  std::string ret = ABI::FunctorEncoder::listArgumentTypes<ConstructorArgs>();
  // TContract::ConstructorArguments is a tuple, so we need to remove the "(...)" from the string
  ret.erase(0,1);
  ret.pop_back();
  return ret;
}

/**
 * Template function to get the constructor ABI data structure of a contract.
 * @tparam Contract The contract to get the constructor argument names of.
 * @return The constructor ABI data structure.
 */
template <typename Contract>
ABI::MethodDescription inline getConstructorDataStructure() {
  if (!isContractRegistered<Contract>()) {
    throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " not registered");
  }
  /// Derive from Contract::ConstructorArguments to get the constructor
  ABI::MethodDescription constructorDescription;
  auto constructorArgs = ABI::FunctorEncoder::listArgumentTypesVFromTuple<typename Contract::ConstructorArguments>();
  auto constructorArgsNames = constructorArgumentNamesMap[Utils::getRealTypeName<Contract>()];
  if (constructorArgs.size() != constructorArgsNames.size()) {
    throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " constructor argument names not registered, wanted: " +
    std::to_string(constructorArgs.size()) + " got: " + std::to_string(constructorArgsNames.size()));
  }
  std::vector<std::pair<std::string,std::string>> constructorArgsDescription;
  for (uint64_t i = 0; i < constructorArgs.size(); i++) {
    constructorArgsDescription.push_back({constructorArgs[i], constructorArgsNames[i]});
  }

  constructorDescription.name = "createNew" + Utils::getRealTypeName<Contract>() + "Contract";
  constructorDescription.inputs = constructorArgsDescription;
  constructorDescription.outputs = {};
  constructorDescription.stateMutability = "nonpayable";
  constructorDescription.type = "function";
  return constructorDescription;
}

/**
 * Template function to get the function ABI data structure of a contract.
 * @tparam Contract The contract to get the function data structure of.
 * @return The function ABI data structure.
 */
template <typename Contract>
std::vector<ABI::MethodDescription> inline getFunctionsDataStructure() {
  if (!isContractRegistered<Contract>()) {
    throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " not registered");
  }
  std::vector<ABI::MethodDescription> descriptions;
  for (const auto& [methodName, methodDescription] : methodDescriptionsMap[Utils::getRealTypeName<Contract>()]) {
    descriptions.push_back(methodDescription);
  }
  return descriptions;
}

/**
 * Getter for the mutability of a method.
  * @param methodName The name of the method to get the mutability of.
  * @return The mutability of the method.
  */
template <typename Contract>
std::string inline getMethodMutability(const std::string& methodName) {
  if (!isContractRegistered<Contract>()) {
    throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " not registered");
  }
  std::string contractName = Utils::getRealTypeName<Contract>();
  auto cIt = methodDescriptionsMap.find(contractName);
  if (cIt != methodDescriptionsMap.end()) {
    const auto& methodMaps = cIt->second;
    /// Construct a empty method description with the name of the method
    auto mIt = methodMaps.find(methodName);
    if (mIt != methodMaps.end()) {
        return mIt->second.stateMutability;
    }
  }
  throw std::runtime_error("Method " + contractName + "::" + methodName + " not found");
}

} // namespace ContractReflectionInterface

#endif // CONTRACTREFLECTIONINTERFACE_H