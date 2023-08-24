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

uint8_t ThrowTestC::getNum() const { return this->num.get(); }

void ThrowTestC::setNum(const uint8_t& valC) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->num = valC;
}

void ThrowTestC::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getNum", &ThrowTestC::getNum, this);
  this->registerMemberFunction("setNum", &ThrowTestC::setNum, this);
}

