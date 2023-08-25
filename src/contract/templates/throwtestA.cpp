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

uint8_t ThrowTestA::getNumA() const { return this->num.get(); }

void ThrowTestA::setNumA(const uint8_t& valA,
  const Address& addB, const uint8_t& valB,
  const Address& addC, const uint8_t& valC
) {
  this->num = valA;
  this->callContractFunction(addB, &ThrowTestB::setNumB, valB, addC, valC);
}

void ThrowTestA::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNumA", &ThrowTestA::getNumA, this);
  this->registerMemberFunction("setNumA", &ThrowTestA::setNumA, this);
}

