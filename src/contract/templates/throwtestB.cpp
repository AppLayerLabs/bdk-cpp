#include "throwtestB.h"

ThrowTestB::ThrowTestB(
  ContractManagerInterface &interface,
  const Address& address, const Address& creator,
  const uint64_t& chainId, const std::unique_ptr<DB> &db
) : DynamicContract(interface, "ThrowTestB", address, creator, chainId, db) {
  registerContractFunctions();
}

ThrowTestB::ThrowTestB(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db) {
  registerContractFunctions();
}

ThrowTestB::~ThrowTestB() { return; }

uint8_t ThrowTestB::getNumB() const { return this->num_.get(); }

void ThrowTestB::setNumB(const uint8_t& valB, const Address& addC, const uint8_t& valC) {
  this->num_ = valB;
  throw std::runtime_error("Intended throw in ThrowTestB");
}

void ThrowTestB::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNumB", &ThrowTestB::getNumB, this);
  this->registerMemberFunction("setNumB", &ThrowTestB::setNumB, this);
}

