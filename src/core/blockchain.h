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
 *
 * NOTE: If you need a testing version of a blockchain node, you should derive it
 * from this class, instead of creating another separate class. All components
 * (State, ...) are allowed to expect a mutable Blockchain& in their constructor,
 * so you may need to create a custom one to wrap the component to be tested.
 */
class Blockchain : public CometListener, public NodeRPCInterface, public Log::LogicalLocationProvider {
  // protected is correct; don't change.
  // all components must be ready for any kind of access/use pattern, as long
  //   as the Blockchain object is still existing, of course.
  protected:
    Options options_; ///< Options singleton.
    Comet comet_;     ///< CometBFT consensus engine driver.
    State state_;     ///< Blockchain state.
    HTTPServer http_; ///< HTTP server.

    // TODO: CometListener

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

  public:
    const std::string instanceId_;
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
