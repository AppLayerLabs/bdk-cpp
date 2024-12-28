/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "throwtestC.h"

ThrowTestC::ThrowTestC(
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("ThrowTestC", address, creator, chainId), num_(this) {
  this->num_.commit();
  registerContractFunctions();
  this->num_.enableRegister();
}

ThrowTestC::ThrowTestC(const Address& address, const DB& db) : DynamicContract(address, db) {
  this->num_ = UintConv::bytesToUint8(db.get(std::string("num_"), this->getDBPrefix()));
  this->num_.commit();
  registerContractFunctions();
  this->num_.enableRegister();
}

ThrowTestC::~ThrowTestC() { return; }

DBBatch ThrowTestC::dump() const {
  DBBatch dbBatch = BaseContract::dump();
  dbBatch.push_back(StrConv::stringToBytes("num_"), UintConv::uint8ToBytes(this->num_.get()), this->getDBPrefix());
  return dbBatch;
}

uint8_t ThrowTestC::getNumC() const { return this->num_.get(); }

void ThrowTestC::setNumC(const uint8_t& valC) {
  this->num_ = valC;
}

void ThrowTestC::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNumC", &ThrowTestC::getNumC, FunctionTypes::View, this);
  this->registerMemberFunction("setNumC", &ThrowTestC::setNumC, FunctionTypes::NonPayable, this);
}

