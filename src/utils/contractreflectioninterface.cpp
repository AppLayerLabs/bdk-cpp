/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractreflectioninterface.h"

namespace ContractReflectionInterface {
  /**
   * Map of registered contracts.
   * Key is the contract class name, value is a bool saying whether it is registered or not.
   */
  std::unordered_map<std::string, bool> contractsMap;

  /**
   * Map of contract constructor argument names.
   * Key is the contract class name, value is a list of constructor argument names.
   */
  std::unordered_map<std::string, std::vector<std::string>> ctorArgNamesMap;

  /**
   * Map of contract method descriptions.
   * Key is the contract class name, value is a map of each method's name and description.
   * Multimap is used for overloaded methods.
   */
  std::unordered_map<std::string, std::unordered_multimap<std::string, ABI::MethodDescription>> methodDescsMap;

  /**
   * Map of contract events.
   * Key is the contract name + event name separated by a dot (as in "ContractName.EventName").
   * Value is a pair of anonymous flag and a list of tuples for parameters (name, type, indexed flag).
   */
  std::unordered_map<std::string,
    std::pair<bool, std::vector<std::tuple<std::string, std::string, bool>>>
  > eventsMap;
}

