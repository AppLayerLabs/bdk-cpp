/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "throwtestC.h"

ThrowTestC::ThrowTestC(
  const Address& address, const Address& creator,
  const uint64_t& chainId
) : DynamicContract("ThrowTestC", address, creator, chainId) {
  registerContractFunctions();
}
/*
ThrowTestC::ThrowTestC(
  const Address& address,
  const DB& db
) : DynamicContract(address, db) {
  registerContractFunctions();
}
*/
ThrowTestC::~ThrowTestC() { return; }

//DBBatch ThrowTestC::dump() const
//{
//  return BaseContract::dump();
//}

uint8_t ThrowTestC::getNumC() const { return this->num_.get(); }

void ThrowTestC::setNumC(const uint8_t& valC) {
  this->num_ = valC;
}

void ThrowTestC::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNumC", &ThrowTestC::getNumC, FunctionTypes::View, this);
  this->registerMemberFunction("setNumC", &ThrowTestC::setNumC, FunctionTypes::NonPayable, this);
}

