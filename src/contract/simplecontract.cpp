#include "simplecontract.h"

SimpleContract::SimpleContract(
  const std::string& name,
  uint256_t value,
  ContractManager::ContractManagerInterface &interface,
  const Address& address,
  const Address& creator,
  const uint64_t& chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "SimpleContract", address, creator, chainId, db), name(this), value(this) {
  this->name = name;
  this->value = value;
  registerContractFunctions();
  this->updateState(true);
}

SimpleContract::SimpleContract(
  ContractManager::ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), name(this), value(this) {
  this->name = db->get("name", DBPrefix::contracts + this->getContractAddress().get());
  this->value = Utils::bytesToUint256(db->get("value", DBPrefix::contracts + this->getContractAddress().get()));
  registerContractFunctions();
  this->updateState(true);
}

SimpleContract::~SimpleContract() {
  this->db->put("name", this->name.get(), DBPrefix::contracts + this->getContractAddress().get());
  this->db->put("value", Utils::uint256ToBytes(this->value.get()), DBPrefix::contracts + this->getContractAddress().get());
  return;
}

std::string SimpleContract::getName() const { return ABI::Encoder({this->name.get()}).getRaw(); }

std::string SimpleContract::getValue() const { return ABI::Encoder({this->value.get()}).getRaw(); }

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
  registerMemberFunction("getName", &SimpleContract::getName, this);
  registerMemberFunction("getValue", &SimpleContract::getValue, this);
  registerMemberFunction("setName", &SimpleContract::setName, this);
  registerMemberFunction("setValue", &SimpleContract::setValue, this);
}