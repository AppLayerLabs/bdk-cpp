#ifndef NATIVEWRAPPER_H
#define NATIVEWRAPPER_H

#include <memory>

#include "../utils/db.h"
#include "../utils/contractreflectioninterface.h"
#include "abi.h"
#include "dynamiccontract.h"
#include "variables/safestring.h"
#include "variables/safeuint256_t.h"
#include "variables/safeuint8_t.h"
#include "variables/safeunorderedmap.h"

/// Template for a NativeWrapper contract.
class NativeWrapper : public DynamicContract {
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
     * Updates both balance of the address and total supply of the token.
     * @param address The account that will receive the created tokens.
     * @param value The amount of tokens that will be created.
     */
    void _mintValue(const Address& address, const uint256_t& value);

    /// Function for calling the register functions for contracts
    void registerContractFunctions() override;

  public:

    using ConstructorArguments = std::tuple<const std::string &, const std::string &,
                                 const uint8_t &>;
    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    NativeWrapper(
      ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param erc20_name The name of the token.
     * @param erc20_symbol The symbol of the token.
     * @param erc20_decimals The decimals of the token.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain id of the contract.
     * @param db Reference to the database object.
     */
    NativeWrapper(
      const std::string &erc20_name, const std::string &erc20_symbol,
      const uint8_t &erc20_decimals,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator,
      const uint64_t &chainId, const std::unique_ptr<DB> &db
    );

    /// Destructor.
    ~NativeWrapper() override;

    /**
     * Get the name of the token. Solidity counterpart:
     * function name() public view returns (string memory) { return _name; }
     * @return The name of the token.
     */
    Bytes name() const;

    /**
     * Get the symbol/ticker of the token. Solidity counterpart:
     * function symbol() public view returns (string memory) { return _symbol; }
     * @return The symbol/ticker of the token.
     */
    Bytes symbol() const;

    /**
     * Get the number of decimals of the token. Solidity counterpart:
     * function decimals() public view returns (uint8) { return _decimals; }
     * @return The decimals of the token.
     */
    Bytes decimals() const;

    /**
     * Get the total supply of the token. Solidity counterpart:
     * function totalSupply() public view returns (uint256) { return _totalSupply; }
     * @return The total supply of the token.
     */
    Bytes totalSupply() const;

    /**
     * Get the token balance of a specified address. Solidity counterpart:
     * function balanceOf(address _owner) public view returns (uint256) { return _balances[_owner]; }
     * @param _owner The address to get the balance from.
     * @return The total token balance of the address.
     */
    Bytes balanceOf(const Address& _owner) const;

    /**
     * Transfer an amount of tokens from the caller's account to a given address.
     * Solidity counterpart: function transfer(address _to, uint256 _value) public returns (bool)
     * @param _to The address to transfer to.
     * @param _value The amount to be transferred.
     */
    void transfer(const Address& _to, const uint256_t& _value);

    /**
     * Approve a given address to spend a specified amount of tokens on behalf of the caller.
     * Solidity counterpart: function approve(address _spender, uint256 _value) public returns (bool)
     * @param _spender The address which will spend the funds.
     * @param _value The amount of tokens to be spent.
     */
    void approve(const Address& _spender, const uint256_t& _value);

    /**
     * Get the amount that spender is still allowed to withdraw from owner.
     * Solidity counterpart: function allowance(address _owner, address _spender) public view returns (uint256)
     * @param _owner The address of the account that owns the tokens.
     * @param _spender The address of the account able to transfer the tokens.
     * @return The remaining allowed amount.
     */
    Bytes allowance(const Address& _owner, const Address& _spender) const;

    /**
     * Transfer tokens from one address to another. Solidity counterpart:
     * function transferFrom(address _from, address _to, uint _value) public returns (bool)
     * @param _from The address to send tokens from.
     * @param _to The address to send tokens to.
     * @param _value The amount of tokens to be sent.
     */
    void transferFrom(const Address& _from, const Address& _to, const uint256_t& _value);

    /// Deposit tokens to the contract. Solidity counterpart: function deposit() public payable
    void deposit();

    /**
     * Withdraw tokens from the contract. Solidity counterpart: function withdraw(uint256 _value) public payable
     * @param _value The amount of tokens to be withdrawn.
     */
    void withdraw(const uint256_t& _value);

    /// Register contract using ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        NativeWrapper, std::string &, std::string &, uint8_t &,
        ContractManagerInterface &, const Address &,
        const Address &, const uint64_t &, const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc20_name", "erc20_symbol", "erc20_decimals"},
        std::make_tuple("name", &NativeWrapper::name, "view", std::vector<std::string>{}),
        std::make_tuple("symbol", &NativeWrapper::symbol, "view", std::vector<std::string>{}),
        std::make_tuple("decimals", &NativeWrapper::decimals, "view", std::vector<std::string>{}),
        std::make_tuple("totalSupply", &NativeWrapper::totalSupply, "view", std::vector<std::string>{}),
        std::make_tuple("balanceOf", &NativeWrapper::balanceOf, "view", std::vector<std::string>{"_owner"}),
        std::make_tuple("transfer", &NativeWrapper::transfer, "nonpayable", std::vector<std::string>{"_to", "_value"}),
        std::make_tuple("approve", &NativeWrapper::approve, "nonpayable", std::vector<std::string>{"_spender", "_value"}),
        std::make_tuple("allowance", &NativeWrapper::allowance, "view", std::vector<std::string>{"_owner", "_spender"}),
        std::make_tuple("transferFrom", &NativeWrapper::transferFrom, "nonpayable", std::vector<std::string>{"_from", "_to", "_value"}),
        std::make_tuple("deposit", &NativeWrapper::deposit, "payable", std::vector<std::string>{}),
        std::make_tuple("withdraw", &NativeWrapper::withdraw, "payable", std::vector<std::string>{"_value"})
      );
    }
};

#endif // NATIVEWRAPPER_H
