#include "erc20.h"

// Default Constructor when loading contract from DB.
ERC20::ERC20(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db) :
  DynamicContract(interface, address, db), _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowances(this) {

  this->_name = Utils::bytesToString(db->get(std::string("_name"), this->getDBPrefix()));
  this->_symbol = Utils::bytesToString(db->get(std::string("_symbol"), this->getDBPrefix()));
  this->_decimals = Utils::bytesToUint8(db->get(std::string("_decimals"), this->getDBPrefix()));
  this->_totalSupply = Utils::bytesToUint256(db->get(std::string("_totalSupply"), this->getDBPrefix()));
  auto balances = db->getBatch(this->getNewPrefix("_balances"));
  for (const auto& dbEntry : balances) {
    this->_balances[Address(dbEntry.key)] = Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }

  auto allowances = db->getBatch(this->getNewPrefix("_allowed"));
  for (const auto& dbEntry : allowances) {
    BytesArrView valueView(dbEntry.value);
    this->_allowances[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = Utils::fromBigEndian<uint256_t>(valueView.subspan(20));
  }
  this->registerContractFunctions();

  this->_name.commit();
  this->_symbol.commit();
  this->_decimals.commit();
  this->_totalSupply.commit();
  this->_balances.commit();
  this->_allowances.commit();
}

ERC20::ERC20(
  const std::string& erc20_name, const std::string& erc20_symbol,
  const uint8_t& erc20_decimals, const uint256_t& mintValue,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator, const uint64_t& chainId,
  const std::unique_ptr<DB>& db
) : DynamicContract(interface, "ERC20", address, creator, chainId, db),
    _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowances(this)
{
  _name = erc20_name;
  _symbol = erc20_symbol;
  _decimals = erc20_decimals;
  _mint(creator, mintValue);
  this->registerContractFunctions();
  _name.commit();
  _symbol.commit();
  _decimals.commit();
}

ERC20::ERC20(
  const std::string &derivedTypeName, const std::string& erc20_name, const std::string& erc20_symbol,
  const uint8_t& erc20_decimals, const uint256_t& mintValue,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator, const uint64_t& chainId,
  const std::unique_ptr<DB>& db
) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db),
    _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowances(this)
{
  _name = erc20_name;
  _symbol = erc20_symbol;
  _decimals = erc20_decimals;
  _mint(creator, mintValue);
  this->registerContractFunctions();
}


ERC20::~ERC20() {
  DBBatch batchOperations;
  this->db->put(std::string("_name"), _name.get(), this->getDBPrefix());
  this->db->put(std::string("_symbol"), _symbol.get(), this->getDBPrefix());
  this->db->put(std::string("_decimals"), Utils::uint8ToBytes(_decimals.get()), this->getDBPrefix());
  this->db->put(std::string("_totalSupply"), Utils::uint256ToBytes(_totalSupply.get()), this->getDBPrefix());

  for (auto it = _balances.cbegin(); it != _balances.cend(); ++it) {
    const auto& key = it->first.get();
    Bytes value = Utils::uintToBytes(it->second);
    batchOperations.push_back(key, value, this->getNewPrefix("_balances"));
  }

  for (auto it = _allowances.cbegin(); it != _allowances.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      const auto& key = it->first.get();
      Bytes value = it2->first.asBytes();
      Utils::appendBytes(value, Utils::uintToBytes(it2->second));
      batchOperations.push_back(key, value, this->getNewPrefix("_allowed"));
    }
  }
  this->db->putBatch(batchOperations);
}

void ERC20::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("name", &ERC20::name, this);
  this->registerMemberFunction("symbol", &ERC20::symbol, this);
  this->registerMemberFunction("decimals", &ERC20::decimals, this);
  this->registerMemberFunction("totalSupply", &ERC20::totalSupply, this);
  this->registerMemberFunction("balanceOf", &ERC20::balanceOf, this);
  this->registerMemberFunction("allowance", &ERC20::allowance, this);
  this->registerMemberFunction("transfer", &ERC20::transfer, this);
  this->registerMemberFunction("approve", &ERC20::approve, this);
  this->registerMemberFunction("transferFrom", &ERC20::transferFrom, this);
  this->registerMemberFunction("increaseAllowance", &ERC20::increaseAllowance, this);
  this->registerMemberFunction("decreaseAllowance", &ERC20::decreaseAllowance, this);
}
void ERC20::_transfer(const Address& from, const Address& to, const uint256_t& value) {
  if (from == Address()) {
    throw std::runtime_error("ERC20::_transfer: from address is empty");
  }
  if (to == Address()) {
    throw std::runtime_error("ERC20::_transfer: to address is empty");
  }
  this->_update(from, to, value);
}

void ERC20::_mint(const Address& address, const uint256_t& value) {
  if (address == Address()) {
    throw std::runtime_error("ERC20::_mint: address is empty");
  }
  this->_update(Address(), address, value);
}

void ERC20::_burn(const Address& address, const uint256_t& value) {
  if (address == Address()) {
    throw std::runtime_error("ERC20::_burn: address is empty");
  }
  this->_update(address, Address(), value);
}

void ERC20::_approve(const Address& owner, const Address& spender, const uint256_t& value) {
  if (owner == Address()) {
    throw std::runtime_error("ERC20::_approve: owner address is empty");
  }
  if (spender == Address()) {
    throw std::runtime_error("ERC20::_approve: spender address is empty");
  }
  this->_allowances[owner][spender] = value;
}

void ERC20::_spendAllowance(const Address& owner, const Address& spender, const uint256_t& value) {
  auto allowance = this->allowance(owner, spender);
  if (allowance < value) {
    throw std::runtime_error("ERC20::_spendAllowance: allowance is less than value");
  }
  this->_approve(owner, spender, allowance - value);
}

void ERC20::_update(const Address& from, const Address& to, const uint256_t& value) {
  if (from == Address()) {
    this->_totalSupply += value;
  } else {
    auto fromBalance = this->_balances[from];
    if (fromBalance < value) {
      throw std::runtime_error("ERC20::_update: fromBalance is less than value");
    }
    this->_balances[from] = fromBalance - value;
  }
  if (to == Address()) {
    this->_totalSupply -= value;
  } else {
    this->_balances[to] += value;
  }
}

std::string ERC20::name() const { return this->_name.get(); }

std::string ERC20::symbol() const { return this->_symbol.get(); }

uint8_t ERC20::decimals() const { return this->_decimals.get(); }

uint256_t ERC20::totalSupply() const { return this->_totalSupply.get(); }

uint256_t ERC20::balanceOf(const Address& _owner) const {
  const auto& it = std::as_const(this->_balances).find(_owner);
  return (it == this->_balances.end())
         ? 0 : it->second;
}

void ERC20::transfer(const Address &to, const uint256_t &value) {
  this->_transfer(this->getCaller(), to, value);
}

void ERC20::approve(const Address &_spender, const uint256_t &value) {
  this->_approve(this->getCaller(), _spender, value);
}

uint256_t ERC20::allowance(const Address& _owner, const Address& _spender) const {
  const auto& it = std::as_const(this->_allowances).find(_owner);
  if (it == this->_allowances.end()) {
    return 0;
  } else {
    const auto& it2 = it->second.find(_spender);
    if (it2 == it->second.end()) {
      return 0;
    } else {
      return it2->second;
    }
  }
}

void ERC20::transferFrom(
  const Address &_from, const Address &to, const uint256_t &_value
) {
  this->_spendAllowance(_from, this->getCaller(), _value);
  this->_transfer(_from, to, _value);
}

void ERC20::increaseAllowance(const Address& spender, const uint256_t& _addedValue) {
  this->_approve(this->getCaller(), spender, this->allowance(this->getCaller(), spender) + _addedValue);
}

void ERC20::decreaseAllowance(const Address &spender, const uint256_t &_requestedDecrease) {
  uint256_t currentAllowance = this->allowance(this->getCaller(), spender);
  if (currentAllowance < _requestedDecrease) {
    throw std::runtime_error("ERC20::decreaseAllowance: currentAllowance is less than _requestedDecrease");
  } else {
    this->_approve(this->getCaller(), spender, currentAllowance - _requestedDecrease);
  }
}