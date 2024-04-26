/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "throwtestB.h"

ThrowTestB::ThrowTestB(
  const Address& address, const Address& creator,
  const uint64_t& chainId, DB& db
) : DynamicContract("ThrowTestB", address, creator, chainId, db) {
  registerContractFunctions();
}

ThrowTestB::ThrowTestB(
  const Address& address,
  DB& db
) : DynamicContract(address, db) {
  registerContractFunctions();
}

ThrowTestB::~ThrowTestB() { return; }

DBBatch ThrowTestB::dump() const
{
  return BaseContract::dump();
}

uint8_t ThrowTestB::getNumB() const { return this->num_.get(); }

[[noreturn]] void ThrowTestB::setNumB(
  const uint8_t& valB, const Address&, const uint8_t&
) {
  this->num_ = valB;
  throw DynamicException("Intended throw in ThrowTestB");
}

void ThrowTestB::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNumB", &ThrowTestB::getNumB, FunctionTypes::View, this);
  this->registerMemberFunction("setNumB", &ThrowTestB::setNumB, FunctionTypes::NonPayable, this);
}

