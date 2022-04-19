#ifndef GRPCSERVER_H
#define GRPCSERVER_H

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sstream>
#include <fstream>
#include <chrono>
#include <csignal>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>


#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/message.h>
#include "../proto/vm.grpc.pb.h"
#include "grpcclient.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/thread.hpp>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#include "json.hpp"

#include "block.h"
#include "utils.h"
#include "db.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

// Forward declaration.
class Subnet;
class Validation;


class VMServiceImplementation final : public vm::VM::Service, public std::enable_shared_from_this<VMServiceImplementation> {
  public:
  std::atomic_bool isMining = false;
  bool initialized = false;
  std::shared_ptr<Subnet> subnet;
  std::shared_ptr<VMCommClient> commClient;
  std::shared_ptr<Validation> validation;

  VMServiceImplementation (std::shared_ptr<Subnet> subnet_ptr_) : subnet(subnet_ptr_) {};

  void blockRequester() {
    boost::this_thread::sleep_for(boost::chrono::seconds(1));
    commClient->requestBlock();
    return;
  }

  Status Initialize(ServerContext* context, const vm::InitializeRequest* request, vm::InitializeResponse* reply) override;

  Status SetState(ServerContext* context, const vm::SetStateRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("SetState called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Shutdown(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) override;
  Status CreateHandlers(ServerContext* context, const google::protobuf::Empty* request, vm::CreateHandlersResponse* reply) override {
    Utils::logToFile("CreateHandlers Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status CreateStaticHandlers(ServerContext* context, const google::protobuf::Empty* request, vm::CreateStaticHandlersResponse* reply) override {
    Utils::logToFile("CreateStaticHandlers Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Connected(ServerContext* context, const vm::ConnectedRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("Connected Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Disconnected(ServerContext* context, const vm::DisconnectedRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("Disconnected Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BuildBlock(ServerContext* context, const google::protobuf::Empty* request, vm::BuildBlockResponse* reply) override;

  Status ParseBlock(ServerContext* context, const vm::ParseBlockRequest* request, vm::ParseBlockResponse* reply) override {
    Utils::logToFile("ParseBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetBlock(ServerContext* context, const vm::GetBlockRequest* request, vm::GetBlockResponse* reply) override {
    Utils::logToFile("GetBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK; 
  }

  Status SetPreference(ServerContext* context, const vm::SetPreferenceRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("SetPreference Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Health(ServerContext* context, const vm::HealthRequest* request, vm::HealthResponse* reply) override {
    Utils::logToFile("Health called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Version(ServerContext* context, const google::protobuf::Empty* request, vm::VersionResponse* reply) override {
    Utils::logToFile("Version Called");
    reply->set_version("1");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppRequest(ServerContext* context, const vm::AppRequestMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppRequest called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppRequestFailed(ServerContext* context, const vm::AppRequestFailedMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppRequestFailed called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppResponse(ServerContext* context, const vm::AppResponseMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppResponse called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppGossip(ServerContext* context, const vm::AppGossipMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppGossip called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Gather(ServerContext* context, const google::protobuf::Empty* request, vm::GatherResponse* reply) override {
    Utils::logToFile("Gather called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockVerify(ServerContext* context, const vm::BlockVerifyRequest* request, vm::BlockVerifyResponse* reply) override {
    Utils::logToFile("BlockVerify called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockAccept(ServerContext* context, const vm::BlockAcceptRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("BlockAccept called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockReject(ServerContext* context, const vm::BlockRejectRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("BlockReject called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetAncestors(ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply) override {
    Utils::logToFile("GetAncestors called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BatchedParseBlock(ServerContext* context, const vm::BatchedParseBlockRequest* request, vm::BatchedParseBlockResponse* reply) override {
    Utils::logToFile("BatchedParseBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status VerifyHeightIndex(ServerContext* context, const google::protobuf::Empty* request, vm::VerifyHeightIndexResponse* reply) override {
    Utils::logToFile("VerifyHeightIndex called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetBlockIDAtHeight(ServerContext* context, const vm::GetBlockIDAtHeightRequest* request, vm::GetBlockIDAtHeightResponse* reply) override {
    Utils::logToFile("GetBlockIDAtHeight called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }
  
  std::string processRPCMessage(std::string message);
};

#endif // GRPCSERVER_H