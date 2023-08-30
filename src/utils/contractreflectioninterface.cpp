/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractreflectioninterface.h"

namespace ContractReflectionInterface {
  std::unordered_map<std::string, std::string> methodMutabilityMap; ///< Map of method names to mutability.
  std::unordered_map<std::string, std::vector<std::string>> constructorArgumentNamesMap; ///< Map of constructor argument names.
  std::unordered_map<std::string, std::vector<std::string>> argumentNamesMap; ///< Map of method argument names.
  std::unordered_map<std::string, std::vector<std::string>> methodArgumentsTypesMap; ///< Map of method argument types.
  std::unordered_map<meta::any_type, ABI::Types> typeMap; ///< Map of types to ABI types.
}
