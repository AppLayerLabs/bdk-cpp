/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractreflectioninterface.h"

namespace ContractReflectionInterface {
  /// Map of C++ types to ABI types.
  std::unordered_map<meta::any_type, ABI::Types> typeMap;

  /**
   * Map of method names to mutabilities.
   * Key is the method name, value is the method mutability.
   */
  std::unordered_map<std::string, std::string> methodMutabilityMap;

  /**
   * Map of constructor argument names.
   * Key is the contract's name, value is a list of ctor arg names.
   */
  std::unordered_map<std::string, std::vector<std::string>> constructorArgumentNamesMap;

  /**
   * Map of method argument names.
   * Key is the method's name, value is a list of arg names.
   */
  std::unordered_map<std::string, std::vector<std::string>> argumentNamesMap;

  /**
   * Map of method argument types.
   * Key is the method's fully-scoped name (as in "ContractName::MethodName"), value is a list of arg types.
   */
  std::unordered_map<std::string, std::vector<std::string>> methodArgumentsTypesMap;

  /**
   * Map of events.
   * Key is the contract name + event name separated by a dot (as in "ContractName.EventName").
   * Value is a pair of anonymous flag and a list of tuples for parameters (name, type, indexed flag).
   */
  std::unordered_map<std::string,
    std::pair<bool, std::vector<std::tuple<std::string, std::string, bool>>>
  > eventsMap;
}

