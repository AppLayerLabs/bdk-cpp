#include "contractreflectioninterface.h"

namespace ContractReflectionInterface {
  std::unordered_map<std::string, std::string> methodMutabilityMap; ///< Map of method names to mutability.
  std::unordered_map<std::string, std::vector<std::string>> constructorArgumentNamesMap; ///< Map of constructor argument names.
  std::unordered_map<std::string, std::vector<std::string>> argumentNamesMap; ///< Map of method argument names.
  std::unordered_map<std::string, std::vector<std::string>> methodArgumentsTypesMap; ///< Map of method argument types.
}
