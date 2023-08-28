#include "nativewrapper.h"

// Default Constructor when loading contract from DB.
NativeWrapper::NativeWrapper(
  ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db
) : ERC20(interface, address, db)
{
  this->registerContractFunctions();
}

NativeWrapper::NativeWrapper(
  const std::string &erc20_name, const std::string &erc20_symbol,
  const uint8_t &erc20_decimals,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator,
  const uint64_t &chainId,const std::unique_ptr<DB> &db
) : ERC20("NativeWrapper", erc20_name, erc20_symbol, erc20_decimals,
  0, interface, address, creator, chainId, db
) {
  this->registerContractFunctions();
}

NativeWrapper::~NativeWrapper() {}

void NativeWrapper::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("deposit", &NativeWrapper::deposit, this);
  this->registerMemberFunction("withdraw", &NativeWrapper::withdraw, this);
}

void NativeWrapper::deposit() {
  this->mintValue_(this->getCaller(), this->getValue());
}

void NativeWrapper::withdraw(const uint256_t &_value) {
  this->burnValue_(this->getCaller(), _value);
  this->sendTokens(this->getCaller(), _value);
}

