/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "simplecontract.h"

SimpleContract::SimpleContract(
  const std::string& name,
  const uint256_t& value,
  const std::tuple<std::string, uint256_t>& tuple,
  ContractManagerInterface &interface,
  const Address& address,
  const Address& creator,
  const uint64_t& chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "SimpleContract", address, creator, chainId, db),
  name_(this), value_(this), tuple_(this)
{
  this->name_ = name;
  this->value_ = value;
  this->tuple_ = tuple;
  registerContractFunctions();
}

SimpleContract::SimpleContract(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), name_(this), value_(this), tuple_(this) {
  this->name_ = Utils::bytesToString(db->get(std::string("name_"), this->getDBPrefix()));
  this->value_ = Utils::bytesToUint256(db->get(std::string("value_"), this->getDBPrefix()));
  this->tuple_ = std::make_tuple(
    Utils::bytesToString(db->get(std::string("tuple_name"), this->getDBPrefix())),
    Utils::bytesToUint256(db->get(std::string("tuple_value"), this->getDBPrefix()))
  );
  registerContractFunctions();
}

SimpleContract::~SimpleContract() {
  this->db_->put(std::string("name_"), Utils::stringToBytes(this->name_.get()), this->getDBPrefix());
  this->db_->put(std::string("value_"), Utils::uint256ToBytes(this->value_.get()), this->getDBPrefix());
  this->db_->put(std::string("tuple_name"), Utils::stringToBytes(get<0>(this->tuple_)), this->getDBPrefix());
  this->db_->put(std::string("tuple_value"), Utils::uint256ToBytes(get<1>(this->tuple_)), this->getDBPrefix());
}

void SimpleContract::setName(const std::string& argName) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = argName;
  this->nameChanged(this->name_.get());
}

void SimpleContract::setNames(const std::vector<std::string>& argName) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = "";
  for (const auto& name : argName) this->name_ += name;
  this->nameChanged(this->name_.get());
}

void SimpleContract::setValue(const uint256_t& argValue) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->value_ = argValue;
  this->valueChanged(this->value_.get());
}

void SimpleContract::setValues(const std::vector<uint256_t>& argValue) {
  this->value_ = 0;
  for (const auto& value : argValue) this->value_ += value;
  this->valueChanged(this->value_.get());
}

void SimpleContract::setNamesAndValues(
  const std::vector<std::string>& argName, const std::vector<uint256_t>& argValue
) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = "";
  this->value_ = 0;
  for (const auto& name : argName) this->name_ += name;
  for (const auto& value : argValue) this->value_ += value;
  this->nameAndValueChanged(this->name_.get(), this->value_.get());
}

void SimpleContract::setNamesAndValuesInTuple(
  const std::vector<std::tuple<std::string, uint256_t>>& argNameAndValue
) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = "";
  this->value_ = 0;
  for (const auto& [name, value] : argNameAndValue) { this->name_ += name; this->value_ += value; }
  this->nameAndValueTupleChanged(std::make_tuple(this->name_.get(), this->value_.get()));
}

void SimpleContract::setNamesAndValuesInArrayOfArrays(
  const std::vector<std::vector<std::tuple<std::string, uint256_t>>> &argNameAndValue
) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->name_ = "";
  this->value_ = 0;
  for (const auto& nameAndValue : argNameAndValue) {
    for (const auto& [name, value] : nameAndValue) { this->name_ += name; this->value_ += value; }
  }
  this->nameAndValueChanged(this->name_.get(), this->value_.get());
}

void SimpleContract::setTuple(const std::tuple<std::string, uint256_t>& argTuple) {
  if (this->getCaller() != this->getContractCreator()) {
    throw std::runtime_error("Only contract creator can call this function.");
  }
  this->tuple_ = argTuple;
  this->tupleChanged(std::make_tuple(get<0>(this->tuple_), get<1>(this->tuple_)));
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

uint256_t SimpleContract::getValue(const uint256_t& i) const { return this->value_.get() + i; }

std::vector<uint256_t> SimpleContract::getValues(const uint256_t& i) const {
  std::vector<uint256_t> values;
  for (uint256_t j = 0; j < i; j++) values.emplace_back(this->value_.get());
  return values;
}

std::tuple<std::string, uint256_t> SimpleContract::getNameAndValue() const {
  return std::make_tuple(this->name_.get(), this->value_.get());
}

std::tuple<std::vector<std::string>, std::vector<uint256_t>>
SimpleContract::getNamesAndValues(const uint256_t& i) const {
  std::vector<std::string> names;
  std::vector<uint256_t> values;
  for (uint256_t j = 0; j < i; j++) {
    names.emplace_back(this->name_.get());
    values.emplace_back(this->value_.get());
  }
  return std::make_tuple(names, values);
}

std::vector<std::tuple<std::string, uint256_t>>
SimpleContract::getNamesAndValuesInTuple(const uint256_t& i) const {
  std::vector<std::tuple<std::string, uint256_t>> namesAndValues;
  for (uint256_t j = 0; j < i; j++) {
    namesAndValues.emplace_back(std::make_tuple(this->name_.get(), this->value_.get()));
  }
  return namesAndValues;
}

std::vector<std::vector<std::tuple<std::string, uint256_t>>>
SimpleContract::getNamesAndValuesInArrayOfArrays(const uint256_t& i) const {
  std::vector<std::vector<std::tuple<std::string, uint256_t>>> namesAndValues;
  for (uint256_t j = 0; j < i; j++) {
    std::vector<std::tuple<std::string, uint256_t>> nameAndValuesInternal;
    for (uint256_t k = 0; k < i; k++) {
      nameAndValuesInternal.emplace_back(std::make_tuple(this->name_.get(), this->value_.get()));
    }
    namesAndValues.emplace_back(nameAndValuesInternal);
  }
  return namesAndValues;
}

std::tuple<std::string, uint256_t> SimpleContract::getTuple() const {
  return std::make_tuple(get<0>(this->tuple_), get<1>(this->tuple_));
}

void SimpleContract::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("setName", &SimpleContract::setName, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNames", &SimpleContract::setNames, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setValue", &SimpleContract::setValue, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setValues", &SimpleContract::setValues, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNamesAndValues", &SimpleContract::setNamesAndValues, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNamesAndValuesInTuple", &SimpleContract::setNamesAndValuesInTuple, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNamesAndValuesInArrayOfArrays", &SimpleContract::setNamesAndValuesInArrayOfArrays, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setTuple", &SimpleContract::setTuple, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getName", &SimpleContract::getName, FunctionTypes::View, this);
  this->registerMemberFunction("getNames", &SimpleContract::getNames, FunctionTypes::View, this);
  this->registerMemberFunction("getValue", static_cast<uint256_t(SimpleContract::*)() const>(&SimpleContract::getValue), FunctionTypes::View, this);
  this->registerMemberFunction("getValue", static_cast<uint256_t(SimpleContract::*)(const uint256_t&) const>(&SimpleContract::getValue), FunctionTypes::View, this);
  this->registerMemberFunction("getValues", &SimpleContract::getValues, FunctionTypes::View, this);
  this->registerMemberFunction("getNameAndValue", &SimpleContract::getNameAndValue, FunctionTypes::View, this);
  this->registerMemberFunction("getNamesAndValues", &SimpleContract::getNamesAndValues, FunctionTypes::View, this);
  this->registerMemberFunction("getNamesAndValuesInTuple", &SimpleContract::getNamesAndValuesInTuple, FunctionTypes::View, this);
  this->registerMemberFunction("getNamesAndValuesInArrayOfArrays", &SimpleContract::getNamesAndValuesInArrayOfArrays, FunctionTypes::View, this);
  this->registerMemberFunction("getTuple", &SimpleContract::getTuple, FunctionTypes::View, this);
}

