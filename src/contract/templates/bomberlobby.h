#ifndef BOMBERLOBBY_H
#define BOMBERLOBBY_H

#include "ownable.h" // includes SafeAddress
#include "bombergame.h"

#include "../variables/safeuint.h"
#include "../variables/safeunorderedmap.h"

#include <cmath>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>
#include <string>

/// Enum for the several states a room can be in.
enum RoomStatus { NOTREADY, READY, COUNTING, DONE };

/**
 * Contract responsible for acting as a lobby to BomberGame.
 * Players interact with this one first, then get sent to the other one.
 * NOTE: DO NOT change MAX_SLOTS - unless you want to crack your head with changing the whole spawn position logic in BomberGame::createGame()
 */
class BomberLobby : public virtual DynamicContract, public Ownable {
  private:
    const uint8_t MAX_SLOTS = 5;        ///< Maximum number of open slots for each room in the lobby.
    const float ROOM_WAIT_TIME = 5.0f;  ///< Total countdown time in seconds for each room when everyone is ready.
    bool ROOM_TICK_ACTIVE = false;      ///< Flag for keeping the room tick update thread alive.

    SafeAddress GAME_ADDRESS;           ///< Address for the BomberGame contract.
    SafeUint8_t MAX_ROOMS;              ///< Maximum number of open rooms in the lobby.

    /// The list of rooms and their respective slots, in order: room ID, player address and ready flag.
    SafeUnorderedMap<uint8_t, std::vector<std::tuple<Address, bool>>> rooms;

    /// The list of room statuses, in order: room ID, status enum and timer.
    SafeUnorderedMap<uint8_t, std::tuple<RoomStatus, float>> roomStatuses;

    /// Mutex for managing read/write access to the room variables.
    std::mutex roomMutex;

    /**
     * Check if a room exists by querying its ID.
     * @param id The ID of the room to query.
     * @return `true` if room exists, `false` otherwise.
     */
    bool roomExists(uint8_t id) const;

    /**
     * Check if a room has all players ready.
     * @param roomId The ID of the room to check.
     * @returns `true` if game can begin, `false` otherwise.
     */
    bool roomIsReady(uint8_t roomId) const;

    /**
     * (Almost) Perpetual function that runs in a separate thread and keeps track of room timers.
     * Only way to stop this is through resizeLobby() which unsets the bool flag
     * that keeps the thread active, then calls the function again to restart scanning.
     */
    //void lobbyTickLoop();

    void registerContractFunctions() override;  ///< Register contract functions.

  public:
    /// Event for when the BomberGame address is changed.
    void gameAddressChanged(const EventParam<Address, false>& newAdd) {
      this->emitEvent(__func__, std::make_tuple(newAdd));
    }

    /// Event for when the lobby is resized.
    void lobbySizeUpdated(const EventParam<uint8_t, false>& newSize) {
      this->emitEvent(__func__, std::make_tuple(newSize));
    }

    /// Event for when a player enters a room.
    void roomEntered(
      const EventParam<Address, true>& player,
      const EventParam<uint8_t, false>& roomId
    ) {
      this->emitEvent(__func__, std::make_tuple(player, roomId));
    }

    /// Event for when a player exits a room.
    void roomExited(
      const EventParam<Address, true>& player,
      const EventParam<uint8_t, false>& roomId
    ) {
      this->emitEvent(__func__, std::make_tuple(player, roomId));
    }

    /// Event for when all players are booted out of a room (due to resizing or a game starting).
    void roomCleared(const EventParam<uint8_t, false>& roomId) {
      this->emitEvent(__func__, std::make_tuple(roomId));
    }

    /// Event for when a player signals they're ready to play.
    void playerReady(
      const EventParam<Address, true>& player,
      const EventParam<uint8_t, false>& roomId
    ) {
      this->emitEvent(__func__, std::make_tuple(player, roomId));
    }

    /// Event for when a player signals they're not yet ready to play.
    void playerNotReady(
      const EventParam<Address, true>& player,
      const EventParam<uint8_t, false>& roomId
    ) {
      this->emitEvent(__func__, std::make_tuple(player, roomId));
    }

    /// Event for when a room's countdown starts ticking every second.
    void gameStartingIn(
      const EventParam<uint8_t, false>& roomId,
      const EventParam<uint8_t, false>& secondsLeft
    ) {
      this->emitEvent(__func__, std::make_tuple(roomId, secondsLeft));
    }

    using ConstructorArguments = std::tuple<>;  ///< Constructor argument types.

    /**
     * Constructor from create. Create contract and save it to database.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    BomberLobby(const Address& address, const Address& creator, const uint64_t& chainId);

    /**
     * Constructor from load. Load contract from database.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    BomberLobby(const Address& address, const DB& db);

    ~BomberLobby();  ///< Destructor.

    /**
     * Get the current lobby data.
     * @return A tuple with the current number of open rooms, slots per room, and how many players are currently in each room.
     */
    std::tuple<uint8_t, uint8_t, std::vector<uint8_t>> getLobbyData() const;

    /**
     * Get a given room's data.
     * @return A list of the players currently in the room and their ready statuses.
     * @throw DynamicException if room doesn't exist.
     */
    std::vector<std::tuple<Address, bool>> getRoomData(uint8_t roomId) const;

    /**
     * Set the BomberGame contract address.
     * Only the contract owner can call this.
     * @param add The contract address.
     */
    void setGameAddress(const Address& add);

    /**
     * Add or remove open rooms in the lobby. Does nothing if new size is the same as old.
     * Only the contract owner can call this.
     * @param newSize The number of open rooms the lobby should have.
     * @throw DynamicException if not called by contract creator
     *        or if number is zero (at least 1 room is required).
     */
    void resizeLobby(uint8_t newSize);

    /**
     * Derivative of exitRoom() called when a player disconnects.
     * Only the contract owner can call this.
     * @param player The address of the disconnected player.
     * @param roomId The room the player was in before disconnecting.
     * @throw DynamicException if not called by contract creator or if room doesn't exist.
     */
    void playerDisconnect(const Address& player, uint8_t roomId);

    /**
     * Make a player (contract caller) enter a given room.
     * @param roomId The ID of the room to enter.
     * @throw DynamicException if room is full, doesn't exist or player is already in there.
     */
    void enterRoom(uint8_t roomId);

    /**
     * Make a player (contract caller) exit the room they're in.
     * Does nothing if player isn't in the room for some reason.
     * @param roomId The ID of the room the player is in.
     * @throw DynamicException if room doesn't exist.
     */
    void exitRoom(uint8_t roomId);

    /**
     * Make a player (contract caller) toggle its ready flag status.
     * Does nothing if player isn't in the room for some reason.
     * @param roomId The ID of the room the player is in.
     * @throw DynamicException if room doesn't exist.
     */
    void toggleReady(uint8_t roomId);

    /// Register the contract structure.
    static void registerContract() {
      static std::once_flag once;
      std::call_once(once, []() {
        DynamicContract::registerContractMethods<BomberLobby>(
          std::vector<std::string>{},
          std::make_tuple("getLobbyData", &BomberLobby::getLobbyData, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("getRoomData", &BomberLobby::getRoomData, FunctionTypes::View, std::vector<std::string>{}),
          std::make_tuple("setGameAddress", &BomberLobby::setGameAddress, FunctionTypes::NonPayable, std::vector<std::string>{"add"}),
          std::make_tuple("resizeLobby", &BomberLobby::resizeLobby, FunctionTypes::NonPayable, std::vector<std::string>{"newSize"}),
          std::make_tuple("playerDisconnect", &BomberLobby::playerDisconnect, FunctionTypes::NonPayable, std::vector<std::string>{"player", "roomId"}),
          std::make_tuple("enterRoom", &BomberLobby::enterRoom, FunctionTypes::NonPayable, std::vector<std::string>{"roomId"}),
          std::make_tuple("exitRoom", &BomberLobby::exitRoom, FunctionTypes::NonPayable, std::vector<std::string>{"roomId"}),
          std::make_tuple("toggleReady", &BomberLobby::toggleReady, FunctionTypes::NonPayable, std::vector<std::string>{"roomId"})
        );
        ContractReflectionInterface::registerContractEvents<BomberLobby>(
          std::make_tuple("gameAddressChanged", false, &BomberLobby::gameAddressChanged, std::vector<std::string>{"newAdd"}),
          std::make_tuple("lobbySizeUpdated", false, &BomberLobby::lobbySizeUpdated, std::vector<std::string>{"newSize"}),
          std::make_tuple("roomEntered", false, &BomberLobby::roomEntered, std::vector<std::string>{"player", "roomId"}),
          std::make_tuple("roomExited", false, &BomberLobby::roomExited, std::vector<std::string>{"player", "roomId"}),
          std::make_tuple("roomCleared", false, &BomberLobby::roomCleared, std::vector<std::string>{"roomId"}),
          std::make_tuple("playerReady", false, &BomberLobby::playerReady, std::vector<std::string>{"player", "roomId"}),
          std::make_tuple("playerNotReady", false, &BomberLobby::playerNotReady, std::vector<std::string>{"player", "roomId"}),
          std::make_tuple("gameStartingIn", false, &BomberLobby::gameStartingIn, std::vector<std::string>{"roomId", "secondsLeft"})
        );
      });
    }

    DBBatch dump() const override;  ///< Dump contract data to the database.
};

#endif  // BOMBERLOBBY_H
