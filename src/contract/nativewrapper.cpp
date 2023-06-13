#include "nativewrapper.h"

/// Default Constructor when loading contract from DB.
NativeWrapper::NativeWrapper(ContractManager::ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db) :
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
    BytesArrView keyView(dbEntry.value);
    this->_allowed[Address(dbEntry.key)][Address(keyView.subspan(0, 20))] = Utils::fromBigEndian<uint256_t>(keyView.subspan(20));
  }
  this->registerContractFunctions();
  updateState(true);
}

NativeWrapper::NativeWrapper(
  const std::string &erc20_name, const std::string &erc20_symbol,
  const uint8_t &erc20_decimals,
  ContractManager::ContractManagerInterface &interface,
  const Address &address, const Address &creator,
  const uint64_t &chainId,const std::unique_ptr<DB> &db
) : DynamicContract(interface, "NativeWrapper", address, creator, chainId, db),
  _name(this), _symbol(this), _decimals(this), _totalSupply(this), _balances(this), _allowed(this)
{
  _name = erc20_name;
  _symbol = erc20_symbol;
  _decimals = erc20_decimals;
  this->registerContractFunctions();
  updateState(true);
}

NativeWrapper::~NativeWrapper() {
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

void NativeWrapper::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("name", &NativeWrapper::name, this);
  this->registerMemberFunction("symbol", &NativeWrapper::symbol, this);
  this->registerMemberFunction("decimals", &NativeWrapper::decimals, this);
  this->registerMemberFunction("totalSupply", &NativeWrapper::totalSupply, this);
  this->registerMemberFunction("balanceOf", &NativeWrapper::balanceOf, this);
  this->registerMemberFunction("allowance", &NativeWrapper::allowance, this);
  this->registerMemberFunction("transfer", &NativeWrapper::transfer, this);
  this->registerMemberFunction("approve", &NativeWrapper::approve, this);
  this->registerMemberFunction("transferFrom", &NativeWrapper::transferFrom, this);
  this->registerMemberFunction("deposit", &NativeWrapper::deposit, this);
  this->registerMemberFunction("withdraw", &NativeWrapper::withdraw, this);
}

void NativeWrapper::_mintValue(const Address& address, const uint256_t& value) {
  _balances[address] += value;
  _totalSupply += value;
}

Bytes NativeWrapper::name() const {
  return ABI::Encoder({this->_name.get()}).getData();
}

Bytes NativeWrapper::symbol() const {
  return ABI::Encoder({this->_symbol.get()}).getData();
}

Bytes NativeWrapper::decimals() const {
  return ABI::Encoder({this->_decimals.get()}).getData();
}

Bytes NativeWrapper::totalSupply() const {
  return ABI::Encoder({this->_totalSupply.get()}).getData();
}

Bytes NativeWrapper::balanceOf(const Address& _owner) const {
  const auto& it = std::as_const(this->_balances).find(_owner);
  if (it == this->_balances.end()) {
    return ABI::Encoder({0}).getData();
  } else {
    return ABI::Encoder({it->second}).getData();
  }
}

void NativeWrapper::transfer(const Address &_to, const uint256_t &_value) {
  this->_balances[this->getCaller()] -= _value;
  this->_balances[_to] += _value;
}

void NativeWrapper::approve(const Address &_spender, const uint256_t &_value) {
  this->_allowed[this->getCaller()][_spender] = _value;
}

Bytes NativeWrapper::allowance(const Address& _owner, const Address& _spender) const {
  const auto& it = std::as_const(this->_allowed).find(_owner);
  if (it == this->_allowed.end()) {
    return ABI::Encoder({0}).getData();
  } else {
    const auto& it2 = it->second.find(_spender);
    if (it2 == it->second.end()) {
      return ABI::Encoder({0}).getData();
    } else {
      return ABI::Encoder({it2->second}).getData();
    }
  }
}

void NativeWrapper::transferFrom(
  const Address &_from, const Address &_to, const uint256_t &_value
) {
  this->_allowed[_from][this->getCaller()] -= _value;
  this->_balances[_from] -= _value;
  this->_balances[_to] += _value;
}

void NativeWrapper::deposit() {
  this->_balances[this->getCaller()] += this->getValue();
}

void NativeWrapper::withdraw(const uint256_t &_value) {
  this->_balances[this->getCaller()] -= _value;
  this->sendTokens(this->getCaller(), _value);
}



