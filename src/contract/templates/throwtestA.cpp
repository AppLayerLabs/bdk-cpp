/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "throwtestA.h"

ThrowTestA::ThrowTestA(
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("ThrowTestA", address, creator, chainId), num_(this) {
  this->num_.commit();
  registerContractFunctions();
  this->num_.enableRegister();
}

ThrowTestA::ThrowTestA(const Address& address, const DB& db) : DynamicContract(address, db) {
  this->num_ = UintConv::bytesToUint8(db.get(std::string("num_"), this->getDBPrefix()));
  this->num_.commit();
  registerContractFunctions();
  this->num_.enableRegister();
}

ThrowTestA::~ThrowTestA() { return; }

DBBatch ThrowTestA::dump() const {
  DBBatch dbBatch = BaseContract::dump();
  dbBatch.push_back(StrConv::stringToBytes("num_"), UintConv::uint8ToBytes(this->num_.get()), this->getDBPrefix());
  return dbBatch;
}

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
  this->registerMemberFunction("getNumA", &ThrowTestA::getNumA, FunctionTypes::View, this);
  this->registerMemberFunction("setNumA", &ThrowTestA::setNumA, FunctionTypes::NonPayable, this);
}

