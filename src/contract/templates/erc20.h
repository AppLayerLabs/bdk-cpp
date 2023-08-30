/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef ERC20_H
#define ERC20_H

#include <memory>

#include "../../utils/contractreflectioninterface.h"
#include "../../utils/db.h"
#include "../../utils/utils.h"
#include "../abi.h"
#include "../dynamiccontract.h"
#include "../variables/safestring.h"
#include "../variables/safeuint.h"
#include "../variables/safeunorderedmap.h"

/// Template for an ERC20 contract.
class ERC20 : public DynamicContract {
  protected:
    /// Solidity: string internal name_;
    SafeString name_;

    /// Solidity: string internal symbol_;
    SafeString symbol_;

    /// Solidity: uint8 internal decimals_;
    SafeUint8_t decimals_;

    /// Solidity: uint256 internal totalSupply_;
    SafeUint256_t totalSupply_;

    /// Solidity: mapping(address => uint256) internal balances_;
    SafeUnorderedMap<Address, uint256_t> balances_;

    /// Solidity: mapping(address => mapping(address => uint256)) internal allowed_;
    SafeUnorderedMap<Address, std::unordered_map<Address, uint256_t, SafeHash>> allowed_;

    /**
     * Mint new tokens and assign them to the specified address.
     * Updates both balance of the address and total supply of the ERC-20 token.
     * @param address The account that will receive the created tokens.
     * @param value The amount of tokens that will be created.
     */
    void mintValue_(const Address& address, const uint256_t& value);

    /**
     * Burn tokens from the specified address.
     * Updates both balance of the address and total supply of the ERC-20 token.
     * @param address The account that will lose the tokens.
     * @param value The amount of tokens that will be burned.
     */
    void burnValue_(const Address& address, const uint256_t& value);

    /// Function for calling the register functions for contracts.
    void registerContractFunctions() override;

  public:

    /**
    * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
    */
    using ConstructorArguments = std::tuple<const std::string&, const std::string&, const uint8_t&, const uint256_t&>;
    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
    */
    ERC20(
      ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param erc20name The name of the ERC20 token.
     * @param erc20symbol The symbol of the ERC20 token.
     * @param erc20decimals The decimals of the ERC20 token.
     * @param mintValue The amount of tokens that will be minted.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    ERC20(
      const std::string &erc20name, const std::string &erc20symbol,
      const uint8_t &erc20decimals, const uint256_t &mintValue,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    /// Constructor for derived types!
    ERC20(
      const std::string &derivedTypeName,
      const std::string &erc20name, const std::string &erc20symbol,
      const uint8_t &erc20decimals, const uint256_t &mintValue,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    /// Destructor.
    ~ERC20() override;

    /**
     * Get the name of the ERC20 token. Solidity counterpart:
     * function name() public view returns (string memory) { return name_; }
     * @return The name of the ERC20 token.
     */
    std::string name() const;


    /**
     * Get the symbol/ticker of the ERC20 token. Solidity counterpart:
     * function symbol() public view returns (string memory) { return symbol_; }
     * @return The symbol/ticker of the ERC20 token.
     */
    std::string symbol() const;


    /**
     * Get the number of decimals of the ERC20 token. Solidity counterpart:
     * function decimals() public view returns (uint8) { return decimals_; }
     * @return The decimals of the ERC20 token.
     */
    uint8_t decimals() const;


    /**
     * Get the total supply of the ERC20 token. Solidity counterpart:
     * function totalSupply() public view returns (uint256) { return totalSupply_; }
     * @return The total supply of the ERC20 token.
     */
    uint256_t totalSupply() const;

    /**
     * Get the balance of the ERC20 token from the specified address. Solidity counterpart:
     * function balanceOf(address owner) public view returns (uint256) { return balances_[owner]; }
     * @param owner The address to get the balance from.
     * @return The total balance of the specified address.
     */
    uint256_t balanceOf(const Address& owner) const;


    /**
     * Transfer an amount of ERC20 tokens from the sender's account to the specified address.
     * Solidity counterpart: function transfer(address to, uint256 value) public returns (bool)
     * @param to The address to transfer to.
     * @param value The amount to be transferred.
     */
    void transfer(const Address& to, const uint256_t& value);

    /**
     * Set the allowance of the specified address to the specified amount of ERC20 tokens.
     * Solidity counterpart: function approve(address spender, uint256 value) public returns (bool)
     * @param spender The address to approve.
     * @param value The amount to be approved.
     */
    void approve(const Address& spender, const uint256_t& value);

    /**
     * Get the amount which spender is still allowed to withdraw from owner.
     * Solidity counterpart: function allowance(address owner, address spender) public view returns (uint256)
     * @param owner The address to check the allowance of.
     * @param spender The address to check the allowance for.
     * @return The remaining allowed amount from spender.
     */
    uint256_t allowance(const Address& owner, const Address& spender) const;

    /**
     * Transfer an amount of ERC20 tokens from one address to another.
     * Solidity counterpart: function transferFrom(address from, address to, uint value) public returns (bool)
     * @param from The address to transfer from.
     * @param to The address to transfer to.
     * @param value The amount to be transferred.
     */
    void transferFrom(
      const Address& from, const Address& to, const uint256_t& value
    );

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ERC20, const std::string &, const std::string &, const uint8_t &,
        const uint256_t &, ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc20name", "erc20symbol", "erc20decimals", "mintValue"},
        std::make_tuple("name", &ERC20::name, "view", std::vector<std::string>{}),
        std::make_tuple("symbol", &ERC20::symbol, "view", std::vector<std::string>{}),
        std::make_tuple("decimals", &ERC20::decimals, "view", std::vector<std::string>{}),
        std::make_tuple("totalSupply", &ERC20::totalSupply, "view", std::vector<std::string>{}),
        std::make_tuple("balanceOf", &ERC20::balanceOf, "view", std::vector<std::string>{"owner"}),
        std::make_tuple("transfer", &ERC20::transfer, "nonpayable", std::vector<std::string>{"to", "value"}),
        std::make_tuple("approve", &ERC20::approve, "nonpayable", std::vector<std::string>{"spender", "value"}),
        std::make_tuple("allowance", &ERC20::allowance, "view", std::vector<std::string>{"owner", "spender"}),
        std::make_tuple("transferFrom", &ERC20::transferFrom, "nonpayable", std::vector<std::string>{"from", "to", "value"})
      );
    }
};

#endif /// ERC20_H
