/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "erc20wrapper.h"

ERC20Wrapper::ERC20Wrapper(const Address& contractAddress, const DB& db
) : DynamicContract(contractAddress, db), tokensAndBalances_(this)
{
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("tokensAndBalances_"))) {
    View<Bytes> valueView(dbEntry.value);
    this->tokensAndBalances_[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = Utils::fromBigEndian<uint256_t>(valueView.subspan(20));
  }

  this->tokensAndBalances_.commit();

  registerContractFunctions();

  this->tokensAndBalances_.enableRegister();
}

ERC20Wrapper::ERC20Wrapper(const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("ERC20Wrapper", address, creator, chainId), tokensAndBalances_(this)
{
  this->tokensAndBalances_.commit();

  registerContractFunctions();

  this->tokensAndBalances_.enableRegister();
}

ERC20Wrapper::~ERC20Wrapper() {}

uint256_t ERC20Wrapper::getContractBalance(const Address& token) const {
  return this->callContractViewFunction(token, &ERC20::balanceOf, this->getContractAddress());
}

uint256_t ERC20Wrapper::getUserBalance(const Address& token, const Address& user) const {
  auto it = this->tokensAndBalances_.find(token);
  if (it == this->tokensAndBalances_.cend()) return 0;
  auto itUser = it->second.find(user);
  if (itUser == it->second.cend()) return 0;
  return itUser->second;
}

void ERC20Wrapper::withdraw(const Address& token, const uint256_t& value) {
  auto it = this->tokensAndBalances_.find(token);
  if (it == this->tokensAndBalances_.end()) throw DynamicException("Token not found");
  auto itUser = it->second.find(this->getCaller());
  if (itUser == it->second.end()) throw DynamicException("User not found");
  if (itUser->second <= value) throw DynamicException("ERC20Wrapper: Not enough balance");
  itUser->second -= value;
  this->callContractFunction(token, &ERC20::transfer, this->getCaller(), value);
}

void ERC20Wrapper::transferTo(const Address& token, const Address& to, const uint256_t& value) {
  auto it = this->tokensAndBalances_.find(token);
  if (it == this->tokensAndBalances_.end()) throw DynamicException("Token not found");
  auto itUser = it->second.find(this->getCaller());
  if (itUser == it->second.end()) throw DynamicException("User not found");
  if (itUser->second <= value) throw DynamicException("ERC20Wrapper: Not enough balance");
  itUser->second -= value;
  this->callContractFunction(token, &ERC20::transfer, to, value);
}

void ERC20Wrapper::deposit(const Address& token, const uint256_t& value) {
  this->callContractFunction(token, &ERC20::transferFrom, this->getCaller(), this->getContractAddress(), value);
  this->tokensAndBalances_[token][this->getCaller()] += value;
}

void ERC20Wrapper::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getContractBalance", &ERC20Wrapper::getContractBalance, FunctionTypes::View, this);
  this->registerMemberFunction("getUserBalance", &ERC20Wrapper::getUserBalance, FunctionTypes::View, this);
  this->registerMemberFunction("withdraw", &ERC20Wrapper::withdraw, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("transferTo", &ERC20Wrapper::transferTo, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("deposit", &ERC20Wrapper::deposit, FunctionTypes::NonPayable, this);
}

DBBatch ERC20Wrapper::dump() const {
  DBBatch dbBatch = BaseContract::dump();
  for (auto i = tokensAndBalances_.cbegin(); i != tokensAndBalances_.cend(); ++i) {
    for (auto j = i->second.cbegin(); j != i->second.cend(); ++j) {
      const auto& key = i->first;
      Bytes value = j->first.asBytes();
      Utils::appendBytes(value, Utils::uintToBytes(j->second));
      dbBatch.push_back(key, value, this->getNewPrefix("tokensAndBalances_"));
    }
  }
  return dbBatch;
}
