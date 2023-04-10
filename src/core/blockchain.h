#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "storage.h"
#include "rdpos.h"
#include "state.h"
#include "../net/p2p/p2pmanagerbase.h"
#include "../net/http/httpserver.h"
#include "../utils/options.h"
#include "../utils/db.h"

/// Forward declaration
class Syncer;

class Blockchain {
  private:
    /// Options Object
    const std::unique_ptr<Options> options;

    /// DB Object
    const std::unique_ptr<DB> db;

    /// Storage Object (Blockchain DB)
    const std::unique_ptr<Storage> storage;

    /// rdPoS Object (Blockchain Consensus)
    const std::unique_ptr<rdPoS> rdpos;

    /// State Object (Blockchain State)
    const std::unique_ptr<State> state;

    /// P2PManagerBase Object (Blockchain P2P)
    const std::unique_ptr<P2P::ManagerNormal> p2p;

    /// HTTPServer Object (Blockchain HTTP)
    const std::unique_ptr<HTTPServer> http;

    /// Syncer Object (Blockchain Syncer)
    const std::unique_ptr<Syncer> syncer;

  public:
    Blockchain(std::string blockchainPath);
    ~Blockchain() {};

    /**
     * Start Blockchain Syncing Engine
     * Besides initializing the p2p server and http server, this function will start the Syncing engine
     * described within the Syncer class.
     */

    void start();

    void stop();

    /// Getters (mostly used by Test Cases)
    const std::unique_ptr<Options>& getOptions() const { return this->options; };
    const std::unique_ptr<DB>& getDB() const { return this->db; };
    const std::unique_ptr<Storage>& getStorage() const { return this->storage; };
    const std::unique_ptr<rdPoS>& getrdPoS() const { return this->rdpos; };
    const std::unique_ptr<State>& getState() const { return this->state; };
    const std::unique_ptr<P2P::ManagerNormal>& getP2P() const { return this->p2p; };
    const std::unique_ptr<HTTPServer>& getHTTP() const { return this->http; };
    const std::unique_ptr<Syncer>& getSyncer() const { return this->syncer; };
    const std::atomic<bool>& isSynced() const;

    friend class Syncer;
};


/**
 * Syncer class is the main class where the magic happens between the nodes on the network
 * This function is responsible for syncing the node with the network, broadcast transactions
 * and also for creating new blocks if the node is a validator
 * TODO: This function can also be responsible for slashing validators if they are not behaving correctlyng
 * TODO: Maybe it is better to move rdPoSWorker to Syncer?
 * Currently Syncer is *single threaded*, meaning that it doesn't required mutexes.
 */
class Syncer {
  private:
    /// Reference back to the blockchain
    Blockchain& blockchain;
    /// List of currently connected nodes and their info
    std::unordered_map<Hash, P2P::NodeInfo, SafeHash> currentlyConnectedNodes;

    /// Pointer to latestBlock
    std::shared_ptr<const Block> latestBlock;

    /// Function to update currentlyConnectedNodes
    void updateCurrentlyConnectedNodes();

    /// Function to check latest block (used by validatorLoop())
    bool checkLatestBlock();

    /// Syncing function
    void doSync();

    /**
     * If the node is a validator and it has to create a new block, this function will be called
     * The new block will be created based on the State and rdPoS objects, and then it will be broadcasted
     * Called by validatorLoop()
     */
    void doValidatorBlock();

    /**
     * If the node is a validator, this function will be called to make this node waits until it receives a new block
     * Called by validatorLoop()
     */
    void doValidatorTx();

    /// Function loop if the node is a validator
    void validatorLoop();

    /// Function loop if the node is a not a validator
    void nonValidatorLoop();

    /// Function loop for worker
    bool syncerLoop();

    /// Future object holding the thread for the Syncer loop
    std::future<bool> syncerLoopFuture;

    /// Syncer stopper
    std::atomic<bool> stopSyncer = false;

    /// Atomic bool to tell we are synced
    std::atomic<bool> synced = false;

  public:
    Syncer(Blockchain& blockchain) : blockchain(blockchain) {};
    ~Syncer() { this->stop(); };

    /// Start Syncer Loop
    void start();

    /// Stop Syncer Loop
    void stop();

    /// Getter for synced
    const std::atomic<bool>& isSynced() const { return this->synced; }
};


#endif /// BLOCKCHAIN_H