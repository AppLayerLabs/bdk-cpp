#ifndef ERC20_H
#define ERC20_H

#include <memory>

#include "variables/safeunorderedmap.h"
#include "variables/safestring.h"
#include "variables/safeuint8_t.h"
#include "variables/safeuint256_t.h"
#include "contract.h"
#include "../utils/db.h"
#include "abi.h"

/**
 * C++ OrbiterSDK Recreation of a ERC20 Contract
 */

class ERC20 : public Contract {
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
    SafeUnorderedMap<Address, std::unordered_map<Address, uint256_t, SafeHash>> _allowed;


    void _mintValue(const Address& address, const uint256_t& value);

    void registerContractFunctions() override;

  public:

    /// Default Constructor when loading contract from DB.
    ERC20(const Address& address, const std::unique_ptr<DB> &db);
    /// Constructor to be used when creating a new contract.
    ERC20(const std::string& erc20_name, const std::string& erc20_symbol, const uint8_t& erc20_decimals, const uint256_t& mintValue, const std::string& contractName,
          const Address& address, const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db);

    ~ERC20();

    /// function name() public view returns (string memory) { return _name; }
    std::string name() const;

    /// function symbol() public view returns (string memory) { return _symbol; }
    std::string symbol() const;

    /// function decimals() public view returns (uint8) { return _decimals; }
    std::string decimals() const;

    /// function totalSupply() public view returns (uint256) { return _totalSupply; }
    std::string totalSupply() const;

    /// function balanceOf(address _owner) public view returns (uint256) { return _balances[_owner]; }
    std::string balanceOf(const Address& _owner) const;

    /// function transfer(address _to, uint256 _value) public returns (bool)
    void transfer(const Address& _to, const uint256_t& _value);

    /// function approve(address _spender, uint256 _value) public returns (bool)
    void approve(const Address& _spender, const uint256_t& _value);

    /// function allowance(address _owner, address _spender) public view returns (uint256)
    std::string allowance(const Address& _owner, const Address& _spender) const;

    /// function transferFrom(address _from, address _to, uint _value) public returns (bool)
    void transferFrom(const Address& _from, const Address& _to, const uint256_t& _value);
};





#endif /// ERC20_H