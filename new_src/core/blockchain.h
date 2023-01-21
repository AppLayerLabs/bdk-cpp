#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <mutex>

#include "rdpos.h"
#include "snowmanVM.h"
#include "storage.h"
//#include "state.h"  TODO: fix circular dep

#include "../libs/json.hpp"

#include "../net/P2PManager.h"
#include "../net/httpserver.h"
#include "../net/grpcserver.h"
#include "../utils/block.h"
#include "../utils/db.h"

using json = nlohmann::ordered_json;
using grpc::Server;

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
    const std::shared_ptr<Server> server;         ///< Pointer to the (generic) gRPC server.
    const std::shared_ptr<HTTPServer> httpServer; ///< Pointer to the HTTP server.
    const std::shared_ptr<P2PManager> p2p;        ///< Pointer to the P2P connection manager.
    bool initialized = false;                     ///< Indicates if the blockchain is initialized.
    bool shutdown = false;                        ///< Indicates if the blockchain will shutdown.
    bool isValidator = false;                     ///< Indicates if the blockchain is a Validator.

  public:
    void start();           ///< Start the blockchain.
    void stop();            ///< Stop the blockchain.
    void shutdownServer();  ///< Shutdown the generic gRPC server.

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
    std::pair<int, string> validateTx(const TxBlock&& tx);

    /**
     * Validate a given Validator transaction.
     * Called by gRPCServer.
     * @param tx The transaction to validate.
     */
    void validateValidatorTx(const TxValidator& tx);

    /**
     * Get the Validator transaction mempool from the rdPoS/block manager.
     * Called by P2PManager.
     * @return A copy of the Validator mempool.
     */
    inline std::unordered_map<Hash, TxValidator, SafeHash> getValidatorMempool() {
      return this->rdpos->getMempoolCopy();
    }
};

#endif  // BLOCKCHAIN_H
