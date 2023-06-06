#ifndef NATIVEWRAPPER_H
#define NATIVEWRAPPER_H

#include <memory>

#include "../utils/db.h"
#include "abi.h"
#include "dynamiccontract.h"
#include "variables/safestring.h"
#include "variables/safeuint256_t.h"
#include "variables/safeuint8_t.h"
#include "variables/safeunorderedmap.h"

/**
 * C++ OrbiterSDK Recreation of a NativeWrapper Contract
 */

class NativeWrapper : public DynamicContract {
private:
  /// string internal _name;
  SafeString _name;
  /// string internal _symbol;
  SafeString _symbol;
  /// uint8 internal _decimals;
  SafeUint8_t _decimals;
  /// uint256 internal _totalSupply;
  SafeUint256_t _totalSupply;
  /// mapping(address => uint256) internal _balances;
  SafeUnorderedMap<Address, uint256_t> _balances;
  /// mapping(address => mapping(address => uint256)) internal _allowed;
  SafeUnorderedMap<Address, std::unordered_map<Address, uint256_t, SafeHash>>
      _allowed;

  /**
   * Internal function that mints new tokens and assign them to the specified
   * address. It updates both the balance of the address and the total supply of
   * the ERC-20 token
   * @param account The account that will receive the created tokens.
   * @param value The amount that will be created.
   */
  void _mintValue(const Address &address, const uint256_t &value);

  /**
   * Function for calling the register functions for contracts
   */
  void registerContractFunctions() override;

public:
  /**
   * @brief Default Constructor when loading contract from DB.
   * @param interface Reference to the contract manager interface.
   * @param address The address where the contract will be deployed.
   * @param db Reference to the database object.
   */
  NativeWrapper(ContractManager::ContractManagerInterface &interface,
                const Address &address, const std::unique_ptr<DB> &db);

  /**
  @brief Constructor to be used when creating a new contract.
  @param interface Reference to the contract manager interface.
  @param erc20_name The name of the ERC20 token.
  @param erc20_symbol The symbol of the ERC20 token.
  @param erc20_decimals The decimals of the ERC20 token.
  @param address The address where the contract will be deployed.
  @param creator The address of the creator of the contract.
  @param chainId The chain id of the contract.
  @param db Reference to the database object.
  */
  NativeWrapper(const std::string &erc20_name, const std::string &erc20_symbol,
                const uint8_t &erc20_decimals,
                ContractManager::ContractManagerInterface &interface,
                const Address &address, const Address &creator,
                const uint64_t &chainId, const std::unique_ptr<DB> &db);

  static void registerContract() {
    ContractReflectionInterface::registerContract<
        NativeWrapper, std::string &, std::string &, uint8_t &,
        ContractManager::ContractManagerInterface &, const Address &,
        const Address &, const uint64_t &, const std::unique_ptr<DB> &>(
        std::vector<std::string>{"erc20_name", "erc20_symbol",
                                 "erc20_decimals"},
        std::make_tuple("name", &NativeWrapper::name, "view",
                        std::vector<std::string>{}),
        std::make_tuple("symbol", &NativeWrapper::symbol, "view",
                        std::vector<std::string>{}),
        std::make_tuple("decimals", &NativeWrapper::decimals, "view",
                        std::vector<std::string>{}),
        std::make_tuple("totalSupply", &NativeWrapper::totalSupply, "view",
                        std::vector<std::string>{}),
        std::make_tuple("balanceOf", &NativeWrapper::balanceOf, "view",
                        std::vector<std::string>{"_owner"}),
        std::make_tuple("transfer", &NativeWrapper::transfer, "nonpayable",
                        std::vector<std::string>{"_to", "_value"}),
        std::make_tuple("approve", &NativeWrapper::approve, "nonpayable",
                        std::vector<std::string>{"_spender", "_value"}),
        std::make_tuple("allowance", &NativeWrapper::allowance, "view",
                        std::vector<std::string>{"_owner", "_spender"}),
        std::make_tuple("transferFrom", &NativeWrapper::transferFrom,
                        "nonpayable",
                        std::vector<std::string>{"_from", "_to", "_value"}),
        std::make_tuple("deposit", &NativeWrapper::deposit, "payable",
                        std::vector<std::string>{}),
        std::make_tuple("withdraw", &NativeWrapper::withdraw, "payable",
                        std::vector<std::string>{"_value"}));
  }

  /**
   * @brief Default Destructor.
   */
  ~NativeWrapper() override;

  /// function name() public view returns (string memory) { return _name; }

  /**
   * @brief Returns the name of the ERC20 token.
   * @return The name of the ERC20 token.
   */
  std::string name() const;

  /// function symbol() public view returns (string memory) { return _symbol; }

  /**
   * @brief Returns the symbol of the ERC20 token.
   * @return The symbol of the ERC20 token.
   */
  std::string symbol() const;

  /// function decimals() public view returns (uint8) { return _decimals; }

  /**
   * @brief Returns the decimals of the ERC20 token.
   * @return The decimals of the ERC20 token.
   */
  std::string decimals() const;

  /// function totalSupply() public view returns (uint256) { return
  /// _totalSupply; }

  /**
   * @brief Returns the total supply of the ERC20 token.
   * @return The total supply of the ERC20 token.
   */
  std::string totalSupply() const;

  /// function balanceOf(address _owner) public view returns (uint256) { return
  /// _balances[_owner]; }

  /**
   * @brief Returns the balance of the specified address.
   * @param _owner The address to query the balance of.
   * @return The balance of the specified address.
   */
  std::string balanceOf(const Address &_owner) const;

  /// function transfer(address _to, uint256 _value) public returns (bool)

  /**
   * @brief Transfers tokens from the caller's account to the specified
   * address.
   * @param _to The address to transfer to.
   * @param _value The amount to be transferred.
   */
  void transfer(const Address &_to, const uint256_t &_value);

  /// function approve(address _spender, uint256 _value) public returns (bool)

  /**
   * @brief Approves the passed address to spend the specified amount of tokens
   * on behalf of the caller.
   * @param _spender The address which will spend the funds.
   * @param _value The amount of tokens to be spent.
   */
  void approve(const Address &_spender, const uint256_t &_value);

  /// function allowance(address _owner, address _spender) public view returns
  /// (uint256)

  /**
   * @brief Returns the amount which _spender is still allowed to withdraw from
   * _owner.
   * @param _owner The address of the account owning tokens.
   * @param _spender The address of the account able to transfer the tokens.
   * @return The amount which _spender is still allowed to withdraw from
   * _owner.
   */
  std::string allowance(const Address &_owner, const Address &_spender) const;

  /// function transferFrom(address _from, address _to, uint _value) public
  /// returns (bool)

  /**
   * @brief Transfers tokens from one address to another.
   * @param _from The address which you want to send tokens from.
   * @param _to The address which you want to transfer to.
   * @param _value The amount of tokens to be transferred.
   */
  void transferFrom(const Address &_from, const Address &_to,
                    const uint256_t &_value);

  /// function deposit() public payable

  /**
   * @brief Deposits tokens to the contract.
   */
  void deposit();

  /// function withdraw(uint256 _value) public payable

  /**
   * @brief Withdraws tokens from the contract.
   * @param _value The amount of tokens to be withdrawn.
   */
  void withdraw(const uint256_t &_value);
};

#endif /// NATIVEWRAPPER_H