/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STORAGE_H
#define STORAGE_H

#include "../utils/db.h"
#include "../utils/randomgen.h" // utils.h
#include "../utils/safehash.h" // tx.h -> ecdsa.h -> utils.h -> bytes/join.h, strings.h -> libs/zpp_bits.h
#include "../utils/options.h"

#include "../contract/calltracer.h"
#include "../contract/event.h"

/**
 * Abstraction of the blockchain history.
 * Used to store blocks in memory and on disk, and helps the State process
 * new blocks, transactions and RPC queries.
 */
class Storage : public Log::LogicalLocationProvider {
  // TODO: possibly replace `std::shared_ptr<const Block>` with a better solution.
  private:
    std::atomic<std::shared_ptr<const FinalizedBlock>> latest_; ///< Pointer to the latest block in the blockchain.
    DB blocksDb_;  ///< Database object that contains all the blockchain blocks
    DB eventsDb_; ///< DB exclusive to events (should be removed in future)
    const Options& options_;  ///< Reference to the options singleton.
    const std::string instanceIdStr_; ///< Identifier for logging

    void initializeBlockchain(); ///< Initialize the blockchain.

    /**
     * Get a transaction from a block based on a given transaction index.
     * @param blockData The raw block string.
     * @param txIndex The index of the transaction to get.
     */
    TxBlock getTxFromBlockWithIndex(View<Bytes> blockData, uint64_t txIndex) const;

  public:
    /**
     * Constructor. Automatically loads the chain from the database
     * and starts the periodic save thread.
     * @param instanceIdStr Instance ID string to use for logging.
     * @param options Reference to the options singleton.
     */
    Storage(std::string instanceIdStr, const Options& options);

    /// Log instance (provided in ctor).
    std::string getLogicalLocation() const override { return instanceIdStr_; }

    /// Wrapper for `pushBackInternal()`. Use this as it properly locks `chainLock_`.
    void pushBlock(FinalizedBlock block);

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

    /**
     * Stores additional transaction data
     * @param txData The additional transaction data
     */
    void putTxAdditionalData(const TxAdditionalData& txData);

    /**
     * Retrieve the stored additional transaction data.
     * @param txHash The target transaction hash.
     * @return The transaction data if existent, or an empty optional otherwise.
     */
    std::optional<TxAdditionalData> getTxAdditionalData(const Hash& txHash) const;

    /**
     * Store a transaction call trace.
     * @param txHash The transaction hash.
     * @param callTrace The call trace of the transaction.
     */
    void putCallTrace(const Hash& txHash, const trace::Call& callTrace);

    /**
     * Retrieve the stored call trace of the target transaction.
     * @param txHash The target transaction hash.
     * @return The transation call trace if existent, or an empty optional otherwise.
     */
    std::optional<trace::Call> getCallTrace(const Hash& txHash) const;

    /**
     * Store an event.
     * @param event The event to be stored.
     */
    void putEvent(const Event& event);

    /**
     * Retrieve all events from a given range of block numbers.
     * @param fromBlock The initial block number (included).
     * @param toBlock The last block number (included).
     * @param address The address that emitted the events.
     * @param topics The event's topics.
     * @return The requested events if existent, or an empty vector otherwise.
     */
    std::vector<Event> getEvents(uint64_t fromBlock, uint64_t toBlock, const Address& address, const std::vector<Hash>& topics) const;

    /**
     * Retrieve all events from given block index and transaction index.
     * @param blockIndex The number of the block that contains the transaction.
     * @param txIndex The transaction index within the block.
     * @return The requested events if existent, or an empty vector otherwise.
     */
    std::vector<Event> getEvents(uint64_t blockIndex, uint64_t txIndex) const;

    /**
     * Get the indexing mode of the storage.
     * @returns The indexing mode of the storage.
     */
    inline IndexingMode getIndexingMode() const { return options_.getIndexingMode(); }

    /**
     * Helper function for checking if an event has certain topics.
     * @param event The event to check.
     * @param topics A list of topics to check for.
     * @return `true` if all topics match (or if no topics were provided), `false` otherwise.
     */
    static bool topicsMatch(const Event& event, const std::vector<Hash>& topics);

    /**
     * Helper function for storing a block in the database.
     * @param db Reference to the database.
     * @param block The block to store.
     * @param indexingEnabled Whether the node has indexing enabled or not.
     */
    static void storeBlock(DB& db, const FinalizedBlock& block, bool indexingEnabled);
};

#endif  // STORAGE_H
