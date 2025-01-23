/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef NATIVEWRAPPER_H
#define NATIVEWRAPPER_H

#include <memory>

#include "../../utils/db.h"
#include "../../utils/utils.h"
#include "../../utils/contractreflectioninterface.h"
#include "../abi.h"
#include "../dynamiccontract.h"
#include "../variables/safestring.h"
#include "../variables/safeuint.h"
#include "../variables/safeunorderedmap.h"
#include "erc20.h"

/// Template for a NativeWrapper contract.
class NativeWrapper : public ERC20 {
  private:
    /// Function for calling the register functions for contracts
    void registerContractFunctions() override;

  public:
    /// ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
    using ConstructorArguments = std::tuple<
      const std::string &, const std::string &, const uint8_t &
    >;

    /**
     * Constructor for loading contract from DB.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    NativeWrapper(const Address& address, const DB& db);

    /**
     * Constructor to be used when creating a new contract.
     * @param erc20_name The name of the token.
     * @param erc20_symbol The symbol of the token.
     * @param erc20_decimals The decimals of the token.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain id of the contract.
     */
    NativeWrapper(
      const std::string &erc20_name, const std::string &erc20_symbol,
      const uint8_t &erc20_decimals,
      const Address &address, const Address &creator,
      const uint64_t &chainId
    );

    /// Destructor.
    ~NativeWrapper() override;

    /**
     * Deposit tokens to the contract.
     * Solidity counterpart: function deposit() public payable
     */
    void deposit();

    /**
     * Withdraw tokens from the contract.
     * Solidity counterpart: function withdraw(uint256 value) public payable
     * @param value The amount of tokens to be withdrawn.
     */
    void withdraw(const uint256_t& value);

    /// Register contract using ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        NativeWrapper, std::string &, std::string &, uint8_t &,
        const Address &,
        const Address &, const uint64_t &//, DB&
      >(
        std::vector<std::string>{"erc20_name", "erc20_symbol", "erc20_decimals"},
        std::make_tuple("deposit", &NativeWrapper::deposit, FunctionTypes::Payable, std::vector<std::string>{}),
        std::make_tuple("withdraw", &NativeWrapper::withdraw, FunctionTypes::Payable, std::vector<std::string>{"value"})
      );
    }

    /// Dump method
    DBBatch dump() const override;
};

#endif // NATIVEWRAPPER_H
