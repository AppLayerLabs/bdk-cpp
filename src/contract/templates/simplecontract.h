/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SIMPLECONTRACT_H
#define SIMPLECONTRACT_H

#include "../dynamiccontract.h"
#include "../variables/safestring.h"
#include "../../utils/utils.h" // SafeUintX_t aliases declared here

/**
 * SimpleContract is a simple contract that stores a name and a value.
 * It is used to test the contract manager.
 */
class SimpleContract : public DynamicContract {
  private:
    SafeString name_; ///< The name of the contract.
    SafeUint256_t value_; ///< The value of the contract.
    void registerContractFunctions() override; ///< Register the contract functions.

  protected:
    /// Event for when the name changes.
    void nameChanged(const std::string& name) { this->emitEvent(__func__, { {name, true} }); }

    /// Event for when the value changes.
    void valueChanged(uint256_t value) { this->emitEvent(__func__, { {value, true} }); }

  public:
    using ConstructorArguments = std::tuple<const std::string&, uint256_t>; ///< The constructor arguments type.

    /**
    * Constructor from create. Create contract and save it to database.
    * @param name The name of the contract.
    * @param value The value of the contract.
    * @param interface The interface to the contract manager.
    * @param address The address of the contract.
    * @param creator The address of the creator of the contract.
    * @param chainId The chain ID.
    * @param db The database to use.
    */
    SimpleContract(
      const std::string& name,
      uint256_t value,
      ContractManagerInterface &interface,
      const Address& address,
      const Address& creator,
      const uint64_t& chainId,
      const std::unique_ptr<DB> &db
    );

    /**
    * Constructor from load. Load contract from database.
    * @param interface The interface to the contract manager.
    * @param address The address of the contract.
    * @param db The database to use.
    */
    SimpleContract(
      ContractManagerInterface &interface,
      const Address& address,
      const std::unique_ptr<DB> &db
    );

    ~SimpleContract() override; ///< Destructor.

    std::string getName() const;  ///< function getName() public view returns(string memory)
    uint256_t getValue() const;   ///< function getValue() public view returns(uint256)
    void setName(const std::string& argName); ///< function setName(string memory argName) public
    void setValue(uint256_t argValue);  ///< function setValue(uint256 argValue) public

    /**
    * Register the contract structure.
    */
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        SimpleContract, const std::string&, uint256_t,
        ContractManagerInterface&,
        const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{"name_", "value_"},
        std::make_tuple("getName", &SimpleContract::getName, "view", std::vector<std::string>{}),
        std::make_tuple("getValue", &SimpleContract::getValue, "view", std::vector<std::string>{}),
        std::make_tuple("setName", &SimpleContract::setName, "nonpayable", std::vector<std::string>{"argName"}),
        std::make_tuple("setValue", &SimpleContract::setValue, "nonpayable", std::vector<std::string>{"argValue"})
      );
    }
};

#endif // SIMPLECONTRACT_H
