#include "bombergame.h"

BomberGame::BomberGame(
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("BomberGame", address, creator, chainId),
  Ownable(creator, address, creator, chainId), LOBBY_ADDRESS(this)
{
  // BomberLobby address is set after deploy (we don't know which address it'll be)
  this->LOBBY_ADDRESS.commit();
  registerContractFunctions();
  this->LOBBY_ADDRESS.enableRegister();
}

BomberGame::BomberGame(
  const Address& address, const DB& db
) : DynamicContract(address, db), Ownable(address, db), LOBBY_ADDRESS(this) {
  this->LOBBY_ADDRESS = Address(db.get(std::string("LOBBY_ADDRESS"), this->getDBPrefix()));
  this->LOBBY_ADDRESS.commit();
  registerContractFunctions();
  this->LOBBY_ADDRESS.enableRegister();
}

BomberGame::~BomberGame() {};

std::shared_ptr<Game> BomberGame::getGame(const uint256_t& id) const {
  auto ret = this->activeGames.find(id);
  return (ret != this->activeGames.cend()) ? std::make_shared<Game>(ret->second) : nullptr;
}

std::shared_ptr<Player> BomberGame::getPlayerInGame(const Address& add, const std::shared_ptr<Game>& game) const {
  for (auto it = game->players.cbegin(); it != game->players.cend(); it++) {
    if (it->id == add) return std::make_shared<Player>(*it);
  }
  return nullptr;
}

void BomberGame::explodeBombInGame(const Pos& bombPos, std::shared_ptr<Game>& game) {
  // Check if cell is actually a bomb
  Cell& bomb = game->cells[Pos(bombPos.x, bombPos.y)];
  if (bomb.cellType != CellType::BOMB) {
    throw DynamicException("Game " + game->id.str() + " has no bomb on coords (" + std::to_string(bombPos.x) + "," + std::to_string(bombPos.y) + ")");
  }

  // Get game arena cell coordinates using bomb coordinates.
  // Center is always fixed, directions are always in a cross (+).
  // Check each direction extension in a separate step for ease of understanding.
  // Prefix increments/decrements are on purpose - do NOT bring them back to postfix otherwise logic breaks
  std::vector<Pos> blastCoords = { Pos(bombPos.x, bombPos.y) };
  std::vector<Pos> wallCoords = {};
  std::vector<Pos> bombCoords = {};

  // Logic for spreading the blast in each direction is as follows:
  // * Stop and don't spread to out of bounds, bombs, hard walls and soft walls that are already burning
  // * If there's a bomb in that direction, detonate it too
  // * Stop but spread to soft walls that are not burning yet
  // * If none of the above, continue extending until max blast radius is reached
  // * Air, powerup and other fire cells here are one and the same - they will be burnt along anyway so we just replace them altogether

  // Extend blast upwards
  for (uint8_t uBlast = 1; uBlast <= bomb.blastRadius; uBlast++) {
    if (bombPos.y == 0 || bombPos.y - uBlast == 0) break;
    Pos nextPos(bombPos.x, bombPos.y - uBlast);
    CellType cT = game->cells[nextPos].cellType;
    if (cT == CellType::HARDWALL || cT == CellType::FIREWALL) break;
    if (cT == CellType::SOFTWALL) { wallCoords.push_back(nextPos); break; }
    if (cT == CellType::BOMB) { bombCoords.push_back(nextPos); break; }
    blastCoords.push_back(nextPos);
  }

  // Extend blast downwards
  for (uint8_t dBlast = 1; dBlast <= bomb.blastRadius; dBlast++) {
    if (bombPos.y == game->size - 1 || bombPos.y + dBlast == game->size - 1) break;
    Pos nextPos(bombPos.x, bombPos.y + dBlast);
    CellType cT = game->cells[nextPos].cellType;
    if (cT == CellType::HARDWALL || cT == CellType::FIREWALL) break;
    if (cT == CellType::SOFTWALL) { wallCoords.push_back(nextPos); break; }
    if (cT == CellType::BOMB) { bombCoords.push_back(nextPos); break; }
    blastCoords.push_back(nextPos);
  }

  // Extend blast to the left
  for (uint8_t lBlast = 1; lBlast <= bomb.blastRadius; lBlast++) {
    if (bombPos.x == 0 || bombPos.x - lBlast == 0) break;
    Pos nextPos(bombPos.x - lBlast, bombPos.y);
    CellType cT = game->cells[nextPos].cellType;
    if (cT == CellType::HARDWALL || cT == CellType::FIREWALL) break;
    if (cT == CellType::SOFTWALL) { wallCoords.push_back(nextPos); break; }
    if (cT == CellType::BOMB) { bombCoords.push_back(nextPos); break; }
    blastCoords.push_back(nextPos);
  }

  // Extend blast to the right
  for (uint8_t rBlast = 1; rBlast <= bomb.blastRadius; rBlast++) {
    if (bombPos.x == game->size - 1 || bombPos.x + rBlast == game->size - 1) break;
    Pos nextPos(bombPos.x + rBlast, bombPos.y);
    CellType cT = game->cells[nextPos].cellType;
    if (cT == CellType::HARDWALL || cT == CellType::FIREWALL) break;
    if (cT == CellType::SOFTWALL) { wallCoords.push_back(nextPos); break; }
    if (cT == CellType::BOMB) { bombCoords.push_back(nextPos); break; }
    blastCoords.push_back(nextPos);
  }

  // Operate on each affected cell coordinate
  std::vector<std::tuple<uint8_t, uint8_t>> blastSpread = {};
  std::vector<std::tuple<uint8_t, uint8_t>> wallsBurnt = {};
  for (Pos coord : blastCoords) {
    game->cells[coord].cellType = CellType::FIRE;
    game->cells[coord].owner = bomb.owner;
    game->cells[coord].time = 1.00f;
    game->cells[coord].powerupType = PowerUpType::NONE; // Powerups are burnt along
    blastSpread.push_back(std::make_tuple(coord.x, coord.y));
  }
  for (Pos coord : wallCoords) {
    game->cells[coord].cellType = CellType::FIREWALL;
    game->cells[coord].time = 1.00f;
    wallsBurnt.push_back(std::make_tuple(coord.x, coord.x));
  }
  for (Pos coord : bombCoords) game->cells[coord].time = 0.10f;  // We just cut the bomb fuse short so it explodes on its own

  // Communicate publicly the practice of arson (because you have no shame)
  this->bombExploded(game->id, blastSpread, wallsBurnt);
}

void BomberGame::gameTickLoop(std::shared_ptr<Game>& game) {
  // Vectors for recurring event data and delay time for game end
  std::vector<std::tuple<uint8_t, uint8_t>> firesOut;
  std::vector<std::tuple<uint8_t, uint8_t, std::string>> powerupsIn;
  float gameEndTime = 1.00f;

  // Keep running the loop until game timer runs out or only one player remains
  while (game->time > 0.00f) {
    // Check for any existing fire or burning walls to convert and/or decrease the timer
    for (uint8_t i = 0; i < game->size; i++) {
      for (uint8_t j = 0; j < game->size; j++) {
        Cell& c = game->cells[Pos(i,j)];
        if (c.time <= 0.00f) {
          // Fire and burning wall cells always go back to air.
          // If a burning wall has a powerup, spawn it instead.
          // All other cell types are ignored as they don't make use of time
          if (c.cellType == CellType::FIREWALL && c.powerupType != PowerUpType::NONE) {
            c.cellType = CellType::POWERUP;
            std::string pStr = "";
            switch (c.powerupType) {
              case PowerUpType::FIREUP: pStr = "FIREUP"; break;
              case PowerUpType::BOMBUP: pStr = "BOMBUP"; break;
              case PowerUpType::SPEEDUP: pStr = "SPEEDUP"; break;
            }
            powerupsIn.push_back(std::make_tuple(i, j, pStr));
          } else if (c.cellType == CellType::FIRE || (c.cellType == CellType::FIREWALL && c.powerupType == PowerUpType::NONE)) {
            c.cellType = CellType::AIR;
            c.owner = Address();
            c.time = 0.00f;
            c.blastRadius = 0;
            c.powerupType = PowerUpType::NONE;
            firesOut.push_back(std::make_tuple(i, j));
          }
        } else {
          // Floats are innacurate so we do a little decimal rounding
          c.time = std::round((c.time - 0.01f) * 100) / 100;
        }
      }
    }

    // Emit the respective events (if there are any) and clear the vectors for next tick
    if (!firesOut.empty()) { this->firesExtinguished(game->id, firesOut); firesOut.clear(); }
    if (!powerupsIn.empty()) { this->powerupsSpawned(game->id, powerupsIn); powerupsIn.clear(); }

    // Check if a player has been blasted and kill it if so, also count down move cooldown
    for (auto playerIt = game->players.begin(); playerIt != game->players.end(); playerIt++) {
      if (playerIt->moveCool > 0.00f) {
        // Floats are innacurate so we do a little decimal rounding
        playerIt->moveCool = std::round((playerIt->moveCool - 0.01f) * 100) / 100;
      }
      if (game->cells[playerIt->pos].cellType == CellType::FIRE) {
        Address killer = game->cells[playerIt->pos].owner;
        this->playerKilled(game->id, playerIt->id, killer);
        game->players.erase(playerIt);
      }
    }

    // Check if there are any existing bombs to explode and/or decrease the timer.
    // If actually on game end timer, skip to avoid more bombs from this point forward
    // blowing up and unjustly killing the winner (this simulates original game behaviour)
    if (game->players.size() > 1 && !game->activeBombs.empty()) {
      for (Pos bombPos : game->activeBombs) {
        Cell& c = game->cells[bombPos];
        if (c.cellType != CellType::BOMB) continue;
        if (c.time <= 0.00f) {
          this->explodeBombInGame(bombPos, game);
          game->activeBombs.erase(std::find(game->activeBombs.begin(), game->activeBombs.end(), bombPos));
        } else {
          // Floats are innacurate so we do a little decimal rounding
          c.time = std::round((c.time - 0.01f) * 100) / 100;
        }
      }
    }

    // Game runs in 0.1s ticks, hardcoded for simplicity purposes.
    // When there's one player left, wait a bit to see if they won't also die due to "afterburners"
    // (being caught by leftover fire, this simulates original game behaviour)
    if (game->players.size() > 1) {
      // Floats are innacurate so we do a little decimal rounding
      game->time = std::round((game->time - 0.01f) * 100) / 100;
      if (game->time - std::trunc(game->time) == 0) {
        this->gameTimeUpdate(std::trunc(game->time)); // Signal time left every second
      }
    } else {
      // Floats are innacurate so we do a little decimal rounding
      gameEndTime = std::round((gameEndTime - 0.01f) * 100) / 100;
      if (gameEndTime <= 0.00f) break;  // Game over!
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 1 tick = 0.01s
  }

  // Define the winner or draw based on who's left in the player list.
  // If player list is empty by then it is considered a draw (we return an empty address to signal that).
  // This includes the rare edge case where all players somehow
  // disconnect or die at the exact same moment (there's no winner if no one's there)
  Address winner = (game->players.size() == 1) ? game->players.back().id : Address();
  this->gameEnded(game->id, winner);

  // Destroy the game
  for (auto gameIt = this->activeGames.begin(); gameIt != this->activeGames.end(); gameIt++) {
    if (gameIt->first == game->id) { this->activeGames.erase(gameIt); break; }
  }
}

// ===========================================================================

void BomberGame::setLobbyAddress(const Address& add) {
  this->onlyOwner(); this->LOBBY_ADDRESS = add; this->lobbyAddressChanged(this->LOBBY_ADDRESS.get());
}

void BomberGame::createGame(const std::vector<Address>& players) {
  if (this->LOBBY_ADDRESS == Address()) {
    throw DynamicException("Lobby contract address is not set");
  }
  if (this->getCaller() != this->LOBBY_ADDRESS) {
    throw DynamicException("Only the lobby contract can call this function");
  }

  // Create the game and initialize the RNG
  RandomGen r(Hash(bytes::random()));
  Game g;
  g.id = r.getSeed().operator uint256_t();
  g.size = this->GAME_SIZE;
  g.time = this->GAME_TIME;

  // Generate the arena cells (air, hard and soft walls)
  for (uint8_t i = 0; i < g.size; i++) {
    for (uint8_t j = 0; j < g.size; j++) {
      // Always generate an air cell by default and a hard wall for every odd cell (e.g. (1,1), (1,3), (3,5), etc.)
      Cell c;
      c.cellType = (i % 2 != 0 && j % 2 != 0) ? CellType::HARDWALL : CellType::AIR;
      c.owner = Address();
      c.time = 0.0f;
      c.blastRadius = 0;
      c.powerupType = PowerUpType::NONE;

      // For air cells, try generating wall cells based on RandomGen output.
      // Pick a random number between 0.0 and "100.0" (uint256::max()).
      // If hit < 75.0 (75% = (num * 4) / 3), air cell becomes a soft wall.
      // If hit < 25.0 (25% = num / 4), said soft wall should spawn a random powerup when blown up.
      // PS1 Bomberman seems to use 70% and 27.5% respectively (deduced by looking at a random gameplay and crunching the numbers)
      if (c.cellType == AIR) {
        uint256_t rand = r();
        if (rand < (r.max() / 4) * 3) {
          c.cellType = CellType::SOFTWALL;
          if (rand < r.max() / 4) {
            switch ((int)(rand % 3)) {
              case 0: c.powerupType = PowerUpType::FIREUP; break;
              case 1: c.powerupType = PowerUpType::BOMBUP; break;
              case 2: c.powerupType = PowerUpType::SPEEDUP; break;
            }
          }
        }
      }

      // Store the cell in the matrix
      g.cells[Pos(i,j)] = std::move(c);
    }
  }

  // Add players to arena on random specific spawn points (corners and center).
  // Also clear the space around their spawn points of walls if there are any
  std::vector<uint8_t> spawns = {0, 1, 2, 3, 4};
  r.setSeed(Hash(g.id));  // Reset RandomGen to pre-cell generation state
  r.shuffle(spawns);
  for (const Address& add : players) {
    Player p;
    p.id = add;
    p.spd = 0;
    p.moveCool = 0.0f;
    p.maxBombs = 1;
    p.maxBlastRadius = 1;
    switch (spawns.back()) {
      case 0: // Top-left corner
        p.pos.x = 0;
        p.pos.y = 0;
        g.cells[Pos(p.pos.x+1,p.pos.y)].cellType = CellType::AIR; // Right
        g.cells[Pos(p.pos.x,p.pos.y+1)].cellType = CellType::AIR; // Down
        break;
      case 1: // Top-right corner
        p.pos.x = g.size - 1;
        p.pos.y = 0;
        g.cells[Pos(p.pos.x-1,p.pos.y)].cellType = CellType::AIR; // Left
        g.cells[Pos(p.pos.x,p.pos.y+1)].cellType = CellType::AIR; // Down
        break;
      case 2: // Bottom-left corner
        p.pos.x = 0;
        p.pos.y = g.size - 1;
        g.cells[Pos(p.pos.x+1,p.pos.y)].cellType = CellType::AIR; // Right
        g.cells[Pos(p.pos.x,p.pos.y-1)].cellType = CellType::AIR; // Up
        break;
      case 3: // Bottom-right corner
        p.pos.x = g.size - 1;
        p.pos.y = g.size - 1;
        g.cells[Pos(p.pos.x-1,p.pos.y)].cellType = CellType::AIR; // Left
        g.cells[Pos(p.pos.x,p.pos.y-1)].cellType = CellType::AIR; // Up
        break;
      case 4: // Center
        p.pos.x = g.size / 2;  // Values are truncated (no decimals)
        p.pos.y = g.size / 2;
        g.cells[Pos(p.pos.x+1,p.pos.y)].cellType = CellType::AIR; // Right
        g.cells[Pos(p.pos.x-1,p.pos.y)].cellType = CellType::AIR; // Left
        g.cells[Pos(p.pos.x,p.pos.y+1)].cellType = CellType::AIR; // Down
        g.cells[Pos(p.pos.x,p.pos.y-1)].cellType = CellType::AIR; // Up
        break;
    }
    g.cells[Pos(p.pos.x,p.pos.y)].cellType = CellType::AIR; // Don't forget the spawn point itself!
    spawns.pop_back(); // Next random value
    g.players.push_back(std::move(p));
  }

  // Collect data for the game creation event
  uint256_t gameId = g.id;
  uint8_t gameTime = g.time;
  std::vector<Address> gamePlayers;
  std::vector<std::tuple<uint8_t, uint8_t, std::string>> gameCells;
  for (Player p : g.players) gamePlayers.push_back(p.id);
  for (uint8_t i = 0; i < g.size; i++) {
    for (uint8_t j = 0; j < g.size; j++) {
      std::string cellTypeStr = "";
      switch (g.cells[Pos(i,j)].cellType) {
        case CellType::HARDWALL: cellTypeStr = "HARDWALL"; break;
        case CellType::SOFTWALL: cellTypeStr = "SOFTWALL"; break;
        case CellType::AIR: cellTypeStr = "AIR"; break;
      }
      gameCells.push_back(std::make_tuple(i, j, cellTypeStr));
    }
  }

  // Create the game (but don't start it yet!) and emit the event
  this->activeGames[g.id] = std::move(g);
  this->gameCreated(std::make_tuple(gameId, gameTime), gamePlayers, gameCells);
}

void BomberGame::playerDisconnect(const Address& player, const uint256_t& gameId) {
  this->onlyOwner();
  // Check if game exists
  std::shared_ptr<Game> g = this->getGame(gameId);
  if (g == nullptr) throw DynamicException("Game " + gameId.str() + " not found");

  // If a player disconnects at any time while in a game, treat it as "being killed".
  // The "killer" is the contract creator itself (that signals a disconnect)
  for (auto playerIt = g->players.begin(); playerIt != g->players.end(); playerIt++) {
    if (playerIt->id == player) {
      this->playerKilled(g->id, playerIt->id, this->getContractCreator());
      g->players.erase(playerIt);
      break;
    }
  }
}

void BomberGame::startGame(const uint256_t& gameId) {
  this->onlyOwner();
  // Check if game actually was created
  std::shared_ptr<Game> g = this->getGame(gameId);
  if (g == nullptr) throw DynamicException("Game " + gameId.str() + " does not exist");

  // Start the game's tick loop
  [[maybe_unused]] std::future<void> f = std::async([this, &g](){ this->gameTickLoop(g); });
  this->gameStarted(gameId);
}

void BomberGame::move(const uint256_t& gameId, Direction dir) {
  // Check if game is going on and if player is actually playing that game (not dead or disconnected)
  std::shared_ptr<Game> g = this->getGame(gameId);
  if (g == nullptr) throw DynamicException("Game " + gameId.str() + " not found");
  if (g->time >= this->GAME_TIME) throw DynamicException("Game " + gameId.str() + " hasn't started yet");
  std::shared_ptr<Player> p = this->getPlayerInGame(this->getCaller(), g);
  if (p == nullptr) throw DynamicException("Player " + this->getCaller().hex().get() + " not found in game " + gameId.str());

  // Check if player can actually move (cooldown ended)
  if (p->moveCool > 0.00f) throw DynamicException("Player " + this->getCaller().hex().get() + " moving too fast");

  // Simulate the new position based on the given direction, doing sanity checks to ensure the player can actually move there
  uint8_t newX = p->pos.x;
  uint8_t newY = p->pos.y;
  std::string err = "";

  // Check for movement out of bounds
  switch (dir) {
    case Direction::UP:
      if (newY == 0) err = "Player " + this->getCaller().hex().get() + " moving out of bounds (up)"; else newY -= 1;
      break;
    case Direction::DOWN:
      if (newY == g->size - 1) err = "Player " + this->getCaller().hex().get() + " moving out of bounds (down)"; else newY += 1;
      break;
    case Direction::LEFT:
      if (newX == 0) err = "Player " + this->getCaller().hex().get() + " moving out of bounds (left)"; else newX -= 1;
      break;
    case Direction::RIGHT:
      if (newY == g->size - 1) err = "Player " + this->getCaller().hex().get() + " moving out of bounds (right)"; else newX += 1;
      break;
    default:
      err = "Player " + this->getCaller().hex().get() + " doing invalid input direction";
      break;
  }
  if (err != "") throw DynamicException(err);

  // Then check if the target cell is not a solid (walls or bombs)
  err = "";
  switch (g->cells[Pos(newX,newY)].cellType) {
    case CellType::HARDWALL:
    case CellType::SOFTWALL: err = "Player " + this->getCaller().hex().get() + " moving towards a wall"; break;
    case CellType::BOMB: err = "Player " + this->getCaller().hex().get() + " moving towards a placed bomb"; break;
    default: break;
  }
  if (err != "") throw DynamicException(err);

  // Actually move the player now (and reset the cooldown)
  // Floats are innacurate so we do a little decimal rounding
  p->pos.x = newX;
  p->pos.y = newY;
  p->moveCool = 0.50f - (p->spd * 0.05f);
  p->moveCool = std::round(p->moveCool * 100) / 100;
  this->playerMoved(g->id, p->id, std::make_tuple(p->pos.x, p->pos.y));

  // Check if player picked up a powerup
  if (g->cells[p->pos].cellType == CellType::POWERUP) {
    // Give the powerup to the player
    PowerUpType pT = g->cells[p->pos].powerupType;
    std::string pS = "";
    switch (pT) {
      // Player stats are maxed at 9
      case PowerUpType::FIREUP: if (p->maxBlastRadius < 9) p->maxBlastRadius += 1; pS = "FIREUP"; break;
      case PowerUpType::BOMBUP: if (p->maxBombs < 9) p->maxBombs += 1; pS = "BOMBUP"; break;
      case PowerUpType::SPEEDUP: if (p->spd < 9) p->spd += 1; pS = "SPEEDUP"; break;
      default: break;
    }
    // Convert the powerup cell back to air and signal that the player picked it up
    g->cells[p->pos].cellType = CellType::AIR;
    g->cells[p->pos].powerupType = PowerUpType::NONE;
    this->powerupGotten(g->id, p->id, pS);
  }
}

void BomberGame::placeBomb(const uint256_t& gameId) {
  // Check if game is going on and if player is actually playing that game (not dead or disconnected)
  std::shared_ptr<Game> g = this->getGame(gameId);
  if (g == nullptr) throw DynamicException("Game " + gameId.str() + " not found");
  if (g->time >= this->GAME_TIME) throw DynamicException("Game " + gameId.str() + " hasn't started yet");
  std::shared_ptr<Player> p = this->getPlayerInGame(this->getCaller(), g);
  if (p == nullptr) throw DynamicException("Player " + this->getCaller().hex().get() + " not found in game " + gameId.str());

  // Check if player is on an empty space (could have just placed a bomb and trying to place it again on the same cell)
  if (g->cells[p->pos].cellType != CellType::AIR) {
    throw DynamicException("Player " + this->getCaller().hex().get() + " can't place bomb (not an empty space)");
  }

  // Check if player can actually deploy another bomb
  uint8_t playerBombs = 0;
  for (unsigned int i = 0; i < g->size; i++) {
    for (unsigned int j = 0; j < g->size; j++) {
      if (g->cells[Pos(i,j)].cellType == CellType::BOMB && g->cells[Pos(i,j)].owner == p->id) playerBombs++;
      if (playerBombs >= p->maxBombs) throw DynamicException(
        "Player " + this->getCaller().hex().get() + " can't place bomb (max " + std::to_string(p->maxBombs) + ")"
      );
    }
  }

  // Replace the cell the player is standing on with a bomb.
  // All bombs automatically count down during the game tick loop,
  // so all we have to do is add the newly-placed bomb to the active bombs list
  g->cells[p->pos].cellType = CellType::BOMB;
  g->cells[p->pos].owner = p->id;
  g->cells[p->pos].time = 2.00f;
  g->cells[p->pos].blastRadius = p->maxBlastRadius;
  g->activeBombs.push_back(p->pos);
  this->bombPlaced(g->id, p->id, std::make_tuple(p->pos.x, p->pos.y));
}

void BomberGame::registerContractFunctions() {
  registerContract();
  this->registerMemberFunctions(
    std::make_tuple("setLobbyAddress", &BomberGame::setLobbyAddress, FunctionTypes::NonPayable, this),
    std::make_tuple("createGame", &BomberGame::createGame, FunctionTypes::NonPayable, this),
    std::make_tuple("playerDisconnect", &BomberGame::playerDisconnect, FunctionTypes::NonPayable, this),
    std::make_tuple("startGame", &BomberGame::startGame, FunctionTypes::NonPayable, this),
    std::make_tuple("move", &BomberGame::move, FunctionTypes::NonPayable, this),
    std::make_tuple("placeBomb", &BomberGame::placeBomb, FunctionTypes::NonPayable, this)
  );
}

DBBatch BomberGame::dump() const {
  // We need to dump all the data from the parent classes as well
  DBBatch dbBatch = BaseContract::dump();
  const DBBatch ownableDump = Ownable::dump();
  for (const auto& dbItem : ownableDump.getPuts()) dbBatch.push_back(dbItem);
  for (const auto& dbItem : ownableDump.getDels()) dbBatch.delete_key(dbItem);

  // Then we dump the class' contents
  dbBatch.push_back(StrConv::stringToBytes("LOBBY_ADDRESS"), this->LOBBY_ADDRESS.get().asBytes(), this->getDBPrefix());
  return dbBatch;
}

