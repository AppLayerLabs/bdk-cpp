/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contractcallstate.h"
#include "contractmanager.h"
#include "dynamiccontract.h"
#include "contractfactory.h"

ContractCallState::ContractCallState(ContractManager& manager) : manager(manager) {}

ContractCallState::~ContractCallState() {
  if (this->commitCall) {
    this->commit();
  } else {
    this->revert();
  }
  this->manager.factory->clearRecentContracts();
  this->balances.clear();
  this->usedVars.clear();
}

void ContractCallState::commit() {
  for (auto rbegin = this->usedVars.rbegin(); rbegin != this->usedVars.rend(); rbegin++) {
    rbegin->get().commit();
  }
}

void ContractCallState::revert() {
  for (auto rbegin = this->usedVars.rbegin(); rbegin != this->usedVars.rend(); rbegin++) {
    rbegin->get().revert();
  }
  for (const Address& badContract : this->manager.factory->getRecentContracts()) {
    this->manager.contracts.erase(badContract); // Erase failed contract creations
  }
}