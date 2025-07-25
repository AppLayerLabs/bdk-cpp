/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "nativewrapper.h"

NativeWrapper::NativeWrapper(const Address& address, const DB& db
) : DynamicContract(address, db), ERC20(address, db)
{
  this->registerContractFunctions();
}

NativeWrapper::NativeWrapper(
  const std::string &erc20_name, const std::string &erc20_symbol,
  const uint8_t &erc20_decimals,
  const Address &address, const Address &creator,
  const uint64_t &chainId
) :  DynamicContract("NativeWrapper", address, creator, chainId),
    ERC20("NativeWrapper", erc20_name, erc20_symbol, erc20_decimals,
  0, address, creator, chainId
) {
  this->registerContractFunctions();
}

NativeWrapper::~NativeWrapper() = default;

DBBatch NativeWrapper::dump() const {
  // We need to dump all the data from the parent class as well
  DBBatch batch = ERC20::dump();
  DBBatch baseDump = BaseContract::dump();
  for (const auto& dbItem : baseDump.getPuts()) batch.push_back(dbItem);
  for (const auto& dbItem : baseDump.getDels()) batch.delete_key(dbItem);
  return batch;
}

void NativeWrapper::registerContractFunctions() {
  registerContract();
  this->registerMemberFunctions(
    std::make_tuple("deposit", &NativeWrapper::deposit, FunctionTypes::Payable, this),
    std::make_tuple("withdraw", &NativeWrapper::withdraw, FunctionTypes::Payable, this)
  );
}

void NativeWrapper::deposit() {
  this->mint_(this->getCaller(), this->getValue());
}

void NativeWrapper::withdraw(const uint256_t &value) {
  this->burnValue_(this->getCaller(), value);
  this->sendTokens(this->getCaller(), value);
}

