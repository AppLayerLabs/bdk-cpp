#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sstream>
#include <fstream>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "../proto/vm.grpc.pb.h"
#include "libs/CommonData.h"

#include <boost/lexical_cast.hpp>
#include "json.hpp"

using json = nlohmann::json;


using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

std::mutex log_lock;

void logToFile(std::string str) {
  log_lock.lock();
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
  log.close();
  log_lock.unlock();
}

// Logic and data behind the server's behavior.
class VMServiceImplementation final : public vmproto::VM::Service {
  Status Initialize(ServerContext* context, const vmproto::InitializeRequest* request,
                  vmproto::InitializeResponse* reply) override {
    logToFile("Initialized Called");

    json debugJson;
    debugJson["call"] = "Initialize";
    json initializeRequest;
    initializeRequest["network_id"] = boost::lexical_cast<std::string>(request->network_id());
    initializeRequest["subnet_id"] = request->subnet_id().c_str();
    initializeRequest["chain_id"] = request->chain_id().c_str();
    initializeRequest["node_id"] = request->node_id().c_str();
    initializeRequest["x_chain_id"] = request->x_chain_id().c_str();
    initializeRequest["avax_asset_id"] = request->avax_asset_id().c_str();
    initializeRequest["genesis_bytes"] = request->genesis_bytes().c_str();
    initializeRequest["upgrade_bytes"] = request->upgrade_bytes().c_str();
    initializeRequest["config_bytes"] = request->config_bytes().c_str();
    initializeRequest["VersionedDBServer"] = json::array();
    for (auto dbServer : request->db_servers()) {
      json dbJson;
      dbJson["db_server"] = boost::lexical_cast<std::string>(dbServer.db_server());
      dbJson["version"] = dbServer.version();
    }
    initializeRequest["init_server"] = boost::lexical_cast<std::string>(request->init_server());
    debugJson["request"] = initializeRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status SetState(ServerContext* context, const vmproto::SetStateRequest* request, google::protobuf::Empty* reply) override {
    logToFile("SetState called");
    json debugJson;
    debugJson["call"] = "SetState";
    json setStateRequest;
    setStateRequest["state"] = boost::lexical_cast<std::string>(request->state());
    debugJson["request"] = setStateRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status Shutdown(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) override {
    logToFile("Shutdown Called");
    return Status::OK;
  }

  Status CreateHandlers(ServerContext* context, const google::protobuf::Empty* request, vmproto::CreateHandlersResponse* reply) override {
    logToFile("CreateHandlers Called");
    return Status::OK;
  }

  Status CreateStaticHandlers(ServerContext* context, const google::protobuf::Empty* request, vmproto::CreateStaticHandlersResponse* reply) override {
    logToFile("CreateStaticHandlers Called");
    return Status::OK;
  }

  Status Connected(ServerContext* context, const vmproto::ConnectedRequest* request, google::protobuf::Empty* reply) override {
    logToFile("Connected Called");
    json debugJson;
    debugJson["call"] = "Connected";
    json connectedRequest;
    connectedRequest["node_id"] = request->node_id().c_str();
    connectedRequest["version"] = request->version();
    debugJson["request"] = connectedRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status Disconnected(ServerContext* context, const vmproto::DisconnectedRequest* request, google::protobuf::Empty* reply) override {
    logToFile("Disconnected Called");
    return Status::OK;
  }

  Status BuildBlock(ServerContext* context, const google::protobuf::Empty* request, vmproto::BuildBlockResponse* reply) override {
    logToFile("BuildBlock called");
    return Status::OK;
  }

  Status ParseBlock(ServerContext* context, const vmproto::ParseBlockRequest* request, vmproto::ParseBlockResponse* reply) override {
    logToFile("ParseBlock called");
    json debugJson;
    debugJson["call"] = "ParseBlock";
    json parseBlockRequest;
    parseBlockRequest["bytes"] = request->bytes().c_str();
    debugJson["request"] = parseBlockRequest;
    logToFile(debugJson.dump(2));

    return Status::OK;
  }

  Status GetBlock(ServerContext* context, const vmproto::GetBlockRequest* request, vmproto::GetBlockResponse* reply) override {
    logToFile("GetBlock called");
    json debugJson;
    debugJson["call"] = "GetBlock";
    json getBlockRequest;
    getBlockRequest["id"] = request->id().c_str();
    debugJson["request"] = getBlockRequest;
    logToFile(debugJson.dump(2));
    return Status::OK; 
  }

  Status SetPreference(ServerContext* context, const vmproto::SetPreferenceRequest* request, google::protobuf::Empty* reply) override {
    logToFile("SetPreference Called");
    json debugJson;
    debugJson["call"] = "SetPreference";
    json setPreferenceRequest;
    setPreferenceRequest["id"] = request->id().c_str();
    debugJson["request"] = setPreferenceRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status Health(ServerContext* context, const google::protobuf::Empty* request, vmproto::HealthResponse* reply) override {
    logToFile("Health called");
    return Status::OK;
  }

  Status Version(ServerContext* context, const google::protobuf::Empty* request, vmproto::VersionResponse* reply) override {
    logToFile("Version Called");
    return Status::OK;
  }

  Status AppRequest(ServerContext* context, const vmproto::AppRequestMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppRequest called");
    json debugJson;
    debugJson["call"] = "AppRequest";
    json appRequest;
    appRequest["node_id"] = request->node_id().c_str();
    appRequest["request_id"] = boost::lexical_cast<std::string>(request->request_id());
    appRequest["deadline"] = request->deadline().c_str();
    appRequest["request"] = request->request().c_str();
    debugJson["request"] = appRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status AppRequestFailed(ServerContext* context, const vmproto::AppRequestFailedMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppRequestFailed called");
    json debugJson; 
    debugJson["call"] = "AppRequestFailed";
    json appRequestFailedRequest;
    appRequestFailedRequest["node_id"] = request->node_id().c_str();
    appRequestFailedRequest["request_id"] = boost::lexical_cast<std::string>(request->request_id());
    debugJson["request"] = appRequestFailedRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status AppResponse(ServerContext* context, const vmproto::AppResponseMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppResponse called");
    json debugJson;
    debugJson["call"] = "AppResponse";
    json appResponseRequest;
    appResponseRequest["node_id"] = request->node_id().c_str();
    appResponseRequest["request_id"] = boost::lexical_cast<std::string>(request->request_id());
    appResponseRequest["response"] = request->response().c_str();
    debugJson["request"] = appResponseRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status AppGossip(ServerContext* context, const vmproto::AppGossipMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppGossip called");
    json debugJson;
    debugJson["call"] = "AppGossip";
    json appGossipRequest;
    appGossipRequest["node_id"] = request->node_id().c_str();
    appGossipRequest["msg"] = request->msg().c_str();
    debugJson["request"] = appGossipRequest;
    logToFile(debugJson.dump(2));
    
    return Status::OK;
  }

  Status Gather(ServerContext* context, const google::protobuf::Empty* request, vmproto::GatherResponse* reply) override {
    logToFile("Gather called");
    return Status::OK;
  }

  Status BlockVerify(ServerContext* context, const vmproto::BlockVerifyRequest* request, vmproto::BlockVerifyResponse* reply) override {
    logToFile("BlockVerify called");
    json debugJson;
    debugJson["call"] = "BlockVerify";
    json blockVerifyRequest;
    blockVerifyRequest["bytes"] = request->bytes().c_str();
    debugJson["request"] = blockVerifyRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status BlockAccept(ServerContext* context, const vmproto::BlockAcceptRequest* request, google::protobuf::Empty* reply) override {
    logToFile("BlockAccept called");
    json debugJson;
    debugJson["call"] = "BlockAccept";
    json blockAcceptRequest;
    blockAcceptRequest["id"] = request->id().c_str();
    debugJson["request"] = blockAcceptRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status BlockReject(ServerContext* context, const vmproto::BlockRejectRequest* request, google::protobuf::Empty* reply) override {
    logToFile("BlockReject called");
    json debugJson;
    debugJson["call"] = "BlockReject";
    json blockRejectRequest;
    blockRejectRequest["id"] = request->id().c_str();
    debugJson["request"] = blockRejectRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status GetAncestors(ServerContext* context, const vmproto::GetAncestorsRequest* request, vmproto::GetAncestorsResponse* reply) override {
    logToFile("GetAncestors called");
    json debugJson;
    debugJson["call"] = "GetAncestors";
    json getAncestorsRequest;
    getAncestorsRequest["blk_id"] = request->blk_id().c_str();
    getAncestorsRequest["max_blocks_num"] = boost::lexical_cast<std::string>(request->max_blocks_num());
    getAncestorsRequest["max_blocks_size"] = boost::lexical_cast<std::string>(request->max_blocks_size());
    getAncestorsRequest["max_blocks_retrival_time"] = boost::lexical_cast<std::string>(request->max_blocks_retrival_time());
    debugJson["request"] = getAncestorsRequest;
    logToFile(debugJson.dump(2));
    return Status::OK;
  }

  Status BatchedParseBlock(ServerContext* context, const vmproto::BatchedParseBlockRequest* request, vmproto::BatchedParseBlockResponse* reply) override {
    logToFile("BatchedParseBlock called");
    json debugJson;
    debugJson["call"] = "BatchedParseBlock";
    json batchedParseBlockRequest = json::array();
    for (auto req : request->request()) {
      json tmp;
      tmp["bytes"] = req.c_str();
      batchedParseBlockRequest.push_back(tmp);
    }
    debugJson["request"] = batchedParseBlockRequest;
    logToFile(debugJson.dump(2));
    
    return Status::OK;
  }

  Status VerifyHeightIndex(ServerContext* context, const google::protobuf::Empty* request, vmproto::VerifyHeightIndexResponse* reply) override {
    logToFile("VerifyHeightIndex called");
    return Status::OK;
  }

  Status GetBlockIDAtHeight(ServerContext* context, const vmproto::GetBlockIDAtHeightRequest* request, vmproto::GetBlockIDAtHeightResponse* reply) override {
    logToFile("GetBlockIDAtHeight called");
    json debugJson;
    debugJson["call"] = "GetBlockIDAtHeight";
    json getBlockIDAtHeight;
    getBlockIDAtHeight["height"] = boost::lexical_cast<std::string>(request->height());
    debugJson["request"] = getBlockIDAtHeight;
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  VMServiceImplementation service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "1|11|tcp|" << server_address << "|grpc\n"<< std::flush;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}