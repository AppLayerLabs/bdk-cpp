#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sstream>
#include <fstream>
#include <chrono>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>


#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/message.h>
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
    std::string jsonRequest;
    google::protobuf::util::JsonOptions options;
    google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
    logToFile(jsonRequest);
    // We only have the genesis for now, answer it

    std::string zeroStr;
    // last_accepted_id == 32 bytes, last blockhash
    // last_accepted_parent_id == 32 bytes, last block parent blockhash
    for (uint64_t i = 0; i < 32; ++i) {
      zeroStr += "0";
    }
    logToFile(zeroStr);
    logToFile(boost::lexical_cast<std::string>(sizeof(zeroStr)));
    reply->set_last_accepted_id(zeroStr);
    reply->set_last_accepted_parent_id(zeroStr);
    reply->set_status(1);
    reply->set_height(1);
    // bytes -> last block bytes.
    reply->set_bytes(std::string("aaa")); 
    const auto p1 = std::chrono::system_clock::now();
    std::string timestampStr;
    // HEX timestamp (bytes->hex) 15 bytes.
    // 0x010000000ed9d0262f3b11354aff10
    // 0x01 -- Version
    // not sure if the below information is correct, more testing and check with the rust code needed.
    // 00 00 00 0e d9 d0 26 2f -- Seconds
    // 3b 11 35 4a -- Milliseconds 
    // ff 10 -- Nanoseconds
    // information took from here: https://github.com/archisgore/landslide/blob/main/src/timestampvm/state.rs#L383
    // EXAMPLE
    // t := time.Date(2001, 2, 1, 14, 30, 12, 05, time.UTC)
    // 0x010000000eb20b69F400000005ffff

    // i64 (8 bytes)

    timestampStr.resize(15);
    timestampStr[0] = 0x01;
    timestampStr[1] = 0x00;
    timestampStr[2] = 0x00;
    timestampStr[3] = 0x00;
    timestampStr[4] = 0x0e;
    timestampStr[5] = 0xd9;
    timestampStr[6] = 0xd0;
    timestampStr[7] = 0x26;
    timestampStr[8] = 0x2f;
    timestampStr[9] = 0x3b;
    timestampStr[10] = 0x11;
    timestampStr[11] = 0x35;
    timestampStr[12] = 0x4a;
    timestampStr[13] = 0xff;
    timestampStr[14] = 0x10;

    reply->set_timestamp(timestampStr);
    std::string jsonAnswer;
    google::protobuf::util::MessageToJsonString(*reply, &jsonAnswer, options);
    logToFile(jsonAnswer);
    return Status::OK;
  }

  Status SetState(ServerContext* context, const vmproto::SetStateRequest* request, google::protobuf::Empty* reply) override {
    logToFile("SetState called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status Shutdown(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) override {
    logToFile("Shutdown Called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status CreateHandlers(ServerContext* context, const google::protobuf::Empty* request, vmproto::CreateHandlersResponse* reply) override {
    logToFile("CreateHandlers Called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status CreateStaticHandlers(ServerContext* context, const google::protobuf::Empty* request, vmproto::CreateStaticHandlersResponse* reply) override {
    logToFile("CreateStaticHandlers Called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status Connected(ServerContext* context, const vmproto::ConnectedRequest* request, google::protobuf::Empty* reply) override {
    logToFile("Connected Called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status Disconnected(ServerContext* context, const vmproto::DisconnectedRequest* request, google::protobuf::Empty* reply) override {
    logToFile("Disconnected Called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status BuildBlock(ServerContext* context, const google::protobuf::Empty* request, vmproto::BuildBlockResponse* reply) override {
    logToFile("BuildBlock called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status ParseBlock(ServerContext* context, const vmproto::ParseBlockRequest* request, vmproto::ParseBlockResponse* reply) override {
    logToFile("ParseBlock called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetBlock(ServerContext* context, const vmproto::GetBlockRequest* request, vmproto::GetBlockResponse* reply) override {
    logToFile("GetBlock called");
    logToFile(request->DebugString());
    return Status::OK; 
  }

  Status SetPreference(ServerContext* context, const vmproto::SetPreferenceRequest* request, google::protobuf::Empty* reply) override {
    logToFile("SetPreference Called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status Health(ServerContext* context, const google::protobuf::Empty* request, vmproto::HealthResponse* reply) override {
    logToFile("Health called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status Version(ServerContext* context, const google::protobuf::Empty* request, vmproto::VersionResponse* reply) override {
    logToFile("Version Called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppRequest(ServerContext* context, const vmproto::AppRequestMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppRequest called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppRequestFailed(ServerContext* context, const vmproto::AppRequestFailedMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppRequestFailed called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppResponse(ServerContext* context, const vmproto::AppResponseMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppResponse called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppGossip(ServerContext* context, const vmproto::AppGossipMsg* request, google::protobuf::Empty* reply) override {
    logToFile("AppGossip called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status Gather(ServerContext* context, const google::protobuf::Empty* request, vmproto::GatherResponse* reply) override {
    logToFile("Gather called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockVerify(ServerContext* context, const vmproto::BlockVerifyRequest* request, vmproto::BlockVerifyResponse* reply) override {
    logToFile("BlockVerify called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockAccept(ServerContext* context, const vmproto::BlockAcceptRequest* request, google::protobuf::Empty* reply) override {
    logToFile("BlockAccept called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockReject(ServerContext* context, const vmproto::BlockRejectRequest* request, google::protobuf::Empty* reply) override {
    logToFile("BlockReject called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetAncestors(ServerContext* context, const vmproto::GetAncestorsRequest* request, vmproto::GetAncestorsResponse* reply) override {
    logToFile("GetAncestors called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status BatchedParseBlock(ServerContext* context, const vmproto::BatchedParseBlockRequest* request, vmproto::BatchedParseBlockResponse* reply) override {
    logToFile("BatchedParseBlock called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status VerifyHeightIndex(ServerContext* context, const google::protobuf::Empty* request, vmproto::VerifyHeightIndexResponse* reply) override {
    logToFile("VerifyHeightIndex called");
    logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetBlockIDAtHeight(ServerContext* context, const vmproto::GetBlockIDAtHeightRequest* request, vmproto::GetBlockIDAtHeightResponse* reply) override {
    logToFile("GetBlockIDAtHeight called");
    logToFile(request->DebugString());
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