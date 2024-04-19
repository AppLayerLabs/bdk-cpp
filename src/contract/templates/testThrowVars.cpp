#include "testThrowVars.h"

TestThrowVars::TestThrowVars(
  const std::string& var1, const std::string& var2, const std::string& var3,
  ContractManagerInterface &interface, const Address& address,
  const Address& creator, const uint64_t& chainId, DB& db)
  : DynamicContract(interface, "TestThrowVars", address, creator, chainId, db),
    var1_(this),
    var2_(this),
    var3_(this)
{
  this->var1_ = "var1";
  this->var1_ = "var2";
  this->var1_ = "var3";

  this->var1_.commit();
  this->var2_.commit();
  this->var3_.commit();

  this->registerContractFunctions();

  throw DynamicException("Throw from create ctor");

  this->var1_.enableRegister();
  this->var2_.enableRegister();
  this->var3_.enableRegister();
}

TestThrowVars::TestThrowVars(
  ContractManagerInterface &interface, const Address& address, DB& db)
  : DynamicContract(interface, address, db),
    var1_(this),
    var2_(this),
    var3_(this)
{
  // Do nothing
}

TestThrowVars::~TestThrowVars()
{
  // Do nothing
}

void TestThrowVars::registerContractFunctions()
{
  registerContract();
}

DBBatch TestThrowVars::dump() const
{
  DBBatch dbBatch;

  return dbBatch;
}

