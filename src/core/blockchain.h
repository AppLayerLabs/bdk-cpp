/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "../utils/options.h"
#include "comet.h"
#include "state.h"
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
 */
class Blockchain : public CometListener, public NodeRPCInterface, public Log::LogicalLocationProvider {
  protected: // protected is correct; don't change.
    const std::string instanceId_; ///< Instance ID for logging.
    Options options_; ///< Options singleton.
    Comet comet_;     ///< CometBFT consensus engine driver.
    State state_;     ///< Blockchain state.
    HTTPServer http_; ///< HTTP server.

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
      const uint64_t height, const uint64_t syncingToHeight, const std::vector<Bytes>& txs, const Bytes& proposerAddr, const uint64_t timeNanos,
      Bytes& appHash, std::vector<CometExecTxResult>& txResults, std::vector<CometValidatorUpdate>& validatorUpdates
    ) override;
    virtual void buildBlockProposal(const std::vector<Bytes>& txs, std::unordered_set<size_t>& delTxIds) override;
    virtual void validateBlockProposal(const uint64_t height, const std::vector<Bytes>& txs, bool& accept) override;
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
    virtual json eth_getTransactionByBlockHashAndIndex(const json& request)override;
    virtual json eth_getTransactionByBlockNumberAndIndex(const json& request) override;
    virtual json eth_getTransactionReceipt(const json& request) override;
    virtual json eth_getUncleByBlockHashAndIndex() override;

    std::string getLogicalLocation() const override { return instanceId_; }

    /**
     * Constructor.
     * @param blockchainPath Root path of the blockchain.
     */
    explicit Blockchain(const std::string& blockchainPath, std::string instanceId = "");
    ~Blockchain() = default;  ///< Default destructor.
    void start(); ///< Start the blockchain node.
    void stop();  ///< Stop the blockchain node.

    ///@{
    /** Getter. */
    Options& opt() { return this->options_; }
    Comet& comet() { return this->comet_; }
    State& state() { return this->state_; }
    HTTPServer& http() { return this->http_; }
    ///@}
};

#endif // BLOCKCHAIN_H
