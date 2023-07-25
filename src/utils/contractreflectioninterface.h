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

/**
 * This map is used to map a type to an ABI type.
 *
 */
static const std::unordered_map<meta::any_type, ABI::Types> typeMap = {
    {meta::resolve_type<uint256_t>(), ABI::TypeToEnum<uint256_t>::value},
    {meta::resolve_type<uint256_t &>(), ABI::TypeToEnum<uint256_t &>::value},
    {meta::resolve_type<const uint256_t &>(), ABI::TypeToEnum<const uint256_t &>::value},
    {meta::resolve_type<std::vector<uint256_t>>(), ABI::TypeToEnum<std::vector<uint256_t>>::value},
    {meta::resolve_type<std::vector<uint256_t> &>(), ABI::TypeToEnum<std::vector<uint256_t> &>::value},
    {meta::resolve_type<const std::vector<uint256_t> &>(), ABI::TypeToEnum<const std::vector<uint256_t> &>::value},
    {meta::resolve_type<uint8_t>(), ABI::TypeToEnum<uint8_t>::value},
    {meta::resolve_type<uint8_t &>(), ABI::TypeToEnum<uint8_t &>::value},
    {meta::resolve_type<const uint8_t &>(), ABI::TypeToEnum<const uint8_t &>::value},
    {meta::resolve_type<std::vector<uint8_t>>(), ABI::TypeToEnum<std::vector<uint8_t>>::value},
    {meta::resolve_type<std::vector<uint8_t> &>(), ABI::TypeToEnum<std::vector<uint8_t> &>::value},
    {meta::resolve_type<const std::vector<uint8_t> &>(), ABI::TypeToEnum<const std::vector<uint8_t> &>::value},
    {meta::resolve_type<uint16_t>(), ABI::TypeToEnum<uint16_t>::value},
    {meta::resolve_type<uint16_t &>(), ABI::TypeToEnum<uint16_t &>::value},
    {meta::resolve_type<const uint16_t &>(), ABI::TypeToEnum<const uint16_t &>::value},
    {meta::resolve_type<std::vector<uint16_t>>(), ABI::TypeToEnum<std::vector<uint16_t>>::value},
    {meta::resolve_type<std::vector<uint16_t> &>(), ABI::TypeToEnum<std::vector<uint16_t> &>::value},
    {meta::resolve_type<const std::vector<uint16_t> &>(), ABI::TypeToEnum<const std::vector<uint16_t> &>::value},
    {meta::resolve_type<uint32_t>(), ABI::TypeToEnum<uint32_t>::value},
    {meta::resolve_type<uint32_t &>(), ABI::TypeToEnum<uint32_t &>::value},
    {meta::resolve_type<const uint32_t &>(), ABI::TypeToEnum<const uint32_t &>::value},
    {meta::resolve_type<std::vector<uint32_t>>(), ABI::TypeToEnum<std::vector<uint32_t>>::value},
    {meta::resolve_type<std::vector<uint32_t> &>(), ABI::TypeToEnum<std::vector<uint32_t> &>::value},
    {meta::resolve_type<const std::vector<uint32_t> &>(), ABI::TypeToEnum<const std::vector<uint32_t> &>::value},
    {meta::resolve_type<uint64_t>(), ABI::TypeToEnum<uint64_t>::value},
    {meta::resolve_type<uint64_t &>(), ABI::TypeToEnum<uint64_t &>::value},
    {meta::resolve_type<const uint64_t &>(), ABI::TypeToEnum<const uint64_t &>::value},
    {meta::resolve_type<std::vector<uint64_t>>(), ABI::TypeToEnum<std::vector<uint64_t>>::value},
    {meta::resolve_type<std::vector<uint64_t> &>(), ABI::TypeToEnum<std::vector<uint64_t> &>::value},
    {meta::resolve_type<const std::vector<uint64_t> &>(), ABI::TypeToEnum<const std::vector<uint64_t> &>::value},
    // TODO: Add support for uint128_t
    {meta::resolve_type<Address>(), ABI::TypeToEnum<Address>::value},
    {meta::resolve_type<Address &>(), ABI::TypeToEnum<Address &>::value},
    {meta::resolve_type<const Address &>(), ABI::TypeToEnum<const Address &>::value},
    {meta::resolve_type<std::vector<Address>>(), ABI::TypeToEnum<std::vector<Address>>::value},
    {meta::resolve_type<std::vector<Address> &>(), ABI::TypeToEnum<std::vector<Address> &>::value},
    {meta::resolve_type<const std::vector<Address> &>(), ABI::TypeToEnum<const std::vector<Address> &>::value},
    {meta::resolve_type<bool>(), ABI::TypeToEnum<bool>::value},
    {meta::resolve_type<bool &>(), ABI::TypeToEnum<bool &>::value},
    {meta::resolve_type<const bool &>(), ABI::TypeToEnum<const bool &>::value},
    {meta::resolve_type<std::vector<bool>>(), ABI::TypeToEnum<std::vector<bool>>::value},
    {meta::resolve_type<std::vector<bool> &>(), ABI::TypeToEnum<std::vector<bool> &>::value},
    {meta::resolve_type<const std::vector<bool> &>(), ABI::TypeToEnum<const std::vector<bool> &>::value},
    {meta::resolve_type<std::string>(), ABI::TypeToEnum<std::string>::value},
    {meta::resolve_type<const std::string &>(), ABI::TypeToEnum<const std::string &>::value},
    {meta::resolve_type<std::string &>(), ABI::TypeToEnum<std::string &>::value},
    {meta::resolve_type<std::vector<std::string>>(), ABI::TypeToEnum<std::vector<std::string>>::value},
    {meta::resolve_type<std::vector<std::string> &>(), ABI::TypeToEnum<std::vector<std::string> &>::value},
    {meta::resolve_type<const std::vector<std::string> &>(), ABI::TypeToEnum<const std::vector<std::string> &>::value},
    {meta::resolve_type<Bytes>(), ABI::TypeToEnum<Bytes>::value},
    {meta::resolve_type<const std::string &>(), ABI::TypeToEnum<const Bytes &>::value},
    {meta::resolve_type<Bytes &>(), ABI::TypeToEnum<Bytes &>::value},
    {meta::resolve_type<std::vector<Bytes>>(), ABI::TypeToEnum<std::vector<Bytes>>::value},
    {meta::resolve_type<std::vector<std::string> &>(), ABI::TypeToEnum<std::vector<Bytes> &>::value},
    {meta::resolve_type<const std::vector<Bytes> &>(), ABI::TypeToEnum<const std::vector<Bytes> &>::value}};

/**
 * This function returns the ABI type for a given ABI type string.
 * @param type The ABI type string.
 * @return The ABI type.
 */
ABI::Types inline getABIEnumFromString(const std::string& type) {
  static const std::unordered_map<std::string, ABI::Types> typeMappings = {
      {"uint8", ABI::Types::uint8},
      {"uint8[]", ABI::Types::uint8Arr},
      {"uint16", ABI::Types::uint16},
      {"uint16[]", ABI::Types::uint16Arr},
      {"uint32", ABI::Types::uint32},
      {"uint32[]", ABI::Types::uint32Arr},
      {"uint64", ABI::Types::uint64},
      {"uint64[]", ABI::Types::uint64Arr},
      {"uint256", ABI::Types::uint256},
      {"uint256[]", ABI::Types::uint256Arr},
      {"address", ABI::Types::address},
      {"address[]", ABI::Types::addressArr},
      {"bool", ABI::Types::boolean},
      {"bool[]", ABI::Types::booleanArr},
      {"bytes", ABI::Types::bytes},
      {"bytes[]", ABI::Types::bytesArr},
      {"string", ABI::Types::string},
      {"string[]", ABI::Types::stringArr}
  };

  auto it = typeMappings.find(type);
  if (it != typeMappings.end()) {
    return it->second;
  } else {
    throw std::runtime_error("Invalid type");
  }
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
      types.push_back(getABIEnumFromString(x));
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