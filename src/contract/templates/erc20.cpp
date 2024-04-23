/*
  Copyright (c) [2023-2024] [Sparq Network]

  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#include "erc20.h"

ERC20::ERC20(ContractManagerInterface &interface, const Address& address, DB& db)
  : DynamicContract(interface, address, db), name_(this), symbol_(this), decimals_(this),
    totalSupply_(this), balances_(this), allowed_(this)
{
  this->name_ = Utils::bytesToString(db_.get(std::string("name_"), this->getDBPrefix()));
  this->symbol_ = Utils::bytesToString(db_.get(std::string("symbol_"), this->getDBPrefix()));
  this->decimals_ = Utils::bytesToUint8(db_.get(std::string("decimals_"), this->getDBPrefix()));
  this->totalSupply_ = Utils::bytesToUint256(db_.get(std::string("totalSupply_"), this->getDBPrefix()));
  auto balances = db_.getBatch(this->getNewPrefix("balances_"));
  for (const auto& dbEntry : balances) {
    this->balances_[Address(dbEntry.key)] = Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }
  auto allowances = db_.getBatch(this->getNewPrefix("allowed_"));
  for (const auto& dbEntry : allowances) {
    BytesArrView key(dbEntry.key);
    Address owner(key.subspan(0,20));
    Address spender(key.subspan(20));
    this->allowed_[owner][spender] = Utils::bytesToUint256(dbEntry.value);
  }

  this->name_.commit();
  this->symbol_.commit();
  this->decimals_.commit();
  this->totalSupply_.commit();
  this->balances_.commit();
  this->allowed_.commit();

  this->registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->decimals_.enableRegister();
  this->totalSupply_.enableRegister();
  this->balances_.enableRegister();
  this->allowed_.enableRegister();

}

ERC20::ERC20(
  const std::string& erc20name_, const std::string& erc20symbol_,
  const uint8_t& erc20decimals_, const uint256_t& mintValue,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator, const uint64_t& chainId,
  DB& db
  ) : DynamicContract(interface, "ERC20", address, creator, chainId, db),
      name_(this), symbol_(this), decimals_(this), totalSupply_(this), balances_(this), allowed_(this)
{
  this->name_ = erc20name_;
  this->symbol_ = erc20symbol_;
  this->decimals_ = erc20decimals_;
  this->mintValue_(creator, mintValue);

  this->name_.commit();
  this->symbol_.commit();
  this->decimals_.commit();
  this->totalSupply_.commit();
  this->balances_.commit();
  this->allowed_.commit();

  this->registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->decimals_.enableRegister();
  this->totalSupply_.enableRegister();
  this->balances_.enableRegister();
  this->allowed_.enableRegister();
}

ERC20::ERC20(
  const std::string &derivedTypeName, const std::string& erc20name_, const std::string& erc20symbol_,
  const uint8_t& erc20decimals_, const uint256_t& mintValue,
  ContractManagerInterface& interface,
  const Address& address, const Address& creator, const uint64_t& chainId,
  DB& db
  ) : DynamicContract(interface, derivedTypeName, address, creator, chainId, db),
      name_(this), symbol_(this), decimals_(this), totalSupply_(this), balances_(this), allowed_(this)
{
  this->name_ = erc20name_;
  this->symbol_ = erc20symbol_;
  this->decimals_ = erc20decimals_;
  this->mintValue_(creator, mintValue);

  this->name_.commit();
  this->symbol_.commit();
  this->decimals_.commit();
  this->totalSupply_.commit();
  this->balances_.commit();
  this->allowed_.commit();

  this->registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->decimals_.enableRegister();
  this->totalSupply_.enableRegister();
  this->balances_.enableRegister();
  this->allowed_.enableRegister();
}

ERC20::~ERC20() {}

void ERC20::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("name", &ERC20::name, FunctionTypes::View, this);
  this->registerMemberFunction("symbol", &ERC20::symbol, FunctionTypes::View, this);
  this->registerMemberFunction("decimals", &ERC20::decimals, FunctionTypes::View, this);
  this->registerMemberFunction("totalSupply", &ERC20::totalSupply, FunctionTypes::View, this);
  this->registerMemberFunction("balanceOf", &ERC20::balanceOf, FunctionTypes::View, this);
  this->registerMemberFunction("allowance", &ERC20::allowance, FunctionTypes::View, this);
  this->registerMemberFunction("transfer", &ERC20::transfer, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("approve", &ERC20::approve, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("transferFrom", &ERC20::transferFrom, FunctionTypes::NonPayable, this);
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

DBBatch ERC20::dump() const
{
  DBBatch dbBatch;

  // Name, Symbol, Decimals, Total Supply
  dbBatch.push_back(Utils::stringToBytes("name_"), Utils::stringToBytes(name_.get()), this->getDBPrefix());
  dbBatch.push_back(Utils::stringToBytes("symbol_"), Utils::stringToBytes(symbol_.get()), this->getDBPrefix());
  dbBatch.push_back(Utils::stringToBytes("decimals_"), Utils::uint8ToBytes(decimals_.get()), this->getDBPrefix());
  dbBatch.push_back(Utils::stringToBytes("totalSupply_"), Utils::uint256ToBytes(totalSupply_.get()), this->getDBPrefix());
  // Balances
  for (auto it = balances_.cbegin(); it != balances_.cend(); ++it) {
    const auto& key = it->first.get();
    Bytes value = Utils::uintToBytes(it->second);
    dbBatch.push_back(key, value, this->getNewPrefix("balances_"));
  }
  // SafeUnorderedMap<Address, std::unordered_map<Address, uint256_t, SafeHash>>
  for (auto i = allowed_.cbegin(); i != allowed_.cend(); ++i) {
    for (auto j = i->second.cbegin(); j != i->second.cend(); ++j) {
      // Key = Address + Address, Value = uint256_t
      auto key = i->first.asBytes();
      Utils::appendBytes(key, j->first.asBytes());
      dbBatch.push_back(key,
                        Utils::uint256ToBytes(j->second),
                        this->getNewPrefix("allowed_"));
    }
  }
  return dbBatch;
}
