/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "erc20.h"

#include "../../../utils/uintconv.h"
#include "../../../utils/strconv.h"

ERC20::ERC20(const Address& address, const DB& db)
: DynamicContract(address, db), name_(this), symbol_(this), decimals_(this),
  totalSupply_(this), balances_(this), allowed_(this)
  #ifndef BUILD_TESTNET
   ,counter_(this), values_(this), addresses_(this)
  #endif
{
  this->name_ = StrConv::bytesToString(db.get(std::string("name_"), this->getDBPrefix()));
  this->symbol_ = StrConv::bytesToString(db.get(std::string("symbol_"), this->getDBPrefix()));
  this->decimals_ = UintConv::bytesToUint8(db.get(std::string("decimals_"), this->getDBPrefix()));
  this->totalSupply_ = UintConv::bytesToUint256(db.get(std::string("totalSupply_"), this->getDBPrefix()));
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("balances_"))) {
    this->balances_[Address(dbEntry.key)] = Utils::fromBigEndian<uint256_t>(dbEntry.value);
  }
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("allowed_"))) {
    View<Bytes> key(dbEntry.key);
    Address owner(key.subspan(0,20));
    Address spender(key.subspan(20));
    this->allowed_[owner][spender] = UintConv::bytesToUint256(dbEntry.value);
  }

  this->name_.commit();
  this->symbol_.commit();
  this->decimals_.commit();
  this->totalSupply_.commit();
  this->balances_.commit();
  this->allowed_.commit();
  #ifndef BUILD_TESTNET
    this->counter_.commit();
    this->values_.commit();
    this->addresses_.commit();
  #endif

  this->registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->decimals_.enableRegister();
  this->totalSupply_.enableRegister();
  this->balances_.enableRegister();
  this->allowed_.enableRegister();
  #ifndef BUILD_TESTNET
    this->counter_.enableRegister();
    this->values_.enableRegister();
    this->addresses_.enableRegister();
  #endif
}

ERC20::ERC20(
  const std::string& erc20name_, const std::string& erc20symbol_,
  const uint8_t& erc20decimals_, const uint256_t& mintValue,
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("ERC20", address, creator, chainId),
  name_(this), symbol_(this), decimals_(this), totalSupply_(this), balances_(this), allowed_(this)
  #ifndef BUILD_TESTNET
    , counter_(this), values_(this), addresses_(this)
  #endif
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
  #ifndef BUILD_TESTNET
    this->counter_.commit();
    this->values_.commit();
    this->addresses_.commit();
  #endif

  this->registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->decimals_.enableRegister();
  this->totalSupply_.enableRegister();
  this->balances_.enableRegister();
  this->allowed_.enableRegister();
  #ifndef BUILD_TESTNET
    this->counter_.enableRegister();
    this->values_.enableRegister();
    this->addresses_.enableRegister();
  #endif
}

ERC20::ERC20(
  const std::string &derivedTypeName, const std::string& erc20name_, const std::string& erc20symbol_,
  const uint8_t& erc20decimals_, const uint256_t& mintValue,
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract(derivedTypeName, address, creator, chainId),
    name_(this), symbol_(this), decimals_(this), totalSupply_(this), balances_(this), allowed_(this)
  #ifndef BUILD_TESTNET
    , counter_(this), values_(this), addresses_(this)
  #endif
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
  #ifndef BUILD_TESTNET
    this->counter_.commit();
    this->values_.commit();
    this->addresses_.commit();
  #endif

  this->registerContractFunctions();

  this->name_.enableRegister();
  this->symbol_.enableRegister();
  this->decimals_.enableRegister();
  this->totalSupply_.enableRegister();
  this->balances_.enableRegister();
  this->allowed_.enableRegister();
  #ifndef BUILD_TESTNET
    this->counter_.enableRegister();
    this->values_.enableRegister();
    this->addresses_.enableRegister();
  #endif
}

void ERC20::registerContractFunctions() {
  registerContract();
  // We need to register the member functions separately
  // because ERC20 derives from multiple standard interfaces
  // IERC20Metadata
  this->registerMemberFunctions(
    std::make_tuple("name", &ERC20::name, FunctionTypes::View, this),
    std::make_tuple("symbol", &ERC20::symbol, FunctionTypes::View, this),
    std::make_tuple("decimals", &ERC20::decimals, FunctionTypes::View, this)
  );
  // IERC20
  this->registerMemberFunctions(
    std::make_tuple("totalSupply", &ERC20::totalSupply, FunctionTypes::View, this),
    std::make_tuple("balanceOf", &ERC20::balanceOf, FunctionTypes::View, this),
    std::make_tuple("allowance", &ERC20::allowance, FunctionTypes::View, this),
    std::make_tuple("transfer", &ERC20::transfer, FunctionTypes::NonPayable, this),
    std::make_tuple("approve", &ERC20::approve, FunctionTypes::NonPayable, this),
    std::make_tuple("transferFrom", &ERC20::transferFrom, FunctionTypes::NonPayable, this)
  );
  #ifndef BUILD_TESTNET
    this->registerMemberFunctions(
      std::make_tuple("generate", &ERC20::generate, FunctionTypes::NonPayable, this),
      std::make_tuple("addall", &ERC20::addall, FunctionTypes::NonPayable, this)
    );
  #endif
}

#ifndef BUILD_TESTNET

void ERC20::generate(const std::vector<Address> &addresses) {
  this->counter_ = addresses.size();
  for (const auto& address : addresses) {
    this->values_[this->counter_.get()][address] = 0;
    this->addresses_.push_back(address);
  }
  return;
}

void ERC20::addall() {
  for (uint64_t i = 0; i < this->counter_.get(); i++) {
    // Use this->address_.at() CONST, we need to enforce constness
    this->values_[i][std::as_const(this->addresses_).at(i)] += 1;
  }
  return;
}

#endif

void ERC20::mintValue_(const Address& address, const uint256_t& value) {
  balances_[address] += value;
  totalSupply_ += value;
  // TODO: Allow contract events during constructor, mintValue_ is called during constructor.
  // this->Transfer(Address(), address, value);
}

void ERC20::mint_(const Address& address, const uint256_t& value) {
  balances_[address] += value;
  totalSupply_ += value;
  this->Transfer(Address(), address, value);
}

void ERC20::burnValue_(const Address& address, const uint256_t& value) {
  balances_[address] -= value;
  totalSupply_ -= value;
  this->Transfer(address, Address(), value);
}

std::string ERC20::name() const { return this->name_.get(); }

std::string ERC20::symbol() const { return this->symbol_.get(); }

uint8_t ERC20::decimals() const { return this->decimals_.get(); }

uint256_t ERC20::totalSupply() const { return this->totalSupply_.get(); }

uint256_t ERC20::balanceOf(const Address& owner) const {
  const auto& it = std::as_const(this->balances_).find(owner);
  return (it == this->balances_.cend()) ? 0 : it->second;
}

bool ERC20::transfer(const Address &to, const uint256_t &value) {
  this->balances_[this->getCaller()] -= value;
  this->balances_[to] += value;
  this->Transfer(this->getCaller(), to, value);
  return true;
}

bool ERC20::approve(const Address &spender, const uint256_t &value) {
  this->allowed_[this->getCaller()][spender] = value;
  this->Approval(this->getCaller(), spender, value);
  return true;
}

uint256_t ERC20::allowance(const Address& owner, const Address& spender) const {
  uint256_t ret = 0;
  if (const auto& it = std::as_const(this->allowed_).find(owner); it != this->allowed_.cend()) {
    if (const auto& it2 = it->second.find(spender); it2 != it->second.cend()) ret = it2->second;
  }
  return ret;
}

bool ERC20::transferFrom(
  const Address &from, const Address &to, const uint256_t &value
) {
  this->allowed_[from][this->getCaller()] -= value;
  this->balances_[from] -= value;
  this->balances_[to] += value;
  this->Transfer(from, to, value);
  return true;
}

DBBatch ERC20::dump() const
{
  DBBatch dbBatch = BaseContract::dump();

  // Name, Symbol, Decimals, Total Supply
  dbBatch.push_back(StrConv::stringToBytes("name_"), StrConv::stringToBytes(name_.get()), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("symbol_"), StrConv::stringToBytes(symbol_.get()), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("decimals_"), UintConv::uint8ToBytes(decimals_.get()), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("totalSupply_"), UintConv::uint256ToBytes(totalSupply_.get()), this->getDBPrefix());
  // Balances
  for (auto it = balances_.cbegin(); it != balances_.cend(); ++it) {
    const auto& key = it->first;
    Bytes value = Utils::uintToBytes(it->second);
    dbBatch.push_back(key, value, this->getNewPrefix("balances_"));
  }
  // Allowed
  for (auto i = allowed_.cbegin(); i != allowed_.cend(); ++i) {
    for (auto j = i->second.cbegin(); j != i->second.cend(); ++j) {
      // Key = Address + Address, Value = uint256_t
      auto key = i->first.asBytes();
      Utils::appendBytes(key, j->first.asBytes());
      dbBatch.push_back(key, UintConv::uint256ToBytes(j->second), this->getNewPrefix("allowed_"));
    }
  }
  return dbBatch;
}

