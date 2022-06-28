#ifndef SUBNET_H
#define SUBNET_H

#include "block.h"
#include "grpcserver.h"
#include "grpcclient.h"
#include "db.h"
#include "state.h"
#include "httpserver.h"

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
    std::string gRPCServerAddress; // <- gRPC server address to connect into.
};

// The subnet class acts as being the middleman of every "module" of the subnet
// Every class originating from this, being the gRPC server/client or the inner
// validation status of the system.
// A given sub-module (let's say, the gRPC Server) does a request.
// the gRPC server will call a function on the Subnet class (as it has a reference for it) 
// And then the Subnet class will process the request, this means that the gRPC server cannot access directly
// another sub-module, it has to go through Subnet first.

class Subnet {
  private:
    bool initialized = false;
    // gRPC Server Implementation.
    // vm.proto calls from AvalancheGo can be found here
    std::shared_ptr<VMServiceImplementation> grpcServer;
    // gRPC Client Implementation.
    // aliasreader.proto, appsender.proto, keystore.proto, messenger.proto, metrics.proto and sharedmemory.proto calls to AvalancheGo can be found here.
    std::shared_ptr<VMCommClient> grpcClient;
    // DB Server Implementation.
    // rpcdb.proto calls to AvalancheGo can be found here.
    // Implements a basic key/value DB provided by the AvalancheGo node, Similar to leveldb but with a gRPC interface.
    std::shared_ptr<DBService> dbServer;
    // gRPC Server.
    std::unique_ptr<Server> server;
    // State.
    // Keep track of balances and the inner variables of the blockchain.
    // Memory pool, block parsing/creation 
    std::unique_ptr<State> headState;
    // ChainHead.
    // Keeps track of the blockchain, the blocks/confirmed transactions are stored here.
    // Information can be requested to it.
    std::unique_ptr<ChainHead> chainHead;

    // From initialization request.
    InitializeRequest initParams;

  public:
    void start();
    void stop();

    // To be called by the gRPC server. Initialize the subnet services when AvalancheGo requests for it.
    void initialize(const vm::InitializeRequest* request, vm::InitializeResponse* reply);
    void setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply);
    // To be called by initialize if no info is found on DB.


    // To be called by HTTP Server, from RPC clients (such as Metamask)
    std::string processRPCMessage(std::string &req);
};

#endif // SUBNET_H