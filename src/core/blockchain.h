/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "broadcaster.h"
#include "storage.h"
#include "rdpos.h"
#include "state.h"

#include "../net/p2p/managerbase.h"
#include "../net/p2p/nodeconns.h"
#include "../net/http/httpserver.h"
#include "../utils/options.h"
#include "../utils/db.h"

class Blockchain; // Forward declaration for Syncer.

/**
 * Helper class that syncs the node with other nodes in the network,
 * broadcasts transactions and creates new blocks if the node is a Validator.
 * This is where the magic happens.
 * Currently it's *single threaded*, meaning that it doesn't require mutexes.
 */
class Syncer {
  // TODO: Maybe this class could also be responsible for slashing rdPoS if they are not behaving correctly
  // TODO: Maybe it is better to move rdPoSWorker to Syncer
  private:
    Blockchain& blockchain_;  ///< Reference to the parent blockchain.
    std::future<bool> syncerLoopFuture_;  ///< Future object holding the thread for the syncer loop.
    std::atomic<bool> stopSyncer_ = false;  ///< Flag for stopping the syncer.
    std::atomic<bool> synced_ = false;  ///< Indicates whether or not the syncer is synced.

    void doSync(); ///< Do the syncing.
    bool syncerLoop();  ///< Routine loop for the syncer worker.

  public:
    /**
     * Constructor.
     * @param blockchain Reference to the parent blockchain.
     */
    explicit Syncer(Blockchain& blockchain) : blockchain_(blockchain) {}

    /// Destructor. Automatically stops the syncer.
    ~Syncer() { this->stop(); }

    ///@{
    /** Getter. */
    const std::atomic<bool>& isStopped() const { return this->stopSyncer_; }
    const std::atomic<bool>& isSynced() const { return this->synced_; }
    ///@}

    void start(); ///< Start the syncer routine loop.
    void stop();  ///< Stop the syncer routine loop.
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
    P2P::NodeConns nodeConns_;  ///< Node connection manager.
    Syncer syncer_;             ///< Blockchain syncer.
    Broadcaster broadcaster_;   ///< Block and transaction broadcaster.

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
    P2P::NodeConns& getNodeConns() { return this->nodeConns_; }
    Syncer& getSyncer() { return this->syncer_; }
    Broadcaster& getBroadcaster() { return this->broadcaster_; }
    ///@}

    const std::atomic<bool>& isSynced() const;  ///< Check if the blockchain syncer is synced.

    friend class Syncer; ///< Friend class.
};

#endif // BLOCKCHAIN_H
