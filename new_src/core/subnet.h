#ifndef SUBNET_H
#define SUBNET_H

#include <mutex>

#include "blockChain.h"
#include "blockManager.h"
#include "snowmanVM.h"
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
 * Abstraction of the proper subnet.
 * Contains and acts as the middleman of every other part of the core and net protocols.
 * Those parts interact with one another by communicating through this class.
 */
class Subnet {
  private:
    const std::shared_ptr<DB> db;                     ///< Pointer to the database.
    const std::shared_ptr<State> state;               ///< Pointer to the state.
    const std::shared_ptr<BlockChain> chain;          ///< Pointer to the blockchain.
    const std::shared_ptr<SnowmanVM> snowmanVM;       ///< Pointer to the SnowmanVM.
    const std::shared_ptr<BlockManager> blockManager; ///< Pointer to the block manager.
    const std::shared_ptr<Server> server;             ///< Pointer to the (generic) gRPC server.
    const std::shared_ptr<HTTPServer> httpServer;     ///< Pointer to the HTTP server.
    const std::shared_ptr<P2PManager> p2p;            ///< Pointer to the P2P connection manager.
    bool initialized = false;                         ///< Indicates if the Subnet is initialized.
    bool shutdown = false;                            ///< Indicates if the Subnet will shutdown.
    bool isValidator = false;                         ///< Indicates if the Subnet is a Validator.

  public:
    void start();           ///< Start the Subnet.
    void stop();            ///< Stop the Subnet.
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
     * Get the Validator transaction mempool from the BlockManager.
     * Called by P2PManager.
     * @return A copy of the Validator mempool.
     */
    inline std::unordered_map<Hash, TxValidator, SafeHash> getValidatorMempool() {
      return this->blockManager->getMempoolCopy();
    }
};

#endif  // SUBNET_H
