/*
Copyright (c) [2023-2025] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <cstdint>
#include <array>
#include <utility>
#include <optional>

#include "utils/uintconv.h"
#include "../variables/safeunorderedmap.h"

#ifndef BTVCOMMON_H
#define BTVCOMMON_H

// Forward declaration
class DynamicContract;

namespace BTVUtils {

    /**
     * Basic Block Types
     */
    enum class BlockType {
      AIR,
      SURFACE,
      WALL,
      ENERGYCHEST
    };

    /**
     * World positioning struct
     */
    struct WorldBlockPos {
      int32_t x; // global X
      int32_t y; // global Y (vertical)
      int32_t z; // global Z
      bool operator==(const WorldBlockPos &) const = default;
    };

    struct LocalBlockPos {
      // chunk coordinates (2D, i.e. chunkX, chunkZ)
      int32_t cx;
      int32_t cy;
      // local positions within that chunk
      int32_t x;
      int32_t y;
      int32_t z;
      bool operator==(const LocalBlockPos &) const = default;
    };

    struct Block {
      BlockType type = BlockType::AIR;
      std::optional<uint64_t> placer_ = std::nullopt;
      inline const BlockType& getBlockType() const { return type; }
      inline void setBlockType(BlockType t) { type = t; }
      inline bool hasPlacer() const { return placer_.has_value(); }
      inline void setPlacer(uint64_t placer) { placer_ = placer; }
      inline std::optional<uint64_t> getPlacer() const { return placer_; }
      bool operator==(const Block &) const = default;
    };

    //
    // We store neighbors as pairs of (BlockType*, Position).
    // If the neighbor chunk/block doesn't exist, the BlockType* will be nullptr.
    //
    // front = +x
    // back  = -x
    // top   = +y
    // bottom= -y
    // left  = -z
    // right = +z
    //
    struct ConstNeighborBlocksLocal {
      std::pair<const Block *const, LocalBlockPos> front;
      std::pair<const Block *const, LocalBlockPos> back;
      std::pair<const Block *const, LocalBlockPos> top;
      std::pair<const Block *const, LocalBlockPos> bottom;
      std::pair<const Block *const, LocalBlockPos> left;
      std::pair<const Block *const, LocalBlockPos> right;
    };

    struct ConstNeighborBlocksWorld {
      std::pair<const Block *const, WorldBlockPos> front;
      std::pair<const Block *const, WorldBlockPos> back;
      std::pair<const Block *const, WorldBlockPos> top;
      std::pair<const Block *const, WorldBlockPos> bottom;
      std::pair<const Block *const, WorldBlockPos> left;
      std::pair<const Block *const, WorldBlockPos> right;
    };

    struct NeighborBlocksLocal {
      std::pair<Block *const, LocalBlockPos> front;
      std::pair<Block *const, LocalBlockPos> back;
      std::pair<Block *const, LocalBlockPos> top;
      std::pair<Block *const, LocalBlockPos> bottom;
      std::pair<Block *const, LocalBlockPos> left;
      std::pair<Block *const, LocalBlockPos> right;
    };

    struct NeighborBlocksWorld {
      std::pair<Block *const, WorldBlockPos> front;
      std::pair<Block *const, WorldBlockPos> back;
      std::pair<Block *const, WorldBlockPos> top;
      std::pair<Block *const, WorldBlockPos> bottom;
      std::pair<Block *const, WorldBlockPos> left;
      std::pair<Block *const, WorldBlockPos> right;
    };


    template <int WIDTH, int HEIGHT, int LENGTH>
    using ChunkData = std::array<std::array<std::array<Block, LENGTH>, HEIGHT>, WIDTH>;

    struct Chunk {
      static constexpr int WIDTH  = 16;
      static constexpr int HEIGHT = 64;
      static constexpr int LENGTH = 16;

      // blocks[x][y][z]
      ChunkData<WIDTH, HEIGHT, LENGTH> blocks;

      Bytes serialize() {
        Bytes data;
        for (int x = 0; x < WIDTH; x++) {
          for (int y = 0; y < HEIGHT; y++) {
            for (int z = 0; z < LENGTH; z++) {
              data.push_back(static_cast<uint8_t>(blocks[x][y][z].type));
              // We need to push information about the placer (if it exists)
              // and if it does, we need to push the placer's ID
              if (blocks[x][y][z].getPlacer().has_value()) {
                data.push_back(static_cast<uint8_t>(blocks[x][y][z].hasPlacer()));
                Utils::appendBytes(data, (UintConv::uint64ToBytes(blocks[x][y][z].getPlacer().value())));
              } else {
                data.push_back(static_cast<uint8_t>(0));
              }
            }
          }
        }
        return data;
      }

      static Chunk deserialize(Bytes data) {
        Chunk chunk;
        uint64_t i = 0;
        View<Bytes> dataView(data);
        for (int x = 0; x < WIDTH; x++) {
          for (int y = 0; y < HEIGHT; y++) {
            for (int z = 0; z < LENGTH; z++) {
              chunk.blocks[x][y][z].setBlockType(static_cast<BlockType>(dataView[i]));
              i++;
              if (dataView[i] == 1) {
                i++;
                chunk.blocks[x][y][z].setPlacer(UintConv::bytesToUint64(dataView.subspan(i, 8)));
                i += 8;
              } else {
                i++;
              }
            }
          }
        }
        return chunk;
      }
      bool operator==(const Chunk &) const = default;
    };

    /**
     * A 2D key for chunk lookup (cx, cy)
     * We'll treat 'cx' as chunk-X, 'cy' as chunk-Z
     */
    using ChunkCoord2D = std::pair<int32_t, int32_t>;

    /**
     * World class
     * - 1024x1024 area => 64x64 chunks
     * - Each chunk is 16x64x16
     * - chunk coords in range [-32..31]
     */
    class World
    {
      private:
        // Our chunk map
        SafeUnorderedMap<ChunkCoord2D, Chunk> chunks;
      public:
        static constexpr int WORLD_SIZE   = 1024; // total dimension in X, Z
        static constexpr int CHUNK_SIZE_X = 16;
        static constexpr int CHUNK_SIZE_Y = 64;
        static constexpr int CHUNK_SIZE_Z = 16;
        static constexpr int NUM_CHUNKS   = WORLD_SIZE / CHUNK_SIZE_X; // 64

        // Constructor: build all chunks from -32..31 in X and Z
        World();
        // Constructor (with DynamicContract* as argument to initialize this->chunks)
        World(DynamicContract* contract);
        // Convert a world position to a local position
        static LocalBlockPos worldToLocal(const WorldBlockPos& wpos);
        // Convert a local (in-chunk) position to a world position
        static WorldBlockPos localToWorld(const LocalBlockPos& lpos);
        // Get a block directly from a local position
        const Block *const getBlock(const LocalBlockPos& lp) const;
        // Get a block directly from a world position
        const Block *const getBlock(const WorldBlockPos& wp) const;
        // Get a block directly from a local position (non-const version)
        Block *const getBlock(const LocalBlockPos& lp);
        // Get a block directly from a world position
        Block *const getBlock(const WorldBlockPos& wp);
        // Get neighbors of a local block (using a world position)
        ConstNeighborBlocksLocal getNeighborsLocal(const WorldBlockPos& wpos) const;
        // Get neighbors of a local block (using a local position)
        ConstNeighborBlocksLocal getNeighborsLocal(const LocalBlockPos& base) const;
        // Get neighbors of a world block
        ConstNeighborBlocksWorld getNeighborsWorld(const WorldBlockPos& wpos) const;
        // Get neighbors of a local block (using a world position, non-const version)
        NeighborBlocksLocal getNeighborsLocal(const WorldBlockPos& wpos);
        // Get neighbors of a local block (using a local position, non-const version)
        NeighborBlocksLocal getNeighborsLocal(const LocalBlockPos& base);
        // Get neighbors of a world block (non-const version)
        NeighborBlocksWorld getNeighborsWorld(const WorldBlockPos& wpos);
        // Shifts a given position to the neighbor chunk if needed
        static LocalBlockPos shiftLocalPos(LocalBlockPos base, int dx, int dy, int dz);
        // Out of bounds check
        static bool isOutOfBounds(const LocalBlockPos& lp);
        static bool isOutOfBounds(const WorldBlockPos& wp);

        // Getter for chunks
        const SafeUnorderedMap<ChunkCoord2D, Chunk>& getChunks() const;
        SafeUnorderedMap<ChunkCoord2D, Chunk>& getChunks();
        const Chunk* getChunk(const ChunkCoord2D& coord) const;
        Chunk* getChunk(const ChunkCoord2D& coord);
    };
}



#endif // BTVCOMMON_H