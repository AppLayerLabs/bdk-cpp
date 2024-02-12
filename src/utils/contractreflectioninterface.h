/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACTREFLECTIONINTERFACE_H
#define CONTRACTREFLECTIONINTERFACE_H

#include "contract/abi.h"
#include <unordered_set>

/**
 * Namespace for the reflection interface used for registering contract classes.
 * Only the following functions are used in normal operation (other functions
 * and mappings are accessed for JsonAbi generation purposes only):
 * - registerContract() -> By the derived DynamicContract class, to register contract methods, arguments, etc.
 * - getConstructorArgumentTypesString<TContract>() -> By ContractFactory and ContractManager, to get the list of ctor arg types (e.g "uint256,uint256")
 * - isContractFunctionRegistered<TContract>() -> By ContractFactory and ContractManager, to check if the contract is registered
 */
namespace ContractReflectionInterface {
  // TODO: Add support for overloaded methods! This will require a change in the mappings and templates...
  /**
   * Unique identifier for a pointer to member function.
   * Used to derive the name of a function from a pointer to member function.
   */
  class UniqueFunctionPointerIdentifier {
    // TODO: Not sure if this is undefined behavior or not, but it works
    private:
      std::string returnType; ///< The function's return type.
      std::string className;  ///< The name of the class that contains the function.
      uint64_t ptr1 = 0;      ///< Copy of the first half of the function pointer's memory content.
      uint64_t ptr2 = 0;      ///< Copy of the second half of the function pointer's memory content.

    public:
      /// Specialization for non-const functions without args.
      template <typename TContract, typename R>
      explicit UniqueFunctionPointerIdentifier(R(TContract::*func)()) {
        this->returnType = Utils::getRealTypeName<R>();
        this->className = Utils::getRealTypeName<TContract>();
        // Check if sizeof func is sizeof(uint64_t) * 2
        static_assert(sizeof(func) == sizeof(uint64_t) * 2, "Function pointer size is not 16 bytes");
        // Copy the function pointer into the two uint64_t
        memcpy(&ptr1, &func, sizeof(uint64_t));
        memcpy(&ptr2, ((uint64_t*)&func) + 1, sizeof(uint64_t));
      }

      /// Specialization for const functions without args.
      template <typename TContract, typename R>
      explicit UniqueFunctionPointerIdentifier(R(TContract::*func)() const) {
        this->returnType = Utils::getRealTypeName<R>();
        this->className = Utils::getRealTypeName<TContract>();
        // Check if sizeof func is sizeof(uint64_t) * 2
        static_assert(sizeof(func) == sizeof(uint64_t) * 2, "Function pointer size is not 16 bytes");
        // Copy the function pointer into the two uint64_t
        memcpy(&ptr1, &func, sizeof(uint64_t));
        memcpy(&ptr2, ((uint64_t*)&func) + 1, sizeof(uint64_t));
      }

      /// Specialization for non-const functions with non-const args.
      template <typename TContract, typename R, typename... Args>
      explicit UniqueFunctionPointerIdentifier(R(TContract::*func)(Args...)) {
        this->returnType = Utils::getRealTypeName<R>();
        this->className = Utils::getRealTypeName<TContract>();
        // Check if sizeof func is sizeof(uint64_t) * 2
        static_assert(sizeof(func) == sizeof(uint64_t) * 2, "Function pointer size is not 16 bytes");
        // Copy the function pointer into the two uint64_t
        memcpy(&ptr1, &func, sizeof(uint64_t));
        memcpy(&ptr2, ((uint64_t*)&func) + 1, sizeof(uint64_t));
      }

      /// Specialization for const functions with non-const args.
      template <typename TContract, typename R, typename... Args>
      explicit UniqueFunctionPointerIdentifier(R(TContract::*func)(Args...) const) {
        this->returnType = Utils::getRealTypeName<R>();
        this->className = Utils::getRealTypeName<TContract>();
        // Check if sizeof func is sizeof(uint64_t) * 2
        static_assert(sizeof(func) == sizeof(uint64_t) * 2, "Function pointer size is not 16 bytes");
        // Copy the function pointer into the two uint64_t
        memcpy(&ptr1, &func, sizeof(uint64_t));
        memcpy(&ptr2, ((uint64_t*)&func) + 1, sizeof(uint64_t));
      }

      /// Specialization for non-const functions with const args.
      template <typename TContract, typename R, typename... Args>
      explicit UniqueFunctionPointerIdentifier(R(TContract::*func)(const Args&...)) {
        this->returnType = Utils::getRealTypeName<R>();
        this->className = Utils::getRealTypeName<TContract>();
        // Check if sizeof func is sizeof(uint64_t) * 2
        static_assert(sizeof(func) == sizeof(uint64_t) * 2, "Function pointer size is not 16 bytes");
        // Copy the function pointer into the two uint64_t
        memcpy(&ptr1, &func, sizeof(uint64_t));
        memcpy(&ptr2, ((uint64_t*)&func) + 1, sizeof(uint64_t));
      }

      /// Specialization for const functions with const args.
      template <typename TContract, typename R, typename... Args>
      explicit UniqueFunctionPointerIdentifier(R(TContract::*func)(const Args&...) const) {
        this->returnType = Utils::getRealTypeName<R>();
        this->className = Utils::getRealTypeName<TContract>();
        // Check if sizeof func is sizeof(uint64_t) * 2
        static_assert(sizeof(func) == sizeof(uint64_t) * 2, "Function pointer size is not 16 bytes");
        // Copy the function pointer into the two uint64_t
        memcpy(&ptr1, &func, sizeof(uint64_t));
        memcpy(&ptr2, ((uint64_t*)&func) + 1, sizeof(uint64_t));
      }

      /// Equality operator. Checks both memory halves, class name and return type.
      bool operator==(const UniqueFunctionPointerIdentifier& other) const {
        return ptr1 == other.ptr1 && ptr2 == other.ptr2 && className == other.className && returnType == other.returnType;
      }

      /// Helper struct for hashing a function pointer.
      struct Hash {
        /// Function operator.
        std::size_t operator()(const UniqueFunctionPointerIdentifier& fpw) const {
          return std::hash<uint64_t>{}(fpw.ptr1) ^ std::hash<uint64_t>{}(fpw.ptr2) ^ std::hash<std::string>{}(fpw.className) ^ std::hash<std::string>{}(fpw.returnType);
        }
      };
  };

  // All declared in the cpp file.
  extern std::unordered_map<std::string, bool> registeredContractsFunctionsMap;
  extern std::unordered_map<std::string, bool> registeredContractsEventsMap;
  extern std::unordered_map<std::string, std::vector<std::string>> ctorArgNamesMap;
  extern std::unordered_map<std::string, std::unordered_multimap<std::string, ABI::MethodDescription>> methodDescsMap;
  extern std::unordered_map<std::string, std::unordered_multimap<std::string, ABI::EventDescription>> eventDescsMap;
  extern std::unordered_map<UniqueFunctionPointerIdentifier, std::string, UniqueFunctionPointerIdentifier::Hash> pointerNamesMap;

  /**
   * Helper struct to extract arguments from a function pointer.
   * We need multiple helpers because the function can have no arguments at all.
   * A function can also be <Args... const> or <Args...>, and the function itself can be const.
   * In total there are 6 combinations.
   * Getter functions are static because it is a struct.
   */
  template <typename T> struct populateMethodTypesMapHelper;

  /**
   * Helper struct to extract event arguments from a function pointer.
   * This differs from populateMethodTypesMapHelper because events have EventParam arguments.
   */
  template <typename T> struct populateEventTypesMapHelper;

  /**
   * Specialization for non-const functions without args.
   * @tparam TContract The contract class type.
   * @tparam R The return type.
   */
  template <typename TContract, typename R>
  struct populateMethodTypesMapHelper<R(TContract::*)()> {
    using ReturnType = R;         ///< Return type.
    using ClassType = TContract;  ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() { return {}; }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
    /// Get UniqueFunctionPointerIdentifier.
    static UniqueFunctionPointerIdentifier getUniqueFunctionPointerIdentifier(R(TContract::*func)()) {
      return UniqueFunctionPointerIdentifier(func);
    }
  };

  /**
   * Specialization for const functions without args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   */
  template <typename TContract, typename R>
  struct populateMethodTypesMapHelper<R(TContract::*)() const> {
    using ReturnType = R;         ///< Return type.
    using ClassType = TContract;  ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() { return {}; }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
    /// Get UniqueFunctionPointerIdentifier.
    static UniqueFunctionPointerIdentifier getUniqueFunctionPointerIdentifier(R(TContract::*func)() const) {
      return UniqueFunctionPointerIdentifier(func);
    }
  };

  /**
   * Specialization for non-const functions with non-const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   */
  template <typename TContract, typename R, typename... Args>
  struct populateMethodTypesMapHelper<R(TContract::*)(Args...)> {
    using ReturnType = R;         ///< Return type.
    using ClassType = TContract;  ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() {
      return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
    }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
    /// Get UniqueFunctionPointerIdentifier.
    static UniqueFunctionPointerIdentifier getUniqueFunctionPointerIdentifier(R(TContract::*func)(Args...)) {
      return UniqueFunctionPointerIdentifier(func);
    }
  };

  /**
   * Specialization for const functions with non-const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   */
  template <typename TContract, typename R, typename... Args>
  struct populateMethodTypesMapHelper<R(TContract::*)(Args...) const> {
    using ReturnType = R;         ///< Return type.
    using ClassType = TContract;  ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() {
      return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
    }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
    /// Get UniqueFunctionPointerIdentifier.
    static UniqueFunctionPointerIdentifier getUniqueFunctionPointerIdentifier(R(TContract::*func)(Args...) const) {
      return UniqueFunctionPointerIdentifier(func);
    }
  };

  /**
   * Specialization for non-const functions with const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   */
  template <typename TContract, typename R, typename... Args>
  struct populateMethodTypesMapHelper<R(TContract::*)(const Args&...)> {
    using ReturnType = R;         ///< Return type.
    using ClassType = TContract;  ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() {
      return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
    }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
    /// Get UniqueFunctionPointerIdentifier.
    static UniqueFunctionPointerIdentifier getUniqueFunctionPointerIdentifier(R(TContract::*func)(const Args&...)) {
      return UniqueFunctionPointerIdentifier(func);
    }
  };

  /**
   * Specialization for const functions with const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   */
  template <typename TContract, typename R, typename... Args>
  struct populateMethodTypesMapHelper<R(TContract::*)(const Args&...) const> {
    using ReturnType = R;         ///< Return type.
    using ClassType = TContract;  ///< Class type, derived from the contract class.
    /// Get the function arguments.
    static std::vector<std::string> getFunctionArgs() {
      return ABI::FunctorEncoder::listArgumentTypesV<Args...>();
    }
    /// Get the function return types.
    static std::vector<std::string> getFunctionReturnTypes() {
      return ABI::FunctorEncoder::listArgumentTypesV<R>();
    }
    /// Get UniqueFunctionPointerIdentifier.
    static UniqueFunctionPointerIdentifier getUniqueFunctionPointerIdentifier(R(TContract::*func)(const Args&...) const) {
      return UniqueFunctionPointerIdentifier(func);
    }
  };

  /**
   * Specialization for void functions with any number of EventParam arguments.
   * @tparam TContract The contract type.
   * @tparam Args The argument types.
   * @tparam Flags The argument indexed flags.
   */
  template <typename TContract, typename... Args, bool... Flags>
  struct populateEventTypesMapHelper<void(TContract::*)(const EventParam<Args, Flags>&...)> {
    using ReturnType = void;      ///< Return type.
    using ClassType = TContract;  ///< Class type, derived from the contract class.
    /// Get the event arguments, and whether they are indexed or not.
    static std::vector<std::pair<std::string, bool>> getArgs() {
      return ABI::FunctorEncoder::listEventTypesV<EventParam<Args, Flags>...>::get();
    }
    /// Get UniqueFunctionPointerIdentifier.
    static UniqueFunctionPointerIdentifier getUniqueFunctionPointerIdentifier(void(TContract::*func)(const EventParam<Args, Flags>&...)) {
      return UniqueFunctionPointerIdentifier(func);
    }
  };

  /**
   * Populate the method types map.
   * @tparam TContract The contract type.
   * @param name The method's name.
   * @param mut The method's mutability.
   * @param func The unique function pointer identifier.
   * @param args The method's arguments.
   * @param argsNames The method's argument names.
   * @param rets The method's return types.
   */
  template <typename TContract> void inline populateMethodTypesMap(
    const std::string& name,
    const FunctionTypes& mut,
    const UniqueFunctionPointerIdentifier& func,
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
    pointerNamesMap[func] = name;
  }

  /**
   * Populate the event types map.
   * @param name The event's name.
   * @param anonymous Whether the event is anonymous or not.
   * @param func The unique function pointer identifier.
   * @param args The event's arguments.
   * @param argsNames The event's argument names.
   */
  template <typename TContract> void inline populateEventTypesMap(
    const std::string& name,
    bool anonymous,
    const UniqueFunctionPointerIdentifier& func,
    std::vector<std::pair<std::string, bool>> args,
    std::vector<std::string> argsNames
  ) {
    ABI::EventDescription desc;
    desc.name = name;
    desc.anonymous = anonymous;
    for (uint64_t i = 0; i < args.size(); i++) {
      std::tuple<std::string, std::string, bool> argDesc;
      auto& [argType, argName, argIndexed] = argDesc;
      argType = args[i].first;
      argIndexed = args[i].second;
      argName = (argsNames.size() > i) ? argsNames[i] : "";
      desc.args.push_back(argDesc);
    }
    eventDescsMap[Utils::getRealTypeName<TContract>()].insert(std::make_pair(name, desc));
    pointerNamesMap[func] = name;
  }

  /**
   * Check if a contract's functions are already registered in the map.
   * @tparam TContract The contract to check.
   * @return `true` if the contract exists in the map, `false` otherwise.
   */
  template <typename TContract> bool isContractFunctionsRegistered() {
    return registeredContractsFunctionsMap.contains(Utils::getRealTypeName<TContract>());
  }

  /**
   * Check if a contract's events are already registered in the map.
   * @tparam TContract The contract to check.
   * @return `true` if the contract exists in the map, `false` otherwise.
   */
  template <typename TContract> bool isContractEventsRegistered() {
    return registeredContractsEventsMap.contains(Utils::getRealTypeName<TContract>());
  }

  /**
   * Register a contract's methods.
   * methods is a std::tuple<std::string, FunctionPointer, std::string, std::vector<std::string>>, where:
   * - std::get<0>(methods) = Method name
   * - std::get<1>(methods) = Method pointer
   * - std::get<2>(methods) = Method mutability
   * - std::get<3>(methods) = Method argument names
   *
   * @tparam TContract The contract class to register.
   * @tparam Args The constructor argument types.
   * @tparam Methods The methods to register.
   * @param ctorArgs The constructor argument names.
   * @param methods The methods to register.
   */
  template <typename TContract, typename... Args, typename... Methods>
  void inline registerContractMethods(const std::vector<std::string>& ctorArgs, Methods&&... methods) {
    if (isContractFunctionsRegistered<TContract>()) return; // Skip if contract is already registered
    ctorArgNamesMap[Utils::getRealTypeName<TContract>()] = ctorArgs; // Store ctor arg names in ctorArgNamesMap
    // Register the methods
    ((populateMethodTypesMap<TContract>(
      std::get<0>(methods),
      std::get<2>(methods),
      populateMethodTypesMapHelper<std::decay_t<decltype(std::get<1>(methods))>>::getUniqueFunctionPointerIdentifier(std::get<1>(methods)),
      populateMethodTypesMapHelper<std::decay_t<decltype(std::get<1>(methods))>>::getFunctionArgs(),
      std::get<3>(methods),
      populateMethodTypesMapHelper<std::decay_t<decltype(std::get<1>(methods))>>::getFunctionReturnTypes()
    )), ...);
    registeredContractsFunctionsMap[Utils::getRealTypeName<TContract>()] = true;
  }

  /**
   * Register a contract's events.
   * @tparam TContract The contract class to register events from.
   * @tparam Events The events to register.
   * @param events The events to register.
   */
  template <typename TContract, typename... Events>
  void inline registerContractEvents(Events&&... events) {
    if (isContractEventsRegistered<TContract>()) return; // Skip if contract is already registered
    std::string contractName = typeid(TContract).name();
    ((populateEventTypesMap<TContract>(
      std::get<0>(events),
      std::get<1>(events),
      populateEventTypesMapHelper<std::decay_t<decltype(std::get<2>(events))>>::getUniqueFunctionPointerIdentifier(std::get<2>(events)),
      populateEventTypesMapHelper<std::decay_t<decltype(std::get<2>(events))>>::getArgs(),
      std::get<3>(events)
    )), ...);
    registeredContractsEventsMap[Utils::getRealTypeName<TContract>()] = true;
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
    if (!isContractFunctionsRegistered<Contract>()) throw DynamicException(
      "Contract " + Utils::getRealTypeName<Contract>() + " not registered"
    );
    // Derive from Contract::ConstructorArguments to get the constructor
    auto ctorArgs = ABI::FunctorEncoder::listArgumentTypesVFromTuple<typename Contract::ConstructorArguments>();
    auto ctorArgsNames = ctorArgNamesMap[Utils::getRealTypeName<Contract>()];
    if (ctorArgs.size() != ctorArgsNames.size()) throw DynamicException(
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
    if (!isContractFunctionsRegistered<Contract>()) throw DynamicException(
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
    if (!isContractFunctionsRegistered<Contract>()) throw DynamicException(
      "Contract " + Utils::getRealTypeName<Contract>() + " not registered"
    );
    std::vector<ABI::EventDescription> descriptions;
    for (const auto& [name, desc] : eventDescsMap[Utils::getRealTypeName<Contract>()]) {
      descriptions.push_back(desc);
    }
    return descriptions;
  }

  ///@{
  /**
   * Get a function name from a pointer to member function.
   * Specialization for functions (const and non-const) without args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @param func The pointer to member function.
   */
  template <typename TContract, typename R>
  std::string inline getFunctionName(R(TContract::*func)()) {
    return pointerNamesMap[UniqueFunctionPointerIdentifier(func)];
  }
  template <typename TContract, typename R>
  std::string inline getFunctionName(R(TContract::*func)() const) {
    return pointerNamesMap[UniqueFunctionPointerIdentifier(func)];
  }
  ///@}

  ///@{
  /**
   * Get a function name from a pointer to member function.
   * Specialization for functions (const and non-const) with non-const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   * @param func The pointer to member function.
   */
  template <typename TContract, typename R, typename... Args>
  std::string inline getFunctionName(R(TContract::*func)(Args...)) {
    return pointerNamesMap[UniqueFunctionPointerIdentifier(func)];
  }
  template<typename TContract, typename R, typename... Args>
  std::string inline getFunctionName(R(TContract::*func)(Args...) const) {
    return pointerNamesMap[UniqueFunctionPointerIdentifier(func)];
  }
  ///@}

  ///@{
  /**
   * Get a function name from a pointer to member function.
   * Specialization for functions (const and non-const) with const args.
   * @tparam TContract The contract type.
   * @tparam R The return type.
   * @tparam Args The argument types.
   * @param func The pointer to member function.
   */
  template <typename TContract, typename R, typename... Args>
  std::string inline getFunctionName(R(TContract::*func)(const Args&...)) {
    return pointerNamesMap[UniqueFunctionPointerIdentifier(func)];
  }
  template <typename TContract, typename R, typename... Args>
  std::string inline getFunctionName(R(TContract::*func)(const Args&...) const) {
    return pointerNamesMap[UniqueFunctionPointerIdentifier(func)];
  }
  ///@}
} // namespace ContractReflectionInterface

#endif // CONTRACTREFLECTIONINTERFACE_H
