/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractreflectioninterface.h"

namespace ContractReflectionInterface {
  std::unordered_map<std::string, bool> registeredContractsMap; ///< Map of registered contracts.
  std::unordered_map<std::string, std::vector<std::string>> constructorArgumentNamesMap; ///< Map of constructor argument names.
  std::unordered_map<std::string, std::unordered_multimap<std::string, ABI::MethodDescription>> methodDescriptionsMap; ///< Map of method descriptions. multimap for overloaded methods.
}
