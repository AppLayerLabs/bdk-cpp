/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "ownable.h"

#include "../../utils/strconv.h"

Ownable::Ownable(
  const Address& address, const DB& db
) : DynamicContract(address, db), owner_(this)
{
  this->owner_ = Address(db.get(std::string("owner_"), this->getDBPrefix()));
  this->owner_.commit();
  Ownable::registerContractFunctions();
  this->owner_.enableRegister();
}

Ownable::Ownable(
  const Address& initialOwner, const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("Ownable", address, creator, chainId), owner_(this)
{
  this->owner_ = initialOwner;
  this->owner_.commit();
  Ownable::registerContractFunctions();
  this->owner_.enableRegister();
}

Ownable::Ownable(
  const std::string& derivedTypeName,
  const Address& initialOwner, const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract(derivedTypeName, address, creator, chainId), owner_(this)
{
  this->owner_ = initialOwner;
  this->owner_.commit();
  Ownable::registerContractFunctions();
  this->owner_.enableRegister();
}

void Ownable::registerContractFunctions() {
  Ownable::registerContract();
  this->registerMemberFunction("onlyOwner", &Ownable::onlyOwner, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("owner", &Ownable::owner, FunctionTypes::View, this);
  this->registerMemberFunction("renounceOwnership", &Ownable::renounceOwnership, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("transferOwnership", &Ownable::transferOwnership, FunctionTypes::NonPayable, this);
}

DBBatch Ownable::dump() const {
  DBBatch batch = BaseContract::dump();
  batch.push_back(StrConv::stringToBytes("owner_"), this->owner_.get().asBytes(), this->getDBPrefix());
  return batch;
}

void Ownable::checkOwner_() const {
  if (this->owner_ != this->getCaller()) {
    throw DynamicException("Ownable: caller is not the owner");
  }
}

void Ownable::transferOwnership_(const Address& newOwner) {
  this->owner_ = newOwner;
}


void Ownable::onlyOwner() const {
  this->checkOwner_();
}

Address Ownable::owner() const {
  return this->owner_.get();
}

void Ownable::renounceOwnership() {
  this->onlyOwner();
  this->transferOwnership_(Address());
}

void Ownable::transferOwnership(const Address& newOwner) {
  this->onlyOwner();
  if (newOwner == Address()) {
    throw DynamicException("Ownable: new owner is the zero address");
  }
  this->transferOwnership_(newOwner);
}
