/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractreflectioninterface.h"

namespace ContractReflectionInterface {
  /**
   * Map of contracts which functions have been registered
   * Key is the contract class name, value is a bool saying whether it is registered or not.
   */
  ankerl::unordered_dense::map<std::string, bool> registeredContractsFunctionsMap;

  /**
   * Map of contracts which events have been registered
   * Key is the contract class name, value is a bool saying whether it is registered or not.
   */
  ankerl::unordered_dense::map<std::string, bool> registeredContractsEventsMap;
  /**
   * Map of contract constructor argument names.
   * Key is the contract class name, value is a list of constructor argument names.
   */
  ankerl::unordered_dense::map<std::string, std::vector<std::string>> ctorArgNamesMap;

  /**
   * Map of contract method descriptions.
   * Key is the contract class name, value is a map of each method's name and description.
   * Multimap is used for overloaded methods.
   */
  ankerl::unordered_dense::map<std::string, std::unordered_multimap<std::string, ABI::MethodDescription>> methodDescsMap;

  /**
   * Map of contract events.
   * Key is the contract name, value is a map of each event's name and description.
   * Multimap is used for overloaded events.
   */
  ankerl::unordered_dense::map<std::string, std::unordered_multimap<std::string, ABI::EventDescription>> eventDescsMap;

  /**
   * Map of pointer to member functions of contract methods.
   * Key is a unique identifier derived from the pointer, value is a std::string of the function name.
   */
  ankerl::unordered_dense::map<UniqueFunctionPointerIdentifier, std::string, UniqueFunctionPointerIdentifier::Hash> pointerNamesMap;
}

