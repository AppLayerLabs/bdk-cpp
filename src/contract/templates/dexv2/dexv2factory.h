/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DEXFACTORY_H
#define DEXFACTORY_H

#include "../../../utils/contractreflectioninterface.h"
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
    SafeUnorderedMap<Address, std::unordered_map<Address, Address, SafeHash>> getPair_;

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
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    DEXV2Factory(
      ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param feeToSetter The address of the feeToSetter.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    DEXV2Factory(
      const Address& feeToSetter,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
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
      ContractReflectionInterface::registerContract<
        DEXV2Factory, const Address&,  ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"_feeToSetter"},
        std::make_tuple("feeTo", &DEXV2Factory::feeTo, "view", std::vector<std::string>{}),
        std::make_tuple("feeToSetter", &DEXV2Factory::feeToSetter, "view", std::vector<std::string>{}),
        std::make_tuple("allPairs", &DEXV2Factory::allPairs, "view", std::vector<std::string>{}),
        std::make_tuple("allPairsLength", &DEXV2Factory::allPairsLength, "view", std::vector<std::string>{}),
        std::make_tuple("getPair", &DEXV2Factory::getPair, "view", std::vector<std::string>{"token0", "token1"}),
        std::make_tuple("getPairByIndex", &DEXV2Factory::getPairByIndex, "view", std::vector<std::string>{"index"}),
        std::make_tuple("createPair", &DEXV2Factory::createPair, "nonpayable", std::vector<std::string>{"tokenA", "tokenB"}),
        std::make_tuple("setFeeTo", &DEXV2Factory::setFeeTo, "nonpayable", std::vector<std::string>{"_feeTo"}),
        std::make_tuple("setFeeToSetter", &DEXV2Factory::setFeeToSetter, "nonpayable", std::vector<std::string>{"_feeToSetter"})
      );
    }
};

#endif  // DEXFACTORY_H
