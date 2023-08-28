#include "simplecontract.h"

SimpleContract::SimpleContract(
  const std::string& name,
  uint256_t value,
  ContractManagerInterface &interface,
  const Address& address,
  const Address& creator,
  const uint64_t& chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "SimpleContract", address, creator, chainId, db), name(this), value(this) {
  this->name = name;
  this->value = value;
  registerContractFunctions();
}

SimpleContract::SimpleContract(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), name(this), value(this) {
  this->name = Utils::bytesToString(db->get(std::string("name"), this->getDBPrefix()));
  this->value = Utils::bytesToUint256(db->get(std::string("value"), this->getDBPrefix()));
  registerContractFunctions();
}

SimpleContract::~SimpleContract() {
  this->db->put(std::string("name"), Utils::stringToBytes(this->name.get()), this->getDBPrefix());
  this->db->put(std::string("value"), Utils::uint256ToBytes(this->value.get()), this->getDBPrefix());
  return;
}

std::string SimpleContract::getName() const { return this->name.get(); }

uint256_t SimpleContract::getValue() const { return this->value.get(); }

void SimpleContract::setName(const std::string& argName) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name = argName;
}

void SimpleContract::setValue(uint256_t argValue) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->value = argValue;
}

void SimpleContract::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getName", &SimpleContract::getName, this);
  this->registerMemberFunction("getValue", &SimpleContract::getValue, this);
  this->registerMemberFunction("setName", &SimpleContract::setName, this);
  this->registerMemberFunction("setValue", &SimpleContract::setValue, this);
}

