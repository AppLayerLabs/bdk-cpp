/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACTMANAGER_H
#define CONTRACTMANAGER_H

#include "../utils/contractreflectioninterface.h" // contract/abi.h -> utils.h -> strings.h, libs/json.hpp -> boost/unordered/unordered_flat_map.hpp

#include "contract.h" // core/dump.h -> utils/db.h

#include "blockobservers.h"
#include "../utils/strconv.h"

/**
 * Class that holds all current contract instances in the blockchain state.
 * Responsible for creating and deploying contracts in the chain.
 * Also acts as an access point for contracts to access each other.
 */
class ContractManager : public BaseContract {
  private:
    /// Boolean that accompanies the PointerNullifier for EVM calls
    bool nullifiable_ = true;
    /// Reference to the current dump manager.
    /// Owned by the State
    DumpManager& manager_;
    boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash, SafeCompare>& contracts_;

    /// Functions to create contracts.
    boost::unordered_flat_map<
          Functor,
          std::function<
            void(const evmc_message&,
                 const Address&,
                 boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash, SafeCompare>& contracts_,
                 const uint64_t&,
                 ContractHost*
                 )>,
          SafeHash
        > createContractFuncs_;

    /**
     * Get all deployed contracts.
     * struct Contract { string name; address addr; }
     * function getDeployedContracts() public view returns (Contract[] memory) {
     */
    std::vector<std::tuple<std::string, Address>> getDeployedContracts() const;

    // Temporary variable to allow the proper execution of evmEthCall
    bool executed_ = false;

    /**
     * Get all deployed contracts from a specific creator address.
     * function getDeployedContractsForCreator(Address creator) public view returns (Contract[] memory) {
     */
    std::vector<std::tuple<std::string, Address>> getDeployedContractsForCreator(const Address& creator) const;

    /**
     * Get the information of a specific contract.
     * function getContractInfo(address addr) external view returns (ContractInfo memory);
     */
    std::tuple<bool, std::string> getContractInfo(const Address& addr) const;

    /**
     * Helper function to load all contracts from the database.
     * @tparam Tuple The tuple of contracts to load.
     * @tparam Is The indices of the tuple.
     * @param contract The contract to load.
     * @param contractAddress The address of the contract.
     * @param db Reference to the database.
     * @return `true` if the contract exists in the database, `false` otherwise.
     */
    template <typename Tuple, std::size_t... Is>
    bool loadFromDBHelper(const auto& contract, const Address& contractAddress, const DB& db, BlockObservers& observer, std::index_sequence<Is...>) {
      return (loadFromDBT<std::tuple_element_t<Is, Tuple>>(contract, contractAddress, db, observer) || ...);
    }

    /**
     * Load all contracts from the database.
     * @tparam Tuple The tuple of contracts to load.
     * @param contract The contract to load.
     * @param contractAddress The address of the contract.
     * @param db Reference to the database.
     * @return `true` if the contract exists in the database, `false` otherwise.
     */
    template <typename T>
    bool loadFromDBT(const auto& contract, const Address& contractAddress, const DB& db, BlockObservers& observer) {
      // Here we disable this template when T is a tuple
      static_assert(!Utils::is_tuple<T>::value, "Must not be a tuple");
      if (StrConv::bytesToString(contract.value) == Utils::getRealTypeName<T>()) {
        auto it = this->contracts_.insert(std::make_pair(
          contractAddress, std::make_unique<T>(contractAddress, db)
        ));
        for (const auto& blockObserver : it.first->second->getBlockNumberObservers()) {
          observer.add(blockObserver);
        }
        for (const auto& timestampObserver : it.first->second->getBlockTimestampObservers()) {
          observer.add(timestampObserver);
        }
        // Dont forget to register the contract on the dump manager
        this->manager_.pushBack(it.first->second.get());
        return true;
      }
      return false;
    }

    /**
     * Load all contracts from the database using the helper function.
     * @tparam Tuple The tuple of contracts to load.
     * @param contract The contract to load.
     * @param contractAddress The address of the contract.
     * @param db Reference to the database.
     * @return `true` if the contract exists in the database, `false` otherwise.
     */
    template <typename Tuple> requires Utils::is_tuple<Tuple>::value bool loadFromDB(
      const auto& contract, const Address& contractAddress, const DB& db, BlockObservers& observer
    ) {
      return loadFromDBHelper<Tuple>(
        contract, contractAddress, db, observer, std::make_index_sequence<std::tuple_size<Tuple>::value>{}
      );
    }

  public:
    /**
     * Constructor. Automatically loads contracts from the database and deploys them.
     * @param db Reference to the database.
     * @param contracts Reference to the contracts map.
     * @param manager Reference to the database dumping manager.
     * @param options Reference to the options singleton.
     * @throw DynamicException if contract address doesn't exist in the database.
     */
    ContractManager(const DB& db,
                    boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash, SafeCompare>& contracts,
                    DumpManager& manager,
                    BlockObservers& observer,
                    const Options& options);

    ~ContractManager() override; ///< Destructor. Automatically saves contracts to the database before wiping them.

    /**
     * Override the default contract function call.
     * ContractManager processes things in a non-standard way
     * (you cannot use SafeVariables as contract creation actively writes to DB).
     * @param callInfo The call info to process.
     * @param host Pointer to the contract host.
     * @throw DynamicException if the call is not valid.
     */
    void ethCall(const evmc_message& callInfo, ContractHost* host) override;

    /**
     * Override the default contract function call.
     * ContractManager processes things in a non-standard way
     * (you cannot use SafeVariables as contract creation actively writes to DB).
     * @param callInfo The call info to process.
     * @return The bytes from the call
     * @throw DynamicException if the call is not valid.
     */
    Bytes evmEthCall(const evmc_message& callInfo, ContractHost* host) override;

    /**
     * Override the default contract view function call.
     * ContractManager process things in a non-standard way
     * (you cannot use SafeVariables as contract creation actively writes to DB).
     * @param data The call info to process.
     * @param host Pointer to the contract host.
     * @return A string with the requested info.
     * @throw DynamicException if the call is not valid.
     */
    Bytes ethCallView(const evmc_message& data, ContractHost* host) const override;

    /// Dump override
    DBBatch dump() const override;
};

#endif // CONTRACTMANAGER_H
