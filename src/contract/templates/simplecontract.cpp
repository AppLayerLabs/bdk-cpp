/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "simplecontract.h"

SimpleContract::SimpleContract(
  const std::string& name,
  uint256_t value,
  ContractManagerInterface &interface,
  const Address& address,
  const Address& creator,
  const uint64_t& chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "SimpleContract", address, creator, chainId, db), name_(this), value_(this) {
  this->name_ = name;
  this->value_ = value;
  registerContractFunctions();
}

SimpleContract::SimpleContract(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), name_(this), value_(this) {
  this->name_ = Utils::bytesToString(db->get(std::string("name_"), this->getDBPrefix()));
  this->value_ = Utils::bytesToUint256(db->get(std::string("value_"), this->getDBPrefix()));
  registerContractFunctions();
}

SimpleContract::~SimpleContract() {
  this->db_->put(std::string("name_"), Utils::stringToBytes(this->name_.get()), this->getDBPrefix());
  this->db_->put(std::string("value_"), Utils::uint256ToBytes(this->value_.get()), this->getDBPrefix());
  return;
}

std::string SimpleContract::getName() const { return this->name_.get(); }

uint256_t SimpleContract::getValue() const { return this->value_.get(); }

void SimpleContract::setName(const std::string& argName) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = argName;
}

void SimpleContract::setValue(uint256_t argValue) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->value_ = argValue;
}

void SimpleContract::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("getName", &SimpleContract::getName, this);
  this->registerMemberFunction("getValue", &SimpleContract::getValue, this);
  this->registerMemberFunction("setName", &SimpleContract::setName, this);
  this->registerMemberFunction("setValue", &SimpleContract::setValue, this);
}

