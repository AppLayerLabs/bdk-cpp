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

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

std::mutex log_lock;

void logToFile(std::string str) {
  log_lock.lock();
  std::ofstream log("log.txt");
  log << str << std::endl;
  log.close();
  log_lock.unlock();
}

// Logic and data behind the server's behavior.
class VMServiceImplementation final : public vmproto::VM::Service {
  Status Initialize(ServerContext* context, const vmproto::InitializeRequest* request,
                  vmproto::InitializeResponse* reply) override {
    std::string chainID = dev::toHex(request->chainid());
    std::cout << chainID << std::endl;
    reply->set_height(100);
    return Status::OK;
  }

  Status Bootstrapping(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) override {
    logToFile("Bootstrapping Called");
    return Status::OK;
  }

  Status Bootstrapped(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) override {
    logToFile("Bootstrapped Called");
    return Status::OK;
  }

  Status Shutdown(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) override {
    logToFile("Shutdown called");
    return Status::OK;
  }

  Status CreateHandlers(ServerContext* context, const google::protobuf::Empty* request, vmproto::CreateHandlersResponse* reply) override {
    logToFile("CreateHandlers Called");
    return Status::OK;
  }

  Status CreateStaticHandlers(ServerContext* context, const google::protobuf::Empty* request, vmproto::CreateStaticHandlersResponse* reply) override {
    logToFile("CreateStaticHandlers called");
    return Status::OK;
  }

  Status Connected(ServerContext* context, const vmproto::ConnectedRequest* request, google::protobuf::Empty* reply) override {
    logToFile("Connected Called");
    return Status::OK;
  }

  Status Disconnected(ServerContext* context, const vmproto::DisconnectedRequest* request, google::protobuf::Empty* reply) override {
    logToFile("Disconnected Called");
    return Status::OK;
  }

  Status BuildBlock(ServerContext* context, const google::protobuf::Empty* request, vmproto::BuildBlockResponse* reply) override {
    logToFile("BuildBlock Called");
    return Status::OK;
  }

  Status ParseBlock(ServerContext* context, const vmproto::ParseBlockRequest* request, vmproto::ParseBlockResponse* reply) override {
    logToFile("ParseBlock called");
    return Status::OK;
  }

  Status GetBlock(ServerContext* context, const vmproto::GetBlockRequest* request, vmproto::GetBlockResponse* reply) override {
    logToFile("GetBlock called");
    return Status::OK;
  }

  Status SetPreference(ServerContext* context, const vmproto::SetPreferenceRequest* request, google::protobuf::Empty* reply) override {
    logToFile("SetPreference called");
    return Status::OK;
  }

  Status Health(ServerContext* context, const google::protobuf::Empty* request, vmproto::HealthResponse* reply) override {
    logToFile("Health called");
    return Status::OK;
  }

  Status Version(ServerContext* context, const google::protobuf::Empty* request, vmproto::VersionResponse* reply) override {
    logToFile("Version called");
    return Status::OK;
  }

  Status AppRequest(ServerContext* context, const vmproto::AppRequestMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppRequest called");
    return Status::OK;
  }

  Status AppRequestFailed(ServerContext* context, const vmproto::AppRequestFailedMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppRequestFailed called");
    return Status::OK;
  }

  Status AppResponse(ServerContext* context, const vmproto::AppResponseMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppResponse called");
    return Status::OK;
  }

  Status AppGossip(ServerContext* context, const vmproto::AppGossipMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppGossip called");
    return Status::OK;
  }
  
  Status Gather(ServerContext* context, const google::protobuf::Empty* request, vmproto::GatherResponse* reply) override {
    logToFile("Gather called");
    return Status::OK;
  }

  Status BlockVerify(ServerContext* context, const vmproto::BlockVerifyRequest* request, vmproto::BlockVerifyResponse* reply) override {
    logToFile("BlockVerify called");
    return Status::OK;
  }

  Status BlockAccept(ServerContext* context, const vmproto::BlockAcceptRequest* request, google::protobuf::Empty* reply) override {
    logToFile("BlockAccept called");
    return Status::OK;
  }

  Status BlockReject(ServerContext* context, const vmproto::BlockRejectRequest* request, google::protobuf::Empty* reply) override {
    logToFile("BlockReject called");
    return Status::OK;
  }

  Status GetAncestors(ServerContext* context, const vmproto::GetAncestorsRequest* request, vmproto::GetAncestorsResponse* response) override {
    logToFile("GetAncestors called");
    return Status::OK;
  }

  Status BatchedParseBlock(ServerContext* context, const vmproto::BatchedParseBlockRequest* request, vmproto::BatchedParseBlockResponse* response) override {
    logToFile("BatchedParseBlock called");
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
  std::cout << "1|9|tcp|" << server_address << "|grpc\n"<< std::flush;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}