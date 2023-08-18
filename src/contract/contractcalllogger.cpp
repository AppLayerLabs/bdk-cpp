#include "contractcalllogger.h"
#include "contractmanager.h"
#include "dynamiccontract.h"
#include "contractfactory.h"

ContractCallLogger::ContractCallLogger(ContractManager& manager) : manager(manager) {}

ContractCallLogger::~ContractCallLogger() {
  if (this->commitCall) {
    this->commit();
  } else {
    this->revert();
  }
  this->manager.factory->clearRecentContracts();
  this->balances.clear();
  this->usedVars.clear();
}

void ContractCallLogger::commit() {
  for (auto rbegin = this->usedVars.rbegin(); rbegin != this->usedVars.rend(); rbegin++) {
    rbegin->get().commit();
  }
}

void ContractCallLogger::revert() {
  for (auto rbegin = this->usedVars.rbegin(); rbegin != this->usedVars.rend(); rbegin++) {
    rbegin->get().revert();
  }
  for (const Address& badContract : this->manager.factory->getRecentContracts()) {
    this->manager.contracts.erase(badContract); // Erase failed contract creations
  }
}