/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef TESTTHROWVARS_H
#define TESTTHROWVARS_H

#include "../dynamiccontract.h"
#include "../variables/safestring.h"

/// Contract for testing throws.
class TestThrowVars : public DynamicContract {
  private:
    ///@{
    /** Test variable. */
    SafeString var1_;
    SafeString var2_;
    SafeString var3_;
    ///@}
    void registerContractFunctions() override; ///< Register the contract's functions.

  public:
    /// The contract's constructor arguments.
    using ConstructorArguments = std::tuple<const std::string&, const std::string&, const std::string&>;

    /**
     * Constructor from create. Create contract and save it to database.
     * @param var1 The value of the respective test variable.
     * @param var2 The value of the respective test variable.
     * @param var3 The value of the respective test variable.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    TestThrowVars(
      const std::string& var1, const std::string& var2, const std::string& var3,
      const Address& address, const Address& creator, const uint64_t& chainId
    );

    /**
     * Constructor from load. Load contract from database.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    TestThrowVars(const Address& address, const DB& db);

    ~TestThrowVars() override; ///< Destructor.

    /// Register the contract.
    static void registerContract() {
      static std::once_flag once;
      std::call_once(once, []() {
        DynamicContract::registerContractMethods<TestThrowVars>(
          std::vector<std::string>{"var1", "var2", "var3"}
        );
      });
    }

    DBBatch dump() const override; ///< Dump method.
};

#endif  // TESTTHROWVARS_H
