#include "btvproposals.h"
#include "btvplayer.h"
#include "standards/erc20.h"


BTVProposals::BTVProposals(const Address& address, const DB& db) :
  DynamicContract(address, db),
  Ownable(address, db),
  proposalCount_(this),
  activeProposals_(this),
  completedProposals_(this),
  playerContract_(this),
  energyContract_(this),
  proposalVotes_(this),
  proposalPrice_(this) {


  this->proposalCount_ = UintConv::bytesToUint64(db.get(std::string("proposalCount_"), this->getDBPrefix()));

  // A proposal is stored in the DB in the following format:
  // Key = ID (8 bytes)
  // Value = Energy (32 bytes) + Title (variable length) + Description (variable length)
  // Value = 32 bytes + 8 Bytes (Title Size) + Title + Description
  for (const auto &dbEntry : db.getBatch(this->getNewPrefix("activeProposals_"))) {
    uint64_t id = UintConv::bytesToUint64(dbEntry.key);
    View<Bytes> value(dbEntry.value);
    uint256_t energy = UintConv::bytesToUint256(value.subspan(0, 32));
    uint64_t titleSize = UintConv::bytesToUint64(value.subspan(32, 8));
    std::string title = StrConv::bytesToString(value.subspan(40, titleSize));
    std::string description = StrConv::bytesToString(value.subspan(40 + titleSize));
    this->activeProposals_[id] = std::make_tuple(energy, title, description);
  }

  // Same for completed proposals
  for (const auto &dbEntry : db.getBatch(this->getNewPrefix("completedProposals_"))) {
    uint64_t id = UintConv::bytesToUint64(dbEntry.key);
    View<Bytes> value(dbEntry.value);
    uint256_t energy = UintConv::bytesToUint256(value.subspan(0, 32));
    uint64_t titleSize = UintConv::bytesToUint64(value.subspan(32, 8));
    std::string title = StrConv::bytesToString(value.subspan(40, titleSize));
    std::string description = StrConv::bytesToString(value.subspan(40 + titleSize));
    this->completedProposals_[id] = std::make_tuple(energy, title, description);
  }

  this->playerContract_ = Address(db.get(std::string("playerContract_"), this->getDBPrefix()));
  this->energyContract_ = Address(db.get(std::string("energyContract_"), this->getDBPrefix()));

  // Here is different
  // Key = Proposal ID (8 bytes) + Token ID (8 bytes)
  // Value = Energy (32 bytes)
  for (const auto &dbEntry : db.getBatch(this->getNewPrefix("proposalVotes_"))) {
    View<Bytes> key(dbEntry.key);
    uint64_t proposalId = UintConv::bytesToUint64(key.subspan(0, 8));
    uint64_t tokenId = UintConv::bytesToUint64(key.subspan(8, 8));
    uint256_t energy = UintConv::bytesToUint256(dbEntry.value);
    this->proposalVotes_[proposalId][tokenId] = energy;
  }

  this->proposalPrice_ = UintConv::bytesToUint256(db.get(std::string("proposalPrice_"), this->getDBPrefix()));

  this->proposalCount_.commit();
  this->activeProposals_.commit();
  this->completedProposals_.commit();
  this->playerContract_.commit();
  this->energyContract_.commit();
  this->proposalVotes_.commit();
  this->proposalPrice_.commit();
  this->registerContractFunctions();
  this->proposalCount_.enableRegister();
  this->activeProposals_.enableRegister();
  this->completedProposals_.enableRegister();
  this->playerContract_.enableRegister();
  this->energyContract_.enableRegister();
  this->proposalVotes_.enableRegister();
  this->proposalPrice_.enableRegister();
}

BTVProposals::BTVProposals(const Address &address, const Address &creator, const uint64_t &chainId) :
  DynamicContract("BTVProposals", address, creator, chainId),
  Ownable("BTVProposals", creator, address, creator, chainId),
  proposalCount_(this),
  activeProposals_(this),
  completedProposals_(this),
  playerContract_(this),
  energyContract_(this),
  proposalVotes_(this),
  proposalPrice_(this) {

#ifdef BUILD_TESTNET
  if (creator != Address(Hex::toBytes("0xc2f2ba5051975004171e6d4781eeda927e884024"))) {
    throw DynamicException("Only the Chain Owner can create this contract");
  }
#endif

  this->proposalCount_.commit();
  this->activeProposals_.commit();
  this->completedProposals_.commit();
  this->playerContract_.commit();
  this->energyContract_.commit();
  this->proposalVotes_.commit();
  this->proposalPrice_.commit();
  this->registerContractFunctions();
  this->proposalCount_.enableRegister();
  this->activeProposals_.enableRegister();
  this->completedProposals_.enableRegister();
  this->playerContract_.enableRegister();
  this->energyContract_.enableRegister();
  this->proposalVotes_.enableRegister();
  this->proposalPrice_.enableRegister();
}

void BTVProposals::registerContractFunctions() {
  this->registerMemberFunction("createProposal", &BTVProposals::createProposal, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("voteOnProposal", &BTVProposals::voteOnProposal, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("removeVote", &BTVProposals::removeVote, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("completeProposal", &BTVProposals::completeProposal, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setProposalPrice", &BTVProposals::setProposalPrice, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setPlayerContract", &BTVProposals::setPlayerContract, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setEnergyContract", &BTVProposals::setEnergyContract, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getActiveProposals", &BTVProposals::getActiveProposals, FunctionTypes::View, this);
  this->registerMemberFunction("getCompletedProposals", &BTVProposals::getCompletedProposals, FunctionTypes::View, this);
  this->registerMemberFunction("getProposalVotes", &BTVProposals::getProposalVotes, FunctionTypes::View, this);
  this->registerMemberFunction("getProposalPrice", &BTVProposals::getProposalPrice, FunctionTypes::View, this);
  this->registerMemberFunction("getProposalEnergy", &BTVProposals::getProposalEnergy, FunctionTypes::View, this);
  this->registerMemberFunction("getProposalCount", &BTVProposals::getProposalCount, FunctionTypes::View, this);
}

void BTVProposals::onlyPlayer() const {
  if (this->getCaller() != this->playerContract_.get()) {
    throw DynamicException("BTVProposals: caller is not the player contract");
  }
}


void BTVProposals::createProposal(const std::string& title, const std::string& description) {
  this->onlyPlayer();
  if (this->proposalPrice_.get() != 0) {
    this->callContractFunction(this->energyContract_.get(), &ERC20::transferFrom, this->getCaller(), this->getContractAddress(), this->proposalPrice_.get());
  }
  BTVProposal proposal;
  auto& [energy, titl, desc] = proposal;
  energy = this->proposalPrice_.get();
  titl = title;
  desc = description;
  this->activeProposals_[this->proposalCount_.get()] = proposal;
  ++this->proposalCount_;
}

void BTVProposals::voteOnProposal(const uint64_t &tokenId, const uint64_t &proposalId, const uint256_t &energy) {
  this->onlyPlayer();
  auto proposalIt = this->activeProposals_.find(proposalId);
  if (proposalIt == this->activeProposals_.end()) {
    throw DynamicException("BTVProposals::voteOnProposal : proposal does not exist");
  }
  this->callContractFunction(this->energyContract_.get(), &ERC20::transferFrom, this->getCaller(), this->getContractAddress(), energy);
  this->proposalVotes_[proposalId][tokenId] += energy;
  std::get<0>(proposalIt->second) += energy;
}

void BTVProposals::removeVote(const uint64_t &tokenId, const uint64_t &proposalId, const uint256_t &energy) {
  this->onlyPlayer();
  if (!this->proposalVotes_.contains(proposalId)) {
    throw DynamicException("BTVProposals::removeVote : proposal does not exist");
  }
  auto voteIt = this->proposalVotes_[proposalId].find(tokenId);
  if (voteIt == this->proposalVotes_[proposalId].end()) {
    throw DynamicException("BTVProposals::removeVote : token vote on specific proposal doesnt exist");
  }
  if (voteIt->second < energy) {
    throw DynamicException("BTVProposals::removeVote : not enough energy to remove");
  }
  // If we are taking out ALL the energy, we can remove the vote.
  if (voteIt->second == energy) {
    this->proposalVotes_[proposalId].erase(voteIt);
  } else {
    voteIt->second -= energy;
  }
  this->callContractFunction(this->energyContract_.get(), &ERC20::transfer, this->getCaller(), energy);
  // Take the vote out of proposal if its active.
  auto proposalIt = this->activeProposals_.find(proposalId);
  if (proposalIt != this->activeProposals_.end()) {
    std::get<0>(proposalIt->second) -= energy;
  }
}

void BTVProposals::completeProposal(const uint64_t& proposalId) {
  this->onlyOwner();
  auto proposalIt = this->activeProposals_.find(proposalId);
  if (proposalIt == this->activeProposals_.end()) {
    throw DynamicException("BTVProposals::completeProposal : proposal does not exist");
  }
  this->completedProposals_[proposalId] = proposalIt->second;
  this->activeProposals_.erase(proposalIt);
}

void BTVProposals::setProposalPrice(const uint256_t &price) {
  this->onlyOwner();
  this->proposalPrice_ = price;
}

void BTVProposals::setPlayerContract(const Address &playerContract) {
  this->onlyOwner();
  this->playerContract_ = playerContract;
}

void BTVProposals::setEnergyContract(const Address &energyContract) {
  this->onlyOwner();
  this->energyContract_ = energyContract;
}

std::vector<BTVProposal> BTVProposals::getActiveProposals() const {
  std::vector<BTVProposal> proposals;
  for (auto it = this->activeProposals_.cbegin(); it != this->activeProposals_.cend(); ++it) {
    proposals.emplace_back(it->second);
  }
  return proposals;
}

std::vector<BTVProposal> BTVProposals::getCompletedProposals() const {
  std::vector<BTVProposal> proposals;
  for (auto it = this->completedProposals_.cbegin(); it != this->completedProposals_.cend(); ++it) {
    proposals.emplace_back(it->second);
  }
  return proposals;
}

std::vector<std::tuple<uint64_t, uint256_t>> BTVProposals::getProposalVotes(const uint64_t &proposalId) const {
  auto votesIt = this->proposalVotes_.find(proposalId);
  if (votesIt == this->proposalVotes_.cend()) {
    return {};
  }
  std::vector<std::tuple<uint64_t, uint256_t>> votes;
  for (auto it = votesIt->second.cbegin(); it != votesIt->second.cend(); ++it) {
    votes.emplace_back(it->first, it->second);
  }
  return votes;
}

uint256_t BTVProposals::getProposalPrice() const {
  return this->proposalPrice_.get();
}

uint256_t BTVProposals::getProposalEnergy(const uint64_t &proposalId) const {
  auto proposalIt = this->activeProposals_.find(proposalId);
  if (proposalIt == this->activeProposals_.cend()) {
    throw DynamicException("BTVProposals::getProposalEnergy : proposal does not exist");
  }
  return std::get<0>(proposalIt->second);
}

uint64_t BTVProposals::getProposalCount() const {
  return this->proposalCount_.get();
}

DBBatch BTVProposals::dump() const {
  DBBatch dbBatch = Ownable::dump();
  dbBatch.push_back(StrConv::stringToBytes("proposalCount_"), UintConv::uint64ToBytes(this->proposalCount_.get()), this->getDBPrefix());

  for (auto it = this->activeProposals_.cbegin(); it != this->activeProposals_.cend(); ++it) {
    auto& [id, proposal] = *it;
    auto& [energy, title, description] = proposal;
    Bytes value;
    Utils::appendBytes(value, UintConv::uint256ToBytes(energy));
    Utils::appendBytes(value, UintConv::uint64ToBytes(title.size()));
    Utils::appendBytes(value, StrConv::stringToBytes(title));
    Utils::appendBytes(value, StrConv::stringToBytes(description));
    dbBatch.push_back(UintConv::uint64ToBytes(id), value, this->getNewPrefix("activeProposals_"));
  }

  for (auto it = this->completedProposals_.cbegin(); it != this->completedProposals_.cend(); ++it) {
    auto& [id, proposal] = *it;
    auto& [energy, title, description] = proposal;
    Bytes value;
    Utils::appendBytes(value, UintConv::uint256ToBytes(energy));
    Utils::appendBytes(value, UintConv::uint64ToBytes(title.size()));
    Utils::appendBytes(value, StrConv::stringToBytes(title));
    Utils::appendBytes(value, StrConv::stringToBytes(description));
    dbBatch.push_back(UintConv::uint64ToBytes(id), value, this->getNewPrefix("completedProposals_"));
  }

  dbBatch.push_back(StrConv::stringToBytes("playerContract_"), this->playerContract_.get(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("energyContract_"), this->energyContract_.get(), this->getDBPrefix());

  for (auto it = this->proposalVotes_.cbegin(); it != this->proposalVotes_.cend(); ++it) {
    auto& [proposalId, votes] = *it;
    for (auto it2 = votes.cbegin(); it2 != votes.cend(); ++it2) {
      auto& [tokenId, energy] = *it2;
      Bytes key;
      Utils::appendBytes(key, UintConv::uint64ToBytes(proposalId));
      Utils::appendBytes(key, UintConv::uint64ToBytes(tokenId));
      dbBatch.push_back(key, UintConv::uint256ToBytes(energy), this->getNewPrefix("proposalVotes_"));
    }
  }

  dbBatch.push_back(StrConv::stringToBytes("proposalPrice_"), UintConv::uint256ToBytes(this->proposalPrice_.get()), this->getDBPrefix());
  return dbBatch;
}
