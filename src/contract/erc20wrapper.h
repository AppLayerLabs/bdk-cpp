#ifndef ERC20WRAPPER_H
#define ERC20WRAPPER_H

#include <memory>

#include "../utils/db.h"
#include "abi.h"
#include "dynamiccontract.h"
#include "erc20.h"
#include "variables/safeuint256_t.h"
#include "variables/safeunorderedmap.h"

class ERC20Wrapper : public DynamicContract {
private:
  /// ERC20 Address => UserAddress/UserBalance
  /// mapping(address => mapping(address => uint256)) internal
  /// _tokensAndBalances;
  SafeUnorderedMap<Address, std::unordered_map<Address, uint256_t, SafeHash>>
      _tokensAndBalances;

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
  ERC20Wrapper(ContractManager::ContractManagerInterface &interface,
               const Address &contractAddress, const std::unique_ptr<DB> &db);

  /**
   * @brief Default Constructor when building a new contract
   * @param interface Reference to the contract manager interface.
   * @param address The address where the contract will be deployed.
   * @param creator The address of the creator of the contract.
   * @param chainId The chain id of the contract.
   * @param db Reference to the database object.
   */
  ERC20Wrapper(ContractManager::ContractManagerInterface &interface,
               const Address &address, const Address &creator,
               const uint64_t &chainId, const std::unique_ptr<DB> &db);

  /**
   * @brief Default Destructor
   */
  ~ERC20Wrapper() override;

  /// function getContractBalance(address _token) public view returns (uint256)
  /// { return _tokensAndBalances[_token][address(this)]; }

  /**
   * @brief Returns the balance of the contract for a specific token.
   * @param token The address of the token.
   * @return The balance of the contract for a specific token.
   */
  std::string getContractBalance(const Address &token) const;

  /// function getUserBalance(address _token, address _user) public view returns
  /// (uint256) { return _tokensAndBalances[_token][_user]; }

  /**
   * @brief Returns the balance of a user for a specific token.
   * @param token The address of the token.
   * @param user The address of the user.
   * @return The balance of a user for a specific token.
   */
  std::string getUserBalance(const Address &token, const Address &user) const;

  /// function withdraw (address _token, uint256 _value) public returns (bool)

  /**
   * @brief Withdraws a specific amount of tokens from the contract.
   * @param token The address of the token.
   * @param value The amount of tokens to withdraw.
   * @throws std::runtime_error if the contract does not have enough tokens, or
   * if the token was not found, or if the user was not found.
   */
  void withdraw(const Address &token, const uint256_t &value);

  /// function transferTo(address _token, address _to, uint256 _value) public
  /// returns (bool)

  /**
   * @brief Transfers a specific amount of tokens from the contract to a user.
   * @param token The address of the token.
   * @param to The address of the user.
   * @param value The amount of tokens to transfer.
   * @throws std::runtime_error if the contract does not have enough tokens, or
   * if the token was not found, or if the user was not found.
   */
  void transferTo(const Address &token, const Address &to,
                  const uint256_t &value);

  /// function deposit(address _token, uint256 _value) public returns (bool)

  /**
   * @brief Deposits a specific amount of tokens to the contract.
   * @param token The address of the token.
   * @param value The amount of tokens to deposit.
   */
  void deposit(const Address &token, const uint256_t &value);
};

#endif // ERC20WRAPPER_H