/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractfactory.h"

std::unordered_set<Address, SafeHash> ContractFactory::getRecentContracts() const {
  return this->recentContracts_;
}

void ContractFactory::clearRecentContracts() {
  this->recentContracts_.clear();
}

std::function<void(const ethCallInfo&)> ContractFactory::getCreateContractFunc(Functor func) const {
  std::function<void(const ethCallInfo&)> ret;
  if (
    auto it = this->createContractFuncs_.find(func.asBytes());
    it != this->createContractFuncs_.end()
  ) ret = it->second;
  return ret;
}

