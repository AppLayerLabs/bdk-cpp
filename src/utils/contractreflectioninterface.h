#ifndef CONTRACTREFLECTIONINTERFACE_H
#define CONTRACTREFLECTIONINTERFACE_H

#include "contract/contract.h"
#include "contract/abi.h"
#include "meta_all.hpp"
#include "utils.h" // nlohmann::json
#include <tuple>

/**
 * This namespace contains the reflection interface for the contract
 * classes.
 *
 */

namespace ContractReflectionInterface {
namespace meta = meta_hpp;

/**
 * This struct contains the structure for the contract ABI object.
 *
 */
struct MethodDescription {
  std::string name;
  std::vector<std::pair<std::string, std::string>> inputs;
  std::vector<std::pair<std::string, std::string>> outputs;
  std::string stateMutability;
  std::string type;
};

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
    {meta::resolve_type<const std::vector<std::string> &>(), ABI::TypeToEnum<const std::vector<std::string> &>::value}};

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
  meta::class_<TContract>().template constructor_<Args...>();

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
std::vector<MethodDescription> inline getConstructorDataStructure() {
  const meta::class_type contractType = meta::resolve_type<Contract>();
  std::vector<MethodDescription> descriptions;

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
        MethodDescription description;
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
std::vector<MethodDescription> inline getFunctionDataStructure() {
  const meta::class_type contractType = meta::resolve_type<Contract>();
  std::vector<MethodDescription> descriptions;

  for (const meta::method &methods : contractType.get_methods()) {
    auto arity = methods.get_type().get_arity();

    MethodDescription description;
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
    using DecayedContract = std::decay_t<Contract>;
    if (!isContractRegistered<Contract>()) {
      throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " is not registered.");
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
      throw std::runtime_error("Contract " + Utils::getRealTypeName<Contract>() + " is not registered.");
    }
    auto it = methodMutabilityMap.find(methodName);
    if (it != methodMutabilityMap.end()) {
        return it->second;
    }
    throw std::runtime_error("Method " + methodName + " not found");
}

// JSON UTILS
// Maybe move to a separate file? It uses reflection, but it's not reflection
// itself.

/**
 * This function converts a MethodDescription object to JSON format.
 * @param jsonObject The JSON object to convert to.
 * @param description The MethodDescription object to convert.
 */
inline void to_json(json &jsonObject, const MethodDescription &description) {
  jsonObject["name"] = description.name;
  jsonObject["stateMutability"] = description.stateMutability;
  jsonObject["type"] = description.type;

  for (auto &input : description.inputs) {
    jsonObject["inputs"].push_back({{"internalType", input.second},
                                    {"name", input.first},
                                    {"type", input.second}});
  }

  for (auto &output : description.outputs) {
    jsonObject["outputs"].push_back({{"internalType", output.second},
                                     {"name", output.first},
                                     {"type", output.second}});
  }
}

/**
 * This function registers a contract and gets the contract data in JSON format.
 * @tparam Contract The contract to register.
 * @param contractData The JSON object to store the contract data in.
 */
template <typename Contract>
void registerContractAndGetData(json &contractData) {
  Contract::registerContract();
  auto constructorData = getConstructorDataStructure<Contract>();
  auto functionData = getFunctionDataStructure<Contract>();

  // Convert constructor and function data into JSON format
  for (const auto &constructor : constructorData) {
    json ctorJson;
    to_json(ctorJson, constructor);
    contractData.push_back(ctorJson);
  }
  for (const auto &function : functionData) {
    json funcJson;
    to_json(funcJson, function);
    contractData.push_back(funcJson);
  }
}

/**
 * This function writes the contract data of a contract to a JSON file.
 * @tparam Contract The contract to write the data of.
 * @param outputFilename The name of the output file.
 */
template <typename Contract>
void writeContractToJson() {
  json contractData;
  registerContractAndGetData<Contract>(contractData);

  std::string outputFileName = Utils::getRealTypeName<Contract>();
  if (outputFileName.substr(outputFileName.find_last_of(".") + 1) != "json") {
    outputFileName += ".json";
  }

  const std::string dirName = "ABI";
  if (!std::filesystem::exists(dirName)) {
    std::filesystem::create_directory(dirName);
  }

  const std::string fullOutputFileName = dirName + "/" + outputFileName;

  std::ofstream jsonFile(fullOutputFileName);
  jsonFile << std::setw(4) << contractData << std::endl;
}

/**
  * This function writes the contract data of a list of contracts to JSON files.
  * @tparam FirstContract The first contract to write the data of.
  * @tparam RestContracts The rest of the contracts to write the data of.
  * @return 0 if successful.
  */
template <typename FirstContract, typename... RestContracts>
int writeContractsToJson() {
    writeContractToJson<FirstContract>();

    if constexpr (sizeof...(RestContracts) > 0) {
        return writeContractsToJson<RestContracts...>();
    }

    return 0;
}

} // namespace ContractReflectionInterface

#endif // CONTRACTREFLECTIONINTERFACE_H