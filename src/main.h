#ifndef MAIN_H
#define MAIN_H

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

#include <boost/lexical_cast.hpp>
#include "json.hpp"

#include "block.h"
#include "utils.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

// Logic and data behind the server's behavior.
class VMServiceImplementation final : public vmproto::VM::Service {
  Status Initialize(ServerContext* context, const vmproto::InitializeRequest* request,
                  vmproto::InitializeResponse* reply) override {
    Utils::logToFile("Initialized Called");
    std::string jsonRequest;
    google::protobuf::util::JsonOptions options;
    google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
    Utils::logToFile(jsonRequest);
    // We only have the genesis for now, answer it

    std::string zeroStr;
    // last_accepted_id == 32 bytes, last blockhash
    // last_accepted_parent_id == 32 bytes, last block parent blockhash
    for (uint64_t i = 0; i < 32; ++i) {
      zeroStr += "0";
    }
    Utils::logToFile(zeroStr);
    Utils::logToFile(boost::lexical_cast<std::string>(sizeof(zeroStr)));
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
    // 0x01
    // 0000000eb20b69F4 
    // 00000005 
    // ffff

    // THIS IS A NIGHTMARE:
    // Golang's Zero time is January 1, year 1, 00:00:00.000000000 UTC
    // https://cs.opensource.google/go/go/+/refs/tags/go1.17.6:src/time/time.go;l=97
    // init that as an OffsetDateTime for future use
    // COMPLETE PEPEGA

    // i8 (1 byte) version
    // i64 (8 bytes) seconds
    // i32 (4 bytes) nanos
    // i16 (2 bytes) minute offset (?)

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
    Utils::logToFile(jsonAnswer);
    return Status::OK;
  }

  Status SetState(ServerContext* context, const vmproto::SetStateRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("SetState called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Shutdown(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("Shutdown Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status CreateHandlers(ServerContext* context, const google::protobuf::Empty* request, vmproto::CreateHandlersResponse* reply) override {
    Utils::logToFile("CreateHandlers Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status CreateStaticHandlers(ServerContext* context, const google::protobuf::Empty* request, vmproto::CreateStaticHandlersResponse* reply) override {
    Utils::logToFile("CreateStaticHandlers Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Connected(ServerContext* context, const vmproto::ConnectedRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("Connected Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Disconnected(ServerContext* context, const vmproto::DisconnectedRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("Disconnected Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BuildBlock(ServerContext* context, const google::protobuf::Empty* request, vmproto::BuildBlockResponse* reply) override {
    Utils::logToFile("BuildBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status ParseBlock(ServerContext* context, const vmproto::ParseBlockRequest* request, vmproto::ParseBlockResponse* reply) override {
    Utils::logToFile("ParseBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetBlock(ServerContext* context, const vmproto::GetBlockRequest* request, vmproto::GetBlockResponse* reply) override {
    Utils::logToFile("GetBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK; 
  }

  Status SetPreference(ServerContext* context, const vmproto::SetPreferenceRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("SetPreference Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Health(ServerContext* context, const google::protobuf::Empty* request, vmproto::HealthResponse* reply) override {
    Utils::logToFile("Health called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Version(ServerContext* context, const google::protobuf::Empty* request, vmproto::VersionResponse* reply) override {
    Utils::logToFile("Version Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppRequest(ServerContext* context, const vmproto::AppRequestMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppRequest called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppRequestFailed(ServerContext* context, const vmproto::AppRequestFailedMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppRequestFailed called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppResponse(ServerContext* context, const vmproto::AppResponseMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppResponse called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppGossip(ServerContext* context, const vmproto::AppGossipMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppGossip called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Gather(ServerContext* context, const google::protobuf::Empty* request, vmproto::GatherResponse* reply) override {
    Utils::logToFile("Gather called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockVerify(ServerContext* context, const vmproto::BlockVerifyRequest* request, vmproto::BlockVerifyResponse* reply) override {
    Utils::logToFile("BlockVerify called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockAccept(ServerContext* context, const vmproto::BlockAcceptRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("BlockAccept called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockReject(ServerContext* context, const vmproto::BlockRejectRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("BlockReject called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetAncestors(ServerContext* context, const vmproto::GetAncestorsRequest* request, vmproto::GetAncestorsResponse* reply) override {
    Utils::logToFile("GetAncestors called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BatchedParseBlock(ServerContext* context, const vmproto::BatchedParseBlockRequest* request, vmproto::BatchedParseBlockResponse* reply) override {
    Utils::logToFile("BatchedParseBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status VerifyHeightIndex(ServerContext* context, const google::protobuf::Empty* request, vmproto::VerifyHeightIndexResponse* reply) override {
    Utils::logToFile("VerifyHeightIndex called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetBlockIDAtHeight(ServerContext* context, const vmproto::GetBlockIDAtHeightRequest* request, vmproto::GetBlockIDAtHeightResponse* reply) override {
    Utils::logToFile("GetBlockIDAtHeight called");
    Utils::logToFile(request->DebugString());
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

#endif // MAIN_H