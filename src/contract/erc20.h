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

/// Template for an ERC20 contract.
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

    /// Solidity: string internal _symbol;
    SafeString _symbol;

    /// Solidity: uint8 internal _decimals;
    SafeUint8_t _decimals;

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
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc20_name", "erc20_symbol", "erc20_decimals", "mintValue"},
        std::make_tuple("name", &ERC20::name, "view", std::vector<std::string>{}),
        std::make_tuple("symbol", &ERC20::symbol, "view", std::vector<std::string>{}),
        std::make_tuple("decimals", &ERC20::decimals, "view", std::vector<std::string>{}),
        std::make_tuple("totalSupply", &ERC20::totalSupply, "view", std::vector<std::string>{}),
        std::make_tuple("balanceOf", &ERC20::balanceOf, "view", std::vector<std::string>{"_owner"}),
        std::make_tuple("transfer", &ERC20::transfer, "nonpayable", std::vector<std::string>{"_to", "_value"}),
        std::make_tuple("approve", &ERC20::approve, "nonpayable", std::vector<std::string>{"_spender", "_value"}),
        std::make_tuple("allowance", &ERC20::allowance, "view", std::vector<std::string>{"_owner", "_spender"}),
        std::make_tuple("transferFrom", &ERC20::transferFrom, "nonpayable", std::vector<std::string>{"_from", "_to", "_value"})
      );
    }
};

#endif /// ERC20_H
