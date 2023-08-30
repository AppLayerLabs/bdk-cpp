/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "throwtestA.h"

ThrowTestA::ThrowTestA(
  ContractManagerInterface &interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB> &db
) : DynamicContract(interface, "ThrowTestA", address, creator, chainId, db) {
  registerContractFunctions();
}

ThrowTestA::ThrowTestA(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db) {
  registerContractFunctions();
}

ThrowTestA::~ThrowTestA() { return; }

uint8_t ThrowTestA::getNumA() const { return this->num_.get(); }

void ThrowTestA::setNumA(const uint8_t& valA,
  const Address& addB, const uint8_t& valB,
  const Address& addC, const uint8_t& valC
) {
  this->num_ = valA;
  this->callContractFunction(addB, &ThrowTestB::setNumB, valB, addC, valC);
}

void ThrowTestA::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNumA", &ThrowTestA::getNumA, this);
  this->registerMemberFunction("setNumA", &ThrowTestA::setNumA, this);
}

