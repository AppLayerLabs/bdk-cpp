#ifndef BOMBERGAME_H
#define BOMBERGAME_H

#include "ownable.h" // includes SafeAddress

#include "../variables/safeunorderedmap.h"

#include "../../utils/randomgen.h" // shuffle()

#include "../../bytes/random.h"

#include <algorithm>  // find
#include <cmath>
#include <chrono>
#include <cstdint>
#include <future>
#include <thread>
#include <tuple>
#include <utility>  // pair
#include <vector>

// ===========================================================================
// HELPER ENUMS
// ===========================================================================

/// Enum for the arena cell types.
enum CellType { AIR, FIRE, HARDWALL, SOFTWALL, FIREWALL, BOMB, POWERUP };

/// Enum for the powerup types.
enum PowerUpType { NONE = -1, FIREUP, BOMBUP, SPEEDUP };

/// Enum for the basic directions (for both players and fire spread).
enum Direction { CENTER, UP, DOWN, LEFT, RIGHT };

// ===========================================================================
// HELPER STRUCTS/CLASSES
// ===========================================================================

/// Helper struct for defining position coordinates.
struct Pos {
  uint8_t x;  ///< X coordinate.
  uint8_t y;  ///< Y coordinate.
  Pos() {};  ///< Empty constructor.
  Pos(uint8_t xPos, uint8_t yPos) : x(xPos), y(yPos) {};  ///< Constructor.
  //bool operator<=>(const Pos& other) { return this->x <=> other.x && this->y <=> other.y; }  ///< Spaceship operator.
  auto operator<=>(const Pos&) const = default; ///< Spaceship operator.
};

/// Cell object.
struct Cell {
  CellType cellType;        ///< The type of the cell. Used to identify every cell type.
  Address owner;            ///< Address of the player that set the cell. Used by fire and bomb cells.
  float time;               ///< How many seconds left for the cell to "expire". Used by bomb, fire and burning soft wall cells.
  uint8_t blastRadius;      ///< How many cells to extend towards. Used by bomb cells only.
  PowerUpType powerupType;  ///< Which kind of powerup the cell should spawn. Used by soft wall and powerup cells.
};

/// Player object. Separate from the cells since it moves around.
struct Player {
  Address id;               ///< The address of the player.
  Pos pos;                  ///< The current position of the player.
  uint8_t spd;              ///< Movement speed. Affects move cooldown.
  float moveCool;           ///< Movement cooldown. Affects how fast the player can send inputs.
  uint8_t maxBombs;         ///< Maximum number of bombs the player can deploy at once.
  uint8_t maxBlastRadius;   ///< Maximum blast radius the player's bombs will have (passed to Bomb object).
};

/// Game object. Contains all cells and players. You could also call it "Arena".
struct Game {
  uint256_t id;                 ///< The game's unique ID.
  uint8_t size;                 ///< The game's arena size.
  float time;                   ///< How many seconds left for the game to end.
  std::vector<Player> players;  ///< The list of players in the game.
  std::map<Pos, Cell> cells;    ///< The matrix of cells in the game.
  std::vector<Pos> activeBombs; ///< A list of currently placed bombs in the game.
};

// ===========================================================================
// CONTRACT STARTS HERE
// ===========================================================================

/**
 * Main contract for a Bomberman-esque game clone.
 * Game logic is entirely on chain.
 */
class BomberGame : public virtual DynamicContract, public Ownable {
  private:
    // Those are the baseline minimum values for a Bomberman-esque clone.
    // They could be scaled but were kept on the lows so the game is playable on mobile.
    uint8_t GAME_SIZE = 13;   ///< Size of the game arena as a square (e.g. 13x13). MUST be an odd number due to wall placement.
    uint8_t GAME_TIME = 150;  ///< Total time in seconds for running a game (2.5 minutes, original Bomberman is 180s).

    SafeAddress LOBBY_ADDRESS;  ///< Address for the BomberLobby contract.
    SafeUnorderedMap<uint256_t, Game> activeGames;  ///< List of concurrent games currently going on.

    /**
     * Helper function to get a given game based on its ID.
     * @param id The ID of the game to get.
     * @return A pointer to the game object, or nullptr if it doesn't exist.
     */
    std::shared_ptr<Game> getGame(const uint256_t& id) const;

    /**
     * Helper function to get a given player that is playing a given game (as in not dead).
     * @param add The address of the player.
     * @param game A pointer to the game the player is in.
     * @return A pointer to the player object, or nullptr if the player is not found.
     */
    std::shared_ptr<Player> getPlayerInGame(const Address& add, const std::shared_ptr<Game>& game) const;

    /**
     * Explode a given bomb in a given game.
     * @param bombPos The position of the bomb cell in the arena.
     * @param game A pointer to the game where the bomb will explode.
     * @throw DynamicException if cell is not a bomb.
     */
    void explodeBombInGame(const Pos& bombPos, std::shared_ptr<Game>& game);

    /**
     * Helper function for the game loop itself. Meant to run in a separate thread.
     * @param game A pointer to the game the loop will run for.
     */
    void gameTickLoop(std::shared_ptr<Game>& game);

    void registerContractFunctions() override;  ///< Register contract functions.

  public:
    /// Event for when the BomberLobby address is changed.
    void lobbyAddressChanged(const EventParam<Address, false>& newAdd) {
      this->emitEvent(__func__, std::make_tuple(newAdd));
    }

    /// Event for when a game is created (but not started).
    void gameCreated(
      const EventParam<std::tuple<uint256_t, uint8_t>, true>& idAndTime,
      const EventParam<std::vector<Address>, false>& players,
      const EventParam<std::vector<std::tuple<uint8_t, uint8_t, std::string>>, false>& cells
    ) {
      this->emitEvent(__func__, std::make_tuple(idAndTime, players, cells));
    }

    /// Event for when a game starts.
    void gameStarted(const EventParam<uint256_t, true>& id) {
      this->emitEvent(__func__, std::make_tuple(id));
    }

    /// Event for each second passed in the game.
    void gameTimeUpdate(const EventParam<uint8_t, false>& secsLeft) {
      this->emitEvent(__func__, std::make_tuple(secsLeft));
    }

    /// Event for when a player has moved in a game.
    void playerMoved(
      const EventParam<uint256_t, true>& gameId,
      const EventParam<Address, true>& player,
      const EventParam<std::tuple<uint8_t, uint8_t>, false>& newPos
    ) {
      this->emitEvent(__func__, std::make_tuple(gameId, player, newPos));
    }

    /// Event for when a player picks up a powerup in a game.
    void powerupGotten(
      const EventParam<uint256_t, true>& gameId,
      const EventParam<Address, true>& player,
      const EventParam<std::string, false>& powerup
    ) {
      this->emitEvent(__func__, std::make_tuple(gameId, player, powerup));
    }

    /// Event for when a player has placed a bomb.
    void bombPlaced(
      const EventParam<uint256_t, true>& gameId,
      const EventParam<Address, true>& player,
      const EventParam<std::tuple<uint8_t, uint8_t>, false>& bombPos
    ) {
      this->emitEvent(__func__, std::make_tuple(gameId, player, bombPos));
    }

    /// Event for when a bomb explodes. First blast spread coord is always the center (where the bomb was).
    void bombExploded(
      const EventParam<uint256_t, true>& gameId,
      const EventParam<std::vector<std::tuple<uint8_t, uint8_t>>, false>& blastSpread,
      const EventParam<std::vector<std::tuple<uint8_t, uint8_t>>, false>& wallsBurnt
    ) {
      this->emitEvent(__func__, std::make_tuple(gameId, blastSpread, wallsBurnt));
    }

    /// Event for when one or more fire/firewall cells run out of burn time and get extinguished.
    void firesExtinguished(
      const EventParam<uint256_t, true>& gameId,
      const EventParam<std::vector<std::tuple<uint8_t, uint8_t>>, false>& fireCells
    ) {
      this->emitEvent(__func__, std::make_tuple(gameId, fireCells));
    }

    /// Event for when one or more powerups spawn from burnt soft walls.
    void powerupsSpawned(
      const EventParam<uint256_t, true>& gameId,
      const EventParam<std::vector<std::tuple<uint8_t, uint8_t, std::string>>, false>& powerupCells
    ) {
      this->emitEvent(__func__, std::make_tuple(gameId, powerupCells));
    }

    /// Event for when a player dies.
    void playerKilled(
      const EventParam<uint256_t, true>& gameId,
      const EventParam<Address, true>& killed,
      const EventParam<Address, true>& killer
    ) {
      this->emitEvent(__func__, std::make_tuple(gameId, killed, killer));
    }

    /// Event for when the game ends. If winner is an empty address, it's a draw.
    void gameEnded(
      const EventParam<uint256_t, true>& gameId,
      const EventParam<Address, true>& winner
    ) {
      this->emitEvent(__func__, std::make_tuple(gameId, winner));
    }

    using ConstructorArguments = std::tuple<>;  ///< Constructor argument types.

    /**
     * Constructor from create. Create contract and save it to database.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    BomberGame(const Address& address, const Address& creator, const uint64_t& chainId);

    /**
     * Constructor from load. Load contract from database.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    BomberGame(const Address& address, const DB& db);

    ~BomberGame();  ///< Destructor.

    /**
     * Set the BomberLobby contract address.
     * Only the contract owner can call this.
     * @param add The contract address.
     */
    void setLobbyAddress(const Address& add);

    /**
     * Create a game object with the given player list.
     * Only the BomberLobby contract can call this function.
     * @param players The list of players that will play the game.
     * @throw DynamicException if lobby contract is not set.
     */
    void createGame(const std::vector<Address>& players);

    /**
     * Kick a player out of the game if a disconnect happens.
     * Only the contract owner can call this.
     * Does nothing if player is already out of the game.
     * @param player The address of the disconnected player.
     * @param gameId The ID of the game the player was in before disconnecting.
     * @throw DynamicException if game does not exist.
     */
    void playerDisconnect(const Address& player, const uint256_t& gameId);

    /**
     * Start a given game's tick loop. Called by contract creator only.
     * Only the contract owner can call this.
     * @param gameId The ID of the game to start.
     * @throw DynamicException if game does not exist.
     */
    void startGame(const uint256_t& gameId);

    /**
     * Move a given player (contract caller) one cell towards the respective direction.
     * @param gameId The ID of the game the player is in.
     * @param dir The direction the player wants to move to.
     * @throw DynamicException if movement can't be done (e.g. out of bounds or towards a solid cell like a bomb or a wall).
     */
    void move(const uint256_t& gameId, Direction dir);

    /**
     * Make the player (contract caller) place a bomb in the cell they're standing on.
     * @param gameId The ID of the game the player is in.
     * @throw DynamicException if bomb placement can't be done (e.g. max bombs placed or not on am empty space).
     */
    void placeBomb(const uint256_t& gameId);

    /// Register the contract structure.
    static void registerContract() {
      static std::once_flag once;
      std::call_once(once, []() {
        DynamicContract::registerContractMethods<BomberGame>(
          std::vector<std::string>{},
          std::make_tuple("setLobbyAddress", &BomberGame::setLobbyAddress, FunctionTypes::NonPayable, std::vector<std::string>{"add"}),
          std::make_tuple("createGame", &BomberGame::createGame, FunctionTypes::NonPayable, std::vector<std::string>{"players"}),
          std::make_tuple("playerDisconnect", &BomberGame::playerDisconnect, FunctionTypes::NonPayable, std::vector<std::string>{"player", "gameId"}),
          std::make_tuple("startGame", &BomberGame::startGame, FunctionTypes::NonPayable, std::vector<std::string>{"gameId"}),
          std::make_tuple("move", &BomberGame::move, FunctionTypes::NonPayable, std::vector<std::string>{"gameId", "dir"}),
          std::make_tuple("placeBomb", &BomberGame::placeBomb, FunctionTypes::NonPayable, std::vector<std::string>{"gameId"})
        );
        ContractReflectionInterface::registerContractEvents<BomberGame>(
          std::make_tuple("lobbyAddressChanged", false, &BomberGame::lobbyAddressChanged, std::vector<std::string>{"newAdd"}),
          std::make_tuple("gameCreated", false, &BomberGame::gameCreated, std::vector<std::string>{"idAndTime", "players", "cells"}),
          std::make_tuple("gameStarted", false, &BomberGame::gameStarted, std::vector<std::string>{"id"}),
          std::make_tuple("gameTimeUpdate", false, &BomberGame::gameTimeUpdate, std::vector<std::string>{"secsLeft"}),
          std::make_tuple("playerMoved", false, &BomberGame::playerMoved, std::vector<std::string>{"gameId", "player", "newPos"}),
          std::make_tuple("powerupGotten", false, &BomberGame::powerupGotten, std::vector<std::string>{"gameId", "player", "powerup"}),
          std::make_tuple("bombPlaced", false, &BomberGame::bombPlaced, std::vector<std::string>{"gameId", "player", "bombPos"}),
          std::make_tuple("bombExploded", false, &BomberGame::bombExploded, std::vector<std::string>{"gameId", "blastSpread", "wallsBurnt"}),
          std::make_tuple("firesExtinguished", false, &BomberGame::firesExtinguished, std::vector<std::string>{"gameId", "fireCells"}),
          std::make_tuple("powerupsSpawned", false, &BomberGame::powerupsSpawned, std::vector<std::string>{"gameId", "powerupCells"}),
          std::make_tuple("playerKilled", false, &BomberGame::playerKilled, std::vector<std::string>{"gameId", "killed", "killer"}),
          std::make_tuple("gameEnded", false, &BomberGame::gameEnded, std::vector<std::string>{"gameId", "winner"})
        );
      });
    }

    DBBatch dump() const override;  ///< Dump contract data to the database.
};

#endif  // BOMBERGAME_H
