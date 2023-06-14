#ifndef ERC20_H
#define ERC20_H

#include <memory>

#include "../utils/contractreflectioninterface.h"
#include "../utils/db.h"
#include "abi.h"
#include "dynamiccontract.h"
#include "variables/safestring.h"
#include "variables/safeuint256_t.h"
#include "variables/safeuint8_t.h"
#include "variables/safeunorderedmap.h"

/**
 * C++ OrbiterSDK Recreation of a ERC20 Contract
 */

class ERC20 : public DynamicContract {
private:
  /// string internal _name;
  SafeString _name;
  /// string internal _symbol;
  SafeString _symbol;
  /// uint8 internal _decimals;
  SafeUint256_t _decimals;
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
  @brief Default Constructor when loading contract from DB.
  @param interface Reference to the contract manager interface.
  @param address The address where the contract will be deployed.
  @param db Reference to the database object.
  */
  ERC20(ContractManagerInterface &interface,
        const Address &address, const std::unique_ptr<DB> &db);

  /**
   *@brief Constructor to be used when creating a new contract.
   *@param interface Reference to the contract manager interface.
   *@param erc20_name The name of the ERC20 token.
   *@param erc20_symbol The symbol of the ERC20 token.
   *@param erc20_decimals The decimals of the ERC20 token.
   *@param mintValue The amount of tokens that will be minted.
   *@param address The address where the contract will be deployed.
   *@param creator The address of the creator of the contract.
   *@param chainId The chain where the contract wil be deployed.
   *@param db Reference to the database object.
   */
  ERC20(const std::string &erc20_name, const std::string &erc20_symbol,
        const uint256_t &erc20_decimals, const uint256_t &mintValue,
        ContractManagerInterface &interface, const Address &address,
        const Address &creator, const uint64_t &chainId, const std::unique_ptr<DB> &db);

  /**
   * Register contract class via ContractReflectionInterface.
   */
  static void registerContract() {
    ContractReflectionInterface::registerContract<
        ERC20, const std::string &, const std::string &, const uint256_t &,
        const uint256_t &, ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &>(
        std::vector<std::string>{"erc20_name", "erc20_symbol", "erc20_decimals",
                                 "mintValue"},
        std::make_tuple("name", &ERC20::name, "view",
                        std::vector<std::string>{}),
        std::make_tuple("symbol", &ERC20::symbol, "view",
                        std::vector<std::string>{}),
        std::make_tuple("decimals", &ERC20::decimals, "view",
                        std::vector<std::string>{}),
        std::make_tuple("totalSupply", &ERC20::totalSupply, "view",
                        std::vector<std::string>{}),
        std::make_tuple("balanceOf", &ERC20::balanceOf, "view",
                        std::vector<std::string>{"_owner"}),
        std::make_tuple("transfer", &ERC20::transfer, "nonpayable",
                        std::vector<std::string>{"_to", "_value"}),
        std::make_tuple("approve", &ERC20::approve, "nonpayable",
                        std::vector<std::string>{"_spender", "_value"}),
        std::make_tuple("allowance", &ERC20::allowance, "view",
                        std::vector<std::string>{"_owner", "_spender"}),
        std::make_tuple("transferFrom", &ERC20::transferFrom, "nonpayable",
                        std::vector<std::string>{"_from", "_to", "_value"}));
  }

  /**
   * @brief Default Destructor.
   */
  ~ERC20() override;

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
   * @brief Transfers tokens from the sender's account to the specified
   * address.
   * @param _to The address to transfer to.
   * @param _value The amount to be transferred.
   */
  void transfer(const Address &_to, const uint256_t &_value);

  /// function approve(address _spender, uint256 _value) public returns (bool)

  /**
   * @brief Sets the allowance of the specified address to the specified
   * amount.
   * @param _spender The address to approve.
   * @param _value The amount to be approved.
   */
  void approve(const Address &_spender, const uint256_t &_value);

  /// function allowance(address _owner, address _spender) public view returns
  /// (uint256)

  /**
   * @brief Returns the amount which _spender is still allowed to withdraw from
   * _owner.
   * @param _owner The address to check the allowance of.
   * @param _spender The address to check the allowance for.
   * @return The amount which _spender is still allowed to withdraw from
   * _owner.
   */
  std::string allowance(const Address &_owner, const Address &_spender) const;

  /// function transferFrom(address _from, address _to, uint _value) public
  /// returns (bool)

  /**
   * @brief Transfers tokens from one address to another.
   * @param _from The address to transfer from.
   * @param _to The address to transfer to.
   * @param _value The amount to be transferred.
   */
  void transferFrom(const Address &_from, const Address &_to,
                    const uint256_t &_value);
};

#endif /// ERC20_H