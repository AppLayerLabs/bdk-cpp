#include "bomberlobby.h"

BomberLobby::BomberLobby(
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("BomberLobby", address, creator, chainId),
  Ownable(creator, address, creator, chainId),
  GAME_ADDRESS(this), MAX_ROOMS(this), rooms(this), roomStatuses(this)
{
  // BomberGame address is set after deploy (we don't know which address it'll be)
  this->MAX_ROOMS = 4;

  this->GAME_ADDRESS.commit();
  this->MAX_ROOMS.commit();

  // Create both room and the default room status
  for (uint8_t i = 0; i <= this->MAX_ROOMS.get(); i++) {
    this->rooms[i] = {};
    this->roomStatuses[i] = std::make_tuple(RoomStatus::NOTREADY, this->ROOM_WAIT_TIME);
  }

  this->rooms.commit();
  this->roomStatuses.commit();

  registerContractFunctions();

  this->GAME_ADDRESS.enableRegister();
  this->MAX_ROOMS.enableRegister();
  this->rooms.enableRegister();
  this->roomStatuses.enableRegister();
}

BomberLobby::BomberLobby(
  const Address& address, const DB& db
) : DynamicContract(address, db), Ownable(address, db),
  GAME_ADDRESS(this), MAX_ROOMS(this), rooms(this), roomStatuses(this)
{
  this->GAME_ADDRESS = Address(db.get(std::string("GAME_ADDRESS"), this->getDBPrefix()));
  this->MAX_ROOMS = UintConv::bytesToUint8(db.get(std::string("MAX_ROOMS"), this->getDBPrefix()));

  this->GAME_ADDRESS.commit();
  this->MAX_ROOMS.commit();

  // Create both room and the default room status
  for (uint8_t i = 0; i <= this->MAX_ROOMS.get(); i++) {
    this->rooms[i] = {};
    this->roomStatuses[i] = std::make_tuple(RoomStatus::NOTREADY, this->ROOM_WAIT_TIME);
  }

  this->rooms.commit();
  this->roomStatuses.commit();

  registerContractFunctions();

  this->GAME_ADDRESS.enableRegister();
  this->MAX_ROOMS.enableRegister();
  this->rooms.enableRegister();
  this->roomStatuses.enableRegister();
}

BomberLobby::~BomberLobby() {};

bool BomberLobby::roomExists(uint8_t id) const { return (id != 0 && id <= this->MAX_ROOMS.get()); }

bool BomberLobby::roomIsReady(uint8_t roomId) const {
  if (!roomExists(roomId)) return false;
  if (this->rooms.at(roomId).size() < 2) return false; // Do nothing if minimum player quota is not met
  // Check if everyone in the room is truly ready
  for (auto it = this->rooms.at(roomId).cbegin(); it != this->rooms.at(roomId).cend(); it++) {
    if (!get<1>(*it)) return false;  // Go back at first "not ready" found
  }
  return true;
}

/*
void BomberLobby::lobbyTickLoop() {
  while (this->ROOM_TICK_ACTIVE) {
    {
      std::unique_lock<std::mutex> lock(this->roomMutex);
      for (uint8_t i = 0; i < this->MAX_ROOMS.get(); i++) {
        switch (get<0>(this->roomStatuses[i])) {
          case RoomStatus::NOTREADY:  // Reset room timer if needed, then skip
            if (get<1>(this->roomStatuses[i]) != this->ROOM_WAIT_TIME) {
              get<1>(this->roomStatuses[i]) = this->ROOM_WAIT_TIME;
            }
            break;
          case RoomStatus::READY: // Start counting down
            this->gameStartingIn(i, get<1>(this->roomStatuses[i]));
            get<0>(this->roomStatuses[i]) = RoomStatus::COUNTING;
            break;
          case RoomStatus::COUNTING:  // Keep counting down until done
            // Floats are innacurate so we do a little decimal rounding
            get<1>(this->roomStatuses[i]) -= 0.1f;
            get<1>(this->roomStatuses[i]) = std::round(get<1>(this->roomStatuses[i]) * 100) / 100;
            // Emit the countdown event every 1s (full float time - truncated integer)
            if (get<1>(this->roomStatuses[i]) - std::trunc(get<1>(this->roomStatuses[i])) == 0) {
              this->gameStartingIn(i, get<1>(this->roomStatuses[i]));
            }
            if (get<1>(this->roomStatuses[i]) <= 0.0f) {
              get<0>(this->roomStatuses[i]) = RoomStatus::DONE;
            }
            break;
          case RoomStatus::DONE:  // Start a game
            // BomberGame address must be set beforehand. If it's not,
            // the room will stay in an eternal "DONE" status until it is set
            // or the room status changes on its own
            if (this->GAME_ADDRESS.get() != Address()){
              std::vector<Address> players = {};
              for (const std::tuple<Address, bool>& p : this->rooms[i]) players.push_back(get<0>(p));
              this->callContractFunction(this->GAME_ADDRESS.get(), &BomberGame::createGame, players);
              // Clear the room and reset its status
              this->rooms[i] = {};
              this->roomStatuses[i] = std::make_tuple(RoomStatus::NOTREADY, this->ROOM_WAIT_TIME);
              this->roomCleared(i);
            }
            break;
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 1 tick = 0.1s
  }
}
*/

// ===========================================================================

std::tuple<uint8_t, uint8_t, std::vector<uint8_t>> BomberLobby::getLobbyData() const {
  std::vector<uint8_t> players;
  for (uint8_t i = 1; i <= this->MAX_ROOMS.get(); i++) players.push_back(this->rooms.at(i).size());
  return std::make_tuple(this->MAX_ROOMS.get(), this->MAX_SLOTS, players);
}

std::vector<std::tuple<Address, bool>> BomberLobby::getRoomData(uint8_t roomId) const {
  if (!this->roomExists(roomId)) throw DynamicException("Room " + std::to_string(roomId) + " does not exist");
  return this->rooms.at(roomId);
}

void BomberLobby::setGameAddress(const Address& add) {
  this->onlyOwner(); this->GAME_ADDRESS = add; this->gameAddressChanged(this->GAME_ADDRESS.get());
}

void BomberLobby::resizeLobby(uint8_t newSize) {
  this->onlyOwner(); if (newSize == 0) throw DynamicException("Need at least 1 open room");
  // Only resize if new size is actually not the same as the current size
  if (newSize != this->MAX_ROOMS.get()) {
    this->MAX_ROOMS = newSize;
    std::unique_lock<std::mutex> lock(this->roomMutex);
    if (this->MAX_ROOMS.get() < this->rooms.size()) { // Lobby has shrunk
      for (uint8_t i = this->MAX_ROOMS.get() + 1; i <= this->rooms.size(); i++) {
        // Erase both room and room status and emit the event
        this->roomStatuses.erase(i);
        this->rooms.erase(i);
        this->roomCleared(i);
      }
    } else if (this->MAX_ROOMS.get() > this->rooms.size()) {  // Lobby has grown
      for (uint8_t i = this->rooms.size() + 1; i <= this->MAX_ROOMS.get(); i++) {
        // Create both room and the default room status
        this->rooms[i] = {};
        this->roomStatuses[i] = std::make_tuple(RoomStatus::NOTREADY, this->ROOM_WAIT_TIME);
      }
    }
    this->lobbySizeUpdated(this->MAX_ROOMS.get());
  }
}

void BomberLobby::playerDisconnect(const Address& player, uint8_t roomId) {
  this->onlyOwner();
  if (!this->roomExists(roomId)) throw DynamicException("Room " + std::to_string(roomId) + " does not exist");

  // If player was in a room before disconnecting, kick them out.
  // Same logic as exitRoom() but we pass the player address directly instead of querying the caller
  std::unique_lock<std::mutex> lock(this->roomMutex);
  for (auto it = this->rooms[roomId].begin(); it != this->rooms[roomId].end(); it++) {
    if (get<0>(*it) == player) {
      this->rooms[roomId].erase(it);
      if (this->roomIsReady(roomId) && get<0>(this->roomStatuses[roomId]) == RoomStatus::NOTREADY) {
        get<0>(this->roomStatuses[roomId]) = RoomStatus::READY;
      } else if (!this->roomIsReady(roomId) && get<0>(this->roomStatuses[roomId]) == RoomStatus::READY) {
        get<0>(this->roomStatuses[roomId]) = RoomStatus::NOTREADY;
      }
      this->roomExited(Address(player), roomId);
      break;
    }
  }
}

void BomberLobby::enterRoom(uint8_t roomId) {
  if (!this->roomExists(roomId)) throw DynamicException("Room " + std::to_string(roomId) + " does not exist");

  std::unique_lock<std::mutex> lock(this->roomMutex);

  // Check if room is full
  if (this->rooms[roomId].size() >= this->MAX_SLOTS) {
    throw DynamicException("Room " + std::to_string(roomId) + " is full");
  }

  // Check if player somehow is already in the room for some reason
  for (auto it = this->rooms[roomId].begin(); it != this->rooms[roomId].end(); it++) {
    if (get<0>(*it) == this->getCaller()) throw DynamicException(
      "Player " + get<0>(*it).hex().get() + " is already in room " + std::to_string(roomId)
    );
  }
  // Always enter as "not ready" and force room status back to that
  this->rooms[roomId].push_back(std::make_tuple(this->getCaller(), false));
  get<0>(this->roomStatuses[roomId]) = RoomStatus::NOTREADY;
  this->roomEntered(Address(this->getCaller()), roomId);
}

void BomberLobby::exitRoom(uint8_t roomId) {
  if (!this->roomExists(roomId)) throw DynamicException("Room " + std::to_string(roomId) + " does not exist");

  // Search for the player in the room, remove it if found and check ready status for the room
  std::unique_lock<std::mutex> lock(this->roomMutex);
  for (auto it = this->rooms[roomId].begin(); it != this->rooms[roomId].end(); it++) {
    if (get<0>(*it) == this->getCaller()) {
      this->rooms[roomId].erase(it);
      if (this->roomIsReady(roomId) && get<0>(this->roomStatuses[roomId]) == RoomStatus::NOTREADY) {
        get<0>(this->roomStatuses[roomId]) = RoomStatus::READY;
      } else if (!this->roomIsReady(roomId) && get<0>(this->roomStatuses[roomId]) == RoomStatus::READY) {
        get<0>(this->roomStatuses[roomId]) = RoomStatus::NOTREADY;
      }
      this->roomExited(Address(this->getCaller()), roomId);
      break;
    }
  }
}

void BomberLobby::toggleReady(uint8_t roomId) {
  if (!this->roomExists(roomId)) throw DynamicException("Room " + std::to_string(roomId) + " does not exist");

  // Search for the player in the room, toggle its status if found and check ready status for the room
  bool ready;
  {
    std::unique_lock<std::mutex> lock(this->roomMutex);
    for (auto it = this->rooms[roomId].begin(); it != this->rooms[roomId].end(); it++) {
      if (get<0>(*it) == this->getCaller()) {
        get<1>(*it) = !get<1>(*it);  // Flip "false" to "true" and vice-versa
        ready = get<1>(*it);
        if (this->roomIsReady(roomId) && get<0>(this->roomStatuses[roomId]) == RoomStatus::NOTREADY) {
          get<0>(this->roomStatuses[roomId]) = RoomStatus::READY;
        } else if (!this->roomIsReady(roomId) && get<0>(this->roomStatuses[roomId]) == RoomStatus::READY) {
          get<0>(this->roomStatuses[roomId]) = RoomStatus::NOTREADY;
        }
        break;
      }
    }
  }

  // Emit the event
  if (ready) {
    this->playerReady(Address(this->getCaller()), roomId);
  } else {
    this->playerNotReady(Address(this->getCaller()), roomId);
  }

  // TODO: migrate the game starting routine down here
}

void BomberLobby::registerContractFunctions() {
  registerContract();
  this->registerMemberFunctions(
    std::make_tuple("getLobbyData", &BomberLobby::getLobbyData, FunctionTypes::View, this),
    std::make_tuple("getRoomData", &BomberLobby::getRoomData, FunctionTypes::View, this),
    std::make_tuple("setGameAddress", &BomberLobby::setGameAddress, FunctionTypes::NonPayable, this),
    std::make_tuple("resizeLobby", &BomberLobby::resizeLobby, FunctionTypes::NonPayable, this),
    std::make_tuple("playerDisconnect", &BomberLobby::playerDisconnect, FunctionTypes::NonPayable, this),
    std::make_tuple("enterRoom", &BomberLobby::enterRoom, FunctionTypes::NonPayable, this),
    std::make_tuple("exitRoom", &BomberLobby::exitRoom, FunctionTypes::NonPayable, this),
    std::make_tuple("toggleReady", &BomberLobby::toggleReady, FunctionTypes::NonPayable, this)
  );
}

DBBatch BomberLobby::dump() const {
  // We need to dump all the data from the parent classes as well
  DBBatch dbBatch = BaseContract::dump();
  const DBBatch ownableDump = Ownable::dump();
  for (const auto& dbItem : ownableDump.getPuts()) dbBatch.push_back(dbItem);
  for (const auto& dbItem : ownableDump.getDels()) dbBatch.delete_key(dbItem);

  // Then we dump the class' contents
  dbBatch.push_back(StrConv::stringToBytes("GAME_ADDRESS"), this->GAME_ADDRESS.get().asBytes(), this->getDBPrefix());
  dbBatch.push_back(StrConv::stringToBytes("MAX_ROOMS"), UintConv::uint8ToBytes(this->MAX_ROOMS.get()), this->getDBPrefix());
  return dbBatch;
}

