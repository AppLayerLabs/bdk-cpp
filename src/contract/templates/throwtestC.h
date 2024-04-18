/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef THROWTESTC_H
#define THROWTESTC_H

#include "../dynamiccontract.h"
#include "../../utils/utils.h"

/**
 * ThrowTestC is a simple contract that stores a number.
 * It is used to test the revert functionality for nested calls.
 */
class ThrowTestC : public DynamicContract {
  private:
    SafeUint8_t num_; ///< The number of the contract.
    void registerContractFunctions() override; ///< Register the contract functions.

  public:
    using ConstructorArguments = std::tuple<>; ///< The constructor arguments type.

    /**
    * Constructor from create. Create contract and save it to database.
    * @param interface The interface to the contract manager.
    * @param address The address of the contract.
    * @param creator The address of the creator of the contract.
    * @param chainId The chain ID.
    * @param db The database to use.
    */
    ThrowTestC(const Address& address,
      const Address& creator, const uint64_t& chainId, DB& db
    );

    /**
    * Constructor from load. Load contract from database.
    * @param interface The interface to the contract manager.
    * @param address The address of the contract.
    * @param db The database to use.
    */
    ThrowTestC(const Address& address, DB& db);

    ~ThrowTestC() override; ///< Destructor.

    uint8_t getNumC() const; ///< Getter for `numC`.

    void setNumC(const uint8_t& valC); ///< Setter for `numC`.

    /**
    * Register the contract structure.
    */
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        ThrowTestC, const Address&, const Address&, const uint64_t&, DB&
      >(
        std::vector<std::string>{},
        std::make_tuple("getNumC", &ThrowTestC::getNumC, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("setNumC", &ThrowTestC::setNumC, FunctionTypes::NonPayable, std::vector<std::string>{"valC"})
      );
    }
};

#endif  // THROWTESTB_H
