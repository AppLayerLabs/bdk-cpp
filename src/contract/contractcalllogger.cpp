#include "contractcalllogger.h"
#include "contractmanager.h"
#include "dynamiccontract.h"
#include "contractfactory.h"

ContractCallLogger::ContractCallLogger(ContractManager& manager) : manager_(manager) {}

ContractCallLogger::~ContractCallLogger() {
  if (this->commitCall_) {
    this->commit();
  } else {
    this->revert();
  }
  this->manager_.factory_->clearRecentContracts();
  this->balances_.clear();
  this->usedVars_.clear();
}

void ContractCallLogger::commit() {
  for (auto rbegin = this->usedVars_.rbegin(); rbegin != this->usedVars_.rend(); rbegin++) {
    rbegin->get().commit();
  }
}

void ContractCallLogger::revert() {
  for (auto rbegin = this->usedVars_.rbegin(); rbegin != this->usedVars_.rend(); rbegin++) {
    rbegin->get().revert();
  }
  for (const Address& badContract : this->manager_.factory_->getRecentContracts()) {
    this->manager_.contracts_.erase(badContract); // Erase failed contract creations
  }
}
