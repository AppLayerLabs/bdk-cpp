#include "erc20.h"

/// Default Constructor when loading contract from DB.
ERC20::ERC20(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db) :
  DynamicContract(interface, address, db), _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowed(this) {

  this->_name =
      db->get("_name", DBPrefix::contracts + this->getContractAddress().get());
  this->_symbol = db->get("_symbol", DBPrefix::contracts +
                                         this->getContractAddress().get());
  this->_decimals = Utils::bytesToUint256(db->get(
      "_decimals", DBPrefix::contracts + this->getContractAddress().get()));
  this->_totalSupply = Utils::bytesToUint256(db->get(
      "_totalSupply", DBPrefix::contracts + this->getContractAddress().get()));
  auto balances = db->getBatch(DBPrefix::contracts +
                               this->getContractAddress().get() + "_balances");
  for (const auto &dbEntry : balances) {
    this->_balances[Address(dbEntry.key, true)] =
        Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }

  auto allowances = db->getBatch(DBPrefix::contracts +
                                 this->getContractAddress().get() + "_allowed");
  for (const auto &dbEntry : allowances) {
    this->_allowed[Address(dbEntry.key, true)]
                  [Address(dbEntry.value.substr(0, 20), true)] =
        Utils::fromBigEndian<uint256_t>(dbEntry.value.substr(20));
  }
  this->registerContractFunctions();
  updateState(true);
}

ERC20::ERC20(const std::string &erc20_name, const std::string &erc20_symbol, const uint256_t &erc20_decimals, const uint256_t &mintValue, ContractManagerInterface &interface,
        const Address &address, const Address &creator, const uint64_t &chainId, const std::unique_ptr<DB> &db) :
  DynamicContract(interface, "ERC20", address, creator, chainId, db), _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowed(this) {
  _name = erc20_name;
  _symbol = erc20_symbol;
  _decimals = erc20_decimals;
  _mintValue(creator, mintValue);
  this->registerContractFunctions();
  updateState(true);
}

ERC20::~ERC20() {
  DBBatch balancesBatch;
  DBBatch allowanceBatch;

  this->db->put("_name", _name.get(),
                DBPrefix::contracts + this->getContractAddress().get());
  this->db->put("_symbol", _symbol.get(),
                DBPrefix::contracts + this->getContractAddress().get());
  this->db->put("_decimals", Utils::uint256ToBytes(_decimals.get()),
                DBPrefix::contracts + this->getContractAddress().get());
  this->db->put("_totalSupply", Utils::uint256ToBytes(_totalSupply.get()),
                DBPrefix::contracts + this->getContractAddress().get());

  for (auto it = _balances.cbegin(); it != _balances.cend(); ++it) {
    std::string key = it->first.get();
    std::string value;
    value += Utils::uintToBytes(it->second);
    balancesBatch.puts.emplace_back(DBEntry(key, value));
  }

  for (auto it = _allowed.cbegin(); it != _allowed.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      std::string key = it->first.get();
      std::string value;
      value += it2->first.get();
      value += Utils::uintToBytes(it2->second);
      allowanceBatch.puts.emplace_back(DBEntry(key, value));
    }
  }
  this->db->putBatch(balancesBatch, DBPrefix::contracts +
                                        this->getContractAddress().get() +
                                        "_balances");
  this->db->putBatch(allowanceBatch, DBPrefix::contracts +
                                         this->getContractAddress().get() +
                                         "_allowed");
  this->registerContractFunctions();
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

void ERC20::_mintValue(const Address &address, const uint256_t &value) {
  _balances[address] += value;
  _totalSupply += value;
}

std::string ERC20::name() const {
  return ABI::Encoder({this->_name.get()}).getRaw();
}

std::string ERC20::symbol() const {
  return ABI::Encoder({this->_symbol.get()}).getRaw();
}

std::string ERC20::decimals() const {
  return ABI::Encoder({this->_decimals.get()}).getRaw();
}

std::string ERC20::totalSupply() const {
  return ABI::Encoder({this->_totalSupply.get()}).getRaw();
}

std::string ERC20::balanceOf(const Address &_owner) const {
  const auto &it = std::as_const(this->_balances).find(_owner);
  if (it == this->_balances.end()) {
    return ABI::Encoder({0}).getRaw();
  } else {
    return ABI::Encoder({it->second}).getRaw();
  }
}

void ERC20::transfer(const Address &_to, const uint256_t &_value) {
  this->_balances[this->getCaller()] -= _value;
  this->_balances[_to] += _value;
}

void ERC20::approve(const Address &_spender, const uint256_t &_value) {
  this->_allowed[this->getCaller()][_spender] = _value;
}

std::string ERC20::allowance(const Address &_owner,
                             const Address &_spender) const {
  const auto &it = std::as_const(this->_allowed).find(_owner);
  if (it == this->_allowed.end()) {
    return ABI::Encoder({0}).getRaw();
  } else {
    const auto &it2 = it->second.find(_spender);
    if (it2 == it->second.end()) {
      return ABI::Encoder({0}).getRaw();
    } else {
      return ABI::Encoder({it2->second}).getRaw();
    }
  }
}

void ERC20::transferFrom(const Address &_from, const Address &_to,
                         const uint256_t &_value) {
  this->_allowed[_from][this->getCaller()] -= _value;
  this->_balances[_from] -= _value;
  this->_balances[_to] += _value;
}
