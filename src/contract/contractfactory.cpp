#include "contractfactory.h"

std::unordered_set<Address, SafeHash> ContractFactory::getRecentContracts() const {
  return this->recentContracts;
}

void ContractFactory::clearRecentContracts() {
  this->recentContracts.clear();
}

std::function<void(const ethCallInfo&)> ContractFactory::getCreateContractFunc(Functor func) const {
  std::function<void(const ethCallInfo&)> ret;
  auto createIt = this->createContractFuncs.find(func.asBytes());
  if (createIt != this->createContractFuncs.end()) ret = createIt->second;
  return ret;
}

