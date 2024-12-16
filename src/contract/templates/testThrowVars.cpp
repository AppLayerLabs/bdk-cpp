#include "testThrowVars.h"

TestThrowVars::TestThrowVars(
  const std::string& var1, const std::string& var2, const std::string& var3,
  const Address& address,
  const Address& creator, const uint64_t& chainId
) : DynamicContract("TestThrowVars", address, creator, chainId),
  var1_(this), var2_(this), var3_(this)
{
  this->var1_ = "var1";
  this->var2_ = "var2";
  this->var3_ = "var3";

  this->var1_.commit();
  this->var2_.commit();
  this->var3_.commit();

  this->registerContractFunctions();

  throw DynamicException("Throw from create ctor");

  this->var1_.enableRegister();
  this->var2_.enableRegister();
  this->var3_.enableRegister();
}

/*
TestThrowVars::TestThrowVars(const Address& address, const DB& db
) : DynamicContract(address, db
), var1_(this), var2_(this), var3_(this) {
  // Do nothing
}
*/

TestThrowVars::~TestThrowVars() = default;

//DBBatch TestThrowVars::dump() const { return BaseContract::dump(); }

void TestThrowVars::registerContractFunctions() { registerContract(); }

