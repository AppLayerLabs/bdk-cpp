/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "erc20wrapper.h"

ERC20Wrapper::ERC20Wrapper(
  ContractManagerInterface& interface,
  const Address& contractAddress, const std::unique_ptr<DB>& db
) : DynamicContract(interface, contractAddress, db), tokensAndBalances_(this) {
  registerContractFunctions();
  auto tokensAndBalances = this->db_->getBatch(this->getNewPrefix("tokensAndBalances_"));
  for (const auto& dbEntry : tokensAndBalances) {
    BytesArrView valueView(dbEntry.value);
    this->tokensAndBalances_[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = Utils::fromBigEndian<uint256_t>(valueView.subspan(20));
  }
  tokensAndBalances_.commit();
}

ERC20Wrapper::ERC20Wrapper(
  ContractManagerInterface& interface, const Address& address,
  const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB>& db
) : DynamicContract(interface, "ERC20Wrapper", address, creator, chainId, db),
  tokensAndBalances_(this)
{
  registerContractFunctions();
  tokensAndBalances_.commit();
}

ERC20Wrapper::~ERC20Wrapper() {
  DBBatch tokensAndBalancesBatch;
  for (auto it = tokensAndBalances_.cbegin(); it != tokensAndBalances_.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      const auto& key = it->first.get();
      Bytes value = it2->first.asBytes();
      Utils::appendBytes(value, Utils::uintToBytes(it2->second));
      tokensAndBalancesBatch.push_back(key, value, this->getNewPrefix("tokensAndBalances_"));
    }
  }
  this->db_->putBatch(tokensAndBalancesBatch);
}

uint256_t ERC20Wrapper::getContractBalance(const Address& token) const {
  return this->callContractViewFunction(token, &ERC20::balanceOf, this->getContractAddress());
}

uint256_t ERC20Wrapper::getUserBalance(const Address& token, const Address& user) const {
  auto it = this->tokensAndBalances_.find(token);
  if (it == this->tokensAndBalances_.end()) {
    return 0;
  }
  auto itUser = it->second.find(user);
  if (itUser == it->second.end()) {
    return 0;
  }
  return itUser->second;
}

void ERC20Wrapper::withdraw(const Address& token, const uint256_t& value) {
  auto it = this->tokensAndBalances_.find(token);
  if (it == this->tokensAndBalances_.end()) throw std::runtime_error("Token not found");
  auto itUser = it->second.find(this->getCaller());
  if (itUser == it->second.end()) throw std::runtime_error("User not found");
  if (itUser->second <= value) throw std::runtime_error("ERC20Wrapper: Not enough balance");
  itUser->second -= value;
  this->callContractFunction(token, &ERC20::transfer, this->getCaller(), value);
}

void ERC20Wrapper::transferTo(const Address& token, const Address& to, const uint256_t& value) {
  auto it = this->tokensAndBalances_.find(token);
  if (it == this->tokensAndBalances_.end()) throw std::runtime_error("Token not found");
  auto itUser = it->second.find(this->getCaller());
  if (itUser == it->second.end()) throw std::runtime_error("User not found");
  if (itUser->second <= value) throw std::runtime_error("ERC20Wrapper: Not enough balance");
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

