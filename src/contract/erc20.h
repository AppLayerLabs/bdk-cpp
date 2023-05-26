#ifndef ERC20_H
#define ERC20_H

#include <memory>

#include "../utils/db.h"
#include "abi.h"
#include "dynamiccontract.h"
#include "variables/safestring.h"
#include "variables/safeuint256_t.h"
#include "variables/safeuint8_t.h"
#include "variables/safeunorderedmap.h"

/// Template for an ERC20 contract.
class ERC20 : public DynamicContract {
  private:
    /// Solidity: string internal _name;
    SafeString _name;

    /// Solidity: string internal _symbol;
    SafeString _symbol;

    /// Solidity: uint8 internal _decimals;
    SafeUint8_t _decimals;

    /// Solidity: uint256 internal _totalSupply;
    SafeUint256_t _totalSupply;

    /// Solidity: mapping(address => uint256) internal _balances;
    SafeUnorderedMap<Address, uint256_t> _balances;

    /// Solidity: mapping(address => mapping(address => uint256)) internal _allowed;
    SafeUnorderedMap<Address, std::unordered_map<Address, uint256_t, SafeHash>> _allowed;

    /**
     * Mint new tokens and assign them to the specified address.
     * Updates both balance of the address and total supply of the ERC-20 token.
     * @param address The account that will receive the created tokens.
     * @param value The amount of tokens that will be created.
     */
    void _mintValue(const Address& address, const uint256_t& value);

    /// Function for calling the register functions for contracts.
    void registerContractFunctions() override;

  public:
    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
    */
    ERC20(ContractManager::ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor for creating a new contract from scratch.
     * @param interface Reference to the contract manager interface.
     * @param erc20_name The name of the ERC20 token.
     * @param erc20_symbol The symbol of the ERC20 token.
     * @param erc20_decimals The decimals of the ERC20 token.
     * @param mintValue The amount of tokens that will be minted.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    ERC20(ContractManager::ContractManagerInterface& interface,
      const std::string& erc20_name, const std::string& erc20_symbol,
      const uint8_t& erc20_decimals, const uint256_t& mintValue,
      const Address& address, const Address& creator, const uint64_t& chainId,
      const std::unique_ptr<DB>& db
    );

    /// Destructor.
    ~ERC20() override;

    /**
     * Get the name of the ERC20 token. Solidity counterpart:
     * function name() public view returns (string memory) { return _name; }
     * @return The name of the ERC20 token.
     */
    std::string name() const;


    /**
     * Get the symbol/ticker of the ERC20 token. Solidity counterpart:
     * function symbol() public view returns (string memory) { return _symbol; }
     * @return The symbol/ticker of the ERC20 token.
     */
    std::string symbol() const;


    /**
     * Get the number of decimals of the ERC20 token. Solidity counterpart:
     * function decimals() public view returns (uint8) { return _decimals; }
     * @return The decimals of the ERC20 token.
     */
    std::string decimals() const;


    /**
     * Get the total supply of the ERC20 token. Solidity counterpart:
     * function totalSupply() public view returns (uint256) { return _totalSupply; }
     * @return The total supply of the ERC20 token.
     */
    std::string totalSupply() const;

    /**
     * Get the balance of the ERC20 token from the specified address. Solidity counterpart:
     * function balanceOf(address _owner) public view returns (uint256) { return _balances[_owner]; }
     * @param _owner The address to get the balance from.
     * @return The total balance of the specified address.
     */
    std::string balanceOf(const Address& _owner) const;


    /**
     * Transfer an amount of ERC20 tokens from the sender's account to the specified address.
     * Solidity counterpart: function transfer(address _to, uint256 _value) public returns (bool)
     * @param _to The address to transfer to.
     * @param _value The amount to be transferred.
     */
    void transfer(const Address& _to, const uint256_t& _value);

    /**
     * Set the allowance of the specified address to the specified amount of ERC20 tokens.
     * Solidity counterpart: function approve(address _spender, uint256 _value) public returns (bool)
     * @param _spender The address to approve.
     * @param _value The amount to be approved.
     */
    void approve(const Address& _spender, const uint256_t& _value);

    /**
     * Get the amount which spender is still allowed to withdraw from owner.
     * Solidity counterpart: function allowance(address _owner, address _spender) public view returns (uint256)
     * @param _owner The address to check the allowance of.
     * @param _spender The address to check the allowance for.
     * @return The remaining allowed amount from spender.
     */
    std::string allowance(const Address& _owner, const Address& _spender) const;

    /**
     * Transfer an amount of ERC20 tokens from one address to another.
     * Solidity counterpart: function transferFrom(address _from, address _to, uint _value) public returns (bool)
     * @param _from The address to transfer from.
     * @param _to The address to transfer to.
     * @param _value The amount to be transferred.
     */
    void transferFrom(
      const Address& _from, const Address& _to, const uint256_t& _value
    );
};

#endif /// ERC20_H
