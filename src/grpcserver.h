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
#include "utils.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

class Subnet; // Forward declaration.

class VMServiceImplementation final :
  public vm::VM::Service,
  public std::enable_shared_from_this<VMServiceImplementation>
{
  private:
    Subnet& subnet;

  public:
    VMServiceImplementation(Subnet& _subnet) : subnet(_subnet) {};

    Status Initialize(
      ServerContext* context,
      const vm::InitializeRequest* request,
      vm::InitializeResponse* reply
    ) override;

    Status SetState(
      ServerContext* context,
      const vm::SetStateRequest* request,
      vm::SetStateResponse* reply
    ) override;

    Status Shutdown(
      ServerContext* context,
      const google::protobuf::Empty* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("Shutdown called!!");
      return Status::OK;
    }

    Status CreateHandlers(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::CreateHandlersResponse* reply
    ) override {
      // TODO: Create http handlers, see https://github.com/ava-labs/avalanchego/tree/master/proto/http for example.
      // Yes, we need to create another gRPC Server that answers http requests routed through avalancheGo.
      return Status::OK;
    }

    Status CreateStaticHandlers(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::CreateStaticHandlersResponse* reply
    ) override {
      return Status::OK;
    }

    Status Connected(
      ServerContext* context,
      const vm::ConnectedRequest* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("Connected called!!");
      return Status::OK;
    }

    Status Disconnected(
      ServerContext* context,
      const vm::DisconnectedRequest* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("Disconnected called!!");
      return Status::OK;
    }

    Status BuildBlock(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::BuildBlockResponse* reply
    ) override;

    Status ParseBlock(
      ServerContext* context,
      const vm::ParseBlockRequest* request,
      vm::ParseBlockResponse* reply
    ) override {
      Utils::logToFile("ParseBlock called!!");
      return Status::OK;
    }

    Status GetBlock(
      ServerContext* context,
      const vm::GetBlockRequest* request,
      vm::GetBlockResponse* reply
    ) override {
      Utils::logToFile("GetBlock called!!");
      return Status::OK;
    }

    Status SetPreference(
      ServerContext* context,
      const vm::SetPreferenceRequest* request,
      google::protobuf::Empty* reply
    ) override;

    Status Health(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::HealthResponse* reply
    ) override {
      Utils::logToFile("Health called!!");
      return Status::OK;
    }

    Status Version(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::VersionResponse* reply
    ) override;

    Status AppRequest(
      ServerContext* context,
      const vm::AppRequestMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("AppRequest called!!");
      return Status::OK;
    }

    Status AppRequestFailed(
      ServerContext* context,
      const vm::AppRequestFailedMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("AppRequestFailed called!!");
      return Status::OK;
    }

    Status AppResponse(
      ServerContext* context,
      const vm::AppResponseMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("AppResponse called!!");
      return Status::OK;
    }

    Status AppGossip(
      ServerContext* context,
      const vm::AppGossipMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("AppGossip called!!");
      return Status::OK;
    }

    Status Gather(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::GatherResponse* reply
    ) override {
      Utils::logToFile("Gather called!!");
      return Status::OK;
    }

    Status BlockVerify(
      ServerContext* context,
      const vm::BlockVerifyRequest* request,
      vm::BlockVerifyResponse* reply
    ) override {
      Utils::logToFile("BlockVerify called");
      return Status::OK;
    }

    Status BlockAccept(
      ServerContext* context,
      const vm::BlockAcceptRequest* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("BlockAccept called!!");
      return Status::OK;
    }

    Status BlockReject(
      ServerContext* context,
      const vm::BlockRejectRequest* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("BlockReject called!!");
      return Status::OK;
    }

    Status GetAncestors(
      ServerContext* context,
      const vm::GetAncestorsRequest* request,
      vm::GetAncestorsResponse* reply
    ) override {
      Utils::logToFile("GetAncestors called!!");
      return Status::OK;
    }

    Status BatchedParseBlock(
      ServerContext* context,
      const vm::BatchedParseBlockRequest* request,
      vm::BatchedParseBlockResponse* reply
    ) override {
      Utils::logToFile("BatchedParseBlock called!!");
      return Status::OK;
    }

    Status VerifyHeightIndex(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::VerifyHeightIndexResponse* reply
    ) override;

    Status GetBlockIDAtHeight(
      ServerContext* context,
      const vm::GetBlockIDAtHeightRequest* request,
      vm::GetBlockIDAtHeightResponse* reply
    ) override {
      Utils::logToFile("GetBlockIDAtHeight called!!");
      return Status::OK;
    }

    // TODO: Enable StateSync, awaiting for avalanche documentation (already requested)
    Status StateSyncEnabled(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::StateSyncEnabledResponse* reply
    ) override;

    Status GetOngoingSyncStateSummary(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::GetOngoingSyncStateSummaryResponse* reply
    ) override {
      Utils::logToFile("GetOngoingSyncStateSummary called!!");
      return Status::OK;
    }

    Status GetLastStateSummary(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::GetLastStateSummaryResponse* reply
    ) override {
      Utils::logToFile("GetLastStateSummary called!!");
      return Status::OK;
    }

    Status ParseStateSummary(
      ServerContext* context,
      const vm::ParseStateSummaryRequest* request,
      vm::ParseStateSummaryResponse* reply
    ) override {
      Utils::logToFile("ParseStateSummary called!!");
      return Status::OK;
    }

    Status GetStateSummary(
      ServerContext* context,
      const vm::GetStateSummaryRequest* request,
      vm::GetStateSummaryResponse* reply
    ) override {
      Utils::logToFile("GetStateSummary called!!");
      return Status::OK;
    }
};

#endif // GRPCSERVER_H
