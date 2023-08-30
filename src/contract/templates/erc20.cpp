#include "erc20.h"

// Default Constructor when loading contract from DB.
ERC20::ERC20(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db) :
  DynamicContract(interface, address, db), name_(this), symbol_(this), decimals_(this), totalSupply_(this), balances_(this), allowed_(this) {

  this->name_ = Utils::bytesToString(db->get(std::string("name_"), this->getDBPrefix()));
  this->symbol_ = Utils::bytesToString(db->get(std::string("symbol_"), this->getDBPrefix()));
  this->decimals_ = Utils::bytesToUint8(db->get(std::string("decimals_"), this->getDBPrefix()));
  this->totalSupply_ = Utils::bytesToUint256(db->get(std::string("totalSupply_"), this->getDBPrefix()));
  auto balances = db->getBatch(this->getNewPrefix("balances_"));
  for (const auto& dbEntry : balances) {
    this->balances_[Address(dbEntry.key)] = Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }

  auto allowances = db->getBatch(this->getNewPrefix("allowed_"));
  for (const auto& dbEntry : allowances) {
    BytesArrView valueView(dbEntry.value);
    this->allowed_[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = Utils::fromBigEndian<uint256_t>(valueView.subspan(20));
  }
  this->registerContractFunctions();

  this->name_.commit();
  this->symbol_.commit();
  this->decimals_.commit();
  this->totalSupply_.commit();
  this->balances_.commit();
  this->allowed_.commit();
}

ERC20::ERC20(
  const std::string& erc20name_, const std::string& erc20symbol_,
  const uint8_t& erc20decimals_, const uint256_t& mintValue,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator, const uint64_t& chainId,
  const std::unique_ptr<DB>& db
) : DynamicContract(interface, "ERC20", address, creator, chainId, db),
  name_(this), symbol_(this), decimals_(this), totalSupply_(this), balances_(this), allowed_(this)
{
  name_ = erc20name_;
  symbol_ = erc20symbol_;
  decimals_ = erc20decimals_;
  mintValue_(creator, mintValue);
  this->registerContractFunctions();
  name_.commit();
  symbol_.commit();
  decimals_.commit();
}

ERC20::ERC20(
  const std::string &derivedTypeName, const std::string& erc20name_, const std::string& erc20symbol_,
  const uint8_t& erc20decimals_, const uint256_t& mintValue,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator, const uint64_t& chainId,
  const std::unique_ptr<DB>& db
) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db),
    name_(this), symbol_(this), decimals_(this), totalSupply_(this), balances_(this), allowed_(this)
{
  name_ = erc20name_;
  symbol_ = erc20symbol_;
  decimals_ = erc20decimals_;
  mintValue_(creator, mintValue);
  this->registerContractFunctions();
}


ERC20::~ERC20() {
  DBBatch batchOperations;
  this->db_->put(std::string("name_"), name_.get(), this->getDBPrefix());
  this->db_->put(std::string("symbol_"), symbol_.get(), this->getDBPrefix());
  this->db_->put(std::string("decimals_"), Utils::uint8ToBytes(decimals_.get()), this->getDBPrefix());
  this->db_->put(std::string("totalSupply_"), Utils::uint256ToBytes(totalSupply_.get()), this->getDBPrefix());

  for (auto it = balances_.cbegin(); it != balances_.cend(); ++it) {
    const auto& key = it->first.get();
    Bytes value = Utils::uintToBytes(it->second);
    batchOperations.push_back(key, value, this->getNewPrefix("balances_"));
  }

  for (auto it = allowed_.cbegin(); it != allowed_.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      const auto& key = it->first.get();
      Bytes value = it2->first.asBytes();
      Utils::appendBytes(value, Utils::uintToBytes(it2->second));
      batchOperations.push_back(key, value, this->getNewPrefix("allowed_"));
    }
  }
  this->db_->putBatch(batchOperations);
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

void ERC20::mintValue_(const Address& address, const uint256_t& value) {
  balances_[address] += value;
  totalSupply_ += value;
}

void ERC20::burnValue_(const Address& address, const uint256_t& value) {
  balances_[address] -= value;
  totalSupply_ -= value;
}

std::string ERC20::name() const { return this->name_.get(); }

std::string ERC20::symbol() const { return this->symbol_.get(); }

uint8_t ERC20::decimals() const { return this->decimals_.get(); }

uint256_t ERC20::totalSupply() const { return this->totalSupply_.get(); }

uint256_t ERC20::balanceOf(const Address& owner) const {
  const auto& it = std::as_const(this->balances_).find(owner);
  return (it == this->balances_.end())
    ? 0 : it->second;
}

void ERC20::transfer(const Address &to, const uint256_t &value) {
  this->balances_[this->getCaller()] -= value;
  this->balances_[to] += value;
}

void ERC20::approve(const Address &spender, const uint256_t &value) {
  this->allowed_[this->getCaller()][spender] = value;
}

uint256_t ERC20::allowance(const Address& owner, const Address& spender) const {
  const auto& it = std::as_const(this->allowed_).find(owner);
  if (it == this->allowed_.end()) {
    return 0;
  } else {
    const auto& it2 = it->second.find(spender);
    if (it2 == it->second.end()) {
      return 0;
    } else {
      return it2->second;
    }
  }
}

void ERC20::transferFrom(
  const Address &from, const Address &to, const uint256_t &value
) {
  this->allowed_[from][this->getCaller()] -= value;
  this->balances_[from] -= value;
  this->balances_[to] += value;
}

