/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dexv2factory.h"
#include "dexv2pair.h"

#include "../../../utils/uintconv.h"
#include "../../../utils/strconv.h"

DEXV2Factory::DEXV2Factory(const Address &address, const DB& db
) : DynamicContract(address, db), feeTo_(this), feeToSetter_(this),
  allPairs_(this), getPair_(this)
{
  this->feeTo_ = Address(db.get(std::string("feeTo_"), this->getDBPrefix()));
  this->feeToSetter_ = Address(db.get(std::string("feeToSetter_"), this->getDBPrefix()));
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("allPairs_"))) {
    this->allPairs_.push_back(Address(dbEntry.value));
  }
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("getPair_"))) {
    View<Bytes> valueView(dbEntry.value);
    this->getPair_[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = Address(valueView.subspan(20));
  }

  this->feeTo_.commit();
  this->feeToSetter_.commit();
  this->allPairs_.commit();
  this->getPair_.commit();

  this->registerContractFunctions();

  this->feeTo_.enableRegister();
  this->feeToSetter_.enableRegister();
  this->allPairs_.enableRegister();
  this->getPair_.enableRegister();
}

DEXV2Factory::DEXV2Factory(
  const Address& feeToSetter,
  const Address &address, const Address &creator, const uint64_t &chainId
) : DynamicContract("DEXV2Factory", address, creator, chainId),
  feeTo_(this), feeToSetter_(this), allPairs_(this), getPair_(this)
{
  this->feeToSetter_ = feeToSetter;

  this->feeTo_.commit();
  this->feeToSetter_.commit();
  this->allPairs_.commit();
  this->getPair_.commit();

  this->registerContractFunctions();

  this->feeTo_.enableRegister();
  this->feeToSetter_.enableRegister();
  this->allPairs_.enableRegister();
  this->getPair_.enableRegister();
}

DEXV2Factory::~DEXV2Factory() {};

void DEXV2Factory::registerContractFunctions() {
  registerContract();
  this->registerMemberFunctions(
    std::make_tuple("feeTo", &DEXV2Factory::feeTo, FunctionTypes::View, this),
    std::make_tuple("feeToSetter", &DEXV2Factory::feeToSetter, FunctionTypes::View, this),
    std::make_tuple("allPairs", &DEXV2Factory::allPairs, FunctionTypes::View, this),
    std::make_tuple("allPairsLength", &DEXV2Factory::allPairsLength, FunctionTypes::View, this),
    std::make_tuple("getPair", &DEXV2Factory::getPair, FunctionTypes::View, this),
    std::make_tuple("getPairByIndex", &DEXV2Factory::getPairByIndex, FunctionTypes::View, this),
    std::make_tuple("createPair", &DEXV2Factory::createPair, FunctionTypes::NonPayable, this),
    std::make_tuple("setFeeTo", &DEXV2Factory::setFeeTo, FunctionTypes::NonPayable, this),
    std::make_tuple("setFeeToSetter", &DEXV2Factory::setFeeToSetter, FunctionTypes::NonPayable, this)
  );
}

Address DEXV2Factory::feeTo() const { return this->feeTo_.get(); }

Address DEXV2Factory::feeToSetter() const { return this->feeToSetter_.get(); }

std::vector<Address> DEXV2Factory::allPairs() const { return this->allPairs_.get(); }

uint64_t DEXV2Factory::allPairsLength() const { return this->allPairs_.size(); }

Address DEXV2Factory::getPair(const Address& tokenA, const Address& tokenB) const {
  if (auto it = this->getPair_.find(tokenA); it != this->getPair_.cend()) {
    if (auto itt = it->second.find(tokenB); itt != it->second.cend()) return itt->second;
  }
  return Address();
}

Address DEXV2Factory::getPairByIndex(const uint64_t& index) const {
  if (index >= this->allPairs_.size()) return Address();
  return this->allPairs_[index];
}

Address DEXV2Factory::createPair(const Address& tokenA, const Address& tokenB) {
  if (tokenA == tokenB) throw DynamicException("DEXV2Factory::createPair: IDENTICAL_ADDRESSES");
  auto& token0 = (tokenA < tokenB) ? tokenA : tokenB;
  auto& token1 = (tokenA < tokenB) ? tokenB : tokenA;
  if (token0 == Address()) throw DynamicException("DEXV2Factory::createPair: ZERO_ADDRESS");
  if (this->getPair(token0, token1) != Address()) throw DynamicException("DEXV2Factory::createPair: PAIR_EXISTS");
  auto pair = this->callCreateContract<DEXV2Pair>();
  this->callContractFunction(pair, &DEXV2Pair::initialize, token0, token1);
  getPair_[token0][token1] = pair;
  getPair_[token1][token0] = pair;
  allPairs_.push_back(pair);
  return pair;
}

void DEXV2Factory::setFeeTo(const Address& feeTo) { this->feeTo_ = feeTo; }

void DEXV2Factory::setFeeToSetter(const Address& feeToSetter) { this->feeToSetter_ = feeToSetter; }

DBBatch DEXV2Factory::dump() const {
  DBBatch dbBatch = BaseContract::dump();
  uint32_t i = 0;

  dbBatch.push_back(StrConv::stringToBytes("feeTo_"), this->feeTo_.get().view(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("feeToSetter_"), this->feeToSetter_.get().view(), this->getDBPrefix());

  for (const auto& address : this->allPairs_.get()) {
    dbBatch.push_back(UintConv::uint32ToBytes(i), address.view(), this->getNewPrefix("allPairs_"));
    i++;
  }

  for (auto tokenA = this->getPair_.cbegin(); tokenA != this->getPair_.cend(); tokenA++) {
    for (auto tokenB = tokenA->second.cbegin(); tokenB != tokenA->second.cend(); tokenB++) {
      const auto& key = tokenA->first;
      Bytes value = tokenB->first.asBytes();
      Utils::appendBytes(value, tokenB->second.asBytes());
      dbBatch.push_back(key, value, this->getNewPrefix("getPair_"));
    }
  }

  return dbBatch;
}

