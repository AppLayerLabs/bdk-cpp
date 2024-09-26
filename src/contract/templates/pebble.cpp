#include "pebble.h"

#include "contract/variables/reentrancyguard.h"

Pebble::Pebble(const Address& address, const DB& db)
  : DynamicContract(address, db),
    ERC721(address, db),
    ERC721URIStorage(address, db),
    Ownable(address, db),
    maxSupply_(this),
    tokenIds_(this),
    tokenRarity_(this),
    totalNormal_(this),
    totalGold_(this),
    totalDiamond_(this),
    raritySeed_(this),
    diamondRarity_(this),
    goldRarity_(this) {
  // Load from DB.
  this->maxSupply_ = Utils::bytesToUint256(db.get(std::string("maxSupply_"), this->getDBPrefix()));
  this->tokenIds_ = Utils::bytesToUint256(db.get(std::string("tokenIds_"), this->getDBPrefix()));
  this->totalNormal_ = Utils::fromBigEndian<uint64_t>(db.get(std::string("totalNormal_"), this->getDBPrefix()));
  this->totalGold_ = Utils::fromBigEndian<uint64_t>(db.get(std::string("totalGold_"), this->getDBPrefix()));
  this->totalDiamond_ = Utils::fromBigEndian<uint64_t>(db.get(std::string("totalDiamond_"), this->getDBPrefix()));
  this->raritySeed_ = Utils::bytesToUint256(db.get(std::string("raritySeed_"), this->getDBPrefix()));
  this->diamondRarity_ = Utils::bytesToUint256(db.get(std::string("diamondRarity_"), this->getDBPrefix()));
  this->goldRarity_ = Utils::bytesToUint256(db.get(std::string("goldRarity_"), this->getDBPrefix()));
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("tokenRarity_"))) {
    this->tokenRarity_[Utils::fromBigEndian<uint64_t>(dbEntry.key)] = static_cast<Rarity>(Utils::fromBigEndian<uint8_t>(dbEntry.value));
  }
  this->maxSupply_.commit();
  this->tokenIds_.commit();
  this->tokenRarity_.commit();
  this->totalNormal_.commit();
  this->totalGold_.commit();
  this->totalDiamond_.commit();
  this->raritySeed_.commit();
  this->diamondRarity_.commit();
  this->goldRarity_.commit();
  Pebble::registerContractFunctions();
  this->maxSupply_.enableRegister();
  this->tokenIds_.enableRegister();
  this->tokenRarity_.enableRegister();
  this->totalNormal_.enableRegister();
  this->totalGold_.enableRegister();
  this->totalDiamond_.enableRegister();
  this->raritySeed_.enableRegister();
  this->diamondRarity_.enableRegister();
  this->goldRarity_.enableRegister();
}

Pebble::Pebble(const uint256_t& maxSupply, const Address& address, const Address& creator, const uint64_t& chainId)
  : DynamicContract("Pebble", address, creator, chainId),
    ERC721("Pebble", "Pebble", "PBL", address, creator, chainId),
    ERC721URIStorage("Pebble", "Pebble", "PBL", address, creator, chainId),
    Ownable(creator, address, creator, chainId),
    maxSupply_(this, maxSupply),
    tokenIds_(this, 0),
    tokenRarity_(this),
    totalNormal_(this, 0),
    totalGold_(this, 0),
    totalDiamond_(this, 0),
    raritySeed_(this, 1000000),
    diamondRarity_(this, 1),
    goldRarity_(this, 10) {
  #ifdef BUILD_TESTNET
    if (creator != Address(Hex::toBytes("0xc2f2ba5051975004171e6d4781eeda927e884024"))) {
      throw DynamicException("Only the Chain Owner can create this contract");
    }
  #endif
  this->maxSupply_.commit();
  this->tokenIds_.commit();
  this->tokenRarity_.commit();
  this->totalNormal_.commit();
  this->totalGold_.commit();
  this->totalDiamond_.commit();
  this->raritySeed_.commit();
  this->diamondRarity_.commit();
  this->goldRarity_.commit();
  Pebble::registerContractFunctions();
  this->maxSupply_.enableRegister();
  this->tokenIds_.enableRegister();
  this->tokenRarity_.enableRegister();
  this->totalNormal_.enableRegister();
  this->totalGold_.enableRegister();
  this->totalDiamond_.enableRegister();
  this->raritySeed_.enableRegister();
  this->diamondRarity_.enableRegister();
  this->goldRarity_.enableRegister();
}

DBBatch Pebble::dump() const {
  // We need to dump all the data from the parent classes as well.
  DBBatch batch = ERC721URIStorage::dump();
  const auto ownableDump = Ownable::dump();
  for (const auto& dbItem : ownableDump.getPuts()) {
    batch.push_back(dbItem);
  }
  for (const auto& dbItem : ownableDump.getDels()) {
    batch.delete_key(dbItem);
  }
  // Then, dump the contents of this class.
  batch.push_back(Utils::stringToBytes("maxSupply_"), Utils::uint256ToBytes(this->maxSupply_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("tokenIds_"), Utils::uint256ToBytes(this->tokenIds_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("totalNormal_"), Utils::uint64ToBytes(this->totalNormal_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("totalGold_"), Utils::uint64ToBytes(this->totalGold_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("totalDiamond_"), Utils::uint64ToBytes(this->totalDiamond_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("raritySeed_"), Utils::uint256ToBytes(this->raritySeed_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("diamondRarity_"), Utils::uint256ToBytes(this->diamondRarity_.get()), this->getDBPrefix());
  batch.push_back(Utils::stringToBytes("goldRarity_"), Utils::uint256ToBytes(this->goldRarity_.get()), this->getDBPrefix());
  for (auto it = this->tokenRarity_.cbegin(); it != this->tokenRarity_.cend(); ++it) {
    batch.push_back(Utils::uint256ToBytes(it->first), Utils::uint8ToBytes(static_cast<uint8_t>(it->second)), this->getNewPrefix("tokenRarity_"));
  }
  return batch;
}

Pebble::Rarity Pebble::determineRarity(const uint256_t& randomNumber) const {
  auto value = randomNumber % this->raritySeed_.get();
  //                                         1.000.000
  /**
   * gold: 1: 100 000 (0.01%)
   * Diamond: 1: 1000 000 (0.001%)
   */
  if (value <= this->diamondRarity_.get()) {
    return Rarity::Diamond;
  } else if (value <= this->goldRarity_.get()) {
    return Rarity::Gold;
  } else {
    return Rarity::Normal;
  }
}

std::string Pebble::rarityToString(const Rarity& rarity) const {
  std::string ret = "";
  switch (rarity) {
    case Rarity::Normal: ret = "Normal"; break;
    case Rarity::Gold: ret = "Gold"; break;
    case Rarity::Diamond: ret = "Diamond"; break;
  }
  return ret;
}

Address Pebble::update_(const Address& to, const uint256_t& tokenId, const Address& auth) {
  ERC721URIStorage::update_(to, tokenId, auth);
  return ERC721::update_(to, tokenId, auth);
}

void Pebble::mintNFT(const Address& to, const uint64_t& num) {
  ReentrancyGuard guard(this->reentrancyLock_);
  if (num > 25) throw DynamicException("You can only mint 25 tokens in a single transaction");
  for (uint64_t i = 0; i < num; ++i) {
    if (this->tokenIds_ >= this->maxSupply_) throw DynamicException("Max supply reached");
    this->mint_(to, this->tokenIds_.get());
    Rarity rarity = this->determineRarity(this->getRandom());
    switch (rarity) {
      case Rarity::Normal: ++this->totalNormal_; break;
      case Rarity::Gold: ++this->totalGold_; break;
      case Rarity::Diamond: ++this->totalDiamond_; break;
    }
    this->tokenRarity_[static_cast<uint64_t>(this->tokenIds_.get())] = rarity;
    this->MintedNFT(to, this->tokenIds_.get(), rarity);
    ++this->tokenIds_;
  }
}

std::string Pebble::getTokenRarity(const uint256_t& tokenId) const {
  auto it = this->tokenRarity_.find(static_cast<uint64_t>(tokenId));
  if (it == this->tokenRarity_.cend()) {
    return "Unknown";
  }
  return this->rarityToString(it->second);
}

uint256_t Pebble::totalSupply() const {
  return this->tokenIds_.get();
}

uint64_t Pebble::totalNormal() const {
  return this->totalNormal_.get();
}

uint64_t Pebble::totalGold() const {
  return this->totalGold_.get();
}

uint64_t Pebble::totalDiamond() const {
  return this->totalDiamond_.get();
}

uint256_t Pebble::raritySeed() const {
  return this->raritySeed_.get();
}

uint256_t Pebble::diamondRarity() const {
  return this->diamondRarity_.get();
}

uint256_t Pebble::goldRarity() const {
  return this->goldRarity_.get();
}

void Pebble::setRaritySeed(const uint256_t &seed) {
  this->onlyOwner();
  this->raritySeed_ = seed;
}

void Pebble::setDiamondRarity(const uint256_t &rarity) {
  this->onlyOwner();
  this->diamondRarity_ = rarity;
}

void Pebble::setGoldRarity(const uint256_t &rarity) {
  this->onlyOwner();
  this->goldRarity_ = rarity;
}

std::string Pebble::tokenURI(const uint256_t &tokenId) const {
  auto it = this->tokenRarity_.find(static_cast<uint64_t>(tokenId));
  if (it == this->tokenRarity_.cend()) {
    return "";
  }
  return std::string("https://s3.amazonaws.com/com.applayer.pebble/") + this->rarityToString(it->second) + ".json";
}

uint256_t Pebble::maxSupply() const {
  return this->maxSupply_.get();
}

void Pebble::registerContractFunctions() {
  Pebble::registerContract();
  this->registerMemberFunction("mintNFT", &Pebble::mintNFT, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getTokenRarity", &Pebble::getTokenRarity, FunctionTypes::View, this);
  this->registerMemberFunction("totalSupply", &Pebble::totalSupply, FunctionTypes::View, this);
  this->registerMemberFunction("maxSupply", &Pebble::maxSupply, FunctionTypes::View, this);
  this->registerMemberFunction("tokenURI", &Pebble::tokenURI, FunctionTypes::View, this);
  this->registerMemberFunction("totalNormal", &Pebble::totalNormal, FunctionTypes::View, this);
  this->registerMemberFunction("totalGold", &Pebble::totalGold, FunctionTypes::View, this);
  this->registerMemberFunction("totalDiamond", &Pebble::totalDiamond, FunctionTypes::View, this);
  this->registerMemberFunction("determineRarity", &Pebble::determineRarity, FunctionTypes::View, this);
  this->registerMemberFunction("rarityToString", &Pebble::rarityToString, FunctionTypes::View, this);
  this->registerMemberFunction("raritySeed", &Pebble::raritySeed, FunctionTypes::View, this);
  this->registerMemberFunction("diamondRarity", &Pebble::diamondRarity, FunctionTypes::View, this);
  this->registerMemberFunction("goldRarity", &Pebble::goldRarity, FunctionTypes::View, this);
  this->registerMemberFunction("setRaritySeed", &Pebble::setRaritySeed, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setDiamondRarity", &Pebble::setDiamondRarity, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setGoldRarity", &Pebble::setGoldRarity, FunctionTypes::NonPayable, this);
}
