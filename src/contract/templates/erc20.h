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

/// Template for an ERC20 Token
/// Based on the OpenZeppelin implementation of the ERC20 token
class ERC20 : public DynamicContract {
  protected:
    /// Solidity: string internal _name;
    SafeString _name;

    /// Solidity: string internal _symbol;
    SafeString _symbol;

    /// Solidity: uint8 internal _decimals;
    SafeUint8_t _decimals;

    /// Solidity: uint256 internal totalSupply;
    SafeUint256_t _totalSupply;

    /// Solidity: mapping(address => uint256) internal _balances;
    SafeUnorderedMap<Address, uint256_t> _balances;

    /// Solidity: mapping(address => mapping(address => uint256)) internal _allowed;
    SafeUnorderedMap<Address, std::unordered_map<Address, uint256_t, SafeHash>> _allowances;

    /**
     * Transfer the tokens from to the specified address.
     * Solidity counterpart: function _transfer(address from, address to, uint256 value) internal
     * @param from The address to transfer from.
     * @param to The address to transfer to.
     */
    void _transfer(const Address& from, const Address& to, const uint256_t& value);

    /**
     * Mint new tokens and assign them to the specified address.
     * Solidity counterpart: function _mint(address account, uint256 value) internal
     * @param address The account that will receive the created tokens.
     * @param value The amount of tokens that will be created.
     */
    void _mint(const Address& address, const uint256_t& value);

    /**
     * Burn tokens from the specified address.
     * Solidity counterpart: function _burn(address account, uint256 value) internal
     * @param address The account that will lose the tokens.
     * @param value The amount of tokens that will be burned.
     */
    void _burn(const Address& address, const uint256_t& value);

    /**
     * Set the allowance of the specified address to the specified amount of ERC20 tokens.
     * Solidity counterparit: function _approve(address owner, address spender, uint256 value) internal
     * @param owner The address that will approve the allowance.
     * @param spender The address that will be allowed to spend the tokens.
     * @param value The amount of tokens that will be allowed to be spent.
     */
    void _approve(const Address& owner, const Address& spender, const uint256_t& value);

    /**
     * Spends the allowance of the specified address.
     * Solidity counterpart: function _spendAllowance(address owner, address spender, uint256 value) internal
     * @param owner The address that will spend the allowance.
     * @param spender The address that will be allowed to spend the tokens.
     * @param value The amount of tokens that will be spent.
     */
    void _spendAllowance(const Address& owner, const Address& spender, const uint256_t& value);

    /**
     * Transfer the value of tokens from to to, or alternatively mints or burns if from or to is the zero address.
     * Solidity counterpart: function _update(address from, address to, uint256 value) internal virtual
     * @params from The address to transfer from.
     * @params to The address to transfer to.
     * @params value The amount of tokens that will be transferred.
     */

    void _update(const Address& from, const Address& to, const uint256_t& value);

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
     * @param erc20_name The name of the ERC20 token.
     * @param erc20_symbol The symbol of the ERC20 token.
     * @param erc20_decimals The decimals of the ERC20 token.
     * @param mintValue The amount of tokens that will be minted.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    ERC20(
      const std::string &erc20_name, const std::string &erc20_symbol,
      const uint8_t &erc20_decimals, const uint256_t &mintValue,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    /// Constructor for derived types!
    ERC20(
      const std::string &derivedTypeName,
      const std::string &erc20_name, const std::string &erc20_symbol,
      const uint8_t &erc20_decimals, const uint256_t &mintValue,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    /// Destructor.
    virtual ~ERC20() override;

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
    uint8_t decimals() const;


    /**
     * Get the total supply of the ERC20 token. Solidity counterpart:
     * function totalSupply() public view returns (uint256) { return totalSupply; }
     * @return The total supply of the ERC20 token.
     */
    uint256_t totalSupply() const;

    /**
     * Get the balance of the ERC20 token from the specified address. Solidity counterpart:
     * function balanceOf(address owner) public view returns (uint256) { return _balances[owner]; }
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
     * Get the amount which spender is still allowed to withdraw from owner.
     * Solidity counterpart: function allowance(address owner, address spender) public view returns (uint256)
     * @param owner The address to check the allowance of.
     * @param spender The address to check the allowance for.
     * @return The remaining allowed amount from spender.
     */
    uint256_t allowance(const Address& owner, const Address& spender) const;

    /**
     * Set the allowance of the specified address to the specified amount of ERC20 tokens.
     * Solidity counterpart: function approve(address spender, uint256 value) public returns (bool)
     * @param spender The address to approve.
     * @param value The amount to be approved.
     */
    void approve(const Address& spender, const uint256_t& value);

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

    /**
     * Increase allowance of the specified address by the specified amount of ERC20 tokens.
     * Solidity counterpart: function increaseAllowance(address spender, uint256 addedValue) public virtual returns
     * @param spender The address to increase the allowance for.
     * @param _addedValue The amount to increase the allowance by.
     */
    void increaseAllowance(const Address& spender, const uint256_t& _addedValue);

    /**
     * Decrease allowance of the specified address by the specified amount of ERC20 tokens.
     * Solidity counterpart: function decreaseAllowance(address spender, uint256 requestedDecrease) public virtual returns (bool)
     * @param spender The address to decrease the allowance for.
     * @param _requestedDecrease The amount to decrease the allowance by.
     */
    void decreaseAllowance(const Address& spender, const uint256_t& _requestedDecrease);

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ERC20, const std::string &, const std::string &, const uint8_t &,
        const uint256_t &, ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc20_name", "erc20_symbol", "erc20_decimals", "mintValue"},
        std::make_tuple("name", &ERC20::name, "view", std::vector<std::string>{}),
        std::make_tuple("symbol", &ERC20::symbol, "view", std::vector<std::string>{}),
        std::make_tuple("decimals", &ERC20::decimals, "view", std::vector<std::string>{}),
        std::make_tuple("totalSupply", &ERC20::totalSupply, "view", std::vector<std::string>{}),
        std::make_tuple("balanceOf", &ERC20::balanceOf, "view", std::vector<std::string>{"owner"}),
        std::make_tuple("transfer", &ERC20::transfer, "nonpayable", std::vector<std::string>{"to", "value"}),
        std::make_tuple("approve", &ERC20::approve, "nonpayable", std::vector<std::string>{"spender", "value"}),
        std::make_tuple("allowance", &ERC20::allowance, "view", std::vector<std::string>{"owner", "spender"}),
        std::make_tuple("transferFrom", &ERC20::transferFrom, "nonpayable", std::vector<std::string>{"from", "to", "value"}),
        std::make_tuple("increaseAllowance", &ERC20::increaseAllowance, "nonpayable", std::vector<std::string>{"spender", "addedValue"}),
        std::make_tuple("decreaseAllowance", &ERC20::decreaseAllowance, "nonpayable", std::vector<std::string>{"spender", "requestedDecrease"})
      );
    }
};

#endif /// ERC20_H
