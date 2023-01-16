#ifndef SUBNET_H
#define SUBNET_H

#include <mutex>

#include "block.h"
#include "blockChain.h"
#include "blockMempool.h"
//#include "state.h"  TODO: fix circular dep

#include "../libs/json.hpp"

#include "../net/P2PManager.h"
#include "../net/grpcclient.h"
#include "../net/grpcserver.h"
#include "../net/httpserver.h"
#include "../utils/db.h"

using json = nlohmann::ordered_json;
using grpc::Server;
using grpc::ServerContext;

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

class Subnet {
  private:
    const std::shared_ptr<DB> db;
    const std::shared_ptr<State> state;
    const std::shared_ptr<BlockChain> chain;
    const std::shared_ptr<BlockMempool> mempool;
    const std::shared_ptr<BlockManager> blockManager;
    const std::shared_ptr<Server> server;
    const std::shared_ptr<gRPCServer> grpcServer;
    const std::shared_ptr<gRPCClient> grpcClient;
    const std::shared_ptr<HTTPServer> httpServer;
    const std::shared_ptr<P2PManager> p2p;
    InitializeRequest initParams;
    std::vector<std::string> connectedNodes;
    std::mutex connectedNodesLock;
    bool initialized = false;
    bool shutdown = false;
    bool isValidator = false;
  public:
    void start();
    void stop();
    void initialize(
      const vm::InitializeRequest* request, vm::InitializeResponse* reply
    );
    bool parseBlock(
      ServerContext* context, const std::string& blockBytes, vm::ParseBlockResponse* reply
    );
    void setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply);
    bool blockRequest(ServerContext* context, vm::BuildBlockResponse* reply);
    void getBlock(
      ServerContext* context, const vm::GetBlockRequest* request, vm::GetBlockResponse* reply
    );
    bool getAncestors(
      ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply
    );
    const std::shared_ptr<const Block> verifyBlock(const std::string bytes);
    bool acceptBlock(const Hash& hash);
    void rejectBlock(const Hash& hash);
    void shutdownServer();
    void setPreference(ServerContext* context, const vm::SetPreferenceRequest* request);
    std::string parseRPC(std::string& msg);
    std::pair<int, string> validateTx(const Tx&& tx);
    void validateValidatorTx(const Tx& tx);
    void connectNode(const std::string& id);
    void disconnectNode(const std::string& id);
    std::unordered_map<Hash, Tx, SafeHash> getValidatorMempool();
};

#endif  // SUBNET_H
