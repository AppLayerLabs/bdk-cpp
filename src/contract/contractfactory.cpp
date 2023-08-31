/*
Copyright (c) [2023] [Sparq Network]

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
  auto createIt = this->createContractFuncs_.find(func.asBytes());
  if (createIt != this->createContractFuncs_.end()) ret = createIt->second;
  return ret;
}

