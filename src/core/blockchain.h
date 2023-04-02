#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "storage.h"
#include "rdpos.h"
#include "state.h"
#include "../net/p2p/p2pmanagerbase.h"
#include "../net/http/httpserver.h"
#include "../utils/options.h"
#include "../utils/db.h"



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


  public:
    Blockchain(std::string blockchainPath);
    ~Blockchain() {};

    /**
     * Start Blockchain Syncing Engine
     * Besides initializing the p2p server and http server, this function will start the Syncing engine
     * described within the Syncer class.
     */

    void start();

    friend class Syncer;
};



#endif /// BLOCKCHAIN_H