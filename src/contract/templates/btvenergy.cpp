#include "btvenergy.h"


BTVEnergy::BTVEnergy(const Address &address, const DB &db) :
  DynamicContract(address, db),
  ERC20(address, db),
  Ownable(address, db) {
  this->registerContractFunctions();
}


BTVEnergy::BTVEnergy(const std::string &erc20_name, const std::string &erc20_symbol, const uint8_t &erc20_decimals, const Address &address, const Address &creator, const uint64_t &chainId) :
  DynamicContract("BTVEnergy", address, creator, chainId),
  ERC20("BTVEnergy", erc20_name, erc20_symbol, erc20_decimals, 0, address, creator, chainId),
  Ownable("BTVEnergy", creator, address, creator, chainId) {
#ifdef BUILD_TESTNET
  if (creator != Address(Hex::toBytes("0xc2f2ba5051975004171e6d4781eeda927e884024"))) {
    throw DynamicException("Only the Chain Owner can create this contract");
  }
#endif

  this->registerContractFunctions();
}


void BTVEnergy::mint(const Address &to, const uint256_t& value) {
  this->onlyOwner();
  this->mintValue_(to, value);
}

void BTVEnergy::burn(const Address& from, const uint256_t& value) {
  this->onlyOwner();
  this->burnValue_(from, value);
}

void BTVEnergy::registerContractFunctions() {
  this->registerMemberFunction("mint", &BTVEnergy::mint, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("burn", &BTVEnergy::burn, FunctionTypes::NonPayable, this);
}

DBBatch BTVEnergy::dump() const {
  DBBatch dbBatch = ERC20::dump();
  const auto ownableDump = Ownable::dump();
  for (const auto& dbItem : ownableDump.getPuts()) {
    dbBatch.push_back(dbItem);
  }
  return dbBatch;
}


