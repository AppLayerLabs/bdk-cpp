/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STORAGE_H
#define STORAGE_H

#include <atomic>

#include "../utils/db.h"
#include "../utils/ecdsa.h"
#include "../utils/randomgen.h"
#include "../utils/safehash.h"
#include "../utils/utils.h"
#include "../utils/options.h"
#include "../contract/calltracer.h"
#include "../libs/zpp_bits.h"

/**
 * Abstraction of the blockchain history.
 * Used to store blocks in memory and on disk, and helps the State process
 * new blocks, transactions and RPC queries.
 * TODO: Replace std::unordered_map with boost::unordered_flat_map AFTER we finish Storage refactor (Currently being done by Leonardo)
 */
class Storage : public Log::LogicalLocationProvider {
  // TODO: possibly replace `std::shared_ptr<const Block>` with a better solution.
private:
  std::atomic<std::shared_ptr<const FinalizedBlock>> latest_;
  DB db_;  ///< Database object that contains all the blockchain blocks
  const Options& options_;  ///< Reference to the options singleton.
  const std::string instanceIdStr_; ///< Identifier for logging

  void initializeBlockchain();

  TxBlock getTxFromBlockWithIndex(bytes::View blockData, uint64_t txIndex) const;

public:
  /**
   * Constructor. Automatically loads the chain from the database
   * and starts the periodic save thread.
   * @param instanceIdStr Instance ID string to use for logging.
   * @param options Reference to the options singleton.
   */
  Storage(std::string instanceIdStr, const Options& options);
  std::string getLogicalLocation() const override { return instanceIdStr_; } ///< Log instance (provided in ctor)
  void pushBlock(FinalizedBlock block); ///< Wrapper for `pushBackInternal()`. Use this as it properly locks `chainLock_`.

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
  bool blockExists(uint64_t height) const;

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
  std::shared_ptr<const FinalizedBlock> getBlock(uint64_t height) const;

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
    > getTxByBlockNumberAndIndex(uint64_t blockHeight, uint64_t blockIndex) const;

    /// Get the most recently added block from the chain.
    std::shared_ptr<const FinalizedBlock> latest() const;

  /// Get the number of blocks currently in the chain (nHeight of latest block + 1).
  uint64_t currentChainSize() const;

  void putTxAdditionalData(const TxAdditionalData& txData);

  std::optional<TxAdditionalData> getTxAdditionalData(const Hash& txHash) const;

  void putCallTrace(const Hash& txHash, const trace::Call& callTrace);

  std::optional<trace::Call> getCallTrace(const Hash& txHash) const;

  inline IndexingMode getIndexingMode() const { return options_.getIndexingMode(); }
};

#endif  // STORAGE_H
