#ifndef CONTRACTREFLECTIONINTERFACE_H
#define CONTRACTREFLECTIONINTERFACE_H

#define NAME_OF( v ) #v

#include "contract/contract.h"
#include "contract/dynamiccontract.h"
#include "meta_all.hpp"
#include <cstdint>
#include <vector>


/**
 * @brief This namespace contains the reflection interface for the contract
 * classes.
 *
 */

namespace ContractReflectionInteface {
namespace meta = meta_hpp;

/**
 * @brief This function registers the core contract classes.
 *
 */
void inline registerCoreContractClasses() {
  meta::class_<ContractGlobals>();
  meta::class_<ContractLocals>().base_<ContractGlobals>();
  meta::class_<BaseContract>()
      .base_<ContractLocals>()
      .constructor_<std::string, Address, Address, uint64_t,
                    std::unique_ptr<DB>>();
  meta::class_<DynamicContract>()
      .base_<BaseContract>()
      .constructor_<ContractManager::ContractManagerInterface &, std::string,
                    Address, Address, uint64_t, const std::unique_ptr<DB> &>();
}

template <typename TContract, typename... Args, typename... Methods>
void inline registerContract(Methods &&...methods) {
  meta::class_<TContract>().template constructor_<Args...>();

  // Register methods
  (meta::class_<TContract>().method_(
       std::get<0>(std::forward<Methods>(methods)),
       std::get<1>(std::forward<Methods>(methods))),
   ...);

   
}

template <typename TContract> bool isContractRegistered() {
  const meta::class_type contractType = meta::resolve_type<TContract>();
  return !contractType.get_constructors().empty();
}

/**
 * @brief Template struct to map a type to an ABI type.
 * @tparam T The type to map.
 */
template <typename T> struct TypeToEnum;

/**
 * @brief Specialization of TypeToEnum for uint256_t.
 */
template <> struct TypeToEnum<uint256_t> {
  static constexpr ABI::Types value = ABI::Types::uint256;
};

/**
 * @brief Specialization of TypeToEnum for reference to uint256_t.
 */
template <> struct TypeToEnum<uint256_t &> {
  static constexpr ABI::Types value = ABI::Types::uint256;
};

/**
 * @brief Specialization of TypeToEnum for const reference to uint256_t.
 */
template <> struct TypeToEnum<const uint256_t &> {
  static constexpr ABI::Types value = ABI::Types::uint256;
};
/**
 * @brief Specialization of TypeToEnum for std::vector<uint256_t>.
 */
template <> struct TypeToEnum<std::vector<uint256_t>> {
  static constexpr ABI::Types value = ABI::Types::uint256Arr;
};

/**
 * @brief Specialization of TypeToEnum for reference to std::vector<uint256_t>.
 */
template <> struct TypeToEnum<std::vector<uint256_t> &> {
  static constexpr ABI::Types value = ABI::Types::uint256Arr;
};

/**
 * @brief Specialization of TypeToEnum for const reference to
 * std::vector<uint256_t>.
 */

template <> struct TypeToEnum<const std::vector<uint256_t> &> {
  static constexpr ABI::Types value = ABI::Types::uint256Arr;
};

/**
 * @brief Specialization of TypeToEnum for Address.
 */
template <> struct TypeToEnum<Address> {
  static constexpr ABI::Types value = ABI::Types::address;
};

/**
 * @brief Specialization of TypeToEnum for reference to Address.
 */
template <> struct TypeToEnum<Address &> {
  static constexpr ABI::Types value = ABI::Types::address;
};

/**
 * @brief Specialization of TypeToEnum for const reference to Address.
 */
template <> struct TypeToEnum<const Address &> {
  static constexpr ABI::Types value = ABI::Types::address;
};

/**
 * @brief Specialization of TypeToEnum for std::vector<Address>.
 */
template <> struct TypeToEnum<std::vector<Address>> {
  static constexpr ABI::Types value = ABI::Types::addressArr;
};

/**
 * @brief Specialization of TypeToEnum for reference to std::vector<Address>.
 */
template <> struct TypeToEnum<std::vector<Address> &> {
  static constexpr ABI::Types value = ABI::Types::addressArr;
};

/**
 * @brief Specialization of TypeToEnum for const reference to
 * std::vector<Address>.
 */
template <> struct TypeToEnum<const std::vector<Address> &> {
  static constexpr ABI::Types value = ABI::Types::addressArr;
};

/**
 * @brief Specialization of TypeToEnum for bool.
 */
template <> struct TypeToEnum<bool> {
  static constexpr ABI::Types value = ABI::Types::boolean;
};

/**
 * @brief Specialization of TypeToEnum for reference to bool.
 */
template <> struct TypeToEnum<bool &> {
  static constexpr ABI::Types value = ABI::Types::boolean;
};

/**
 * @brief Specialization of TypeToEnum for const reference to bool.
 */
template <> struct TypeToEnum<const bool &> {
  static constexpr ABI::Types value = ABI::Types::boolean;
};

/**
 * @brief Specialization of TypeToEnum for std::vector<bool>.
 */
template <> struct TypeToEnum<std::vector<bool>> {
  static constexpr ABI::Types value = ABI::Types::booleanArr;
};

/**
 * @brief Specialization of TypeToEnum for reference to std::vector<bool>.
 */
template <> struct TypeToEnum<std::vector<bool> &> {
  static constexpr ABI::Types value = ABI::Types::booleanArr;
};

/**
 * @brief Specialization of TypeToEnum for const reference to
 * std::vector<bool>.
 */
template <> struct TypeToEnum<const std::vector<bool> &> {
  static constexpr ABI::Types value = ABI::Types::booleanArr;
};

/**
 * @brief Specialization of TypeToEnum for std::string.
 */
template <> struct TypeToEnum<std::string> {
  static constexpr ABI::Types value = ABI::Types::bytes;
};

template <> struct TypeToEnum<std::string &> {
  static constexpr ABI::Types value = ABI::Types::bytes;
};

template <> struct TypeToEnum<const std::string &> {
  static constexpr ABI::Types value = ABI::Types::bytes;
};

/**
 * @brief Specialization of TypeToEnum for std::vector<std::string>.
 */
template <> struct TypeToEnum<std::vector<std::string>> {
  static constexpr ABI::Types value = ABI::Types::bytesArr;
};

/**
 * @brief Specialization of TypeToEnum for reference to
 * std::vector<std::string>.
 */
template <> struct TypeToEnum<std::vector<std::string> &> {
  static constexpr ABI::Types value = ABI::Types::bytesArr;
};

/**
 * @brief Specialization of TypeToEnum for const reference to
 * std::vector<std::string>.
 */
template <> struct TypeToEnum<const std::vector<std::string> &> {
  static constexpr ABI::Types value = ABI::Types::bytesArr;
};

/**
 * @brief This map is used to map a type to an ABI type.
 *
 */
static const std::unordered_map<meta::any_type, ABI::Types> typeMap = {
    {meta::resolve_type<uint256_t>(), TypeToEnum<uint256_t>::value},
    {meta::resolve_type<uint256_t &>(), TypeToEnum<uint256_t &>::value},
    {meta::resolve_type<const uint256_t &>(),
     TypeToEnum<const uint256_t &>::value},
    {meta::resolve_type<std::vector<uint256_t>>(),
     TypeToEnum<std::vector<uint256_t>>::value},
    {meta::resolve_type<std::vector<uint256_t> &>(),
     TypeToEnum<std::vector<uint256_t> &>::value},
    {meta::resolve_type<const std::vector<uint256_t> &>(),
     TypeToEnum<const std::vector<uint256_t> &>::value},
    {meta::resolve_type<Address>(), TypeToEnum<Address>::value},
    {meta::resolve_type<Address &>(), TypeToEnum<Address &>::value},
    {meta::resolve_type<const Address &>(), TypeToEnum<const Address &>::value},
    {meta::resolve_type<std::vector<Address>>(),
     TypeToEnum<std::vector<Address>>::value},
    {meta::resolve_type<std::vector<Address> &>(),
     TypeToEnum<std::vector<Address> &>::value},
    {meta::resolve_type<const std::vector<Address> &>(),
     TypeToEnum<const std::vector<Address> &>::value},
    {meta::resolve_type<bool>(), TypeToEnum<bool>::value},
    {meta::resolve_type<bool &>(), TypeToEnum<bool &>::value},
    {meta::resolve_type<const bool &>(), TypeToEnum<const bool &>::value},
    {meta::resolve_type<std::vector<bool>>(),
     TypeToEnum<std::vector<bool>>::value},
    {meta::resolve_type<std::vector<bool> &>(),
     TypeToEnum<std::vector<bool> &>::value},
    {meta::resolve_type<const std::vector<bool> &>(),
     TypeToEnum<const std::vector<bool> &>::value},
    {meta::resolve_type<std::string>(), TypeToEnum<std::string>::value},
    {meta::resolve_type<const std::string &>(),
     TypeToEnum<const std::string &>::value},
    {meta::resolve_type<std::string &>(), TypeToEnum<std::string &>::value},
    {meta::resolve_type<std::vector<std::string>>(),
     TypeToEnum<std::vector<std::string>>::value},
    {meta::resolve_type<std::vector<std::string> &>(),
     TypeToEnum<std::vector<std::string> &>::value},
    {meta::resolve_type<const std::vector<std::string> &>(),
     TypeToEnum<const std::vector<std::string> &>::value}};

/**
 * @brief Template helper to check if a type is one of a list of types.
 * @tparam T The type to check.
 * @tparam Ts The list of types to check against.
 */
template <typename T, typename... Ts>
constexpr bool isAnyOf = (... || std::is_same_v<T, Ts>);

/**
 * @brief Template function to check if a type is a known type.
 * @tparam T The type to check.
 * @return True if the type is a known type, false otherwise.
 */
template <typename T> constexpr bool isKnownType() {
  return isAnyOf<T, uint256_t, std::vector<uint256_t>, Address,
                 std::vector<Address>, bool, std::vector<bool>, std::string,
                 std::vector<std::string>>;
}

/**
 * @brief Template function to get the list of constructor argument types of a
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

    // We are considering only the constructors with 5 arguments, which are the
    // ones that we are interested in.
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

  for (const meta::method &methods : contractType.get_methods()) {
    auto arity = methods.get_type().get_arity();
    if (arity > 0) {
      auto args = methods.get_arguments();
      for (const meta::argument &arg : args) {
        std::cout << "name: " << name << std::endl;
        meta::any_type type = arg.get_type();

        auto it = typeMap.find(type);
        if (it != typeMap.end()) {
          argumentTypes.push_back(it->second);
        }
      }
    }
  }

  return argumentTypes;
}

} // namespace ContractReflectionInteface

#endif // CONTRACTREFLECTIONINTERFACE_H