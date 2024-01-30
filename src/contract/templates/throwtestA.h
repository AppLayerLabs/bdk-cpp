/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef THROWTESTA_H
#define THROWTESTA_H

#include "../dynamiccontract.h"
#include "../../utils/utils.h"
#include "throwtestB.h"

/**
 * ThrowTestA is a simple contract that stores a number.
 * It is used to test the revert functionality for nested calls.
 */
class ThrowTestA : public DynamicContract {
  private:
    SafeUint8_t num_; ///< The number of the contract.
    void registerContractFunctions() override; ///< Register the contract functions.

  public:
    using ConstructorArguments = std::tuple<>; ///< The constructor arguments type.

    /**
    * Constructor.
    * @param interface The interface to the contract manager.
    * @param address The address of the contract.
    * @param creator The address of the creator of the contract.
    * @param chainId The chain ID.
    * @param db The database to use.
    */
    ThrowTestA(ContractManagerInterface &interface, const Address& address,
      const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    );

    /**
    * Constructor for contract loading.
    * @param interface The interface to the contract manager.
    * @param address The address of the contract.
    * @param db The database to use.
    */
    ThrowTestA(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db);

    ~ThrowTestA() override; ///< Destructor.

    uint8_t getNumA() const; ///< Getter for `numA`.

    /**
    * Setter for `numA`.
    * @param valA The value to set `numA` to.
    * @param addB The address of the contract to call.
    * @param valB The value to set `numB` to.
    * @param addC The address of the contract to call.
    * @param valC The value to set `numC` to.
    */
    void setNumA(const uint8_t& valA,
      const Address& addB, const uint8_t& valB,
      const Address& addC, const uint8_t& valC
    );

    /**
    * Register the contract structure.
    */
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        ThrowTestA, ContractManagerInterface&, const Address&, const Address&, const uint64_t&, const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{},
        std::make_tuple("getNumA", &ThrowTestA::getNumA, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("setNumA", &ThrowTestA::setNumA, FunctionTypes::NonPayable, std::vector<std::string>{"valA", "addB", "valB", "addC", "valC"})
      );
    }
};

#endif  // THROWTESTA_H
