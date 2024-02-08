/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "simplecontract.h"

SimpleContract::SimpleContract(
  const std::string& name,
  const uint256_t& number,
  const std::tuple<std::string, uint256_t>& tuple,
  ContractManagerInterface &interface,
  const Address& address,
  const Address& creator,
  const uint64_t& chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "SimpleContract", address, creator, chainId, db),
  name_(this), number_(this), tuple_(this)
{
  this->name_ = name;
  this->number_ = number;
  this->tuple_ = tuple;
  registerContractFunctions();
}

SimpleContract::SimpleContract(
  ContractManagerInterface &interface,
  const Address& address,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), name_(this), number_(this), tuple_(this) {
  this->name_ = Utils::bytesToString(db->get(std::string("name_"), this->getDBPrefix()));
  this->number_ = Utils::bytesToUint256(db->get(std::string("number_"), this->getDBPrefix()));
  this->tuple_ = std::make_tuple(
    Utils::bytesToString(db->get(std::string("tuple_name"), this->getDBPrefix())),
    Utils::bytesToUint256(db->get(std::string("tuple_number"), this->getDBPrefix()))
  );
  registerContractFunctions();
}

SimpleContract::~SimpleContract() {
  this->db_->put(std::string("name_"), Utils::stringToBytes(this->name_.get()), this->getDBPrefix());
  this->db_->put(std::string("number_"), Utils::uint256ToBytes(this->number_.get()), this->getDBPrefix());
  this->db_->put(std::string("tuple_name"), Utils::stringToBytes(get<0>(this->tuple_)), this->getDBPrefix());
  this->db_->put(std::string("tuple_number"), Utils::uint256ToBytes(get<1>(this->tuple_)), this->getDBPrefix());
}

void SimpleContract::setName(const std::string& argName) {
  if (this->getCaller() != this->getContractCreator()) {
    throw DynamicException("Only contract creator can call this function.");
  }
  this->name_ = argName;
  this->nameChanged(this->name_.get());
}

void SimpleContract::setNames(const std::vector<std::string>& argName) {
  if (this->getCaller() != this->getContractCreator()) {
    throw DynamicException("Only contract creator can call this function.");
  }
  this->name_ = "";
  for (const auto& name : argName) this->name_ += name;
  this->nameChanged(this->name_.get());
}

void SimpleContract::setNumber(const uint256_t& argNumber) {
  if (this->getCaller() != this->getContractCreator()) {
    throw DynamicException("Only contract creator can call this function.");
  }
  this->number_ = argNumber;
  this->numberChanged(this->number_.get());
}

void SimpleContract::setNumbers(const std::vector<uint256_t>& argNumber) {
  this->number_ = 0;
  for (const auto& number : argNumber) this->number_ += number;
  this->numberChanged(this->number_.get());
}

void SimpleContract::setNamesAndNumbers(
  const std::vector<std::string>& argName, const std::vector<uint256_t>& argNumber
) {
  if (this->getCaller() != this->getContractCreator()) {
    throw DynamicException("Only contract creator can call this function.");
  }
  this->name_ = "";
  this->number_ = 0;
  for (const auto& name : argName) this->name_ += name;
  for (const auto& number : argNumber) this->number_ += number;
  this->nameAndNumberChanged(this->name_.get(), this->number_.get());
}

void SimpleContract::setNamesAndNumbersInTuple(
  const std::vector<std::tuple<std::string, uint256_t>>& argNameAndNumber
) {
  if (this->getCaller() != this->getContractCreator()) {
    throw DynamicException("Only contract creator can call this function.");
  }
  this->name_ = "";
  this->number_ = 0;
  for (const auto& [name, number] : argNameAndNumber) { this->name_ += name; this->number_ += number; }
  this->nameAndNumberTupleChanged(std::make_tuple(this->name_.get(), this->number_.get()));
}

void SimpleContract::setNamesAndNumbersInArrayOfArrays(
  const std::vector<std::vector<std::tuple<std::string, uint256_t>>> &argNameAndNumber
) {
  if (this->getCaller() != this->getContractCreator()) {
    throw DynamicException("Only contract creator can call this function.");
  }
  this->name_ = "";
  this->number_ = 0;
  for (const auto& nameAndNumber : argNameAndNumber) {
    for (const auto& [name, number] : nameAndNumber) { this->name_ += name; this->number_ += number; }
  }
  this->nameAndNumberChanged(this->name_.get(), this->number_.get());
}

void SimpleContract::setTuple(const std::tuple<std::string, uint256_t>& argTuple) {
  if (this->getCaller() != this->getContractCreator()) {
    throw DynamicException("Only contract creator can call this function.");
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

uint256_t SimpleContract::getNumber() const { return this->number_.get(); }

uint256_t SimpleContract::getNumber(const uint256_t& i) const { return this->number_.get() + i; }

std::vector<uint256_t> SimpleContract::getNumbers(const uint256_t& i) const {
  std::vector<uint256_t> numbers;
  for (uint256_t j = 0; j < i; j++) numbers.emplace_back(this->number_.get());
  return numbers;
}

std::tuple<std::string, uint256_t> SimpleContract::getNameAndNumber() const {
  return std::make_tuple(this->name_.get(), this->number_.get());
}

std::tuple<std::vector<std::string>, std::vector<uint256_t>>
SimpleContract::getNamesAndNumbers(const uint256_t& i) const {
  std::vector<std::string> names;
  std::vector<uint256_t> numbers;
  for (uint256_t j = 0; j < i; j++) {
    names.emplace_back(this->name_.get());
    numbers.emplace_back(this->number_.get());
  }
  return std::make_tuple(names, numbers);
}

std::vector<std::tuple<std::string, uint256_t>>
SimpleContract::getNamesAndNumbersInTuple(const uint256_t& i) const {
  std::vector<std::tuple<std::string, uint256_t>> namesAndNumbers;
  for (uint256_t j = 0; j < i; j++) {
    namesAndNumbers.emplace_back(this->name_.get(), this->number_.get());
  }
  return namesAndNumbers;
}

std::vector<std::vector<std::tuple<std::string, uint256_t>>>
SimpleContract::getNamesAndNumbersInArrayOfArrays(const uint256_t& i) const {
  std::vector<std::vector<std::tuple<std::string, uint256_t>>> namesAndNumbers;
  for (uint256_t j = 0; j < i; j++) {
    std::vector<std::tuple<std::string, uint256_t>> nameAndNumbersInternal;
    for (uint256_t k = 0; k < i; k++) {
      nameAndNumbersInternal.emplace_back(this->name_.get(), this->number_.get());
    }
    namesAndNumbers.emplace_back(nameAndNumbersInternal);
  }
  return namesAndNumbers;
}

std::tuple<std::string, uint256_t> SimpleContract::getTuple() const {
  return std::make_tuple(get<0>(this->tuple_), get<1>(this->tuple_));
}

void SimpleContract::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("setName", &SimpleContract::setName, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNames", &SimpleContract::setNames, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNumber", &SimpleContract::setNumber, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNumbers", &SimpleContract::setNumbers, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNamesAndNumbers", &SimpleContract::setNamesAndNumbers, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNamesAndNumbersInTuple", &SimpleContract::setNamesAndNumbersInTuple, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setNamesAndNumbersInArrayOfArrays", &SimpleContract::setNamesAndNumbersInArrayOfArrays, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setTuple", &SimpleContract::setTuple, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getName", &SimpleContract::getName, FunctionTypes::View, this);
  this->registerMemberFunction("getNames", &SimpleContract::getNames, FunctionTypes::View, this);
  this->registerMemberFunction("getNumber", static_cast<uint256_t(SimpleContract::*)() const>(&SimpleContract::getNumber), FunctionTypes::View, this);
  this->registerMemberFunction("getNumber", static_cast<uint256_t(SimpleContract::*)(const uint256_t&) const>(&SimpleContract::getNumber), FunctionTypes::View, this);
  this->registerMemberFunction("getNumbers", &SimpleContract::getNumbers, FunctionTypes::View, this);
  this->registerMemberFunction("getNameAndNumber", &SimpleContract::getNameAndNumber, FunctionTypes::View, this);
  this->registerMemberFunction("getNamesAndNumbers", &SimpleContract::getNamesAndNumbers, FunctionTypes::View, this);
  this->registerMemberFunction("getNamesAndNumbersInTuple", &SimpleContract::getNamesAndNumbersInTuple, FunctionTypes::View, this);
  this->registerMemberFunction("getNamesAndNumbersInArrayOfArrays", &SimpleContract::getNamesAndNumbersInArrayOfArrays, FunctionTypes::View, this);
  this->registerMemberFunction("getTuple", &SimpleContract::getTuple, FunctionTypes::View, this);
}

