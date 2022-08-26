#ifndef SUBNET_H
#define SUBNET_H

#include "../libs/json.hpp"
#include "../net/grpcclient.h"
#include "../net/grpcserver.h"
#include "../net/httpserver.h"
#include "block.h"
#include "db.h"
#include "state.h"
#include "chainTip.h"

using json = nlohmann::ordered_json;
using grpc::Server;
using grpc::ServerContext;

// Forward declarations.
class HTTPServer;
class VMServiceImplementation;

struct InitializeRequest {
  uint32_t networkId;
  std::string subnetId;
  std::string chainId;
  std::string nodeId;
  std::string xChainId;
  std::string avaxAssetId;
  std::string genesisBytes;
  std::string upgradeBytes;
  std::string configBytes;
  std::vector<DBServer> dbServers;
  std::string gRPCServerAddress; // gRPC server address to connect to
};

/**
 * The Subnet class acts as the middleman of every "module" of the subnet.
 * Every class originates from this, being the gRPC server/client or the inner
 * validation status of the system.
 * Example: a given sub-module (e.g. the gRPC Server) does a request.
 * The gRPC server will call a function on the Subnet class (as it has a
 * reference for it) and then the Subnet class will process the request.
 * This means that the gRPC server cannot access another "brother"/sub-module
 * directly, it has to go through Subnet first.
 */
class Subnet {
  private:
    bool initialized = false;
    bool shutdown = false;

    // gRPC Server Implementation. vm.proto calls from AvalancheGo can be found here.
    std::shared_ptr<VMServiceImplementation> grpcServer;

    /**
     * gRPC Client Implementation. aliasreader.proto, appsender.proto,
     * keystore.proto, messenger.proto, metrics.proto and sharedmemory.proto
     * calls to AvalancheGo can be found here.
     */
    std::shared_ptr<VMCommClient> grpcClient;

    /**
     * DB Server Implementation. rpcdb.proto calls to AvalancheGo can be found here.
     * Implements a basic key/value DB provided by the AvalancheGo node,
     * which is similar to leveldb but with a gRPC interface.
     */
    std::shared_ptr<DBService> dbServer;

    std::unique_ptr<Server> server; // gRPC Server.

    /**
     * State. Keeps track of balances and the inner variables of the blockchain
     * (e.g. memory pool, block parsing/creation, etc.).
     */
    std::unique_ptr<State> headState;

    /**
     * ChainHead. Keeps track of the blockchain itself.
     * The blocks/confirmed transactions are stored here.
     * Information can be requested to it.
     */
    std::unique_ptr<ChainHead> chainHead;

    /**
     * HTTP Server for JSON-RPC Requests
     */

    std::unique_ptr<HTTPServer> httpServer;

    /**
     * ChainTip. Keeps track of processing blocks and refused blocks.
     * also keeps track of the prefered block to be accepted.
     */

    std::unique_ptr<ChainTip> chainTip;

    InitializeRequest initParams; // From initialization request.

  public:
    void start(); // Start the subnet.
    void stop();  // Stop the subnet.
    bool isShutdown() { return this->shutdown; }; // Used by the http server to know if it should stop.

    // To be called by the gRPC server. Initialize the subnet services when AvalancheGo requests for it.
    void initialize(const vm::InitializeRequest* request, vm::InitializeResponse* reply);

    // To be called by the gRPC server. Parse a given block, if necessary push it to the blockchain.
    // Answers back with block status and a pointer to the block, if exists.
    bool parseBlock(ServerContext* context, const vm::ParseBlockRequest* request, vm::ParseBlockResponse* reply);

    // To be called by initialize if no info is found on DB.
    void setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply);

    // To be called by the grpcServer when avalancheGo requests a block to be created
    void blockRequest(ServerContext* context, vm::BuildBlockResponse* reply);

    // To be called by the grpcServer when avalancheGo requests a block to be loaded.
    void getBlock(ServerContext* context, const vm::GetBlockRequest* request, vm::GetBlockResponse* reply);

    // To be called by the grpcServer when avalancheGo requests a given number of ancestors of a block
    void getAncestors(ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply);

    // To be called by the grpcServer when avalancheGo requests a block to be verified.
    // Returns a const pointer to the block, or nullptr in case of error.
    const std::shared_ptr<const Block> verifyBlock(const std::string &blockBytes);

    // To be called by the grpcServer when avalancheGo sends a block for us to accept.
    bool acceptBlock(const std::string &blockHash);

    // To be called by grpcServer after a shutdown call.
    void shutdownServer();

    // To be called by grpcServer, when avalancheGo sets the current preference for block acceptance.
    void setPreference(ServerContext* context, const vm::SetPreferenceRequest* request);

    // To be called by HTTP Server, from RPC clients (such as Metamask).
    std::string processRPCMessage(std::string &req);
};

#endif // SUBNET_H
