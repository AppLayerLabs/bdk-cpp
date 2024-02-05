/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "storage.h"
#include "rdpos.h"
#include "state.h"
#include "../net/p2p/managerbase.h"
#include "../net/http/httpserver.h"
#include "../utils/options.h"
#include "../utils/db.h"

// Forward declarations.
class Syncer;

/**
 * Master class that represents the blockchain as a whole.
 * Contains, and acts as the middleman of, every other part of the core and net protocols.
 * Those parts interact with one another by communicating through this class.
 */
class Blockchain {
  private:
    const std::unique_ptr<Options> options_; ///< Pointer to the options singleton.
    const std::unique_ptr<DB> db_; ///< Pointer to the database.
    const std::unique_ptr<Storage> storage_; ///< Pointer to the blockchain storage.
    const std::unique_ptr<State> state_; ///< Pointer to the blockchain state.
    const std::unique_ptr<rdPoS> rdpos_; ///< Pointer to the rdPoS object (consensus).
    const std::unique_ptr<P2P::ManagerNormal> p2p_; ///< Pointer to the P2P connection manager.
    const std::unique_ptr<HTTPServer> http_; ///< Pointer to the HTTP server.
    const std::unique_ptr<Syncer> syncer_; ///< Pointer to the blockchain syncer.

  public:
    /**
     * Constructor.
     * @param blockchainPath Root path of the blockchain.
     */
    explicit Blockchain(const std::string& blockchainPath);

    /// Default destructor.
    ~Blockchain() = default;

    /**
     * Start the blockchain.
     * Initializes P2P, HTTP and Syncer, in this order.
     */
    void start();

    /**
     * Stop/shutdown the blockchain.
     * Stops Syncer, HTTP and P2P, in this order (reverse order of start()).
     */
    void stop();

    /// Getter for `options_`.
    const std::unique_ptr<Options>& getOptions() const { return this->options_; };

    /// Getter for `db_`.
    const std::unique_ptr<DB>& getDB() const { return this->db_; };

    /// Getter for `storage_`.
    const std::unique_ptr<Storage>& getStorage() const { return this->storage_; };

    /// Getter for `rdpos_`.
    const std::unique_ptr<rdPoS>& getrdPoS() const { return this->rdpos_; };

    /// Getter for `state_`.
    const std::unique_ptr<State>& getState() const { return this->state_; };

    /// Getter for `p2p_`.
    const std::unique_ptr<P2P::ManagerNormal>& getP2P() const { return this->p2p_; };

    /// Getter for `http_`.
    const std::unique_ptr<HTTPServer>& getHTTP() const { return this->http_; };

    /// Getter for `syncer_`.
    const std::unique_ptr<Syncer>& getSyncer() const { return this->syncer_; };

    /**
     * Check if the blockchain syncer is synced.
     * @return `true` if the syncer is synced, `false` otherwise.
     */
    const std::atomic<bool>& isSynced() const;

    friend class Syncer;
};

/**
 * Helper class that syncs the node with the network.
 * This is where the magic happens between the nodes on the network, as the
 * class is responsible for syncing both, broadcasting transactions and also
 * creating new blocks if the node is a Validator.
 * Currently it's *single threaded*, meaning that it doesn't require mutexes.
 * TODO: This could also be responsible for slashing rdPoS if they are not behaving correctly
 * TODO: Maybe it is better to move rdPoSWorker to Syncer?
 */
class Syncer {
  private:
    /// Reference to the parent blockchain.
    Blockchain& blockchain_;

    /// List of currently connected nodes and their info.
    std::unordered_map<P2P::NodeID, P2P::NodeInfo, SafeHash> currentlyConnectedNodes_;

    /// Pointer to the blockchain's latest block.
    std::shared_ptr<const Block> latestBlock_;

    /// Update `currentlyConnectedNodes`.
    void updateCurrentlyConnectedNodes();

    /// Check latest block (used by validatorLoop()).
    bool checkLatestBlock();

    /// Do the syncing.
    void doSync();

    /**
     * Create and broadcast a Validator block (called by validatorLoop()).
     * If the node is a Validator and it has to create a new block,
     * this function will be called, the new block will be created based on the
     * current State and rdPoS objects, and then it will be broadcasted.
     * @throw std::runtime_error if block is invalid.
     */
    void doValidatorBlock();

    /**
     * Wait for a new block (called by validatorLoop()).
     * If the node is a Validator, this function will be called to make the
     * node wait until it receives a new block.
     */
    void doValidatorTx() const;

    /// Routine loop for when the node is a Validator.
    void validatorLoop();

    /// Routine loop for when the node is NOT a Validator.
    void nonValidatorLoop() const;

    /// Routine loop for the syncer worker.
    bool syncerLoop();

    /// Future object holding the thread for the syncer loop.
    std::future<bool> syncerLoopFuture_;

    /// Flag for stopping the syncer.
    std::atomic<bool> stopSyncer_ = false;

    /// Indicates whether or not the syncer is synced.
    std::atomic<bool> synced_ = false;

  public:
    /**
     * Constructor.
     * @param blockchain Reference to the parent blockchain.
     */
    explicit Syncer(Blockchain& blockchain) : blockchain_(blockchain) {};

    /**
     * Destructor.
     * Automatically stops the syncer.
     */
    ~Syncer() { this->stop(); };

    /// Getter for `synced`.
    const std::atomic<bool>& isSynced() const { return this->synced_; }

    /// Start the syncer routine loop.
    void start();

    /// Stop the syncer routine loop.
    void stop();
};

#endif // BLOCKCHAIN_H
