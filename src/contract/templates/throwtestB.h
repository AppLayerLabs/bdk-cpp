/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef THROWTESTB_H
#define THROWTESTB_H

#include "../dynamiccontract.h"
#include "../../utils/utils.h"
#include "throwtestC.h"

/**
 * ThrowTestB is a simple contract that stores a number.
 * It is used to test the revert functionality for nested calls.
 */
class ThrowTestB : public DynamicContract {
  private:
    SafeUint8_t num_; ///< The number of the contract.
    void registerContractFunctions() override; ///< Register the contract functions.

  public:
    using ConstructorArguments = std::tuple<>; ///< The constructor arguments type.

    /**
    * Constructor from create. Create contract and save it to database.
    * @param address The address of the contract.
    * @param creator The address of the creator of the contract.
    * @param chainId The chain ID.
    */
    ThrowTestB(const Address& address, const Address& creator, const uint64_t& chainId);

    /**
    * Constructor from load. Load contract from database.
    * @param address The address of the contract.
    * @param db The database to use.
    */
    ThrowTestB(const Address& address, const DB& db);

    ~ThrowTestB() override; ///< Destructor.

    uint8_t getNumB() const; ///< Getter for `numB`.

    [[noreturn]] void setNumB(const uint8_t& valB, const Address& addC, const uint8_t& valC); ///< Setter for `numB`.

    /**
    * Register the contract structure.
    */
    static void registerContract() {
      static std::once_flag once;
      std::call_once(once, []() {
        DynamicContract::registerContractMethods<ThrowTestB>(
          std::vector<std::string>{},
          std::make_tuple("getNumB", &ThrowTestB::getNumB, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("setNumB", &ThrowTestB::setNumB, FunctionTypes::NonPayable, std::vector<std::string>{"valB", "addC", "valC"})
        );
      });
    }

    /// Dump method
    DBBatch dump() const override;
};

#endif  // THROWTESTB_H
