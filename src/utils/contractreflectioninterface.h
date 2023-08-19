#ifndef CONTRACTREFLECTIONINTERFACE_H
#define CONTRACTREFLECTIONINTERFACE_H

#include "contract/abi.h"
#include "meta_all.hpp"

/**
 * This namespace contains the reflection interface for the contract
 * classes.
 *
 */

namespace ContractReflectionInterface {
namespace meta = meta_hpp;

extern std::unordered_map<std::string, std::vector<std::string>>
    constructorArgumentNamesMap; /// Map to store constructor argument names
extern std::unordered_map<std::string, std::string>
    methodMutabilityMap; //// Map to store method mutability
extern std::unordered_map<std::string, std::vector<std::string>>
    argumentNamesMap; /// Map to store method argument names
extern std::unordered_map<meta::any_type, ABI::Types>
    typeMap; /// Map to store type to ABI type mapping

/**
* Template function to map a type to an ABI type.
* @tparam T The type to map.
* @param map The map to insert the type to ABI type mapping into.
*/
template<typename T>
void insertTypes(std::unordered_map<meta::any_type, ABI::Types>& map)
{
    map[meta::resolve_type<T>()] = ABI::TypeToEnum<T>::value;
    map[meta::resolve_type<T&>()] = ABI::TypeToEnum<T&>::value;
    map[meta::resolve_type<const T&>()] = ABI::TypeToEnum<const T&>::value;
    map[meta::resolve_type<std::vector<T>>()]= ABI::TypeToEnum<std::vector<T>>::value;
    map[meta::resolve_type<std::vector<T>&>()] = ABI::TypeToEnum<std::vector<T>&>::value;
    map[meta::resolve_type<const std::vector<T>&>()] = ABI::TypeToEnum<const std::vector<T>&>::value;
}

/**
* Template function to populate the type map.
* @tparam Ts The types to populate the type map with.
* @param map The map to insert the type to ABI type mapping into.
*/
template<typename... Ts>
void populateTypeMap(std::unordered_map<meta::any_type, ABI::Types>& map)
{
    (insertTypes<Ts>(map), ...);
}

extern std::unordered_map<std::string, std::vector<std::string>> methodArgumentsTypesMap; ///< Map to store method argument types

/**
 * This function populates the constructor argument names map.
 * @tparam TContract The contract type.
 */
template <typename TContract>
void inline populateMethodArgumentsTypesMap() {
  const meta::class_type contractType = meta::resolve_type<TContract>();
  std::string methodName;
  for (const meta::method &methods : contractType.get_methods()) {
    methodName = methods.get_name();
    auto arity = methods.get_type().get_arity();
    if (arity > 0) {
      std::vector<meta::argument> args = methods.get_arguments();
      std::vector<std::string> argumentTypes;
      for (size_t i = 0; i < args.size(); i++) {
        meta::any_type type = args[i].get_type();
        auto it = typeMap.find(type);
        if (it != typeMap.end()) {
          std::string stringType = ABI::getStringFromABIEnum(it->second);
          argumentTypes.push_back(stringType);
        }
      }
      methodArgumentsTypesMap[methodName] = argumentTypes;
    }
  }
}

/**
 * Template function to get the constructor data structure of a contract.
 * @tparam TContract The contract to get the constructor data structure of.
 * @return The constructor data structure in ABI format.
 */
template <typename TContract> bool isContractRegistered() {
  using DecayedT = std::remove_reference_t<std::remove_pointer_t<TContract>>;
  const meta::class_type contractType = meta::resolve_type<DecayedT>();
  return !contractType.get_constructors().empty();
}

/**
 * This function returns the type (or list of types) of a method's arguments in string format.
  * @param methodName The name of the method.
  * @return The type (or list of types) of the method's arguments.
  */
template <typename TContract>
std::vector<std::string> inline getMethodArgumentsTypesString(
    const std::string &methodName) {
  if (!isContractRegistered<TContract>()) {
    throw std::runtime_error("Contract " + Utils::getRealTypeName<TContract>() + " not registered");
  }
  auto it = methodArgumentsTypesMap.find(methodName);
  if (it != methodArgumentsTypesMap.end()) {
    return it->second;
  }
  return std::vector<std::string>();
}

/**
 * This function returns the type (or list of types) of a method's arguments in ABI enum format.
  * @param methodName The name of the method.
  * @return The type (or list of types) of the method's arguments.
  */
template <typename TContract>
std::vector<ABI::Types> inline getMethodArgumentsTypesABI(
    const std::string &methodName) {
  if (!isContractRegistered<TContract>()) {
    throw std::runtime_error("Contract " + Utils::getRealTypeName<TContract>() + " not registered");
  }
  auto it = methodArgumentsTypesMap.find(methodName);
  if (it != methodArgumentsTypesMap.end()) {
    std::vector<ABI::Types> types;
    for (auto const &x : it->second) {
      types.push_back(ABI::getABIEnumFromString(x));
    }
    return types;
  }
  return std::vector<ABI::Types>();
}

/**
 * Template function to register a contract class.
 * @tparam TContract The contract class to register.
 * @tparam Args The constructor argument types.
 * @tparam Methods The methods to register.
 * @param ctorArgs The constructor argument names.
 * @param methods The methods to register.
 */
template <typename TContract, typename... Args, typename... Methods>
void inline registerContract(const std::vector<std::string> &ctorArgs,
                             Methods &&...methods) {
  meta::class_<TContract>().template constructor_<Args...>(meta::constructor_policy::as_shared_pointer);

  // Store constructor argument names in the constructorArgumentNamesMap
  constructorArgumentNamesMap[typeid(TContract).name()] = ctorArgs;

  // Register methods and store the stateMutability string and argument names
  ((meta::class_<TContract>().method_(
        std::get<0>(std::forward<Methods>(methods)),
        std::get<1>(std::forward<Methods>(methods))),
    methodMutabilityMap[std::get<0>(std::forward<Methods>(methods))] =
        std::get<2>(std::forward<Methods>(methods)),
    argumentNamesMap[std::get<0>(std::forward<Methods>(methods))] =
        std::get<3>(std::forward<Methods>(methods))),
   ...);

    if (typeMap.empty())
    {
      populateTypeMap<uint8_t, uint16_t, uint24_t, uint32_t, uint40_t, uint48_t,
                    uint56_t, uint64_t, uint72_t, uint80_t, uint88_t, uint96_t,
                    uint104_t, uint112_t, uint120_t, uint128_t, uint136_t,
                    uint144_t, uint152_t, uint160_t, uint168_t, uint176_t,
                    uint184_t, uint192_t, uint200_t, uint208_t, uint216_t,
                    uint224_t, uint232_t, uint240_t, uint248_t, uint256_t,
                    int8_t, int16_t, int24_t, int32_t, int40_t, int48_t, int56_t,
                    int64_t, int72_t, int80_t, int88_t, int96_t, int104_t,
                    int112_t, int120_t, int128_t, int136_t, int144_t, int152_t,
                    int160_t, int168_t, int176_t, int184_t, int192_t, int200_t,
                    int208_t, int216_t, int224_t, int232_t, int240_t, int248_t,
                    int256_t,
                    Address, bool, std::string, Bytes>(typeMap);
    }
    populateMethodArgumentsTypesMap<TContract>();


}

/**
 * Template function to get the list of constructor argument types of a
 * contract.
 * @tparam Contract The contract to get the constructor argument types of.
 * @return The list of constructor argument in ABI format.
 */
template <typename Contract>
std::vector<ABI::Types> inline getConstructorArgumentTypes() {
  if (!isContractRegistered<Contract>()) {
    throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " not registered");
  }
  const meta::class_type contractType = meta::resolve_type<Contract>();
  std::vector<ABI::Types> constructorArgumentTypes;
  std::vector<ABI::Types> argumentTypes;

  for (const meta::constructor &ctor : contractType.get_constructors()) {
    std::vector<meta::argument> args = ctor.get_arguments();

    // We are considering that the last 5 arguments aren't part of the
    // pertinent arguments
    if (args.size() >= 5) {
      args.resize(args.size() - 5);
    } else {
      continue;
    }

    for (const meta::argument &arg : args) {
      meta::any_type type = arg.get_type();

      auto it = typeMap.find(type);
      if (it != typeMap.end()) {
        constructorArgumentTypes.push_back(it->second);
      }
    }
  }

  return constructorArgumentTypes;
}

/**
* Template function to get the list of constructor argument types of a
* contract.
* @tparam Contract The contract to get the constructor argument types of.
* @return The list of constructor argument in string format.
*/
template <typename TContract>
std::vector<std::string> inline getConstructorArgumentTypesString() {
  std::vector<ABI::Types> types = getConstructorArgumentTypes<TContract>();
  std::vector<std::string> typesString;
  for (auto const &x : types) {
    typesString.push_back(ABI::getStringFromABIEnum(x));
  }
  return typesString;
}

/**
 * Template function to get the constructor ABI data structure of a contract.
 * @tparam Contract The contract to get the constructor argument names of.
 * @return The constructor ABI data structure.
 */
template <typename Contract>
std::vector<ABI::MethodDescription> inline getConstructorDataStructure() {
  if (!isContractRegistered<Contract>()) {
    throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " not registered");
  }
  const meta::class_type contractType = meta::resolve_type<Contract>();
  std::vector<ABI::MethodDescription> descriptions;

  auto it = constructorArgumentNamesMap.find(typeid(Contract).name());
  if (it != constructorArgumentNamesMap.end()) {
    const std::vector<std::string> &ctorArgNames = it->second;

    for (const meta::constructor &ctor : contractType.get_constructors()) {
      std::vector<meta::argument> args = ctor.get_arguments();

      // We are considering that the last 5 arguments aren't part of the
      // pertinent arguments
      if (args.size() >= 5) {
        args.resize(args.size() - 5);
      } else {
        continue;
      }

      // We are considering only the constructors with the same number of
      // arguments as provided names
      if (args.size() == ctorArgNames.size()) {
        ABI::MethodDescription description;
        description.type = "constructor";
        description.stateMutability = "nonpayable";

        for (size_t i = 0; i < args.size(); ++i) {
          meta::any_type type = args[i].get_type();

          auto typeIt = typeMap.find(type);
          if (typeIt != typeMap.end()) {
            std::string stringType = ABI::getStringFromABIEnum(typeIt->second);
            description.inputs.push_back({ctorArgNames[i], stringType});
          }
        }

        descriptions.push_back(description);
      }
    }
  }

  return descriptions;
}

/**
 * Template function to get the function ABI data structure of a contract.
 * @tparam Contract The contract to get the function data structure of.
 * @return The function ABI data structure.
 */
template <typename Contract>
std::vector<ABI::MethodDescription> inline getFunctionDataStructure() {
  const meta::class_type contractType = meta::resolve_type<Contract>();
  std::vector<ABI::MethodDescription> descriptions;

  for (const meta::method &methods : contractType.get_methods()) {
    auto arity = methods.get_type().get_arity();

    ABI::MethodDescription description;
    description.name = methods.get_name();
    description.type = "function";

    auto mutabilityIt = methodMutabilityMap.find(description.name);
    if (mutabilityIt != methodMutabilityMap.end()) {
      description.stateMutability = mutabilityIt->second;
    }

    if (arity > 0) {
      std::vector<meta::argument> args = methods.get_arguments();
      auto argNamesIt = argumentNamesMap.find(description.name);
      if (argNamesIt != argumentNamesMap.end()) {
        auto &argNames = argNamesIt->second;
        for (size_t i = 0; i < args.size(); i++) {
          meta::any_type type = args[i].get_type();
          auto it = typeMap.find(type);
          if (it != typeMap.end()) {
            std::string stringType = ABI::getStringFromABIEnum(it->second);
            description.inputs.push_back({argNames[i], stringType});
          }
        }
      }
    }
    meta::any_type returnType = methods.get_type().get_return_type();
    auto it = typeMap.find(returnType);
    if (it != typeMap.end()) {
      std::string stringType = ABI::getStringFromABIEnum(it->second);
      description.outputs = {{"", stringType}};
    }

    descriptions.push_back(description);
  }

  return descriptions;
}

/**
 * Helper function to check if a method has arguments.
  * @param methodName The name of the method to check.
  * @return True if the method has arguments, false otherwise.
  */
template <typename Contract>
bool inline methodHasArguments(const std::string& methodName) {
    if (!isContractRegistered<Contract>()) {
        throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " not registered");
    }
    auto it = argumentNamesMap.find(methodName);
    if (it != argumentNamesMap.end()) {
        const std::vector<std::string>& argumentNames = it->second;
        return !argumentNames.empty();
    }
    return false;
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
    auto it = methodMutabilityMap.find(methodName);
    if (it != methodMutabilityMap.end()) {
        return it->second;
    }
    throw std::runtime_error("Method " + methodName + " not found");
}


} // namespace ContractReflectionInterface

#endif // CONTRACTREFLECTIONINTERFACE_H