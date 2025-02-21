#include "btvcommon.h"


namespace BTVUtils {
  /**
   * Check if a given block is close to another block
   * a block is to be considered placed on the "middle" of the area based on distance
   * For example, if distance is 1, then it will check a 3x3x3 area around the block with 'a' as the center
   * If distance is 2, then it will check a 5x5x5 area around the block with 'a' as the center
   */
  bool isBlockClose(const WorldBlockPos& a, const WorldBlockPos& b, int distance) {
    return std::abs(a.x - b.x) <= distance &&
           std::abs(a.y - b.y) <= distance &&
           std::abs(a.z - b.z) <= distance;
  }

  World::World() {
    const int half = NUM_CHUNKS / 2; // 32
    const int minC = -half;         // -32
    const int maxC = minC + NUM_CHUNKS; // -32 + 64 = 32

    for (int cx = minC; cx < maxC; cx++) {
      for (int cy = minC; cy < maxC; cy++) {
        ChunkCoord2D cc{ (int32_t)cx, (int32_t)cy };
        Chunk chunk; // defaults to AIR
        chunks.emplace(cc, std::move(chunk));
      }
    }
    for (int lx = 0; lx < 10; lx++) {
      for (int lz = 0; lz < 10; lz++) {
        // Fill 10x10 area at y=5 with SURFACE
        WorldBlockPos pos{ lx, 5, lz };
        LocalBlockPos lpos = worldToLocal(pos);
        chunks.at({ lpos.cx, lpos.cy }).blocks[lpos.x][lpos.y][lpos.z].type = BlockType::SURFACE;
      }
    }
  }
  // Constructor (with DynamicContract* as argument to initialize this->chunks)
  World::World(DynamicContract* contract) : chunks(contract) {
    const int half = NUM_CHUNKS / 2; // 32
    const int minC = -half;         // -32
    const int maxC = minC + NUM_CHUNKS; // -32 + 64 = 32

    for (int cx = minC; cx < maxC; cx++) {
      for (int cy = minC; cy < maxC; cy++) {
        ChunkCoord2D cc{ (int32_t)cx, (int32_t)cy };
        Chunk chunk; // defaults to AIR

        // Special case chunk(0,0): fill 10x10 area at y=5 with SURFACE
        if (cx == 0 && cy == 0) {
          for (int lx = 0; lx < 10; lx++) {
            for (int lz = 0; lz < 10; lz++) {
              chunk.blocks[lx][5][lz].type = BlockType::SURFACE;
            }
          }
        }
        chunks.emplace(cc, std::move(chunk));
      }
    }
  }

  LocalBlockPos World::worldToLocal(const WorldBlockPos& wpos) {
    auto floorDiv = [](int64_t val, int64_t size) {
      if (val >= 0) {
        return val / size;
      } else {
        return (val - (size - 1)) / size;
      }
    };
    int64_t cx = floorDiv(wpos.x, CHUNK_SIZE_X);
    int64_t cy = floorDiv(wpos.z, CHUNK_SIZE_Z);

    int64_t lx = wpos.x - (cx * CHUNK_SIZE_X);
    int64_t lz = wpos.z - (cy * CHUNK_SIZE_Z);
    int64_t ly = wpos.y;

    LocalBlockPos lpos;
    lpos.cx = cx;
    lpos.cy = cy;
    lpos.x  = lx;
    lpos.y  = ly;
    lpos.z  = lz;
    return lpos;
  }

  WorldBlockPos World::localToWorld(const LocalBlockPos& lpos) {
    WorldBlockPos wpos;
    wpos.x = lpos.cx * CHUNK_SIZE_X + lpos.x;
    wpos.y = lpos.y;
    wpos.z = lpos.cy * CHUNK_SIZE_Z + lpos.z;
    return wpos;
  }

  const Block *const World::getBlock(const LocalBlockPos& lp) const {
    ChunkCoord2D key{ (int32_t)lp.cx, (int32_t)lp.cy };
    auto it = chunks.find(key);
    if (it == chunks.cend()) {
      return nullptr;
    }
    const Chunk& c = it->second;

    if (lp.x < 0 || lp.x >= Chunk::WIDTH)   return nullptr;
    if (lp.y < 0 || lp.y >= Chunk::HEIGHT)  return nullptr;
    if (lp.z < 0 || lp.z >= Chunk::LENGTH)  return nullptr;

    return &c.blocks[lp.x][lp.y][lp.z];
  }

  const Block *const World::getBlock(const WorldBlockPos& wp) const {
    LocalBlockPos lp = worldToLocal(wp);
    return getBlock(lp);
  }

  Block *const World::getBlock(const LocalBlockPos& lp) {
    ChunkCoord2D key{ (int32_t)lp.cx, (int32_t)lp.cy };
    auto it = chunks.find(key);
    if (it == chunks.end()) {
      return nullptr;
    }
    Chunk& c = it->second;

    if (lp.x < 0 || lp.x >= Chunk::WIDTH)   return nullptr;
    if (lp.y < 0 || lp.y >= Chunk::HEIGHT)  return nullptr;
    if (lp.z < 0 || lp.z >= Chunk::LENGTH)  return nullptr;

    return &c.blocks[lp.x][lp.y][lp.z];
  }

  Block *const World::getBlock(const WorldBlockPos& wp) {
    LocalBlockPos lp = worldToLocal(wp);
    return getBlock(lp);
  }

  ConstNeighborBlocksLocal World::getNeighborsLocal(const LocalBlockPos& base) const {
    return {
        { getBlock(shiftLocalPos(base, +1, 0, 0)), shiftLocalPos(base, +1, 0, 0) },
        { getBlock(shiftLocalPos(base, -1, 0, 0)), shiftLocalPos(base, -1, 0, 0) },
        { getBlock(shiftLocalPos(base, 0, +1, 0)), shiftLocalPos(base, 0, +1, 0) },
        { getBlock(shiftLocalPos(base, 0, -1, 0)), shiftLocalPos(base, 0, -1, 0) },
        { getBlock(shiftLocalPos(base, 0, 0, -1)), shiftLocalPos(base, 0, 0, -1) },
        { getBlock(shiftLocalPos(base, 0, 0, +1)), shiftLocalPos(base, 0, 0, +1) }
    };
  }

  ConstNeighborBlocksWorld World::getNeighborsWorld(const WorldBlockPos& wpos) const {
    return {
        { getBlock(WorldBlockPos{wpos.x + 1, wpos.y, wpos.z}), {wpos.x + 1, wpos.y, wpos.z} },
        { getBlock(WorldBlockPos{wpos.x - 1, wpos.y, wpos.z}), {wpos.x - 1, wpos.y, wpos.z} },
        { getBlock(WorldBlockPos{wpos.x, wpos.y + 1, wpos.z}), {wpos.x, wpos.y + 1, wpos.z} },
        { getBlock(WorldBlockPos{wpos.x, wpos.y - 1, wpos.z}), {wpos.x, wpos.y - 1, wpos.z} },
        { getBlock(WorldBlockPos{wpos.x, wpos.y, wpos.z - 1}), {wpos.x, wpos.y, wpos.z - 1} },
        { getBlock(WorldBlockPos{wpos.x, wpos.y, wpos.z + 1}), {wpos.x, wpos.y, wpos.z + 1} }
    };
  }

  ConstNeighborBlocksLocal World::getNeighborsLocal(const WorldBlockPos& wpos) const {
    LocalBlockPos lp = worldToLocal(wpos);
    return getNeighborsLocal(lp);
  }

  NeighborBlocksLocal World::getNeighborsLocal(const WorldBlockPos& wpos) {
    LocalBlockPos lp = worldToLocal(wpos);
    return getNeighborsLocal(lp);
  }

  NeighborBlocksLocal World::getNeighborsLocal(const LocalBlockPos& base) {
    return {
        { getBlock(shiftLocalPos(base, +1, 0, 0)), shiftLocalPos(base, +1, 0, 0) },
        { getBlock(shiftLocalPos(base, -1, 0, 0)), shiftLocalPos(base, -1, 0, 0) },
        { getBlock(shiftLocalPos(base, 0, +1, 0)), shiftLocalPos(base, 0, +1, 0) },
        { getBlock(shiftLocalPos(base, 0, -1, 0)), shiftLocalPos(base, 0, -1, 0) },
        { getBlock(shiftLocalPos(base, 0, 0, -1)), shiftLocalPos(base, 0, 0, -1) },
        { getBlock(shiftLocalPos(base, 0, 0, +1)), shiftLocalPos(base, 0, 0, +1) }
    };
  }
  NeighborBlocksWorld World::getNeighborsWorld(const WorldBlockPos& wpos) {
    return {
        { getBlock(WorldBlockPos{wpos.x + 1, wpos.y, wpos.z}), {wpos.x + 1, wpos.y, wpos.z} },
        { getBlock(WorldBlockPos{wpos.x - 1, wpos.y, wpos.z}), {wpos.x - 1, wpos.y, wpos.z} },
        { getBlock(WorldBlockPos{wpos.x, wpos.y + 1, wpos.z}), {wpos.x, wpos.y + 1, wpos.z} },
        { getBlock(WorldBlockPos{wpos.x, wpos.y - 1, wpos.z}), {wpos.x, wpos.y - 1, wpos.z} },
        { getBlock(WorldBlockPos{wpos.x, wpos.y, wpos.z - 1}), {wpos.x, wpos.y, wpos.z - 1} },
        { getBlock(WorldBlockPos{wpos.x, wpos.y, wpos.z + 1}), {wpos.x, wpos.y, wpos.z + 1} }
    };
  }

  LocalBlockPos World::shiftLocalPos(LocalBlockPos base, int dx, int dy, int dz) {
    base.x += dx;
    base.y += dy;
    base.z += dz;

    if (base.x < 0) {
      base.x += Chunk::WIDTH;
      base.cx -= 1;
    } else if (base.x >= Chunk::WIDTH) {
      base.x -= Chunk::WIDTH;
      base.cx += 1;
    }
    if (base.z < 0) {
      base.z += Chunk::LENGTH;
      base.cy -= 1;
    } else if (base.z >= Chunk::LENGTH) {
      base.z -= Chunk::LENGTH;
      base.cy += 1;
    }
    return base;
  }

  bool World::isOutOfBounds(const LocalBlockPos& lp) {
    // We have both cx and cy, and the in-chunk position itself.
    // Check if the chunk is out of bounds
    if (lp.cx < -NUM_CHUNKS / 2 || lp.cx >= NUM_CHUNKS / 2 || lp.cy < -NUM_CHUNKS / 2 || lp.cy >= NUM_CHUNKS / 2) {
      return true;
    }
    // Also check if the in-chunk position is out of bounds
    if (lp.x < 0 || lp.x >= Chunk::WIDTH || lp.y < 0 || lp.y >= Chunk::HEIGHT || lp.z < 0 || lp.z >= Chunk::LENGTH) {
      return true;
    }
    return false;
  }

  bool World::isOutOfBounds(const WorldBlockPos &wp) {
    return World::isOutOfBounds(World::worldToLocal(wp));
  }

  bool World::hasBlockUnder(const WorldBlockPos& wp) const {
    if (isOutOfBounds(wp)) {
      return false;
    }
    // Transform the world position to a local position
    LocalBlockPos lp = worldToLocal(wp);
    // Now, loop all the way down to the bottom, checking if there is a non-air block
    for (int y = lp.y - 1; y >= 0; y--) {
      if (this->chunks.at({ lp.cx, lp.cy }).blocks[lp.x][y][lp.z].type != BlockType::AIR) {
        return true;
      }
    }
    return false;
  }

  bool World::hasBlockOver(const WorldBlockPos &wp) const {
    if (isOutOfBounds(wp)) {
      return false;
    }
    // Transform the world position to a local position
    LocalBlockPos lp = worldToLocal(wp);
    // Now, loop all the way up to the top, checking if there is a non-air block
    for (int y = lp.y + 1; y < Chunk::HEIGHT; y++) {
      if (this->chunks.at({ lp.cx, lp.cy }).blocks[lp.x][y][lp.z].type != BlockType::AIR) {
        return true;
      }
    }
    return false;
  }


  const SafeUnorderedMap<ChunkCoord2D, Chunk>& World::getChunks() const {
    return chunks;
  }

  SafeUnorderedMap<ChunkCoord2D, Chunk>& World::getChunks() {
    return chunks;
  }

  const Chunk* World::getChunk(const ChunkCoord2D& coord) const {
    auto it = chunks.find(coord);
    if (it == chunks.cend()) {
      return nullptr;
    }
    return &it->second;
  }

  Chunk* World::getChunk(const ChunkCoord2D& coord) {
    auto it = chunks.find(coord);
    if (it == chunks.end()) {
      return nullptr;
    }
    return &it->second;
  }

  void World::commitAndEnable() {
    this->chunks.commit();
    this->chunks.enableRegister();
  }
};
