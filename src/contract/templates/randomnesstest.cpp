/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "randomnesstest.h"

#include "../../utils/uintconv.h"
#include "../../utils/strconv.h"

RandomnessTest::RandomnessTest(const Address& address,
  const Address& creator, const uint64_t& chainId
) : DynamicContract("RandomnessTest", address, creator, chainId) {
  this->randomValue_.commit();
  registerContractFunctions();
  this->randomValue_.enableRegister();
}

RandomnessTest::RandomnessTest(const Address& address, const DB& db)
  : DynamicContract(address, db) {
  this->randomValue_ = UintConv::bytesToUint256(db.get(std::string("randomValue_"), this->getDBPrefix()));
  this->randomValue_.commit();
  registerContractFunctions();
  this->randomValue_.enableRegister();
}

RandomnessTest::~RandomnessTest() {}

void RandomnessTest::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("setRandom", &RandomnessTest::setRandom, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getRandom", &RandomnessTest::getRandom, FunctionTypes::View, this);
}

uint256_t RandomnessTest::setRandom() {
  randomValue_ = DynamicContract::getRandom();
  return randomValue_.get();
}

uint256_t RandomnessTest::getRandom() const {
  return randomValue_.get();
}

DBBatch RandomnessTest::dump() const {
  DBBatch dbBatch = BaseContract::dump();
  dbBatch.push_back(StrConv::stringToBytes("randomValue_"), UintConv::uint256ToBytes(randomValue_.get()), this->getDBPrefix());
  return dbBatch;
}

