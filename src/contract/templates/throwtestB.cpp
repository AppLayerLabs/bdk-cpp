/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "throwtestB.h"

ThrowTestB::ThrowTestB(
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("ThrowTestB", address, creator, chainId), num_(this) {
  this->num_.commit();
  registerContractFunctions();
  this->num_.enableRegister();
}

ThrowTestB::ThrowTestB(const Address& address, const DB& db) : DynamicContract(address, db) {
  this->num_ = UintConv::bytesToUint8(db.get(std::string("num_"), this->getDBPrefix()));
  this->num_.commit();
  registerContractFunctions();
  this->num_.enableRegister();
}

ThrowTestB::~ThrowTestB() { return; }

DBBatch ThrowTestB::dump() const {
  DBBatch dbBatch = BaseContract::dump();
  dbBatch.push_back(StrConv::stringToBytes("num_"), UintConv::uint8ToBytes(this->num_.get()), this->getDBPrefix());
  return dbBatch;
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
  this->registerMemberFunctions(
    std::make_tuple("getNumB", &ThrowTestB::getNumB, FunctionTypes::View, this),
    std::make_tuple("setNumB", &ThrowTestB::setNumB, FunctionTypes::NonPayable, this)
  );
}

