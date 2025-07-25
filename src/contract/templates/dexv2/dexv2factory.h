/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DEXFACTORY_H
#define DEXFACTORY_H

#include "../../../utils/db.h"
#include "../../abi.h"
#include "../../dynamiccontract.h"
#include "../../variables/safeaddress.h"
#include "../../variables/safeunorderedmap.h"
#include "../../variables/safevector.h"

/**
 * The DEXV2Factory contract.
 */
class DEXV2Factory : public DynamicContract {
  private:
    /// Solidity: address public feeTo;
    SafeAddress feeTo_;

    /// Solidity: address public feeToSetter;
    SafeAddress feeToSetter_;

    /// Solidity: address[] public allPairs;
    SafeVector<Address> allPairs_;

    /// Solidity: mapping(address => mapping(address => address)) public getPair;
    SafeUnorderedMap<Address, boost::unordered_flat_map<Address, Address, SafeHash>> getPair_;

    /// Function for calling the register functions for contracts.
    void registerContractFunctions() override;

  public:
    /**
     * ConstructorArguments is a tuple of the contract constructor arguments
     * in the order they appear in the constructor.
     */
    using ConstructorArguments = std::tuple<Address>;

    /**
     * Constructor for loading contract from DB.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    DEXV2Factory(const Address& address, const DB& db);

    /**
     * Constructor to be used when creating a new contract.
     * @param feeToSetter The address of the feeToSetter.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     */
    DEXV2Factory(
      const Address& feeToSetter,
      const Address &address, const Address &creator, const uint64_t &chainId
    );

    // Destructor.
    ~DEXV2Factory() override;

    /// Get the feeTo address of the DEXV2Factory.
    Address feeTo() const;

    /// Get the feeToSetter address of the DEXV2Factory.
    Address feeToSetter() const;

    /// Get all the pairs created by the DEXV2Factory.
    std::vector<Address> allPairs() const;

    /// Get the pairs vector size.
    uint64_t allPairsLength() const;

    /// Get a specific pair created by the DEXV2Factory.
    Address getPair(const Address& tokenA, const Address& tokenB) const;

    /// Get a specific pair from the vector given an index.
    Address getPairByIndex(const uint64_t& index) const;

    /**
     * Create a new pair.
     * Solidity counterpart: function createPair(address tokenA, address tokenB) external returns (address pair)
     */
    Address createPair(const Address& tokenA, const Address& tokenB);

    /**
     * set the feeTo address.
     * Solidity counterpart: function setFeeTo(address _feeTo) external
     */
    void setFeeTo(const Address& feeTo);

    /// Set the feeToSetter address.
    void setFeeToSetter(const Address& feeToSetter);

    /// Register the contract functions to the ContractReflectionInterface.
    static void registerContract() {
      static std::once_flag once;
      std::call_once(once, []() {
        DynamicContract::registerContractMethods<DEXV2Factory>(
          std::vector<std::string>{"_feeToSetter"},
          std::make_tuple("feeTo", &DEXV2Factory::feeTo, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("feeToSetter", &DEXV2Factory::feeToSetter, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("allPairs", &DEXV2Factory::allPairs, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("allPairsLength", &DEXV2Factory::allPairsLength, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("getPair", &DEXV2Factory::getPair, FunctionTypes::View, std::vector<std::string>{"token0", "token1"}),
          std::make_tuple("getPairByIndex", &DEXV2Factory::getPairByIndex, FunctionTypes::View, std::vector<std::string>{"index"}),
          std::make_tuple("createPair", &DEXV2Factory::createPair, FunctionTypes::NonPayable, std::vector<std::string>{"tokenA", "tokenB"}),
          std::make_tuple("setFeeTo", &DEXV2Factory::setFeeTo, FunctionTypes::NonPayable, std::vector<std::string>{"_feeTo"}),
          std::make_tuple("setFeeToSetter", &DEXV2Factory::setFeeToSetter, FunctionTypes::NonPayable, std::vector<std::string>{"_feeToSetter"})
        );
      });
    }
  /// Dump method
  DBBatch dump() const override;
};

#endif  // DEXFACTORY_H
