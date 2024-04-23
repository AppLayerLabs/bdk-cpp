/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "consensus.h"
#include "storage.h"
#include "state.h"
#include "rdpos.h"
#include "state.h"

#include "../net/p2p/managerbase.h"
#include "../net/p2p/nodeconns.h"
#include "../net/http/httpserver.h"
#include "../utils/options.h"
#include "../utils/db.h"

class Blockchain; // Forward declaration for Syncer.

/**
 * Helper class that syncs the node with other nodes in the network.
 * Currently it's *single threaded*, meaning that it doesn't require mutexes.
 */
class Syncer {
  private:
    P2P::ManagerNormal& p2p_;  ///< Reference to the P2P networking engine.
    const Storage& storage_;   ///< Reference to the blockchain storage.
    State& state_;             ///< reference to the blockchain state.
    std::atomic<bool> synced_ = false;  ///< Indicates whether or not the syncer is synced.

  public:
    /**
     * Constructor.
     * @param nodeConns Reference to the NodeConns object.
     * @param storage Reference to the blockchain storage.
     */
    explicit Syncer(P2P::ManagerNormal& p2p, const Storage& storage, State& state) :
      p2p_(p2p), storage_(storage), state_(state) {}

    /**
     * Synchronize this node to the latest known blocks among all connected peers at the time this method is called.
     * @param tries If zero, try forever, otherwise try block downloads a set number of times.
     * @return `true` if successfully synced, `false` otherwise.
     */
    bool sync(int tries = 0);

    ///@{
    /** Getter. */
    const std::atomic<bool>& isSynced() const { return this->synced_; }
    ///@}
};

/**
 * Master class that represents the blockchain as a whole.
 * Contains, and acts as the middleman of, every other part of the core and net protocols.
 * Those parts interact with one another by communicating through this class.
 */
class Blockchain {
  private:
    Options options_;           ///< Options singleton.
    DB db_;                     ///< Database.
    Storage storage_;           ///< Blockchain storage.
    State state_;               ///< Blockchain state.
    P2P::ManagerNormal p2p_;    ///< P2P connection manager.
    HTTPServer http_;           ///< HTTP server.
    Syncer syncer_;             ///< Blockchain syncer.
    Consensus consensus_;       ///< Block and transaction processing.

  public:
    /**
     * Constructor.
     * @param blockchainPath Root path of the blockchain.
     */
    explicit Blockchain(const std::string& blockchainPath);
    ~Blockchain() = default;  ///< Default destructor.
    void start(); ///< Start the blockchain. Initializes P2P, HTTP and Syncer, in this order.
    void stop();  ///< Stop/shutdown the blockchain. Stops Syncer, HTTP and P2P, in this order (reverse order of start()).

    ///@{
    /** Getter. */
    Options& getOptions() { return this->options_; }
    DB& getDB() { return this->db_; }
    Storage& getStorage() { return this->storage_; }
    State& getState() { return this->state_; }
    P2P::ManagerNormal& getP2P() { return this->p2p_; }
    HTTPServer& getHTTP() { return this->http_; }
    Syncer& getSyncer() { return this->syncer_; }
    Consensus& getConsensus() { return this->consensus_; }
    ///@}

    /// Check if the blockchain is synced.
    const std::atomic<bool>& isSynced() const { return this->syncer_.isSynced(); }
};

#endif // BLOCKCHAIN_H
