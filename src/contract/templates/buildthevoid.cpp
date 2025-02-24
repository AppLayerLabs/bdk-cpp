#include "buildthevoid.h"
#include "btvplayer.h"
#include "btvenergy.h"

BuildTheVoid::BuildTheVoid(const Address &address, const DB &db) :
  DynamicContract(address, db),
  Ownable(address, db),
  playerContract_(this),
  energyContract_(this),
  activePlayers_(this),
  inactivePlayers_(this),
  deadPlayers_(this),
  surfaceBlocks_(this),
  energyBlockCounter_(this),
  world_(this) {

  this->playerContract_ = Address(db.get(std::string("playerContract_"), this->getDBPrefix()));
  this->energyContract_ = Address(db.get(std::string("energyContract_"), this->getDBPrefix()));

  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("activePlayers_"))) {
    BTVUtils::PlayerInformation player;
    View<Bytes> value(dbEntry.value);
    player.position.x = IntConv::bytesToInt32(value.subspan(0, 4));
    player.position.y = IntConv::bytesToInt32(value.subspan(4, 4));
    player.position.z = IntConv::bytesToInt32(value.subspan(8, 4));
    player.energy = UintConv::bytesToUint256(value.subspan(12, 32));
    player.lastUpdate = UintConv::bytesToUint64(value.subspan(44, 8));
    this->activePlayers_[UintConv::bytesToUint64(dbEntry.key)] = player;
  }

  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("inactivePlayers_"))) {
    BTVUtils::PlayerInformation player;
    View<Bytes> value(dbEntry.value);
    player.position.x = IntConv::bytesToInt32(value.subspan(0, 4));
    player.position.y = IntConv::bytesToInt32(value.subspan(4, 4));
    player.position.z = IntConv::bytesToInt32(value.subspan(8, 4));
    player.energy = UintConv::bytesToUint256(value.subspan(12, 32));
    player.lastUpdate = UintConv::bytesToUint64(value.subspan(44, 8));
    this->inactivePlayers_[UintConv::bytesToUint64(dbEntry.key)] = player;
  }

  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("deadPlayers_"))) {
    BTVUtils::PlayerInformation player;
    View<Bytes> value(dbEntry.value);
    player.position.x = IntConv::bytesToInt32(value.subspan(0, 4));
    player.position.y = IntConv::bytesToInt32(value.subspan(4, 4));
    player.position.z = IntConv::bytesToInt32(value.subspan(8, 4));
    player.energy = UintConv::bytesToUint256(value.subspan(12, 32));
    player.lastUpdate = UintConv::bytesToUint64(value.subspan(44, 8));
    this->deadPlayers_[UintConv::bytesToUint64(dbEntry.key)] = player;
  }

  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("surfaceBlocks_"))) {
    View<Bytes> keyView(dbEntry.key);
    BTVUtils::WorldBlockPos blockPos;
    blockPos.x = IntConv::bytesToInt32(keyView.subspan(0, 4));
    blockPos.y = IntConv::bytesToInt32(keyView.subspan(4, 4));
    blockPos.z = IntConv::bytesToInt32(keyView.subspan(8, 4));
    this->surfaceBlocks_.push_back(blockPos);
  }

  this->energyBlockCounter_ = UintConv::bytesToUint64(db.get(std::string("energyBlockCounter_"), this->getDBPrefix()));

  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("world_"))) {
    View<Bytes> keyView(dbEntry.key);
    BTVUtils::ChunkCoord2D chunkCoords;
    chunkCoords.first = IntConv::bytesToInt32(keyView.subspan(0, 4));
    chunkCoords.second = IntConv::bytesToInt32(keyView.subspan(4, 4));
    this->world_.getChunks()[chunkCoords] = BTVUtils::Chunk::deserialize(dbEntry.value);
  }

  this->playerContract_.commit();
  this->energyContract_.commit();
  this->activePlayers_.commit();
  this->inactivePlayers_.commit();
  this->deadPlayers_.commit();
  this->energyBlockCounter_.commit();
  this->surfaceBlocks_.commit();
  this->registerContractFunctions();
  this->playerContract_.enableRegister();
  this->energyContract_.enableRegister();
  this->activePlayers_.enableRegister();
  this->inactivePlayers_.enableRegister();
  this->deadPlayers_.enableRegister();
  this->energyBlockCounter_.enableRegister();
  this->surfaceBlocks_.enableRegister();
  this->world_.commitAndEnable();
}


BuildTheVoid::BuildTheVoid(const Address &address, const Address &creator, const uint64_t &chainId) :
  DynamicContract("BuildTheVoid", address, creator, chainId),
  Ownable("BuildTheVoid", creator, address, creator, chainId),
  playerContract_(this),
  energyContract_(this),
  activePlayers_(this),
  inactivePlayers_(this),
  deadPlayers_(this),
  surfaceBlocks_(this),
  energyBlockCounter_(this),
  world_(this) {

#ifdef BUILD_TESTNET
  if (creator != Address(Hex::toBytes("0xc2f2ba5051975004171e6d4781eeda927e884024"))) {
    throw DynamicException("Only the Chain Owner can create this contract");
  }
#endif

  // We need to fill the surfaceBlocks_ vector with the surface blocks
  // it is a 10x10 area at y=5
  for (int x = 0; x < 10; x++) {
    for (int z = 0; z < 10; z++) {
      BTVUtils::WorldBlockPos blockPos;
      blockPos.x = x;
      blockPos.y = 5;
      blockPos.z = z;
      this->surfaceBlocks_.push_back(blockPos);
    }
  }

  this->playerContract_.commit();
  this->energyContract_.commit();
  this->activePlayers_.commit();
  this->inactivePlayers_.commit();
  this->deadPlayers_.commit();
  this->energyBlockCounter_.commit();
  this->surfaceBlocks_.commit();
  this->registerContractFunctions();
  this->playerContract_.enableRegister();
  this->energyContract_.enableRegister();
  this->activePlayers_.enableRegister();
  this->inactivePlayers_.enableRegister();
  this->deadPlayers_.enableRegister();
  this->energyBlockCounter_.enableRegister();
  this->surfaceBlocks_.enableRegister();
  this->world_.commitAndEnable();
}

void BuildTheVoid::registerContractFunctions() {
  this->registerMemberFunction("setPlayerContract", &BuildTheVoid::setPlayerContract, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("setEnergyContract", &BuildTheVoid::setEnergyContract, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("forceUpdate", &BuildTheVoid::forceUpdate, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("approve", &BuildTheVoid::approve, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("loginPlayer", &BuildTheVoid::loginPlayer, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("logoutPlayer", &BuildTheVoid::logoutPlayer, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("changeBlock", &BuildTheVoid::changeBlock, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("movePlayer", &BuildTheVoid::movePlayer, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("claimEnergy", &BuildTheVoid::claimEnergy, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("getChunk", &BuildTheVoid::getChunk, FunctionTypes::View, this);
  this->registerMemberFunction("getPlayerContract", &BuildTheVoid::getPlayerContract, FunctionTypes::View, this);
  this->registerMemberFunction("getEnergyContract", &BuildTheVoid::getEnergyContract, FunctionTypes::View, this);
  this->registerMemberFunction("getActivePlayers", &BuildTheVoid::getActivePlayers, FunctionTypes::View, this);
  this->registerMemberFunction("getInnactivePlayers", &BuildTheVoid::getInnactivePlayers, FunctionTypes::View, this);
  this->registerMemberFunction("getDeadPlayers", &BuildTheVoid::getDeadPlayers, FunctionTypes::View, this);
}

void BuildTheVoid::approve() {
  this->onlyOwner();
  this->callContractFunction(this->energyContract_.get(), &ERC20::approve, this->playerContract_.get(), std::numeric_limits<uint256_t>::max());
}

void BuildTheVoid::internalKillPlayer() {
  // We loop the active players, and check if they were unnactive for at least 5 seconds
  // If the player was innactive for 5 seconds, and there is no non-air underlaying block, we kill the player
  // TODO: I believe this is better implemented by creating a vector of "possible players to kill"
  // When a player moves (BuildTheVoid::movePlayer), we check if the underlying blocks are air
  // If they are, they are added to the vector of "possible players to kill" and checked here.
  return;
}

void BuildTheVoid::internalLogoutPlayer() {
  // TODO
  // Almost the same as internalKillPlayer, but we don't kill the player
  // we check if the last activity of the player was more than 30 seconds ago
  // If it was, We need to move the player to the inactivePlayers_ map
  // Not only that, but also call the BTVPlayer contract to add the energy to the player
  // remember that this->getBlockTimestamp() returns the current block timestamp in microseconds
  for (auto it = this->activePlayers_.cbegin(); it != this->activePlayers_.cend(); ++it) {
    if (this->getBlockTimestamp() - it->second.lastUpdate > 30 * 1000000) {
      this->inactivePlayers_[it->first] = it->second;
      this->inactivePlayers_[it->first].energy = 0;
      if (it->second.energy != 0) {
        this->callContractFunction(this->playerContract_.get(), &BTVPlayer::addPlayerEnergy, it->first, it->second.energy);
      }
      this->activePlayers_.erase(it);
      this->PlayerLogout(it->first);
    }
  }
}

void BuildTheVoid::internalSpawnEnergyBlock() {
  // For every 100 blocks, we spawn an energy block
  // The energy block is spawned on top of a surface block
  // Surface blocks are described in the surfaceBlocks_ vector
  uint64_t wantedEnergyBlocks = this->surfaceBlocks_.size() / 100;
  if (this->energyBlockCounter_.get() >= wantedEnergyBlocks) {
    return;
  }
  for (uint64_t i = 0; i < wantedEnergyBlocks - this->energyBlockCounter_.get(); i++) {
    uint64_t randomIndex = static_cast<uint64_t>(this->getRandom() % this->surfaceBlocks_.size());
    std::cout << "Random index: " << randomIndex << std::endl;
    BTVUtils::WorldBlockPos blockPos = this->surfaceBlocks_[randomIndex];
    if (!this->world_.hasBlockOver(blockPos)) {
      // Do not forget to add +1 to the Y position
      blockPos.y += 1;
      auto block = this->world_.getBlock(blockPos);
      block->type = BTVUtils::BlockType::ENERGYCHEST;
      block->modificationTimestamp = 0;
      block->placer_ = std::nullopt;
      ++this->energyBlockCounter_;
      this->BlockChanged(std::numeric_limits<uint64_t>::max(), blockPos.x, blockPos.y, blockPos.z, static_cast<uint8_t>(BTVUtils::BlockType::ENERGYCHEST), this->getBlockTimestamp());
    }
  }
}

void BuildTheVoid::selfcallUpdate() {
  this->internalKillPlayer();
  this->internalLogoutPlayer();
  this->internalSpawnEnergyBlock();
}

void BuildTheVoid::setEnergyContract(const Address& playerContract) {
  this->onlyOwner();
  this->energyContract_ = playerContract;
}

void BuildTheVoid::setPlayerContract(const Address& playerContract) {
  this->onlyOwner();
  this->playerContract_ = playerContract;
}

void BuildTheVoid::forceUpdate() {
  this->onlyOwner();
  // Actually registers on the blockobserver
  this->addBlockObserverByCount(0, &BuildTheVoid::selfcallUpdate);
  // this->selfcallUpdate();
}


void BuildTheVoid::loginPlayer(const uint64_t& playerId, const uint256_t& energy) {
  if (this->callContractViewFunction(this->playerContract_.get(), &BTVPlayer::ownerOf, static_cast<uint256_t>(playerId)) != this->getCaller()) {
    throw DynamicException("BuildTheVoid::loginPlayer: Not the owner of the player");
  }
  if (this->deadPlayers_.contains(playerId)) {
    throw DynamicException("BuildTheVoid::loginPlayer: Player is dead");
  }
  if (energy != 0) {
    this->callContractFunction(this->playerContract_.get(), &BTVPlayer::takePlayerEnergy, playerId, energy);
  }
  BTVUtils::PlayerInformation player;
  auto it = this->inactivePlayers_.find(playerId);
  if (it != this->inactivePlayers_.end()) {
    // The player already existed in the inactivePlayers_ map
    // Take off the player from the inactivePlayers_ map, add the energy and place it in the activePlayers_ map
    player = it->second;
    player.energy = energy;
    player.lastUpdate = this->getBlockTimestamp();
    this->inactivePlayers_.erase(it);
    this->activePlayers_[playerId] = player;
  } else {
    // The player is new, we need to create a new player
    // Place the player above the 10x10 surface area
    player.position.x = 4;
    player.position.y = 6;
    player.position.z = 4;
    player.energy = energy;
    player.lastUpdate = this->getBlockTimestamp();
    this->activePlayers_[playerId] = player;
  }

  this->PlayerLogin(playerId, player.position.x, player.position.y, player.position.z);
  return;
}


void BuildTheVoid::logoutPlayer(const uint64_t &playerId) {
  if (this->callContractViewFunction(this->playerContract_.get(), &BTVPlayer::ownerOf, static_cast<uint256_t>(playerId)) != this->getCaller()) {
    throw DynamicException("BuildTheVoid::logoutPlayer: Not the owner of the player");
  }
  if (this->deadPlayers_.contains(playerId)) {
    throw DynamicException("BuildTheVoid::logoutPlayer: Player is dead");
  }
  auto player = this->activePlayers_.find(playerId);
  if (player == this->activePlayers_.end()) {
    throw DynamicException("BuildTheVoid::logoutPlayer: Player is not active");
  }
  if (player->second.energy != 0) {
    this->callContractFunction(this->playerContract_.get(), &BTVPlayer::addPlayerEnergy, playerId, player->second.energy);
  }
  player->second.energy = 0;
  player->second.lastUpdate = this->getBlockTimestamp();
  this->inactivePlayers_[playerId] = player->second;
  this->activePlayers_.erase(player);
  this->PlayerLogout(playerId);
  return;
}


void BuildTheVoid::changeBlock(const uint64_t& playerId, const int32_t& x, const int32_t& y, const int32_t& z, const BTVUtils::BlockType& type) {
  if (this->callContractViewFunction(this->playerContract_.get(), &BTVPlayer::ownerOf, static_cast<uint256_t>(playerId)) != this->getCaller()) {
    throw DynamicException("BuildTheVoid::logoutPlayer: Not the owner of the player");
  }
  auto it = this->activePlayers_.find(playerId);
  if (it == this->activePlayers_.cend()) {
    throw DynamicException("BuildTheVoid::changeBlock: Player is not active");
  }
  // Get the block itself
  auto block = this->world_.getBlock(BTVUtils::WorldBlockPos{x, y, z});
  if (block->type == BTVUtils::BlockType::SURFACE || block->type == BTVUtils::BlockType::ENERGYCHEST) {
    throw DynamicException("BuildTheVoid::changeBlock: Cannot change a surface or energy block");
  }
  // Check if the block has ownership
  if (block->placer_.has_value()) {
    // Only allow the placer to change the block if the last modification was over 15 minutes ago
    if (this->getBlockTimestamp() - block->modificationTimestamp < (15 * 60 * 1000000)) {
      if (block->placer_.value() != playerId) {
        throw DynamicException("BuildTheVoid::changeBlock: Block is owned by another player");
      }
    }
  }
  // The player needs at least 1 energy (18 decimals do not forget)
  if (it->second.energy < 1000000000000000000) {
    throw DynamicException("BuildTheVoid::changeBlock: Player has no energy");
  }
  // Check if the block is within placing range (7) from the player
  if (BTVUtils::isBlockClose(BTVUtils::WorldBlockPos{x, y, z}, it->second.position, 7)) {
    block->type = type;
    block->placer_ = playerId;
    block->modificationTimestamp = this->getBlockTimestamp();
    this->BlockChanged(playerId, x, y, z, static_cast<uint8_t>(type), block->modificationTimestamp);
    if (type == BTVUtils::BlockType::SURFACE) {
      this->surfaceBlocks_.push_back(BTVUtils::WorldBlockPos{x, y, z});
    }
    // Take 1 energy from the player
    it->second.energy -= 1000000000000000000;
    it->second.lastUpdate = this->getBlockTimestamp();
  } else {
    throw DynamicException("BuildTheVoid::changeBlock: Block is too far away");
  }
}

void BuildTheVoid::movePlayer(const uint64_t &playerId, const int32_t& x, const int32_t& y, const int32_t& z) {
  if (this->callContractViewFunction(this->playerContract_.get(), &BTVPlayer::ownerOf, static_cast<uint256_t>(playerId)) != this->getCaller()) {
    throw DynamicException("BuildTheVoid::movePlayer: Not the owner of the player");
  }
  auto it = this->activePlayers_.find(playerId);
  if (it == this->activePlayers_.cend()) {
    throw DynamicException("BuildTheVoid::movePlayer: Player is not active");
  }
  // Check if the player is moving to a valid position (7 blocks away)
  if (BTVUtils::isBlockClose(BTVUtils::WorldBlockPos{x, y, z}, it->second.position, 7)) {
    it->second.position.x = x;
    it->second.position.y = y;
    it->second.position.z = z;
    it->second.lastUpdate = this->getBlockTimestamp();
    this->PlayerMoved(playerId, x, y, z);
  } else {
    throw DynamicException("BuildTheVoid::movePlayer: Player is moving too far away");
  }
}

void BuildTheVoid::claimEnergy(const uint64_t &playerId, const int32_t& x, const int32_t& y, const int32_t& z) {
  if (this->callContractViewFunction(this->playerContract_.get(), &BTVPlayer::ownerOf, static_cast<uint256_t>(playerId)) != this->getCaller()) {
    throw DynamicException("BuildTheVoid::movePlayer: Not the owner of the player");
  }
  auto it = this->activePlayers_.find(playerId);
  if (it == this->activePlayers_.cend()) {
    throw DynamicException("BuildTheVoid::claimEnergy: Player is not active");
  }
  auto block = this->world_.getBlock(BTVUtils::WorldBlockPos{x, y, z});
  if (block->type != BTVUtils::BlockType::ENERGYCHEST) {
    throw DynamicException("BuildTheVoid::claimEnergy: Block is not an energy block");
  }
  // Check if the player is actually close to the energy block
  if (BTVUtils::isBlockClose(BTVUtils::WorldBlockPos{x, y, z}, it->second.position, 7)) {
    // If the block is a energy type and the user is close, we can safely claim the energy
    // The energy value is something between 1 and 10 (including decimals), do not forget the 18 decimals of the token!
    // We need to POW the energy value by 10^18
    auto randomEnergyValue = static_cast<uint256_t>(this->getRandom() % uint256_t("10000000000000000000") + uint256_t("1000000000000000000"));
    block->type = BTVUtils::BlockType::AIR;
    block->placer_ = std::nullopt;
    block->modificationTimestamp = this->getBlockTimestamp();
    it->second.energy += randomEnergyValue;
    it->second.lastUpdate = this->getBlockTimestamp();
    std::cout << "energyBlockCounter_: " << this->energyBlockCounter_.get() << std::endl;
    --this->energyBlockCounter_;
    // Do not forget to mint the ERC20 tokens to ourselves! players playing the game have their energy stored in the contract
    // and it is sent back when they logout
    this->callContractFunction(this->energyContract_.get(), &BTVEnergy::mint, this->getContractAddress(), randomEnergyValue);
    this->ClaimedEnergy(playerId, randomEnergyValue);
  } else {
    throw DynamicException("BuildTheVoid::claimEnergy: Player is too far away from the energy block");
  }
}

Bytes BuildTheVoid::getChunk(const int32_t& cx, const int32_t& cy) const {
  auto chunk = this->world_.getChunk({cx, cy});
  if (chunk != nullptr) {
    return chunk->serialize();
  }
  return {};
}

Address BuildTheVoid::getPlayerContract() const {
  return this->playerContract_.get();
}

Address BuildTheVoid::getEnergyContract() const {
  return this->energyContract_.get();
}

std::vector<BTVUtils::PlayerInformationData> BuildTheVoid::getActivePlayers() const {
  std::vector<BTVUtils::PlayerInformationData> players;
  for (auto it = this->activePlayers_.cbegin(); it != this->activePlayers_.cend(); ++it) {
    players.push_back(std::make_tuple(it->first, std::make_tuple(it->second.position.x, it->second.position.y, it->second.position.z), it->second.energy, it->second.lastUpdate));
  }
  return players;
}

std::vector<BTVUtils::PlayerInformationData> BuildTheVoid::getInnactivePlayers() const {
  std::vector<BTVUtils::PlayerInformationData> players;
  for (auto it = this->inactivePlayers_.cbegin(); it != this->inactivePlayers_.cend(); ++it) {
    players.push_back(std::make_tuple(it->first, std::make_tuple(it->second.position.x, it->second.position.y, it->second.position.z), it->second.energy, it->second.lastUpdate));
  }
  return players;
}

std::vector<BTVUtils::PlayerInformationData> BuildTheVoid::getDeadPlayers() const {
  std::vector<BTVUtils::PlayerInformationData> players;
  for (auto it = this->deadPlayers_.cbegin(); it != this->deadPlayers_.cend(); ++it) {
    players.push_back(std::make_tuple(it->first, std::make_tuple(it->second.position.x, it->second.position.y, it->second.position.z), it->second.energy, it->second.lastUpdate));
  }
  return players;
}

DBBatch BuildTheVoid::dump() const {
  DBBatch dbBatch = Ownable::dump();
  dbBatch.push_back(StrConv::stringToBytes("energyContract_"), this->energyContract_.get(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("playerContract_"), this->playerContract_.get(), this->getDBPrefix());

  for (auto it = this->activePlayers_.cbegin(); it != this->activePlayers_.cend(); ++it) {
    Bytes value;
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.x));
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.y));
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.z));
    Utils::appendBytes(value, UintConv::uint256ToBytes(it->second.energy));
    Utils::appendBytes(value, UintConv::uint64ToBytes(it->second.lastUpdate));
    dbBatch.push_back(UintConv::uint64ToBytes(it->first), value, this->getNewPrefix("activePlayers_"));
  }

  for (auto it = this->inactivePlayers_.cbegin(); it != this->inactivePlayers_.cend(); ++it) {
    Bytes value;
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.x));
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.y));
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.z));
    Utils::appendBytes(value, UintConv::uint256ToBytes(it->second.energy));
    Utils::appendBytes(value, UintConv::uint64ToBytes(it->second.lastUpdate));
    dbBatch.push_back(UintConv::uint64ToBytes(it->first), value, this->getNewPrefix("inactivePlayers_"));
  }

  for (auto it = this->deadPlayers_.cbegin(); it != this->deadPlayers_.cend(); ++it) {
    Bytes value;
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.x));
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.y));
    Utils::appendBytes(value, IntConv::int32ToBytes(it->second.position.z));
    Utils::appendBytes(value, UintConv::uint256ToBytes(it->second.energy));
    Utils::appendBytes(value, UintConv::uint64ToBytes(it->second.lastUpdate));
    dbBatch.push_back(UintConv::uint64ToBytes(it->first), value, this->getNewPrefix("deadPlayers_"));
  }

  for (auto it = this->surfaceBlocks_.cbegin(); it != this->surfaceBlocks_.cend(); ++it) {
    Bytes key;
    Utils::appendBytes(key, IntConv::int32ToBytes(it->x));
    Utils::appendBytes(key, IntConv::int32ToBytes(it->y));
    Utils::appendBytes(key, IntConv::int32ToBytes(it->z));
    dbBatch.push_back(key, Bytes(), this->getNewPrefix("surfaceBlocks_"));
  }

  dbBatch.push_back(StrConv::stringToBytes("energyBlockCounter_"), UintConv::uint64ToBytes(this->energyBlockCounter_.get()), this->getDBPrefix());

  const auto& chunks = this->world_.getChunks();
  for (auto it = chunks.cbegin(); it != chunks.cend(); ++it) {
    Bytes key;
    Utils::appendBytes(key, IntConv::int32ToBytes(it->first.first));
    Utils::appendBytes(key, IntConv::int32ToBytes(it->first.second));
    dbBatch.push_back(key, it->second.serialize(), this->getNewPrefix("world_"));
  }
  return dbBatch;
}
