/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "../utils/options.h"
#include "../utils/bucketcache.h"
#include "../utils/finalizedblock.h"
#include "comet.h"
#include "state.h"
#include "storage.h"
#include "../net/http/httpserver.h"
#include "../net/http/noderpcinterface.h"

/**
 * A BDK node.
 * All components must be thread-safe.
 *
 * TODO: Should create a BlockchainListener that receives events from the
 * Blockchain class. During normal node operation, it would be null, but it
 * would be useful for testing; e.g. a validatorSetChange() notification.
 * The listener callback methods could also take outparams, allowing the
 * listener in the testcase to react to callbacks and control the node.
 * This could be paired with a testing interface (something like
 * Blockchain::tester() that would return the node's testing interface).
 */
class Blockchain : public CometListener, public NodeRPCInterface, public Log::LogicalLocationProvider {
  protected: // protected is correct; don't change.
    /// A <blockHeight, blockIndex> tuple.
    struct TxCacheValueType {
      uint64_t blockHeight; ///< Height of the block this transaction is included in.
      uint64_t blockIndex; ///< Index inside the block this transaction is included in.
    };

    /**
     * RAM cache of FinalizedBlock objects.
     * NOTE: This class is thread-safe.
     */
    class FinalizedBlockCache
    {
    public:
      explicit FinalizedBlockCache(size_t capacity);
      void insert(std::shared_ptr<const FinalizedBlock> block);
      std::shared_ptr<const FinalizedBlock> getByHeight(uint64_t height);
      std::shared_ptr<const FinalizedBlock> getByHash(const Hash& hash);
      void clear();
    private:
      std::vector<std::shared_ptr<const FinalizedBlock>> ring_;
      size_t nextInsertPos_;
      size_t capacity_;
      std::map<uint64_t, std::shared_ptr<const FinalizedBlock>> byHeight_;
      std::unordered_map<Hash, std::shared_ptr<const FinalizedBlock>, SafeHash> byHash_;
      std::shared_mutex mutex_;
    };

    const std::string instanceId_; ///< Instance ID for logging.

    // NOTE: Comet should be destructed (stopped) before State, hence it should be declared after it.
    Options options_; ///< Options singleton.
    Storage storage_; ///< BDK persistent store front-end.
    State state_;     ///< Blockchain state.
    Comet comet_;     ///< CometBFT consensus engine driver.
    HTTPServer http_; ///< HTTP server.

    std::atomic<std::shared_ptr<const FinalizedBlock>> latest_; ///< Pointer to the "latest" block in the blockchain.
    BucketCache<Hash, TxCacheValueType, SafeHash> txCache_; ///< Cache of transaction hash to (block height, block index).
    BucketCache<uint64_t, Hash, SafeHash> blockHeightToHashCache_; ///< Block height to block hash cache.
    FinalizedBlockCache fbCache_; ///< RAM finalized block/tx cache.

    std::unordered_map<Hash, std::shared_ptr<TxBlock>, SafeHash> mempool_; ///< Cache of pending transactions.
    std::shared_mutex mempoolMutex_; ///< Mutex protecting mempool_.

    bool syncing_ = false; ///< Updated by Blockchain::incomingBlock() when syncingToHeight > height.
    uint64_t persistStateSkipCount_ = 0; ///< Count of non-syncing_ Blockchain::persistState() calls that skipped saveSnapshot().

    std::atomic<bool> started_ = false; ///< Flag to protect the start()/stop() cycle.

    std::mutex incomingBlockLockMutex_; ///< For locking/unlocking block processing.
    std::atomic<bool> incomingBlockLock_ = false; ///< `true` if incomingBlock() is locked.

    /**
     * Given a block JSON obtained via CometBFT RPC, store it in the relevant caches.
     * @param block The full CometBFT block in JSON format.
     * @return Pointer to the (now cached) FinalizedBlock built from parsing `block`.
     */
    std::shared_ptr<const FinalizedBlock> cacheBlock(const json& block);

    /**
     * Fetch a CometBFT block via CometBFT RPC by hash.
     * @param blockHash The block hash (eth sha3).
     * @param ret Outparam set to the JSON response.
     * @return `true` if the request was successful, `false` if it failed.
     */
    bool getBlockRPC(const Hash& blockHash, json& ret);

    /**
     * Fetch a CometBFT block via CometBFT RPC by height.
     * @param blockHeight The block height.
     * @param ret Outparam set to the JSON response.
     * @return `true` if the request was successful, `false` if it failed.
     */
    bool getBlockRPC(const uint64_t blockHeight, json& ret);

    /**
     * Fetch a CometBFT transaction via CometBFT RPC.
     * @param txHash The transaction hash (eth sha3).
     */
    bool getTxRPC(const Hash& txHash, json& ret);

    /**
     * Stores a mapping of transaction hash (sha3) to block height and index
     * within the block in the txChache_.
     * @param tx Transaction hash.
     * @param val Block height and transaction index within that block.
     */
    void putTx(const Hash& tx, const TxCacheValueType& val);

    /**
     * Helper for BDK RPC services.
     */
    static std::tuple<Address, Address, Gas, uint256_t, Bytes> parseMessage(const json& request, bool recipientRequired);

    /**
     * Helper for BDK RPC services.
     */
    json getBlockJson(const FinalizedBlock *block, bool includeTransactions);

    /**
     * Reset all Blockchain state.
     */
    void cleanup();

  public:

    /// A <TxBlock, blockIndex, blockHeight> tuple.
    struct GetTxResultType {
      std::shared_ptr<TxBlock> txBlockPtr; ///< The transaction object.
      uint64_t blockIndex; ///< Index inside the block this transaction is included in.
      uint64_t blockHeight; ///< Height of the block this transaction is included in.
    };

    // ------------------------------------------------------------------
    // CometListener
    // ------------------------------------------------------------------

    virtual void initChain(
      const uint64_t genesisTime, const std::string& chainId, const Bytes& initialAppState, const uint64_t initialHeight,
      const std::vector<CometValidatorUpdate>& initialValidators, Bytes& appHash
    ) override;
    virtual void checkTx(const Bytes& tx, const bool recheck, int64_t& gasWanted, bool& accept) override;
    virtual void incomingBlock(
      const uint64_t syncingToHeight, std::unique_ptr<CometBlock> block, Bytes& appHash,
      std::vector<CometExecTxResult>& txResults, std::vector<CometValidatorUpdate>& validatorUpdates
    ) override;
    virtual void buildBlockProposal(
      const uint64_t maxTxBytes, const CometBlock& block, bool& noChange, std::vector<size_t>& txIds,
      std::vector<Bytes>& injectTxs
    ) override;
    virtual void validateBlockProposal(const CometBlock& block, bool& accept) override;
    virtual void getCurrentState(uint64_t& height, Bytes& appHash, std::string& appSemVer, uint64_t& appVersion) override;
    virtual void persistState(uint64_t& height) override;
    virtual void currentCometBFTHeight(const uint64_t height, const json& lastBlock) override;
    virtual void sendTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash, const Bytes& tx) override;
    virtual void checkTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash) override;
    virtual void rpcAsyncCallResult(const uint64_t tId, const bool success, const json& response, const std::string& method, const json& params) override;
    virtual void cometStateTransition(const CometState newState, const CometState oldState) override;

    // ------------------------------------------------------------------
    // NodeRPCInterface
    // ------------------------------------------------------------------

    virtual json web3_clientVersion(const json& request) override;
    virtual json web3_sha3(const json& request) override;
    virtual json net_version(const json& request) override;
    virtual json net_listening(const json& request) override;
    virtual json eth_protocolVersion(const json& request) override;
    virtual json net_peerCount(const json& request) override;
    virtual json eth_getBlockByHash(const json& request)override;
    virtual json eth_getBlockByNumber(const json& request) override;
    virtual json eth_getBlockTransactionCountByHash(const json& request) override;
    virtual json eth_getBlockTransactionCountByNumber(const json& request) override;
    virtual json eth_chainId(const json& request) override;
    virtual json eth_syncing(const json& request) override;
    virtual json eth_coinbase(const json& request) override;
    virtual json eth_blockNumber(const json& request) override;
    virtual json eth_call(const json& request) override;
    virtual json eth_estimateGas(const json& request) override;
    virtual json eth_gasPrice(const json& request) override;
    virtual json eth_feeHistory(const json& request) override;
    virtual json eth_getLogs(const json& request) override;
    virtual json eth_getBalance(const json& request) override;
    virtual json eth_getTransactionCount(const json& request) override;
    virtual json eth_getCode(const json& request) override;
    virtual json eth_sendRawTransaction(const json& request) override;
    virtual json eth_getTransactionByHash(const json& request) override;
    virtual json eth_getTransactionByBlockHashAndIndex(const json& request) override;
    virtual json eth_getTransactionByBlockNumberAndIndex(const json& request) override;
    virtual json eth_getTransactionReceipt(const json& request) override;
    virtual json eth_getUncleByBlockHashAndIndex(const json& request) override;
    virtual json debug_traceBlockByNumber(const json& request) override;
    virtual json debug_traceTransaction(const json& request) override;
    virtual json txpool_content(const json& request) override;

    std::string getLogicalLocation() const override { return instanceId_; } ///< Log helper.

    /**
     * Constructor.
     * @param blockchainPath Root filesystem path for this blockchain node reading/writing data.
     * @param instanceId Runtime object instance identifier for logging (for multi-node unit tests).
     * @param forceSeedMode `true` forces it to run in seed mode (discovery node); `false` leaves the
     * default specified in the CometBFT config option (which by default sets seed_mode = false).
     */
    explicit Blockchain(const std::string& blockchainPath, std::string instanceId = "", bool forceSeedMode = false);

    /**
     * Constructor.
     * @param options Explicit options object to use.
     * @param instanceId Runtime object instance identifier for logging (for multi-node unit tests).
     * @param forceSeedMode `true` forces it to run in seed mode (discovery node); `false` leaves the
     * default specified in the CometBFT config option (which by default sets seed_mode = false).
     */
    explicit Blockchain(const Options& options,  std::string instanceId = "", bool forceSeedMode = false);

    /**
     * Destructor; ensures node is stopped.
     */
    virtual ~Blockchain();

    /**
     * Lock block processing.
     * Prevents incomingBlock() Comet callback from executing or being in execution after this call.
     * Blockchain::state().getHeight() will only return a non-changing value after this call.
     */
    void lockBlockProcessing();

    /**
     * Cancels lockBlockProcessing(), unlocking incomingBlock().
     */
    void unlockBlockProcessing();

    /**
     * Get number of unconfirmed txs in the CometBFT mempool.
     * @return Integer value returned by RPC num_unconfirmed_txs in result::num_txs, or -1 on error.
     */
    int getNumUnconfirmedTxs();

    /**
     * Set the size of the GetTx() cache.
     * If you set it to 0, you turn off the cache.
     * NOTE: CometBFT has a lag between finalizing a block and indexing transactions,
     * so if you turn off the cache, you need to take that into consideration when
     * using methods like Storage::getTx() which will hit cometbft with a 'tx' RPC
     * call and possibly fail because the transaction hasn't been indexed yet.
     * @param cacheSize Maximum size in entries for each bucket (two rotating buckets).
     */
    void setGetTxCacheSize(const uint64_t cacheSize);

    /**
     * Save the current machine state to <blockchainPath>/snapshots/<current-State-height>.
     * May throw on error.
     */
    void saveSnapshot();

    /**
     * Start the blockchain node. If already running, does nothing.
     * @throws DynamicException if the node fails to start for whatever reason.
     */
    void start();

    /**
     * Stop the blockchain node. If not running, does nothing.
     */
    void stop();

    /**
     * Interrupt the blockchain node.
     * This is safe to call from a signal handler.
     * NOTE: Calling stop() later is still required to properly stop the node.
     */
    void interrupt();

    /**
     * Get the latest block processed into the current blockchain state.
     * @return The last block that was processed to generate the current state; note that it is
     * not necessarily the most recent block available in the backing CometBFT block store.
     */
    std::shared_ptr<const FinalizedBlock> latest() const;

    /**
     * Get the height of the lastest finalized block.
     * @return Height of the latest block, or 0 if no blocks (i.e. blockchain is in genesis state).
     */
    uint64_t getLatestHeight() const;

    /**
     * Get a block from the chain using a given hash.
     * @param hash The block hash to get.
     * @param retPtr If not null, forces an RPC request (bypasses fbCache_) and returns the full JSON response.
     * @return A pointer to the found block, or `nullptr` if block is not found.
     */
    std::shared_ptr<const FinalizedBlock> getBlock(const Hash& hash, json* retPtr = nullptr);

    /**
     * Get a block from the chain using a given height.
     * @param height The block height to get.
     * @param retPtr If not null, forces an RPC request (bypasses fbCache_) and returns the full JSON response.
     * @return A pointer to the found block, or `nullptr` if block is not found.
     */
    std::shared_ptr<const FinalizedBlock> getBlock(uint64_t height, json* retPtr = nullptr);

    /**
     * Get the block hash given a block height.
     * @param height The block height.
     * @param forceRPC Bypass all local caches and force getting the block hash from CometBFT.
     * @return The block hash, or an empty hash if can't find it.
     */
    Hash getBlockHash(const uint64_t height, bool forceRPC = false);

    /**
     * Get a transaction from the chain using a given hash.
     * @param tx The transaction hash to get.
     * @return A tuple with the found shared_ptr<TxBlock>, transaction index in the block, and block height.
     * @throw DynamicException on hash mismatch.
     */
    Blockchain::GetTxResultType getTx(const Hash& tx);

    /**
     * Get a transaction from a block with a specific index.
     * @param blockHash The block hash
     * @param blockIndex the index within the block
     * @return A tuple with the found shared_ptr<TxBlock>, transaction index in the block, and block height.
     * @throw DynamicException on hash mismatch.
     */
    Blockchain::GetTxResultType getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex);

    /**
     * Get a transaction from a block with a specific index.
     * @param blockHeight The block height
     * @param blockIndex The index within the block.
     * @return A tuple with the found shared_ptr<TxBlock>, transaction index in the block, and block height.
     */
    Blockchain::GetTxResultType getTxByBlockNumberAndIndex(uint64_t blockHeight, uint64_t blockIndex);

    /**
     * Get an uncofirmed transaction from our mirror mempool.
     * @param txHash Hash of the transaction.alignas
     * @return An empty pointer if it is not in the mirror mempool, or a shared_ptr to a TxBlock of the
     * pending transaction if it was found.
     */
    std::shared_ptr<TxBlock> getUnconfirmedTx(const Hash& txHash);

    ///@{
    /** Getter. */
    Options& opt() { return this->options_; }
    Comet& comet() { return this->comet_; }
    State& state() { return this->state_; }
    Storage& storage() { return this->storage_; }
    HTTPServer& http() { return this->http_; }
    ///@}
};

#endif // BLOCKCHAIN_H
