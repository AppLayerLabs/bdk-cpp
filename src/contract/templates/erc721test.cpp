#include "erc721test.h"


ERC721Test::ERC721Test(const Address &address, const DB& db)
: ERC721(address, db), tokenIdCounter_(this), maxTokens_(this), totalSupply_(this)
{
  this->tokenIdCounter_ = Utils::bytesToUint64(db.get(std::string("tokenIdCounter_"), this->getDBPrefix()));
  this->maxTokens_ = Utils::bytesToUint64(db.get(std::string("maxTokens_"), this->getDBPrefix()));
  this->totalSupply_ = Utils::bytesToUint64(db.get(std::string("totalSupply_"), this->getDBPrefix()));

  this->tokenIdCounter_.commit();
  this->maxTokens_.commit();
  this->totalSupply_.commit();

  this->registerContractFunctions();

  this->tokenIdCounter_.enableRegister();
  this->maxTokens_.enableRegister();
  this->totalSupply_.enableRegister();
}

ERC721Test::ERC721Test(
  const std::string &erc721name, const std::string &erc721symbol, const uint64_t& maxTokens,
  const Address &address, const Address &creator, const uint64_t &chainId
) : ERC721(erc721name, erc721symbol, address, creator, chainId),
  tokenIdCounter_(this, 0), maxTokens_(this, maxTokens), totalSupply_(this, 0)
{
  this->tokenIdCounter_.commit();
  this->maxTokens_.commit();
  this->totalSupply_.commit();

  this->registerContractFunctions();

  this->tokenIdCounter_.enableRegister();
  this->maxTokens_.enableRegister();
  this->totalSupply_.enableRegister();
}

ERC721Test::~ERC721Test() {}

DBBatch ERC721Test::dump() const {
  DBBatch batch = BaseContract::dump();
  batch.push_back(Utils::stringToBytes("tokenIdCounter_"), Utils::uint64ToBytes(this->tokenIdCounter_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("maxTokens_"), Utils::uint64ToBytes(this->maxTokens_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("totalSupply_"), Utils::uint64ToBytes(this->totalSupply_.get()), this->getDBPrefix());
  return batch;
}

void ERC721Test::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("mint", &ERC721Test::mint, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("burn", &ERC721Test::burn, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("tokenIdCounter", &ERC721Test::tokenIdCounter, FunctionTypes::View, this);
  this->registerMemberFunction("maxTokens", &ERC721Test::maxTokens, FunctionTypes::View, this);
  this->registerMemberFunction("totalSupply", &ERC721Test::totalSupply, FunctionTypes::View, this);
}

void ERC721Test::mint(const Address& to) {
  if (this->tokenIdCounter_ >= this->maxTokens_) throw std::runtime_error("Max tokens reached");
  this->mint_(to, tokenIdCounter_.get());
  ++this->tokenIdCounter_;
  ++this->totalSupply_;
}

void ERC721Test::burn(const uint256_t& tokenId) {
  this->update_(Address(), tokenId, this->getCaller());
  --totalSupply_;
}

