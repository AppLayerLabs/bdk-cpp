/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "../utils/options.h"
//#include "../utils/db.h"
#include "comet.h"
#include "state.h"
#include "storage.h"
#include "../net/http/httpserver.h"
#include "../net/http/noderpcinterface.h"

/**
 * A BDK node.
 * This is the nexus object that brings together multiple blockchain node
 * components by composition. The lifetime of all components and the nexus
 * object are the same.
 * All components must be thread-safe.
 *
 * NOTE: If you need a testing version of a blockchain node, you should derive it
 * from this class, instead of creating another separate class. All components
 * (State, ...) are allowed to expect a mutable Blockchain& in their constructor,
 * so you may need to create a custom one to wrap the component to be tested.
 *
 * NOTE: Each Blockchain instance has one file-backed, durable store (db_) to
 * implement internal features as needed. However, the db_ should not store:
 * (i) any data that the consensus engine (cometbft) already stores and
 * that we have no reason to duplicate, such as blocks (OK to cache in RAM),
 * (ii) State (consistent contract checkpoints/snapshots at some execution
 * height) as those should be serialized/deserialized from/to flat files and
 * not a database,
 * (iii) the list of contract types/templates that exist, since that pertains
 * to the binary itself, and should be built statically in RAM on startup (const)
 * (it is OK-ish to store, in db_, the range of block heights for which a
 * template is or isn't available, while keeping in mind that these are really
 * consensus parameters, that is, changing these means the protocol is forked).
 * What we are going to store in the node db:
 * - Mapping from sha3 to sha256 for every known transaction (so we can query
 * the transaction store in comet for which we will always enable indexing;
 * also note this should be pruned at the same retain-height we send to comet),
 * - Activation block height for contract templates (if absent, the contract
 * template is not yet enabled) -- this should be directly modifiable by node
 * operators when enabling new contracts (these are soft forks).
 * - Events (we will keep these in the BDK to retain control and speed it up)
 * - Any State metadata we may need (e.g. name of latest state snapshot file
 * that was saved, and the last executed, locally seen, final block height).
 */
class Blockchain : public CometListener, public NodeRPCInterface, public Log::LogicalLocationProvider {
  protected: // protected is correct; don't change.
    const std::string instanceId_; ///< Instance ID for logging.

    Options options_; ///< Options singleton.
    Comet comet_;     ///< CometBFT consensus engine driver.
    State state_;     ///< Blockchain state.
    Storage storage_; ///< Persistent store front-end.
    HTTPServer http_; ///< HTTP server.

    // REVIEW: Do we need this if we have storage_ already? storage_ should be the front-end to all our persistent storage needs.
    // Shouldn't we just have just one database directory with everything in it?
    //const DB db_;     ///< Durable data store.

    std::vector<CometValidatorUpdate> validators_; ///< Up-to-date CometBFT validator set.

  public:

    // ------------------------------------------------------------------
    // CometListener
    // ------------------------------------------------------------------

    virtual void initChain(
      const uint64_t genesisTime, const std::string& chainId, const Bytes& initialAppState, const uint64_t initialHeight,
      const std::vector<CometValidatorUpdate>& initialValidators, Bytes& appHash
    ) override;
    virtual void checkTx(const Bytes& tx, int64_t& gasWanted, bool& accept) override;
    virtual void incomingBlock(
      const uint64_t syncingToHeight, std::unique_ptr<CometBlock> block, Bytes& appHash,
      std::vector<CometExecTxResult>& txResults, std::vector<CometValidatorUpdate>& validatorUpdates
    ) override;
    virtual void buildBlockProposal(
      const uint64_t maxTxBytes, const CometBlock& block, bool& noChange, std::vector<size_t>& txIds
    ) override;
    virtual void validateBlockProposal(const CometBlock& block, bool& accept) override;
    virtual void getCurrentState(uint64_t& height, Bytes& appHash, std::string& appSemVer, uint64_t& appVersion) override;
    virtual void getBlockRetainHeight(uint64_t& height) override;
    virtual void currentCometBFTHeight(const uint64_t height) override;
    virtual void sendTransactionResult(const uint64_t tId, const Bytes& tx, const bool success, const std::string& txHash, const json& response) override;
    virtual void checkTransactionResult(const uint64_t tId, const std::string& txHash, const bool success, const json& response) override;
    virtual void rpcAsyncCallResult(const uint64_t tId, const std::string& method, const json& params, const bool success, const json& response) override;
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
    virtual json eth_getUncleByBlockHashAndIndex() override;

    std::string getLogicalLocation() const override { return instanceId_; }

    /**
     * Constructor.
     * @param blockchainPath Root filesystem path for this blockchain node reading/writing data.
     * @param instanceId Runtime object instance identifier for logging (for multi-node unit tests).
     */
    explicit Blockchain(const std::string& blockchainPath, std::string instanceId = "");

    /**
     * Constructor.
     * @param options Explicit options object to use (overrides any existing options file on blockchainPath).
     * @param blockchainPath Root filesystem path for this blockchain node reading/writing data.
     * @param instanceId Runtime object instance identifier for logging (for multi-node unit tests).
     */
    explicit Blockchain(const Options& options, const std::string& blockchainPath, std::string instanceId = "");

    ~Blockchain() = default; ///< Default destructor.

    void start(); ///< Start the blockchain node.

    void stop(); ///< Stop the blockchain node.

    ///@{
    /** Getter. */
    Options& opt() { return this->options_; }
    Comet& comet() { return this->comet_; }
    State& state() { return this->state_; }
    Storage& storage() { return this->storage_; }
    HTTPServer& http() { return this->http_; }
    //const DB& db() { return this->db_; }
    ///@}
};

#endif // BLOCKCHAIN_H
