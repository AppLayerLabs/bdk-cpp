#include "dexv2factory.h"
#include "dexv2pair.h"

DEXV2Factory::DEXV2Factory(
  ContractManagerInterface &interface, const Address &address, const std::unique_ptr<DB> &db
) : DynamicContract(interface, address, db), feeTo_(this), feeToSetter_(this),
  allPairs_(this), getPair_(this)
{
  // Load from DB constructor
  this->feeTo_ = Address(db->get(std::string("feeTo_"), this->getDBPrefix()));
  this->feeToSetter_ = Address(db->get(std::string("feeToSetter_"), this->getDBPrefix()));
  std::vector<DBEntry> allPairs = db->getBatch(this->getNewPrefix("allPairs_"));
  for (const auto& dbEntry : allPairs) this->allPairs_.push_back(Address(dbEntry.value));
  std::vector<DBEntry> getPairs = db->getBatch(this->getNewPrefix("getPair_"));
  for (const auto& dbEntry : getPairs) {
    BytesArrView valueView(dbEntry.value);
    this->getPair_[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = Address(valueView.subspan(20));
  }
  this->registerContractFunctions();
  this->feeTo_.commit();
  this->feeToSetter_.commit();
  this->allPairs_.commit();
  this->getPair_.commit();
}

DEXV2Factory::DEXV2Factory(
  const Address& feeToSetter,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "DEXV2Factory", address, creator, chainId, db),
  feeTo_(this), feeToSetter_(this), allPairs_(this), getPair_(this)
{
  // Create new constructor
  this->feeToSetter_ = feeToSetter;
  this->feeToSetter_.commit();
  this->registerContractFunctions();
}

DEXV2Factory::~DEXV2Factory() {
  DBBatch batchOperations;
  batchOperations.push_back(Utils::stringToBytes("feeTo_"), this->feeTo_.get().view_const(), this->getDBPrefix());
  batchOperations.push_back(Utils::stringToBytes("feeToSetter_"), this->feeToSetter_.get().view_const(), this->getDBPrefix());
  uint32_t index = 0;
  for (const auto& address : this->allPairs_.get()) batchOperations.push_back(
    Utils::uint32ToBytes(index), address.view_const(), this->getNewPrefix("allPairs_")
  );
  for (auto tokenA = this->getPair_.cbegin(); tokenA != this->getPair_.cend(); tokenA++) {
    for (auto tokenB = tokenA->second.cbegin(); tokenB != tokenA->second.cend(); tokenB++) {
      const auto& key = tokenA->first.get();
      Bytes value = tokenB->first.asBytes();
      Utils::appendBytes(value, tokenB->second.asBytes());
      batchOperations.push_back(key, value, this->getNewPrefix("getPair_"));
    }
  }
  this->db->putBatch(batchOperations);
}

void DEXV2Factory::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("feeTo", &DEXV2Factory::feeTo, this);
  this->registerMemberFunction("feeToSetter", &DEXV2Factory::feeToSetter, this);
  this->registerMemberFunction("allPairs", &DEXV2Factory::allPairs, this);
  this->registerMemberFunction("allPairsLength", &DEXV2Factory::allPairsLength, this);
  this->registerMemberFunction("getPair", &DEXV2Factory::getPair, this);
  this->registerMemberFunction("getPairByIndex", &DEXV2Factory::getPairByIndex, this);
  this->registerMemberFunction("createPair", &DEXV2Factory::createPair, this);
  this->registerMemberFunction("setFeeTo", &DEXV2Factory::setFeeTo, this);
  this->registerMemberFunction("setFeeToSetter", &DEXV2Factory::setFeeToSetter, this);
}

Address DEXV2Factory::feeTo() const { return this->feeTo_.get(); }

Address DEXV2Factory::feeToSetter() const { return this->feeToSetter_.get(); }

std::vector<Address> DEXV2Factory::allPairs() const { return this->allPairs_.get(); }

uint64_t DEXV2Factory::allPairsLength() const { return this->allPairs_.size(); }

Address DEXV2Factory::getPair(const Address& tokenA, const Address& tokenB) const {
  auto it = this->getPair_.find(tokenA);
  if (it != this->getPair_.end()) {
    auto itt = it->second.find(tokenB);
    if (itt != it->second.end()) return itt->second;
  }
  return Address();
}

Address DEXV2Factory::getPairByIndex(const uint64_t& index) const {
  if (index >= this->allPairs_.size()) return Address();
  return this->allPairs_[index];
}

Address DEXV2Factory::createPair(const Address& tokenA, const Address& tokenB) {
  if (tokenA == tokenB) throw std::runtime_error("DEXV2Factory::createPair: IDENTICAL_ADDRESSES");
  auto& token0 = (tokenA < tokenB) ? tokenA : tokenB;
  auto& token1 = (tokenA < tokenB) ? tokenB : tokenA;
  if (token0 == Address()) throw std::runtime_error("DEXV2Factory::createPair: ZERO_ADDRESS");
  if (this->getPair(token0, token1) != Address()) throw std::runtime_error("DEXV2Factory::createPair: PAIR_EXISTS");
  Utils::safePrint("DEXV2Factory: creating pair...");
  auto pair = this->callCreateContract<DEXV2Pair>(0, 0, 0);
  Utils::safePrint("DEXV2Factory: pair created...");
  this->callContractFunction(pair, &DEXV2Pair::initialize, token0, token1);
  getPair_[token0][token1] = pair;
  getPair_[token1][token0] = pair;
  allPairs_.push_back(pair);
  return pair;
}

void DEXV2Factory::setFeeTo(const Address& feeTo) { this->feeTo_ = feeTo; }

void DEXV2Factory::setFeeToSetter(const Address& feeToSetter) { this->feeToSetter_ = feeToSetter; }

