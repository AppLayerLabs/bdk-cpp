#include "btvplayer.h"
#include "erc20.h"
#include "btvproposals.h"

BTVPlayer::BTVPlayer(const Address &address, const DB &db) :
  DynamicContract(address, db),
  ERC721(address, db),
  Ownable(address, db),
  proposalContract_(this),
  energyContract_(this),
  worldContract_(this),
  playerNames_(this),
  playerToTokens_(this),
  energyBalance_(this),
  tokenCounter_(this) {

  this->proposalContract_ = Address(db.get(std::string("proposalContract_"), this->getDBPrefix()));
  this->energyContract_ = Address(db.get(std::string("energyContract_"), this->getDBPrefix()));
  this->worldContract_ = Address(db.get(std::string("worldContract_"), this->getDBPrefix()));
  for (const auto &dbEntry : db.getBatch(this->getNewPrefix("playerNames_"))) {
    this->playerNames_[StrConv::bytesToString(dbEntry.key)] = UintConv::bytesToUint64(dbEntry.value);
  }
  for (const auto &dbEntry : db.getBatch(this->getNewPrefix("playerToTokens_"))) {
    this->playerToTokens_[UintConv::bytesToUint64(dbEntry.key)] = StrConv::bytesToString(dbEntry.value);
  }
  for (const auto &dbEntry : db.getBatch(this->getNewPrefix("energyBalance_"))) {
    this->energyBalance_[UintConv::bytesToUint64(dbEntry.key)] = UintConv::bytesToUint256(dbEntry.value);
  }
  this->tokenCounter_ = UintConv::bytesToUint256(db.get(std::string("tokenCounter_"), this->getDBPrefix()));
  this->energyContract_.commit();
  this->proposalContract_.commit();
  this->worldContract_.commit();
  this->playerNames_.commit();
  this->playerToTokens_.commit();
  this->energyBalance_.commit();
  this->tokenCounter_.commit();

  this->registerContractFunctions();

  this->energyContract_.enableRegister();
  this->proposalContract_.enableRegister();
  this->worldContract_.enableRegister();
  this->playerNames_.enableRegister();
  this->playerToTokens_.enableRegister();
  this->energyBalance_.enableRegister();
  this->tokenCounter_.enableRegister();
}

BTVPlayer::BTVPlayer(const std::string &erc721_name, const std::string &erc721_symbol, const Address &address, const Address &creator, const uint64_t &chainId) :
  DynamicContract("BTVPlayer", address, creator, chainId),
  ERC721("BTVPlayer", erc721_name, erc721_symbol, address, creator, chainId),
  Ownable("BTVEnergy", creator, address, creator, chainId),
  proposalContract_(this),
  energyContract_(this),
  worldContract_(this),
  playerNames_(this),
  playerToTokens_(this),
  energyBalance_(this),
  tokenCounter_(this) {
  #ifdef BUILD_TESTNET
    if (creator != Address(Hex::toBytes("0xc2f2ba5051975004171e6d4781eeda927e884024"))) {
      throw DynamicException("Only the Chain Owner can create this contract");
    }
  #endif
  this->proposalContract_.commit();
  this->energyContract_.commit();
  this->worldContract_.commit();
  this->playerNames_.commit();
  this->playerToTokens_.commit();
  this->energyBalance_.commit();
  this->tokenCounter_.commit();
  this->registerContractFunctions();
  this->proposalContract_.enableRegister();
  this->energyContract_.enableRegister();
  this->worldContract_.enableRegister();
  this->playerNames_.enableRegister();
  this->playerToTokens_.enableRegister();
  this->energyBalance_.enableRegister();
  this->tokenCounter_.enableRegister();
}

std::string BTVPlayer::getPlayerName(const uint64_t &tokenId) const {
  auto it = this->playerToTokens_.find(tokenId);
  if (it == this->playerToTokens_.cend()) {
    throw DynamicException("Player does not exist");
  }
  return it->second;
}

bool BTVPlayer::playerExists(const std::string &playerName) const {
  return this->playerNames_.contains(playerName);
}

void BTVPlayer::mintPlayer(const std::string &name, const Address& to) {
  if (this->playerNames_.contains(name)) {
    throw DynamicException("Player already exists");
  }
  uint256_t tokenId = this->totalSupply();
  this->mint_(to, tokenId);
  this->playerNames_[name] = static_cast<uint64_t>(tokenId);
  this->playerToTokens_[static_cast<uint64_t>(tokenId)] = name;
  if (this->worldContract_.get()) {
    this->tokenApprovals_[static_cast<uint64_t>(tokenId)] = this->worldContract_.get();
  }
  ++this->tokenCounter_;
}

void BTVPlayer::setProposalContract(const Address &proposalContract) {
  this->onlyOwner();
  this->proposalContract_ = proposalContract;
}

Address BTVPlayer::getProposalContract() const {
  return this->proposalContract_.get();
}

void BTVPlayer::setEnergyContract(const Address &energyContract) {
  this->onlyOwner();
  this->energyContract_ = energyContract;
}

Address BTVPlayer::getEnergyContract() const {
  return this->energyContract_.get();
}

void BTVPlayer::setWorldContract(const Address &worldContract) {
  this->onlyOwner();
  this->worldContract_ = worldContract;
}

Address BTVPlayer::getWorldContract() const {
  return this->worldContract_.get();
}

uint256_t BTVPlayer::totalSupply() const {
  return this->tokenCounter_.get();
}

uint256_t BTVPlayer::getPlayerEnergy(const uint64_t &tokenId) const {
  auto it = this->energyBalance_.find(tokenId);
  if (it == this->energyBalance_.cend()) {
    return 0;
  }
  return it->second;
}

void BTVPlayer::addPlayerEnergy(const uint64_t &tokenId, const uint256_t &energy) {
  this->callContractFunction(this->energyContract_.get(), &ERC20::transferFrom, this->getCaller(), this->getContractAddress(), energy);
  this->energyBalance_[tokenId] += energy;
}

void BTVPlayer::takePlayerEnergy(const uint64_t &tokenId, const uint256_t &energy) {
  // Only the owner of the token OR the world contract can take energy from a player
  if (this->getCaller() != this->owner() && this->getCaller() != this->ownerOf(tokenId)) {
    throw DynamicException("BTVPlayer::takePlayerEnergy: Caller is not the owner of the token or the world contract");
  }
  auto it = this->energyBalance_.find(tokenId);
  if (it == this->energyBalance_.cend()) {
    throw DynamicException("BTVPlayer::takePlayerEnergy: Player does not exist");
  }
  if (it->second < energy) {
    throw DynamicException("BTVPlayer::takePlayerEnergy: Not enough energy");
  }
  this->callContractFunction(this->energyContract_.get(), &ERC20::transfer, this->getContractAddress(), it->second);
  it->second -= energy;
}

void BTVPlayer::createProposal(const uint64_t &tokenId, const std::string &title, const std::string &description) {
  if (this->getCaller() != this->ownerOf(tokenId)) {
    throw DynamicException("BTVPlayer::createProposal: caller is not the owner of the token");
  }
  // Check if the token has enough energy to create a proposal
  auto it = this->energyBalance_.find(tokenId);
  auto requiredEnergy = this->callContractViewFunction(this->proposalContract_.get(), &BTVProposals::getProposalPrice);
  if (it == this->energyBalance_.cend() || it->second < requiredEnergy) {
    throw DynamicException("BTVPlayer::createProposal: not enough energy to create a proposal");
  }
  this->callContractFunction(this->proposalContract_.get(), &BTVProposals::createProposal, title, description);
  this->energyBalance_[tokenId] -= requiredEnergy;
}

void BTVPlayer::voteOnProposal(const uint64_t &tokenId, const uint64_t &proposalId, const uint256_t &energy) {

  if (this->getCaller() != this->ownerOf(tokenId)) {
    throw DynamicException("BTVPlayer::voteOnProposal: caller is not the owner of the token");
  }
  auto it = this->energyBalance_.find(tokenId);
  if (it == this->energyBalance_.cend() || it->second < energy) {
    throw DynamicException("BTVPlayer::voteOnProposal: not enough energy to vote");
  }
  this->callContractFunction(this->proposalContract_.get(), &BTVProposals::voteOnProposal, tokenId, proposalId, energy);
  it->second -= energy;
}

void BTVPlayer::removeVote(const uint64_t& tokenId, const uint64_t& proposalId, const uint256_t& energy) {
  if (this->getCaller() != this->ownerOf(tokenId)) {
    throw DynamicException("BTVPlayer::removeVote: caller is not the owner of the token");
  }
  auto it = this->energyBalance_.find(tokenId);
  if (it == this->energyBalance_.cend()) {
    throw DynamicException("BTVPlayer::removeVote: player does not exist");
  }
  this->callContractFunction(this->proposalContract_.get(), &BTVProposals::removeVote, tokenId, proposalId, energy);
  it->second += energy;
}

void BTVPlayer::approveProposalSpend() {
  this->onlyOwner();
  this->callContractFunction(energyContract_.get(), &ERC20::approve, proposalContract_.get(), std::numeric_limits<uint256_t>::max());
}

void BTVPlayer::registerContractFunctions() {
  this->registerMemberFunction("getPlayerName", &BTVPlayer::getPlayerName, FunctionTypes::View, this);
  this->registerMemberFunction("playerExists", &BTVPlayer::playerExists, FunctionTypes::View, this);
  this->registerMemberFunction("mintPlayer", &BTVPlayer::mintPlayer, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setProposalContract", &BTVPlayer::setProposalContract, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getProposalContract", &BTVPlayer::getProposalContract, FunctionTypes::View, this);
  this->registerMemberFunction("setEnergyContract", &BTVPlayer::setEnergyContract, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getEnergyContract", &BTVPlayer::getEnergyContract, FunctionTypes::View, this);
  this->registerMemberFunction("setWorldContract", &BTVPlayer::setWorldContract, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getWorldContract", &BTVPlayer::getWorldContract, FunctionTypes::View, this);
  this->registerMemberFunction("totalSupply", &BTVPlayer::totalSupply, FunctionTypes::View, this);
  this->registerMemberFunction("getPlayerEnergy", &BTVPlayer::getPlayerEnergy, FunctionTypes::View, this);
  this->registerMemberFunction("addPlayerEnergy", &BTVPlayer::addPlayerEnergy, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("takePlayerEnergy", &BTVPlayer::takePlayerEnergy, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("createProposal", &BTVPlayer::createProposal, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("voteOnProposal", &BTVPlayer::voteOnProposal, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("removeVote", &BTVPlayer::removeVote, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("approveProposalSpend", &BTVPlayer::approveProposalSpend, FunctionTypes::NonPayable, this);
}

DBBatch BTVPlayer::dump() const {
  DBBatch dbBatch = ERC721::dump();
  const auto ownableDump = Ownable::dump();
  for (const auto& dbItem : ownableDump.getPuts()) {
    dbBatch.push_back(dbItem);
  }
  dbBatch.push_back(StrConv::stringToBytes("proposalContract_"), this->proposalContract_.get(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("energyContract_"), this->energyContract_.get(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("worldContract_"), this->worldContract_.get(), this->getDBPrefix());
  for (auto it = this->playerNames_.cbegin(); it != this->playerNames_.cend(); ++it) {
    dbBatch.push_back(StrConv::stringToBytes(it->first), UintConv::uint64ToBytes(it->second), this->getNewPrefix("playerNames_"));
  }
  for (auto it = this->playerToTokens_.cbegin(); it != this->playerToTokens_.cend(); ++it) {
    dbBatch.push_back(UintConv::uint64ToBytes(it->first), StrConv::stringToBytes(it->second), this->getNewPrefix("playerToTokens_"));
  }
  for (auto it = this->energyBalance_.cbegin(); it != this->energyBalance_.cend(); ++it) {
    dbBatch.push_back(UintConv::uint64ToBytes(it->first), UintConv::uint256ToBytes(it->second), this->getNewPrefix("energyBalance_"));
  }
  dbBatch.push_back(StrConv::stringToBytes("tokenCounter_"), UintConv::uint256ToBytes(this->tokenCounter_.get()), this->getDBPrefix());
  return dbBatch;
}



