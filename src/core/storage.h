#ifndef STORAGE_H
#define STORAGE_H

#include <shared_mutex>

#include "../utils/block.h"
#include "../utils/db.h"
#include "../utils/ecdsa.h"
#include "../utils/randomgen.h"
#include "../utils/safehash.h"
#include "../utils/utils.h"

// Forward declarations.
class Block;

/**
 * Abstraction of the blockchain history.
 * Used to store blocks in memory and on disk, and helps the %State process
 * new blocks, transactions and RPC queries.
 */
class Storage {
  private:
    /// Pointer to the database that contains the blockchain's entire history.
    const std::unique_ptr<DB>& db;

    /**
     * The recent blockchain history, up to the 1000 most recent blocks,
     * Or 10M transactions, whichever comes first.
     * This limit is required because it would be too expensive to keep
     * every single transaction in memory all the time, so once it reaches
     * the limit, or every now and then, the blocks are dumped to the database.
     * This keeps the blockchain lightweight in memory and extremely responsive.
     */
    std::deque<std::shared_ptr<const Block>> chain;

    /// Map that indexes blocks in memory by their respective hashes.
    std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> blockByHash;

    /// Map that indexes blocks in memory by their respective transaction hashes.
    std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> blockByTxHash;

    /// Map that indexes transactions in memory by their respective hashes.
    std::unordered_map<Hash, std::shared_ptr<const TxBlock>, SafeHash> txByHash;

    /// Map that indexes all block heights in the chain by their respective hashes.
    std::unordered_map<Hash, uint64_t, SafeHash> blockHeightByHash;

    /// Map that indexes all block hashes in the chain by their respective heights.
    std::unordered_map<uint64_t, Hash, SafeHash> blockHashByHeight;

    /// Cache space for blocks that will be included in the blockchain.
    mutable std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> cachedBlocks;

    /// Cache space for transactions that will be included in the blockchain.
    mutable std::unordered_map<Hash, std::shared_ptr<const TxBlock>, SafeHash> cachedTxs;

    /// Mutex for managing read/write access to the blockchain.
    mutable std::shared_mutex chainLock;

    /// Thread that periodically saves the blockchain history to the database.
    std::thread periodicSaveThread;

    /// Cooldown for the periodic save thread, in seconds.
    uint64_t periodicSaveCooldown = 15;

    /// Flag for stopping the periodic save thread, if required.
    bool stopPeriodicSave = false;

    /**
     * Add a block to the end of the chain.
     * Only call this function directly if absolutely sure that `chainLock` is locked.
     * @param block The block to add.
     */
    void pushBackInternal(Block&& block);

    /**
     * Add a block to the start of the chain.
     * Only call this function directly if absolutely sure that `chainLock` is locked.
     * @param block The block to add.
     */
    void pushFrontInternal(Block&& block);

    /// Load the latest blocks from database to memory (up to 1000).
    void loadFromDB();

  public:
    /**
     * Constructor. Automatically starts the periodic save thread.
     * @param db Pointer to the database.
     */
    Storage(const std::unique_ptr<DB>& db);
    ~Storage();

    /// Wrapper for `pushBackInternal()`. Use this as it properly locks `chainLock`.
    void pushBack(Block&& block);

    /// Wrapper for `pushBackInternal()`. Use this as it properly locks `chainLock`.
    void pushFront(Block&& block);

    /// Remove a block from the end of the chain.
    void popBack();

    /// Remove a block from the start of the chain.
    void popFront();

    /**
     * Check if a block exists in memory, searching by block hash.
     * @param hash The block hash to search.
     * @return `true` if the block exists, `false` otherwise.
     */
    bool hasBlock(const Hash& hash);

    /**
     * Check if a block exists in memory, searching by block height.
     * @param height The block height to search.
     * @return `true` if the block exists, `false` otherwise.
     */
    bool hasBlock(const uint64_t& height);

    /// Same as `hasBlock()`, but also checks the database.
    bool exists(const Hash& hash);

    /// Same as `hasBlock()`, but also checks the database.
    bool exists(const uint64_t& height);

    /**
     * Get a block from the chain using a given hash.
     * @param hash The block hash to get.
     * @return The found block, or `nullptr` if block is not found.
     */
    const std::shared_ptr<const Block> getBlock(const Hash& hash);

    /**
     * Get a block from the chain using a given height.
     * @param height The block height to get.
     * @return The found block, or `nullptr` if block is not found.
     */
    const std::shared_ptr<const Block> getBlock(const uint64_t& height);

    /**
     * Check if a transaction exists in the chain.
     * @param tx The transaction to check.
     * @return `true` if the transaction exists, `false` otherwise.
     */
    bool hasTx(const Hash& tx);

    /**
     * Get a transaction from the chain using a given hash.
     * @param tx The transaction hash to get.
     * @return The found transaction, or `nullptr` if transaction is not found.
     */
    const std::shared_ptr<const TxBlock> getTx(const Hash& tx);

    /**
     * Get a block from the chain that contains a given transaction.
     * @param tx The transaction hash to look for.
     * @return The found block, or `nullptr` if block is not found.
     */
    const std::shared_ptr<const Block> getBlockFromTx(const Hash& tx);

    /**
     * Get the most recently added block from the chain.
     * @returns The latest block.
     */
    const std::shared_ptr<const Block> latest();

    /// Get the number of blocks currently in the std::deque.
    uint64_t blockSize();

    /// Start the periodic save thread. Called by the constructor.
    void periodicSaveToDB();

    /// Stop the periodic save thread.
    void stopPeriodicSaveToDB() { this->stopPeriodicSave = true; }
};

#endif  // STORAGE_H
