#ifndef STORAGE_H
#define STORAGE_H

#include <shared_mutex>

#include "../utils/block.h"
#include "../utils/db.h"
#include "../utils/ecdsa.h"
#include "../utils/randomgen.h"
#include "../utils/safehash.h"
#include "../utils/utils.h"

/// Enum for the status of a block or transaction inside the storage.
enum StorageStatus { NotFound, OnChain, OnCache, OnDB };

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
     * or 1M transactions, whichever comes first.
     * This limit is required because it would be too expensive to keep
     * every single transaction in memory all the time, so once it reaches
     * the limit, or every now and then, the blocks are dumped to the database.
     * This keeps the blockchain lightweight in memory and extremely responsive.
     * Older blocks always at FRONT, newer blocks always at BACK.
     */
    std::deque<std::shared_ptr<const Block>> chain;

    /// Map that indexes blocks in memory by their respective hashes.
    std::unordered_map<Hash, const std::shared_ptr<const Block>, SafeHash> blockByHash;

    /// Map that indexes Tx, blockHash, blockIndex and blockHeight by their respective hashes
    std::unordered_map<Hash, const std::tuple<const Hash,const uint64_t,const uint64_t>, SafeHash> txByHash;

    /// Map that indexes all block heights in the chain by their respective hashes.
    std::unordered_map<Hash, const uint64_t, SafeHash> blockHeightByHash;

    /// Map that indexes all block hashes in the chain by their respective heights.
    std::unordered_map<uint64_t, const Hash, SafeHash> blockHashByHeight;

    /// Cache space for blocks that will be included in the blockchain.
    mutable std::unordered_map<Hash, const std::shared_ptr<const Block>, SafeHash> cachedBlocks;

    /// Cache space for transactions that will be included in the blockchain.
    /// Value: tx, txBlockHash, txBlockIndex, txBlockHeight
    mutable std::unordered_map<Hash, const std::tuple<const std::shared_ptr<const TxBlock>,const Hash,const uint64_t, const uint64_t>, SafeHash> cachedTxs;

    /// Mutex for managing read/write access to the blockchain.
    mutable std::shared_mutex chainLock;

    /// Mutex to manage read/write access to the cache.
    mutable std::shared_mutex cacheLock;

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

    /**
     * Initializes the blockchain the first time the blockchain binary is ran.
     * This function is called by the constructor.
     * initializeBlockchain will only populate information related with the Storage class
     * such as genesis and mappings.
     */
    void initializeBlockchain();

    /**
     * Parse a given Tx from a serialize block data.
     * Used to get only a specific transaction from a block.
     * @params blockData The serialized block data.
     * @params index The index of the transaction to get.
     * @return const std::shared_ptr<const TxBlock> The transaction.
     */
    static const TxBlock getTxFromBlockWithIndex(const std::string_view blockData, const uint64_t& txIndex);

  public:
    /**
     * Constructor. Automatically loads the chain from the database
     * and starts the periodic save thread.
     * @param db Pointer to the database.
     */
    Storage(const std::unique_ptr<DB>& db);

    /// Destructor. Automatically saves the chain to the database.
    ~Storage();

    /// Wrapper for `pushBackInternal()`. Use this as it properly locks `chainLock`.
    void pushBack(Block&& block);

    /// Wrapper for `pushFrontInternal()`. Use this as it properly locks `chainLock`.
    void pushFront(Block&& block);

    /// Remove a block from the end of the chain.
    void popBack();

    /// Remove a block from the start of the chain.
    void popFront();

    /**
     * Check if a block exists anywhere in storage
     * (memory/chain, then cache, then database).
     * @param blockHash The block hash to search.
     * @return An enum telling where the block is.
     */
    StorageStatus blockExists(const Hash& hash);

    /**
     * Overload of `blockExists` that works with block height instead of hash.
     * @param blockHeight The block height to search.
     * @return An enum telling where the block is.
     */
    StorageStatus blockExists(const uint64_t& height);

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
     * Check if a transaction exists anywhere in storage
     * (memory/chain, then cache, then database).
     * @param tx The transaction to check.
     * @return An enum telling where the transaction is.
     */
    StorageStatus txExists(const Hash& tx);

    /**
     * Get a transaction from the chain using a given hash.
     * @param tx The transaction hash to get.
     * @return std::tuple<const std::shared_ptr<const TxBlock>, const Hash, const uint64_t>
     * @return The found transaction, the block hash it's in, and the block height it's in.
     */
    const std::tuple<const std::shared_ptr<const TxBlock>,
                     const Hash,
                     const uint64_t,
                     const uint64_t> getTx(const Hash& tx);

    /**
     * Get a Tx from a block with a specific index.
     * @param blockHash The block hash
     * @param blockIndex the index within the block
     * @return The found transaction, the block hash ti's in, and the block height it's in.
     */
    const std::tuple<const std::shared_ptr<const TxBlock>,
                     const Hash,
                     const uint64_t,
                     const uint64_t> getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex);

    /**
     * Get a Tx from a block with a specific index.
     * @param blockHeight The block height
     * @param blockIndex the index within the block
     * @return The found transaction, the block hash ti's in, and the block height it's in.
     */
    const std::tuple<const std::shared_ptr<const TxBlock>,
      const Hash,
      const uint64_t,
      const uint64_t> getTxByBlockNumberAndIndex(const uint64_t& blockHeight, const uint64_t blockIndex);


  /**
     * Get the most recently added block from the chain.
     * @returns The latest block.
     */
    const std::shared_ptr<const Block> latest();

    /// Get the number of blocks currently in the chain. (nHeight of latest block + 1)
    uint64_t currentChainSize();

    /// Start the periodic save thread. Called by the constructor.
    void periodicSaveToDB();

    /// Stop the periodic save thread.
    void stopPeriodicSaveToDB() { this->stopPeriodicSave = true; }
};

#endif  // STORAGE_H
