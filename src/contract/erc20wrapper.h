#ifndef ERC20WRAPPER_H
#define ERC20WRAPPER_H

#include <memory>

#include "variables/safeunorderedmap.h"
#include "variables/safeuint256_t.h"
#include "dynamiccontract.h"
#include "../utils/db.h"
#include "abi.h"
#include "erc20.h"

class ERC20Wrapper : public DynamicContract {
  private:
    /// ERC20 Address => UserAddress/UserBalance
    /// mapping(address => mapping(address => uint256)) internal _tokensAndBalances;
    SafeUnorderedMap<Address,std::unordered_map<Address, uint256_t, SafeHash>> _tokensAndBalances;

    void registerContractFunctions() override;

  public:

    /// Default Constructor when loading contract from DB.
    ERC20Wrapper(ContractManager::ContractManagerInterface &interface, const Address& contractAddress, const std::unique_ptr<DB> &db);

    /// Default Constructor when building a new contract
    ERC20Wrapper(ContractManager::ContractManagerInterface &interface,
                 const Address& address, const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db);

    ~ERC20Wrapper() override;

    /// function getContractBalance(address _token) public view returns (uint256) { return _tokensAndBalances[_token][address(this)]; }
    std::string getContractBalance(const Address& token) const;

    /// function getUserBalance(address _token, address _user) public view returns (uint256) { return _tokensAndBalances[_token][_user]; }
    std::string getUserBalance(const Address& token, const Address& user) const;

    /// function withdraw (address _token, uint256 _value) public returns (bool)
    void withdraw(const Address& token, const uint256_t& value);

    /// function transferTo(address _token, address _to, uint256 _value) public returns (bool)
    void transferTo(const Address& token, const Address& to, const uint256_t& value);

    /// function deposit(address _token, uint256 _value) public returns (bool)
    void deposit(const Address& token, const uint256_t& value);



};

#endif // ERC20WRAPPER_H