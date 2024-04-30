/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "nativewrapper.h"

NativeWrapper::NativeWrapper(const Address& address, const DB& db
) : ERC20(address, db)
{
  this->registerContractFunctions();
}

NativeWrapper::NativeWrapper(
  const std::string &erc20_name, const std::string &erc20_symbol,
  const uint8_t &erc20_decimals,
  const Address &address, const Address &creator,
  const uint64_t &chainId
) : ERC20("NativeWrapper", erc20_name, erc20_symbol, erc20_decimals,
  0, address, creator, chainId
) {
  this->registerContractFunctions();
}

NativeWrapper::~NativeWrapper() = default;

DBBatch NativeWrapper::dump() const {
  return BaseContract::dump();
}

void NativeWrapper::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("deposit", &NativeWrapper::deposit, FunctionTypes::Payable, this);
  this->registerMemberFunction("withdraw", &NativeWrapper::withdraw, FunctionTypes::Payable, this);
}

void NativeWrapper::deposit() {
  this->mintValue_(this->getCaller(), this->getValue());
}

void NativeWrapper::withdraw(const uint256_t &value) {
  this->burnValue_(this->getCaller(), value);
  this->sendTokens(this->getCaller(), value);
}

