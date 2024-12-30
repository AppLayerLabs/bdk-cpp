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

class Blockchain;

/**
 * The Storage component keeps any bdkd-side persistent data that is globally relevant
 * for the blockchain that this node is tracking (i.e., for a given Options::rootPath).
 *
 * We should avoid duplicating any data that CometBFT (Options::rootPath + "/comet/") is
 * already storing and managing, such as raw block data (lists of serialized transactions),
 * but we *can* store anything we deem advantageous for us to compute and cache on our side.
 *
 * Ultimately, the Storage should be regularly pruned at the exact same time and height as
 * CometBFT is pruned (via the retain block height cometbft ABCI callback). That is, the
 * height history depth we tell CometBFT to retain should be the same that we use here.
 */
class Storage : public Log::LogicalLocationProvider {
  private:
    Blockchain& blockchain_; ///< Our parent object through which we reach the other components

    // Not retaining any block-related data or metadata for now
    //std::atomic<std::shared_ptr<const FinalizedBlock>> latest_; ///< Pointer to the latest block in the blockchain.
    //DB blocksDb_;  ///< Database object that contains all the blockchain blocks

    // FIXME/TODO: given the comment below for the eventsdb ("should be removed")
    //   maybe what we need is a generic "nodeDb_" that stores everything? or at
    //   least it's all under the same speedb instance / data directory.
    //   Why would we ever need more than one database instance / directory?
    DB blocksDb_;  ///< We need to keep this because the callTrace feature is being stored here
    DB eventsDb_; ///< DB exclusive to events (should be removed in future)

    // Get it via blockchain_.opt() instead
    //const Options& options_;  ///< Reference to the options singleton.

    // We don't store blocks
    //void initializeBlockchain(); ///< Initialize the blockchain.

    /**
     * Get a transaction from a block based on a given transaction index.
     * @param blockData The raw block string.
     * @param txIndex The index of the transaction to get.
     */
    //TxBlock getTxFromBlockWithIndex(bytes::View blockData, uint64_t txIndex) const;

  public:
    /**
     * Constructor. Automatically loads the chain from the database
     * and starts the periodic save thread.
     * @param instanceIdStr Instance ID string to use for logging.
     * @param options Reference to the options singleton.
     */
    Storage(Blockchain& blockchain); //, std::string instanceIdStr, const Options& options);

    ~Storage() = default; ///< Destructor.

    std::string getLogicalLocation() const override;

    /// Wrapper for `pushBackInternal()`. Use this as it properly locks `chainLock_`.
    //void pushBlock(FinalizedBlock block);

    /**
     * Check if a block exists anywhere in storage (memory/chain, then cache, then database).
     * Locks `chainLock_` and `cacheLock_`, to be used by external actors.
     * @param hash The block hash to search.
     * @return `true` if the block exists, `false` otherwise.
     */
    //bool blockExists(const Hash& hash) const;

    /**
     * Overload of blockExists() that works with block height instead of hash.
     * @param height The block height to search.
     * @return `true` if the block exists, `false` otherwise.
     */
    //bool blockExists(uint64_t height) const;

    /**
     * Get a block from the chain using a given hash.
     * @param hash The block hash to get.
     * @return A pointer to the found block, or `nullptr` if block is not found.
     */
    //std::shared_ptr<const FinalizedBlock> getBlock(const Hash& hash) const;

    /**
     * Get a block from the chain using a given height.
     * @param height The block height to get.
     * @return A pointer to the found block, or `nullptr` if block is not found.
     */
    //std::shared_ptr<const FinalizedBlock> getBlock(uint64_t height) const;

    /**
     * Check if a transaction exists anywhere in storage (memory/chain, then cache, then database).
     * @param tx The transaction to check.
     * @return Bool telling if the transaction exists.
     */
    //bool txExists(const Hash& tx) const;

    /**
     * Get a transaction from the chain using a given hash.
     * @param tx The transaction hash to get.
     * @return A tuple with the found transaction, block hash, index and height.
     * @throw DynamicException on hash mismatch.
     */
    //std::tuple<
    //  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
    //> getTx(const Hash& tx) const;

    /**
     * Get a transaction from a block with a specific index.
     * @param blockHash The block hash
     * @param blockIndex the index within the block
     * @return A tuple with the found transaction, block hash, index and height.
     * @throw DynamicException on hash mismatch.
     */
    //std::tuple<
    //  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
    //> getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex) const;

    /**
     * Get a transaction from a block with a specific index.
     * @param blockHeight The block height
     * @param blockIndex The index within the block.
     * @return A tuple with the found transaction, block hash, index and height.
     */
    //std::tuple<
    //  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
    //> getTxByBlockNumberAndIndex(uint64_t blockHeight, uint64_t blockIndex) const;

    /// Get the most recently added block from the chain.
    //std::shared_ptr<const FinalizedBlock> latest() const;

    // You get this from the State (the machine has a height which is the current state)
    // If you really want to know how many blocks are in the chain, ask CometBFT RPC
    // Something like this method would exist if we are keeping a cache of blocks and/or
    //   transactions in the bdkd side, and methods like this would return that info,
    //   that is, they are inspecting the bdkd RAM and/or disk persisted cache.
    //
    /// Get the number of blocks currently in the chain (nHeight of latest block + 1).
    //uint64_t currentChainSize() const;

    /**
     * Stores additional transaction data
     * @param txData The additional transaction data
     */
    //void putTxAdditionalData(const TxAdditionalData& txData);

    /**
     * Retrieve the stored additional transaction data.
     * @param txHash The target transaction hash.
     * @return The transaction data if existent, or an empty optional otherwise.
     */
    //std::optional<TxAdditionalData> getTxAdditionalData(const Hash& txHash) const;

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
    IndexingMode getIndexingMode() const;
};

#endif  // STORAGE_H
