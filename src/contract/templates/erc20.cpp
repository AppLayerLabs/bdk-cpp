#include "erc20.h"

// Default Constructor when loading contract from DB.
ERC20::ERC20(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db) :
  DynamicContract(interface, address, db), _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowed(this) {

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
    this->_allowed[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = Utils::fromBigEndian<uint256_t>(valueView.subspan(20));
  }
  this->registerContractFunctions();

  this->_name.commit();
  this->_symbol.commit();
  this->_decimals.commit();
  this->_totalSupply.commit();
  this->_balances.commit();
  this->_allowed.commit();
}

ERC20::ERC20(
  const std::string& erc20_name, const std::string& erc20_symbol,
  const uint8_t& erc20_decimals, const uint256_t& mintValue,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator, const uint64_t& chainId,
  const std::unique_ptr<DB>& db
) : DynamicContract(interface, "ERC20", address, creator, chainId, db),
  _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowed(this)
{
  _name = erc20_name;
  _symbol = erc20_symbol;
  _decimals = erc20_decimals;
  _mintValue(creator, mintValue);
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
    _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowed(this)
{
  _name = erc20_name;
  _symbol = erc20_symbol;
  _decimals = erc20_decimals;
  _mintValue(creator, mintValue);
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

  for (auto it = _allowed.cbegin(); it != _allowed.cend(); ++it) {
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
}

void ERC20::_mintValue(const Address& address, const uint256_t& value) {
  _balances[address] += value;
  _totalSupply += value;
}

void ERC20::_burnValue(const Address& address, const uint256_t& value) {
  _balances[address] -= value;
  _totalSupply -= value;
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

void ERC20::transfer(const Address &_to, const uint256_t &_value) {
  this->_balances[this->getCaller()] -= _value;
  this->_balances[_to] += _value;
}

void ERC20::approve(const Address &_spender, const uint256_t &_value) {
  this->_allowed[this->getCaller()][_spender] = _value;
}

uint256_t ERC20::allowance(const Address& _owner, const Address& _spender) const {
  const auto& it = std::as_const(this->_allowed).find(_owner);
  if (it == this->_allowed.end()) {
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
  const Address &_from, const Address &_to, const uint256_t &_value
) {
  this->_allowed[_from][this->getCaller()] -= _value;
  this->_balances[_from] -= _value;
  this->_balances[_to] += _value;
}

