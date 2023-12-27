/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "simplecontract.h"

SimpleContract::SimpleContract(
  const std::string& name,
  const uint256_t& value,
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

void SimpleContract::setName(const std::string& argName) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = argName;
}

void SimpleContract::setNames(const std::vector<std::string>& argName) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = "";
  for (const auto& name : argName) {
    this->name_ += name;
  }
}

void SimpleContract::setValue(const uint256_t& argValue) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->value_ = argValue;
}

void SimpleContract::setValues(const std::vector<uint256_t>& argValue) {
  this->value_ = 0;
  for (const auto& value : argValue) {
    this->value_ += value;
  }
}

void SimpleContract::setNamesAndValues(const std::vector<std::string>& argName, const std::vector<uint256_t>& argValue) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = "";
  this->value_ = 0;
  for (const auto& name : argName) {
    this->name_ += name;
  }
  for (const auto& value : argValue) {
    this->value_ += value;
  }
}

void SimpleContract::setNamesAndValuesInTuple(const std::vector<std::tuple<std::string, uint256_t>>& argNameAndValue) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = "";
  this->value_ = 0;
  for (const auto& [name, value] : argNameAndValue) {
    this->name_ += name;
    this->value_ += value;
  }
}

std::string SimpleContract::getName() const { return this->name_.get(); }

std::vector<std::string> SimpleContract::getNames(const uint256_t& i) const {
  std::vector<std::string> names;
  for (uint256_t j = 0; j < i; j++) {
    names.emplace_back(this->name_.get());
  }
  return names;
}

uint256_t SimpleContract::getValue() const { return this->value_.get(); }

std::vector<uint256_t> SimpleContract::getValues(const uint256_t& i) const {
  std::vector<uint256_t> values;
  for (uint256_t j = 0; j < i; j++) {
    values.emplace_back(this->value_.get());
  }
  return values;
}

std::tuple<std::string, uint256_t> SimpleContract::getNameAndValue() const {
  return std::make_tuple(this->name_.get(), this->value_.get());
}

std::tuple<std::vector<std::string>, std::vector<uint256_t>> SimpleContract::getNamesAndValues(const uint256_t& i) const {
  std::vector<std::string> names;
  std::vector<uint256_t> values;
  for (uint256_t j = 0; j < i; j++) {
    names.emplace_back(this->name_.get());
    values.emplace_back(this->value_.get());
  }
  return std::make_tuple(names, values);
}

std::vector<std::tuple<std::string, uint256_t>> SimpleContract::getNamesAndValuesInTuple(const uint256_t& i) const {
  std::vector<std::tuple<std::string, uint256_t>> namesAndValues;
  for (uint256_t j = 0; j < i; j++) {
    namesAndValues.emplace_back(std::make_tuple(this->name_.get(), this->value_.get()));
  }
  return namesAndValues;
}

void SimpleContract::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("setName", &SimpleContract::setName, this);
  this->registerMemberFunction("setNames", &SimpleContract::setNames, this);
  this->registerMemberFunction("setValue", &SimpleContract::setValue, this);
  this->registerMemberFunction("setValues", &SimpleContract::setValues, this);
  this->registerMemberFunction("setNamesAndValues", &SimpleContract::setNamesAndValues, this);
  this->registerMemberFunction("setNamesAndValuesInTuple", &SimpleContract::setNamesAndValuesInTuple, this);
  this->registerMemberFunction("getName", &SimpleContract::getName, this);
  this->registerMemberFunction("getNames", &SimpleContract::getNames, this);
  this->registerMemberFunction("getValue", &SimpleContract::getValue, this);
  this->registerMemberFunction("getValues", &SimpleContract::getValues, this);
  this->registerMemberFunction("getNameAndValue", &SimpleContract::getNameAndValue, this);
  this->registerMemberFunction("getNamesAndValues", &SimpleContract::getNamesAndValues, this);
  this->registerMemberFunction("getNamesAndValuesInTuple", &SimpleContract::getNamesAndValuesInTuple, this);
}