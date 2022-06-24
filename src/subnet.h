#ifndef SUBNET_H
#define SUBNET_H


#include "block.h"
#include "grpcserver.h"
#include "grpcclient.h"
#include "db.h"

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
    std::shared_ptr<VMServiceImplementation> grpcServer;
    std::shared_ptr<VMCommClient> grpcClient;
    std::shared_ptr<DBService> dbServer;
    std::unique_ptr<Server> server;

    // From initialization request.
    InitializeRequest initParams;

  public:
    void start();
    void stop();

    // To be called by the gRPC server. Initialize the subnet services when AvalancheGo requests for it.
    void initialize(const vm::InitializeRequest* request, vm::InitializeResponse* reply);
    void setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply);
    // To be called by initialize if no info is found on DB.
    void createGenesis();
};

#endif // SUBNET_H