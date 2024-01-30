/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "throwtestC.h"

ThrowTestC::ThrowTestC(
  ContractManagerInterface &interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB> &db
) : DynamicContract(interface, "ThrowTestC", address, creator, chainId, db) {
  registerContractFunctions();
}

ThrowTestC::ThrowTestC(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db) {
  registerContractFunctions();
}

ThrowTestC::~ThrowTestC() { return; }

uint8_t ThrowTestC::getNumC() const { return this->num_.get(); }

void ThrowTestC::setNumC(const uint8_t& valC) {
  this->num_ = valC;
}

void ThrowTestC::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNumC", &ThrowTestC::getNumC, FunctionTypes::View, this);
  this->registerMemberFunction("setNumC", &ThrowTestC::setNumC, FunctionTypes::NonPayable, this);
}

