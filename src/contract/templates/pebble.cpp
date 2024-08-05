#include "pebble.h"

#include "contract/variables/reentrancyguard.h"

Pebble::Pebble(const Address& address, const DB& db)
  : DynamicContract(address, db),
    ERC721(address, db),
    ERC721URIStorage(address, db),
    Ownable(address, db),
    maxSupply_(this),
    tokenIds_(this),
    tokenRarity_(this) {
  // Load from DB.
  this->maxSupply_ = Utils::bytesToUint256(db.get(std::string("maxSupply_"), this->getDBPrefix()));
  this->tokenIds_ = Utils::bytesToUint256(db.get(std::string("tokenIds_"), this->getDBPrefix()));
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("tokenRarity_"))) {
    this->tokenRarity_[Utils::fromBigEndian<uint256_t>(dbEntry.key)] = static_cast<Rarity>(Utils::fromBigEndian<uint8_t>(dbEntry.value));
  }

  this->maxSupply_.commit();
  this->tokenIds_.commit();
  this->tokenRarity_.commit();
  Pebble::registerContractFunctions();
  this->maxSupply_.enableRegister();
  this->tokenIds_.enableRegister();
  this->tokenRarity_.enableRegister();
}

Pebble::Pebble(const uint256_t& maxSupply, const Address& address, const Address& creator, const uint64_t& chainId)
  : DynamicContract("Pebble", address, creator, chainId),
    ERC721("Pebble", "Pebble", "PBL", address, creator, chainId),
    ERC721URIStorage("Pebble", "Pebble", "PBL", address, creator, chainId),
    Ownable(creator, address, creator, chainId),
    maxSupply_(this, maxSupply),
    tokenIds_(this, 0),
    tokenRarity_(this) {
  this->maxSupply_.commit();
  this->tokenIds_.commit();
  this->tokenRarity_.commit();
  Pebble::registerContractFunctions();
  this->maxSupply_.enableRegister();
  this->tokenIds_.enableRegister();
  this->tokenRarity_.enableRegister();
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
  for (auto it = this->tokenRarity_.cbegin(); it != this->tokenRarity_.cend(); ++it) {
    batch.push_back(Utils::uint256ToBytes(it->first), Utils::uint8ToBytes(static_cast<uint8_t>(it->second)), this->getNewPrefix("tokenRarity_"));
  }
  return batch;
}

Pebble::Rarity Pebble::determineRarity_(const uint256_t& randomNumber) {
  auto value = randomNumber % 100000;
  //                                         100.000
  /**
   * gold: 1:100,000 (0.001%)
   * Diamond: 1: 1,000,000 (0.0001%)
   */
  if (value <= 1) {
    return Rarity::Diamond;
  } else if (value <= 11) {
    return Rarity::Gold;
  } else {
    return Rarity::Normal;
  }
}

std::string Pebble::rarityToString_(const Rarity& rarity) {
  std::string ret = "";
  switch (rarity) {
    case Rarity::Normal: ret = "Normal"; break;
    case Rarity::Silver: ret = "Silver"; break;
    case Rarity::Gold: ret = "Gold"; break;
    case Rarity::Diamond: ret = "Diamond"; break;
  }
  return ret;
}

Address Pebble::update_(const Address& to, const uint256_t& tokenId, const Address& auth) {
  ERC721URIStorage::update_(to, tokenId, auth);
  return ERC721::update_(to, tokenId, auth);
}

void Pebble::mintNFT(const Address& to) {
  ReentrancyGuard guard(this->reentrancyLock_);
  if (this->tokenIds_ >= this->maxSupply_) throw DynamicException("Max supply reached");
  this->mint_(to, this->tokenIds_.get());
  Rarity rarity = this->determineRarity_(this->getRandom());
  this->tokenRarity_[this->tokenIds_.get()] = rarity;
  std::string tokenURI = std::string("https://s3.amazonaws.com/com.applayer.pebble/") + this->rarityToString_(rarity) + ".json";
  this->setTokenURI_(this->tokenIds_.get(), tokenURI);
  this->MintedNFT(to, this->tokenIds_.get(), rarity);
  ++this->tokenIds_;
}

std::string Pebble::getTokenRarity(const uint256_t& tokenId) const {
  auto it = this->tokenRarity_.find(tokenId);
  if (it == this->tokenRarity_.cend()) {
    return "Unknown";
  }
  return this->rarityToString_(it->second);
}

uint256_t Pebble::totalSupply() const {
  return this->tokenIds_.get();
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
}
