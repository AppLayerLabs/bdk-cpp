#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <mutex>

#include "../net/p2p/p2pmanager.h"
#include "../net/httpserver.h"
#include "../net/grpcserver.h"
#include "../utils/block.h"
#include "../utils/db.h"
#include "../utils/json.hpp"
#include "../utils/tx.h"

#include "rdpos.h"
#include "snowmanVM.h"
#include "storage.h"
#include "state.h"

using json = nlohmann::ordered_json;
using grpc::Server;

// Forward declarations.
class HTTPServer;
class State;

/**
 * Master class that represents the blockchain as a whole.
 * Contains and acts as the middleman of every other part of the core and net protocols.
 * Those parts interact with one another by communicating through this class.
 */
class Blockchain {
  private:
    const std::shared_ptr<DB> db;                 ///< Pointer to the database.
    const std::shared_ptr<State> state;           ///< Pointer to the state.
    const std::shared_ptr<Storage> storage;       ///< Pointer to the blockchain history.
    const std::shared_ptr<SnowmanVM> snowmanVM;   ///< Pointer to the SnowmanVM.
    const std::shared_ptr<rdPoS> rdpos;           ///< Pointer to the rdPoS/block manager.
    const std::unique_ptr<Server> server;         ///< Pointer to the (generic) gRPC server.
    const std::shared_ptr<HTTPServer> httpServer; ///< Pointer to the HTTP server.
    const std::shared_ptr<P2P::Manager> p2p;      ///< Pointer to the P2P connection manager.
    bool initialized = false;                     ///< Indicates if the blockchain is initialized.
    bool shutdown = false;                        ///< Indicates if the blockchain will shutdown.
    bool isValidator = false;                     ///< Indicates if the blockchain is a Validator.

  public:
    /// Start the blockchain.
    void start();

    /// Stop the blockchain.
    void stop();

    /// Shutdown the generic gRPC server.
    void shutdownServer() {
      if (this->initialized && this->shutdown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        this->server->Shutdown();
      }
    }

    /**
     * Parse an RPC message.
     * Called by HTTPServer according to the requests of RPC clients (e.g. MetaMask).
     * @param msg The message string to be parsed.
     * @return The parsed response string to the message.
     */
    std::string parseRPC(std::string& msg);

    /**
     * Validate a given transaction.
     * Called by gRPCServer.
     * @param tx The transaction to validate.
     * @return An error code/message pair with the status of the validation.
     */
    std::pair<int, std::string> validateTx(const TxBlock&& tx);

    /**
     * Validate a given Validator transaction.
     * Called by gRPCServer.
     * @param tx The transaction to validate.
     */
    inline void validateValidatorTx(const TxValidator& tx) { this->rdpos->addValidatorTx(tx); }

    /**
     * Get the Validator transaction mempool from the rdPoS/block manager.
     * Called by P2P::Manager.
     * @return A copy of the Validator mempool.
     */
    inline std::unordered_map<Hash, TxValidator, SafeHash> getValidatorMempool() {
      return this->rdpos->getMempoolCopy();
    }
};

#endif  // BLOCKCHAIN_H
