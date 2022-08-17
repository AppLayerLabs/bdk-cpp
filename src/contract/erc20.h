#ifndef ERC20_H
#define ERC20_H

#include "contract.h"

class ERC20Contract : public Contract {
  private:
    struct allowanceInfo {
      std::string spender;
      dev::u256 allowed;
    };
    std::string _name;
    std::string _symbol;
    uint64_t _decimals;
    dev::u256 _totalSupply;
    std::string _address;
    std::map<std::string,dev::u256> _balances;
    std::map<std::string,allowanceInfo> _allowances;  // key -> owner, value -> spender + allowed
  public:
    ERC20Contract(json &data);
    ERC20Contract(const ERC20Contract& other) {
      this->_name = other._name;
      this->_symbol = other._symbol;
      this->_decimals = other._decimals;
      this->_totalSupply = other._totalSupply;
      this->_address = other._address;
      this->_balances = other._balances;
      this->_allowances = other._allowances;
    }
    std::string name() { return this->_name; }
    std::string symbol() { return this->_symbol; }
    uint64_t decimals() { return this->_decimals; }
    dev::u256 totalSupply() { return this->_totalSupply; }
    std::string address() { return this->_address; }
    std::map<std::string,dev::u256> balances() { return this->_balances; }
    std::map<std::string,allowanceInfo> allowances() { return this->_allowances; }
    bool transfer(std::string from, std::string to, dev::u256 value, bool commit = false);
    bool transferFrom(std::string from, std::string to, dev::u256 value, bool commit = false);
    bool approve(std::string owner, std::string spender, dev::u256 value, bool commit = false);
    bool mint(std::string to, dev::u256 value);
    bool burn(std::string from, dev::u256 value);
    dev::u256 allowance(std::string owner, std::string spender);
    dev::u256 balanceOf(std::string address);
    static void loadAllERC20(DBService &token_db, std::map<std::string, std::shared_ptr<ERC20Contract>> &tokens);
    static bool saveAllERC20(std::map<std::string, std::shared_ptr<ERC20Contract>> &tokens, DBService &token_db);
};

#endif  // ERC20_H
