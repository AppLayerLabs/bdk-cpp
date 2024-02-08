/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef ERC20WRAPPER_H
#define ERC20WRAPPER_H

#include <memory>
#include <tuple>

#include "../../utils/db.h"
#include "../abi.h"
#include "../contractmanager.h"
#include "../dynamiccontract.h"
#include "../variables/safeunorderedmap.h"
#include "erc20.h"

/// Template for an ERC20Wrapper contract.
class ERC20Wrapper : public DynamicContract {
  private:
    /**
     * Map for tokens and balances. Solidity counterpart:
     * mapping(address => mapping(address => uint256)) internal tokensAndBalances_;
     */
    SafeUnorderedMap<Address, std::unordered_map<Address, uint256_t, SafeHash>> tokensAndBalances_;

    /// Function for calling the register functions for contracts.
    void registerContractFunctions() override;

  public:

    /**
    * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
    */
    using ConstructorArguments = std::tuple<>;

    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param contractAddress The address where the contract will be deployed.
     * @param db Reference pointer to the database object.
     */
    ERC20Wrapper(
      ContractManagerInterface& interface,
      const Address& contractAddress, DB& db
    );

    /**
     * Constructor for building a new contract from scratch.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain id of the contract.
     * @param db Reference pointer to the database object.
     */
    ERC20Wrapper(
      ContractManagerInterface& interface,
      const Address& address, const Address& creator,
      const uint64_t& chainId, DB& db
    );

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        ERC20Wrapper, ContractManagerInterface&,
        const Address&, const Address&, const uint64_t&,
        DB&
      >(
        std::vector<std::string>{},
        std::make_tuple("getContractBalance", &ERC20Wrapper::getContractBalance, FunctionTypes::View, std::vector<std::string>{"token"}),
        std::make_tuple("getUserBalance", &ERC20Wrapper::getUserBalance, FunctionTypes::View, std::vector<std::string>{"token", "user"}),
        std::make_tuple("withdraw", &ERC20Wrapper::withdraw, FunctionTypes::NonPayable, std::vector<std::string>{"token", "value"}),
        std::make_tuple("transferTo", &ERC20Wrapper::transferTo, FunctionTypes::NonPayable, std::vector<std::string>{"token", "to", "value"}),
        std::make_tuple("deposit", &ERC20Wrapper::deposit, FunctionTypes::NonPayable, std::vector<std::string>{"token", "value"})
      );
    }

    /// Destructor.
    ~ERC20Wrapper() override;

    /**
     * Get the balance of the contract for a specific token.
     * @param token The address of the token.
     * @return The contract's given token balance.
     */
    uint256_t getContractBalance(const Address& token) const;

    /**
     * Get the balance of a specific user for a specific token.
     * @param token The address of the token.
     * @param user The address of the user.
     * @return The user's given token balance.
     */
    uint256_t getUserBalance(const Address& token, const Address& user) const;

    /**
     * Withdraw a specific amount of tokens from the contract. Solidity counterpart:
     * function withdraw (address _token, uint256 _value) public returns (bool)
     * @param token The address of the token.
     * @param value The amount of tokens to withdraw.
     * @throw DynamicException if the contract does not have enough tokens,
     * or if the token was not found.
     */
    void withdraw(const Address& token, const uint256_t& value);

    /**
     * Transfer a specific amount of tokens from the contract to a user. Solidity counterpart:
     * function transferTo(address _token, address _to, uint256 _value) public returns (bool)
     * @param token The address of the token.
     * @param to The address of the user to send tokens to.
     * @param value The amount of tokens to transfer.
     * @throw DynamicException if the contract does not have enough tokens,
     * or if either the token or the user were not found.
     */
    void transferTo(const Address& token, const Address& to, const uint256_t& value);

    /**
     * Deposit a specific amount of tokens to the contract. Solidity counterpart:
     * function deposit(address _token, uint256 _value) public returns (bool)
     * @param token The address of the token.
     * @param value The amount of tokens to deposit.
     */
    void deposit(const Address& token, const uint256_t& value);
};

#endif // ERC20WRAPPER_H
