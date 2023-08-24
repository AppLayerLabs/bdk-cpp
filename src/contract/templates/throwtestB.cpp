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

uint8_t ThrowTestB::getNum() const { return this->num.get(); }

void ThrowTestB::setNum(const uint8_t& valB, const Address& addC, const uint8_t& valC) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->num = valB;
  this->callContractFunction(addC, &ThrowTestC::setNum, valC);
}

