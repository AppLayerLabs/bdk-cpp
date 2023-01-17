#ifndef BLOCKMEMPOOL_H
#define BLOCKMEMPOOL_H

#include <mutex>

#include "block.h"
#include "blockChain.h"
#include "state.h"
#include "../net/grpcserver.h"
#include "../utils/db.h"
#include "../utils/utils.h"

/**
 * Abstraction of the blockchain's mempool.
 * Blocks in need of processing/consensus will arrive here first.
 * Once consensus is reached, depending on the resolution, they're either
 * dumped to the blockchain or erased altogether.
 */
class BlockMempool {
  private:
    /// Preferred block hash. Set by SetPreference from gRPC.
    Hash preferredBlockHash;

    /// The mempool itself. Lookup is made by block hash.
    std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> mempool;

    /// Cached block status. Lookup is made by block hash.
    std::unordered_map<Hash, BlockStatus, SafeHash> cachedBlockStatus;

    /// Mutex for managing read/write access to the mempool.
    mutable std::mutex mempoolLock;

    /// Pointer to the state.
    std::shared_ptr<State> state;

    /// Pointer to the blockchain.
    std::shared_ptr<BlockChain> chain;

    /// Pointer to the block manager.
    std::shared_ptr<BlockManager> mgr;

  public:
    /**
     * Constructor.
     * @param state Pointer to the state.
     * @param chain Pointer to the blockchain.
     * @param mgr Pointer to the block manager.
     */
    BlockManager(
      const std::shared_ptr<State>& state, const std::shared_ptr<BlockChain>& chain,
      const std::shared_ptr<BlockManager>& mgr
    ) : state(state), chain(chain), mgr(mgr);

    /// Getter for `preferredBlockHash`.
    const Hash& getPreferredBlockHash() { return this->preferredBlockHash; }

    /// Setter for `preferredBlockHash`.
    void setPreferredBlockHash(const Hash& hash) { this->preferredBlockHash = hash; }

    /**
     * Get a block's status in the mempool.
     * @param hash The block hash to get the status from.
     * @return The current block's status.
     */
    const BlockStatus getBlockStatus(const Hash& hash);

    /**
     * Set a block's status in the mempool.
     * @param hash The block hash to set the status to.
     * @param status The status to set.
     */
    void setBlockStatus(const Hash& hash, const BlockStatus& status);

    /**
     * Check if a block has the "Processing" status (no consensus reached yet).
     * @param block The block hash to check.
     * @return `true` if block is processing, `false` otherwise.
     */
    bool isProcessing(const Hash& block);

    /**
     * Accept a block.
     * @param block The block hash to accept.
     * @return `true` on success, `false` on failure.
     */
    bool accept(const Hash& block);

    /**
     * Reject a block.
     * @param block The block hash to reject.
     * @return `true` on success, `false` on failure.
     */
    bool reject(const Hash& block);

    /**
     * Process a block.
     * @param block The block to process.
     */
    void processBlock(std::shared_ptr<Block> block);

    /**
     * Check if a block exists in the mempool.
     * @param block The block hash to check.
     * @return `true` if the block exists, `false` otherwise.
     */
    bool exists(const Hash& block);

    /**
     * Get a block from the mempool by its hash.
     * @param hash The block hash to get.
     * @return The found block, or `nullptr` if block is not found.
     */
    const std::shared_ptr<const Block> getBlock(const Hash& hash);
};

#endif  // BLOCKMEMPOOL_H
