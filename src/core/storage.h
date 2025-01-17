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

/// A <TxBlock, blockHash, blockIndex, blockHeight> tuple.
//using StorageGetTxResultType = std::tuple<
//  std::shared_ptr<TxBlock>, Hash, uint64_t, uint64_t
//>;

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
 *
 * NOTE: Storage should only really deal with data that it actually has stored in a DB.
 * For example, getBlock() is no longer here because storage doesn't store block data in the
 * BDK DBs. Instead, getBlock() is moved to the Blockchain class which is backed by the
 * Comet driver, so it can make the decision of whether it will fetch the block via
 * Comet/RPC or use a local RAM cache.
 * The Storage class can make its own decisions w.r.t. RAM caching, but it will be making
 * those vs. its own internal DBs only.
 */
class Storage : public Log::LogicalLocationProvider {
  private:
    Blockchain& blockchain_; ///< Our parent object through which we reach the other components

    // NOTE: Initial Comet/BDK code integration strategy:
    //
    //       At first, we will simply not store any contract or chain data
    //       at all, and instead of asking the DB, we just make an RPC call
    //       to cometbft, get a Comet object, then construct a BDK object
    //       like TxBlock or FinalizedBlock dynamically to satisfy the caller.
    //       This must be enough for the contract tests to pass, and then
    //       we move on from that baseline state to figure out what exactly
    //       to cache, what the BDK-side objects should look like exactly, etc.

    // NOTE: Ideally, we don't keep a block store, but we may
    //       have to retain a pointer to a latest ETH block *header*,
    //       not the full block data.
    //       FinalizedBlock contains ALL the transactions, so storing FinalizedBlock
    //       objects is in fact storing the entire block.
    //       We do NOT want to do that; we want the blocks of transactions to be
    //       stored in the CometBFT side ONLY.
    //       FinalizedBlock should only contain metadata. If we want the "actual block",
    //       that is, the actual transactions, we should query CometBFT instead.
    //
    // TODO: Remove the vector<TxBlock> field from FinalizedBlock; retrieve
    //       transactions from CometBFT or cache them in RAM as they are pulled
    //       from CometBFT.
    //       OR:
    //       Create different top-level classes to model our various needs
    //       w.r.t. block/tx data.
    //
    // We have to keep this at the very least because our current contract test
    // suite makes heavy use of a latest FinalizedBlock that has TxBlock objects
    // --> moved to Blockchain, which actually generates a FinalizedBlock object
    //std::atomic<std::shared_ptr<const FinalizedBlock>> latest_; ///< Pointer to the latest block in the blockchain.

    DB blocksDb_;  ///< Database object that contains all the blockchain blocks
    DB eventsDb_; ///< DB exclusive to events (should be removed in future)

    // For now, all of the sha3 to sha256 tx hashes are being saved on disk
    //   and never being purged -- the only way to purge them is to stop
    //   the node and delete the database, which will forget about all of the
    //   mappings.
    // The permanent solution to this is to change the transaction hashing
    //   function of CometBFT from sha256 to sha3, either by forking cometbft
    //   or waiting until e.g. cometbft supports setting the hash function as
    //   a config option.
    DB txMapDb_; ///< DB that maps sha3->sha256 txhashes

    // Get it via blockchain_.opt() instead
    //const Options& options_;  ///< Reference to the options singleton.

    // We don't store blocks
    //MOVE to Blockchain or delete.
    //void initializeBlockchain(); ///< Initialize the blockchain.

    /**
     * MOVE to Blockchain or delete.
     *
     * Get a transaction from a block based on a given transaction index.
     * @param blockData The raw block string.
     * @param txIndex The index of the transaction to get.
     */
    //TxBlock getTxFromBlockWithIndex(bytes::View blockData, uint64_t txIndex) const;

    // Moved this to Blockchain
    // Blockchain class should centralize all requests for blocks, txs, etc.
    // As well as RAM caching in FinalizedBlock, TxBlock, etc.
//    std::atomic<uint64_t> txCacheSize_ = 1000000; ///< Transaction cache size in maximum entries per bucket (0 to disable).
//    mutable std::mutex txCacheMutex_; ///< Mutex to protect cache access.
//    std::array<std::unordered_map<Hash, StorageGetTxResultType, SafeHash>, 2> txCache_; ///< Transaction cache as two rotating buckets.
//    uint64_t txCacheBucket_ = 0; ///< Active txCache_ bucket.

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

    /**
     * MOVE block queries to Blockchain class (or delete)
     *
     * Check if a block exists anywhere in storage (memory/chain, then cache, then database).
     * Locks `chainLock_` and `cacheLock_`, to be used by external actors.
     * @param hash The block hash to search.
     * @return `true` if the block exists, `false` otherwise.
     */
    //bool blockExists(const Hash& hash) const;

    /**
     * MOVE block queries to Blockchain class (or delete)
     *
     * Overload of blockExists() that works with block height instead of hash.
     * @param height The block height to search.
     * @return `true` if the block exists, `false` otherwise.
     */
    //bool blockExists(uint64_t height) const;

    /**
     * MOVE block queries to Blockchain class (or delete)
     * checking if a transaction exists can involve Comet, so it should go
     * to Blockchain as well
     *
     * Check if a transaction exists anywhere in storage (memory/chain, then cache, then database).
     * @param tx The transaction to check.
     * @return Bool telling if the transaction exists.
     */
    //bool txExists(const Hash& tx) const;

    //MOVE to Blockchain or delete.
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
    IndexingMode getIndexingMode() const;
};

#endif  // STORAGE_H
