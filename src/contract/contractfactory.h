/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACTFACTORY_H
#define CONTRACTFACTORY_H

#include "../utils/safehash.h"
#include "../utils/strings.h"
#include "../utils/utils.h"
#include "../utils/contractreflectioninterface.h"

#include "abi.h"
#include "contract.h"
#include "contracthost.h"
#include "contractmanager.h"

/// Factory **namespace** that does the setup, creation and registration of contracts to the blockchain.
/// As it is a namespace, it must take the required arguments (such as current contract list, etc.) as parameters.
/**
 *
 * The main argument used through the program is the following:
 * std::unordered_map<
 *       Functor,
 *       std::function<
 *         void(const evmc_message&,
 *              const Address&,
 *              std::unordered_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts_,
 *              const uint64_t&,
 *              DB& db,
 *              ContractHost*
 *              )>,
 *       SafeHash
 *     > createContractFuncs_;
 */
namespace ContractFactory {

    /**
     * Helper function to create a new contract from a given call info.
     * @tparam TContract The contract type to create.
     * @tparam TTuple The tuple type of the contract constructor arguments.
     * @tparam Is The indices of the tuple.
     * @param creator The address of the contract creator.
     * @param derivedContractAddress The address of the contract to create.
     * @param dataTlp The tuple of arguments to pass to the contract constructor.
     * @return A unique pointer to the newly created contract.
     * @throw DynamicException if any argument type mismatches.
     */
    template <typename TContract, typename TTuple, std::size_t... Is>
    std::unique_ptr<TContract> createContractWithTuple(
      const Address& creator,
      const Address& derivedContractAddress,
      const uint64_t& chainId,
      DB& db,
      const TTuple& dataTlp, std::index_sequence<Is...>
    ) {
      try {
        return std::make_unique<TContract>(
          std::get<Is>(dataTlp)...,
          derivedContractAddress, creator,
          chainId, db
        );
      } catch (const std::exception& ex) {
        // TODO: If the contract constructor throws an exception, the contract is not created.
        // But the variables owned by the contract were registered as used in the ContractCallLogger.
        // Meaning: we throw here, the variables are freed (as TContract ceases from existing), but a reference to the variable is still
        // in the ContractCallLogger. This causes a instant segfault when ContractCallLogger tries to revert the variable
        throw DynamicException(
          "Could not construct contract " + Utils::getRealTypeName<TContract>() + ": " + ex.what()
        );
      }
    }

    /**
     * Helper function to create a new contract from a given call info.
     * @param creator The address of the contract creator.
     * @param derivedContractAddress The address of the contract to create.
     * @param dataTpl The vector of arguments to pass to the contract constructor.
     * @throw DynamicException if the size of the vector does not match the number of arguments of the contract constructor.
     */
    template <typename TContract, typename TTuple>
    std::unique_ptr<TContract> createContractWithTuple(
      const Address& creator,
      const Address& derivedContractAddress,
      const uint64_t& chainId,
      DB& db,
      const TTuple& dataTpl
    ) {
      constexpr std::size_t TupleSize = std::tuple_size<TTuple>::value;
      return createContractWithTuple<TContract, TTuple>(
        creator, derivedContractAddress, chainId, db, dataTpl, std::make_index_sequence<TupleSize>{}
      );
    }

    /**
     * Setup data for a new contract before creating/validating it.
     * @param callInfo The call info to process.
     * @return A pair containing the contract address and the ABI decoder.
     * @throw DynamicException if non contract creator tries to create a contract.
     * @throw DynamicException if contract already exists as either a Dynamic or Protocol contract.
     */
    template <typename TContract> auto setupNewContractArgs(const evmc_message& callInfo) {
      // Setup the contract
      using ConstructorArguments = typename TContract::ConstructorArguments;
      using DecayedArguments = decltype(Utils::removeQualifiers<ConstructorArguments>());
      DecayedArguments arguments = std::apply([&callInfo](auto&&... args) {
        return ABI::Decoder::decodeData<std::decay_t<decltype(args)>...>(Utils::getFunctionArgs(callInfo));
      }, DecayedArguments{});
      return arguments;
    }

    /**
     * Create a new contract from a given call info.
     * @param callInfo The call info to process.
     * @throw DynamicException if the call to the ethCall function fails, or if the contract does not exist.
     */
    template <typename TContract> void createNewContract(const evmc_message& callInfo,
                                                         const Address& derivedAddress,
                                                         std::unordered_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts,
                                                         const uint64_t& chainId,
                                                         DB& db,
                                                         ContractHost* host) {
      using ConstructorArguments = typename TContract::ConstructorArguments;
      auto decodedData = setupNewContractArgs<TContract>(callInfo);
      if (!ContractReflectionInterface::isContractFunctionsRegistered<TContract>()) {
        throw DynamicException("Contract " + Utils::getRealTypeName<TContract>() + " is not registered");
      }
      // Update the inner variables of the contract.
      // The constructor can set SafeVariable values from the constructor.
      // We need to take account of that and set the variables accordingly.
      auto contract = createContractWithTuple<TContract, ConstructorArguments>(
        callInfo.sender, derivedAddress, chainId, db, decodedData
      );
      host->registerNewCPPContract(derivedAddress, contract.get());
      contracts.insert(std::make_pair(derivedAddress, std::move(contract)));
    }

    /**
     * Adds contract create and validate functions to the respective maps
     * @tparam Contract Contract type
     * @param createFunc Function to create a new contract
     */
    template <typename Contract>
    void addContractFuncs(const std::function<
                       void(const evmc_message&,
                            const Address&,
                            std::unordered_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts_,
                            const uint64_t&,
                            DB& db,
                            ContractHost* host)>& createFunc
                      ,std::unordered_map<Functor,std::function<void(const evmc_message&,
                                                                     const Address&,
                                                                     std::unordered_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts_,
                                                                     const uint64_t&,
                                                                     DB& db,
                                                                     ContractHost*)>,SafeHash>& createContractFuncs
    ) {
      std::string createSignature = "createNew" + Utils::getRealTypeName<Contract>() + "Contract(";
      // Append args
      createSignature += ContractReflectionInterface::getConstructorArgumentTypesString<Contract>();
      createSignature += ")";
      auto hash = Utils::sha3(Utils::create_view_span(createSignature));
      Functor functor;
      functor.value = Utils::bytesToUint32(hash.view(0,4));
      createContractFuncs[functor] = createFunc;
    }

    /**
     * Register all contracts in the variadic template.
     * @tparam Contracts The contracts to register.
     */
    template <typename Tuple, std::size_t... Is> void addAllContractFuncsHelper(std::unordered_map<Functor,std::function<void(const evmc_message&,
                                                                     const Address&,
                                                                     std::unordered_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts_,
                                                                     const uint64_t&,
                                                                     DB& db,
                                                                     ContractHost*)>,SafeHash>& createContractFuncs,
                                                                   std::index_sequence<Is...>) {
      ((addContractFuncs<std::tuple_element_t<Is, Tuple>>( [&](const evmc_message &callInfo,
                                                                const Address &derivedAddress,
                                                                std::unordered_map<Address, std::unique_ptr<BaseContract>, SafeHash> &contracts,
                                                                const uint64_t &chainId,
                                                                DB &db,
                                                                ContractHost* host) {
        createNewContract<std::tuple_element_t<Is, Tuple>>(callInfo, derivedAddress, contracts, chainId, db, host);
      }, createContractFuncs)), ...);
    }

    /**
     * Add all contract functions to the respective maps using the helper function.
     * @tparam Tuple The tuple of contracts to add.
     */
    template <typename Tuple> requires Utils::is_tuple<Tuple>::value void addAllContractFuncs(
                                                                   std::unordered_map<Functor,std::function<void(const evmc_message&,
                                                                   const Address&,
                                                                   std::unordered_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts_,
                                                                   const uint64_t&,
                                                                   DB& db,
                                                                   ContractHost*)>,SafeHash>& createContractFuncs) {
      addAllContractFuncsHelper<Tuple>(createContractFuncs, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
    }

    /**
     * Struct for calling the registerContract function of a contract.
     * @tparam TContract The contract to register.
     */
    template <class T> struct RegisterContract { RegisterContract() { T::registerContract(); } };

    /**
     * Helper function to register all contracts.
     * @tparam Tuple The tuple of contracts to register.
     * @tparam Is The indices of the tuple.
     */
    template <typename Tuple, std::size_t... Is> void registerContractsHelper(std::index_sequence<Is...>) {
      (RegisterContract<std::tuple_element_t<Is, Tuple>>(), ...);
    }

    /**
     * Register all contracts in the tuple.
     * @tparam Tuple The tuple of contracts to register.
     */
    template <typename Tuple> requires Utils::is_tuple<Tuple>::value void registerContracts() {
      registerContractsHelper<Tuple>(std::make_index_sequence<std::tuple_size<Tuple>::value>{});
    }
};

#endif  // CONTRACTFACTORY_H
