/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STORAGE_H
#define STORAGE_H

#include <shared_mutex>

#include "../utils/mutableblock.h"
#include "../utils/db.h"
#include "../utils/ecdsa.h"
#include "../utils/randomgen.h"
#include "../utils/safehash.h"
#include "../utils/utils.h"
#include "../utils/options.h"

/// Enum for the status of a block or transaction inside the storage.
enum StorageStatus { NotFound, OnChain, OnCache, OnDB };

/**
 * Abstraction of the blockchain history.
 * Used to store blocks in memory and on disk, and helps the State process
 * new blocks, transactions and RPC queries.
 */
class Storage {
  // TODO: possibly replace `std::shared_ptr<const Block>` with a better solution.
private:
  DB& db_;  ///< Reference to the database that contains the blockchain's entire history.
  const Options& options_;  ///< Reference to the options singleton.
  /**
   * Recent blockchain history, up to the 1000 most recent blocks or 1M transactions, whichever comes first.
   * This limit is required because it would be too expensive to keep every single transaction in memory
   * all the time, so once it reaches the limit, or every now and then, older blocks are dumped to the database.
   * This keeps the blockchain lightweight in memory and extremely responsive.
   * Older blocks always at FRONT, newer blocks always at BACK.
   */
  std::deque<std::shared_ptr<const FinalizedBlock>> chain_;

  /// Map that indexes blocks in memory by their respective hashes.
  std::unordered_map<Hash, const std::shared_ptr<const FinalizedBlock>, SafeHash> blockByHash_;

  /// Map that indexes Tx, blockHash, blockIndex and blockHeight by their respective hashes
  std::unordered_map<Hash, const std::tuple<const Hash,const uint64_t,const uint64_t>, SafeHash> txByHash_;

  /// Map that indexes all block heights in the chain by their respective hashes.
  std::unordered_map<Hash, const uint64_t, SafeHash> blockHeightByHash_;

  /// Map that indexes all block hashes in the chain by their respective heights.
  std::unordered_map<uint64_t, const Hash, SafeHash> blockHashByHeight_;

  /// Cache space for blocks that will be included in the blockchain.
  mutable std::unordered_map<Hash, const std::shared_ptr<const FinalizedBlock>, SafeHash> cachedBlocks_;

  /// Cache space for transactions that will be included in the blockchain (tx, txBlockHash, txBlockIndex, txBlockHeight).
  mutable std::unordered_map<Hash,
                             const std::tuple<const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t>,
                             SafeHash> cachedTxs_;

  mutable std::shared_mutex chainLock_; ///< Mutex for managing read/write access to the blockchain.
  mutable std::shared_mutex cacheLock_; ///< Mutex to manage read/write access to the cache.
  std::thread periodicSaveThread_;  ///< Thread that periodically saves the blockchain history to the database.
  uint64_t periodicSaveCooldown_ = 15;  ///< Cooldown for the periodic save thread, in seconds.
  bool stopPeriodicSave_ = false; ///< Flag for stopping the periodic save thread, if required.

  /**
   * Add a block to the end of the chain.
   * Only call this function directly if absolutely sure that `chainLock_` is locked.
   * @param block The block to add.
   * @throw DynamicException on incorrect previous hash or height.
   */
  void pushBackInternal(FinalizedBlock block);

  /**
   * Add a block to the start of the chain.
   * Only call this function directly if absolutely sure that `chainLock_` is locked.
   * @param block The block to add.
   * @throw DynamicException on incorrect previous hash or height.
   */
  void pushFrontInternal(FinalizedBlock block);

  /**
   * Initialize the blockchain the first time the blockchain binary is booted. Called by the constructor.
   * Will only populate information related to the class (e.g. genesis and mappings).
   */
  void initializeBlockchain();

  /**
   * Parse a given transaction from a serialized block data string.
   * Used to get only a specific transaction from a block.
   * @param blockData The serialized block data string.
   * @param txIndex The index of the transaction to get.
   * @return The transaction itself.
   */
  TxBlock getTxFromBlockWithIndex(const BytesArrView blockData, const uint64_t& txIndex) const;

  /**
   * Check if a block exists anywhere in storage (memory/chain, then cache, then database).
   * Does NOT lock `chainLock_` or `cacheLock_`.
   * @param hash The block hash to search.
   * @return An enum telling where the block is.
   */
  StorageStatus blockExistsInternal(const Hash& hash) const;

  /**
   * Overload of blockExistsInternal() that works with block height instead of hash.
   * Does **not** lock `chainLock_` or `cacheLock_`.
   * @param height The block height to search.
   * @return Bool telling if the block exists.
   */
  StorageStatus blockExistsInternal(const uint64_t& height) const;

  /**
   * Check if a transaction exists anywhere in storage (memory/chain, then cache, then database).
   * @param tx The transaction to check.
   * @return Bool telling if the transaction exists.
   */
  StorageStatus txExistsInternal(const Hash& tx) const;

public:
  /**
   * Constructor. Automatically loads the chain from the database
   * and starts the periodic save thread.
   * @param db Reference to the database.
   * @param options Reference to the options singleton.
   */
  Storage(DB& db, const Options& options);
  ~Storage(); ///< Destructor. Automatically saves the chain to the database.
  void pushBack(FinalizedBlock block); ///< Wrapper for `pushBackInternal()`. Use this as it properly locks `chainLock_`.
  void pushFront(FinalizedBlock block);  ///< Wrapper for `pushFrontInternal()`. Use this as it properly locks `chainLock_`.
  void popBack(); ///< Remove a block from the end of the chain.
  void popFront();  ///< Remove a block from the start of the chain.

  /**
   * Check if a block exists anywhere in storage (memory/chain, then cache, then database).
   * Locks `chainLock_` and `cacheLock_`, to be used by external actors.
   * @param hash The block hash to search.
   * @return `true` if the block exists, `false` otherwise.
   */
  bool blockExists(const Hash& hash) const;

  /**
   * Overload of blockExists() that works with block height instead of hash.
   * @param height The block height to search.
   * @return `true` if the block exists, `false` otherwise.
   */
  bool blockExists(const uint64_t& height) const;

  /**
   * Get a block from the chain using a given hash.
   * @param hash The block hash to get.
   * @return A pointer to the found block, or `nullptr` if block is not found.
   */
  std::shared_ptr<const FinalizedBlock> getBlock(const Hash& hash) const;

  /**
   * Get a block from the chain using a given height.
   * @param height The block height to get.
   * @return A pointer to the found block, or `nullptr` if block is not found.
   */
  std::shared_ptr<const FinalizedBlock> getBlock(const uint64_t& height) const;

  /**
   * Check if a transaction exists anywhere in storage (memory/chain, then cache, then database).
   * @param tx The transaction to check.
   * @return Bool telling if the transaction exists.
   */
  bool txExists(const Hash& tx) const;

  /**
   * Get a transaction from the chain using a given hash.
   * @param tx The transaction hash to get.
   * @return A tuple with the found transaction, block hash, index and height.
   * @throw DynamicException on hash mismatch.
   */
  std::tuple<
    const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
    > getTx(const Hash& tx) const;

  /**
   * Get a transaction from a block with a specific index.
   * @param blockHash The block hash
   * @param blockIndex the index within the block
   * @return A tuple with the found transaction, block hash, index and height.
   * @throw DynamicException on hash mismatch.
   */
  std::tuple<
    const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
    > getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex) const;

  /**
   * Get a transaction from a block with a specific index.
   * @param blockHeight The block height
   * @param blockIndex The index within the block.
   * @return A tuple with the found transaction, block hash, index and height.
   */
  std::tuple<
    const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
    > getTxByBlockNumberAndIndex(const uint64_t& blockHeight, const uint64_t blockIndex) const;

  /// Get the most recently added block from the chain.
  std::shared_ptr<const FinalizedBlock> latest() const;

  /// Get the number of blocks currently in the chain (nHeight of latest block + 1).
  uint64_t currentChainSize() const;

  // TODO: both functions below should be called by the ctor/dtor respectively.

  /// Start the periodic save thread.
  void periodicSaveToDB();

  /// Stop the periodic save thread.
  void stopPeriodicSaveToDB() { this->stopPeriodicSave_ = true; }
};

#endif  // STORAGE_H
