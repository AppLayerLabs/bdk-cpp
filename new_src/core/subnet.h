#ifndef SUBNET_H
#define SUBNET_H

#include <mutex>

#include "blockChain.h"
#include "blockMempool.h"
//#include "state.h"  TODO: fix circular dep

#include "../libs/json.hpp"

#include "../net/P2PManager.h"
#include "../net/grpcclient.h"
#include "../net/grpcserver.h"
#include "../net/httpserver.h"
#include "../utils/block.h"
#include "../utils/db.h"

using json = nlohmann::ordered_json;
using grpc::Server;
using grpc::ServerContext;

/**
 * Internal struct that contains data for initializing the subnet.
 * TODO: Ava Labs never answered us on what some of those do.
 */
struct InitializeRequest {
  uint32_t networkId; ///< ID of the network to connect to.
  std::string subnetId; ///< ID of the subnet to connect to.
  std::string chainId; ///< ID of the chain to connect to.
  std::string nodeId; // ID of the node to connect to.
  std::string xChainId; ///< See todo.
  std::string avaxAssetId; ///< See todo.
  std::string genesisBytes; ///< See todo.
  std::string upgradeBytes; ///< See todo.
  std::string configBytes; ///< See todo.
  std::vector<DBServer> dbServers; ///< Array of database server to connect to.
  std::string gRPCServerAddress; ///< gRPC server address to connect to.
};

/**
 * Abstraction of the proper subnet.
 * Contains and acts as the middleman of every other part of the core and net protocols.
 * Those parts interact with one another by going through this class first.
 * e.g. the gRPC server does a request, calls a function from Subnet, and
 * Subnet will process the request for it by calling one of its "brothers".
 */
class Subnet {
  private:
    const std::shared_ptr<DB> db; ///< Pointer to the database.
    const std::shared_ptr<State> state; ///< Pointer to the state.
    const std::shared_ptr<BlockChain> chain; ///< Pointer to the blockchain.
    const std::shared_ptr<BlockMempool> mempool; ///< Pointer to the blockchain's mempool.
    const std::shared_ptr<BlockManager> blockManager; ///< Pointer to the block manager.
    const std::shared_ptr<Server> server; ///< Pointer to the (generic) gRPC server.
    const std::shared_ptr<gRPCServer> grpcServer; ///< Pointer to the gRPC server.
    const std::shared_ptr<gRPCClient> grpcClient; ///< Pointer to the gRPC client.
    const std::shared_ptr<HTTPServer> httpServer; ///< Pointer to the HTTP server.
    const std::shared_ptr<P2PManager> p2p;  ///< Pointer to the P2P connection manager.
    InitializeRequest initParams; ///< Struct with initialization request data.
    std::vector<std::string> connectedNodes; ///< List of all connected nodes.
    std::mutex connectedNodesLock; ///< Mutex for managing read/write access to connectedNodes.
    bool initialized = false; ///< Indicates if the Subnet is initialized.
    bool shutdown = false; ///< Indicates if the Subnet is set to shutdown.
    bool isValidator = false; ///< Indicates if the Subnet is a Validator.

  public:
    void start(); ///< Start the Subnet.
    void stop(); ///< Stop the Subnet.

    /**
     * Initialize the Subnet services.
     * Called by gRPCServer.
     */
    void initialize(
      const vm::InitializeRequest* request, vm::InitializeResponse* reply
    );

    /**
     * Parse a given block and push it to the blockchain if required.
     * Called by gRPCClient.
     * @return `true` if the block was parsed successfully, `false` otherwise.
     */
    bool parseBlock(
      ServerContext* context, const std::string& blockBytes, vm::ParseBlockResponse* reply
    );

    /**
     * Set the state of the Subnet.
     * Called by `initialize()` if no info is found on the database.
     */
    void setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply);

    /**
     * Request a block to be created.
     * Called by gRPCServer.
     * @return `true` if request is successful, `false` otherwise.
     */
    bool blockRequest(ServerContext* context, vm::BuildBlockResponse* reply);

    /**
     * Get a block that was requested.
     * Called by gRPCServer.
     */
    void getBlock(
      ServerContext* context, const vm::GetBlockRequest* request, vm::GetBlockResponse* reply
    );

    /**
     * Get the ancestors of a block.
     * Called by gRPCServer.
     * @return `true` on success, `false` on failure.
     */
    bool getAncestors(
      ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply
    );

    /**
     * Request a block to be verified.
     * Called by gRPCServer.
     * @param bytes The raw block bytes.
     * @return A pointer to the block, or `nullptr` in case of error.
     */
    const std::shared_ptr<const Block> verifyBlock(const std::string bytes);

    /**
     * Accept a block.
     * Called by gRPCServer.
     * @param hash The block hash to accept.
     * @return `true` if block was accepted, `false` otherwise.
     */
    bool acceptBlock(const Hash& hash);

    /**
     * Reject a block.
     * Called by gRPCServer.
     * @param hash The block hash to reject.
     */
    void rejectBlock(const Hash& hash);

    void shutdownServer();  ///< Shutdown the generic gRPC server.

    /**
     * Set the preferred block for acceptance/continuing the chain.
     * Called by gRPCServer.
     */
    void setPreference(ServerContext* context, const vm::SetPreferenceRequest* request);

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
     * Connect to a given node.
     * @param id The node's 20-byte hex ID.
     */
    void connectNode(const std::string& id);

    /**
     * Disconnect from a given node.
     * @param id The node's 20-byte hex ID.
     */
    void disconnectNode(const std::string& id);

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
