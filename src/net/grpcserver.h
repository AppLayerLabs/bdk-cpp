/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef GRPCSERVER_H
#define GRPCSERVER_H

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <grpc/support/log.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>

#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include <boost/algorithm/hex.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include "../../proto/vm.grpc.pb.h"

#include "../core/blockchain.h"
#include "../utils/utils.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

/// Abstraction of the server side of the gRPC protocol.
class gRPCServer final : public vm::VM::Service,
  public std::enable_shared_from_this<gRPCServer>
{
  private:
    Blockchain& blockchain_; ///< Reference to the blockchain.

  public:
    /**
     * Constructor.
     * @param blockchain Reference to the blockchain.
     */
    gRPCServer(Blockchain& blockchain) : blockchain_(blockchain) {}

    /// Called by AvalancheGo to signal to the Subnet that it can be initialized.
    Status Initialize(
      ServerContext* context,
      const vm::InitializeRequest* request,
      vm::InitializeResponse* reply
    ) override;

    /**
     * Set the gRPC server or the blockchain's state.
     * This refers to the State enum on the vm.proto file,
     * which according to Ava Labs, always follows this order:
     * STATE_UNSPECIFIED > STATE_STATE_SYNCING > STATE_BOOTSTRAPPING > STATE_NORMAL_OP
     */
    Status SetState(
      ServerContext* context,
      const vm::SetStateRequest* request,
      vm::SetStateResponse* reply
    ) override;

    /// Shutdown the gRPC server.
    Status Shutdown(
      ServerContext* context,
      const google::protobuf::Empty* request,
      google::protobuf::Empty* reply
    ) override;

    /**
     * Create HTTP handlers using http.proto as a basis for a gRPC client.
     * This is so it's possible to RPC call the AvalancheGo node and have
     * that call routed through the Subnet.
     * See [Ava Labs' docs](https://github.com/ava-labs/avalanchego/blob/master/proto/http/http.proto).
     */
    Status CreateHandlers(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::CreateHandlersResponse* reply
    ) override {
      // TODO: Create http handlers, see https://github.com/ava-labs/avalanchego/tree/master/proto/http for example.
      // Yes, we need to create another gRPC Server that answers http requests routed through avalancheGo.
      return Status::OK;
    }

    /**
     * Create static HTTP handlers using http.proto as a basis for a gRPC client.
     * According to Ava Labs, this is the same as `CreateHandlers()` but the
     * handlers run "detached" from the blockchain and do not access any
     * blockchain data.
     */
    Status CreateStaticHandlers(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::CreateStaticHandlersResponse* reply
    ) override {
      return Status::OK;
    }

    /// Signal whether another AvalancheGo node connected to this one.
    Status Connected(
      ServerContext* context,
      const vm::ConnectedRequest* request,
      google::protobuf::Empty* reply
    ) override;

    /// Signal whether another AvalancheGo node disconnected from this one.
    Status Disconnected(
      ServerContext* context,
      const vm::DisconnectedRequest* request,
      google::protobuf::Empty* reply
    ) override;

    /// Create a new block using the one set through `SetPreference()` as its parent.
    Status BuildBlock(
      ServerContext* context,
      const vm::BuildBlockRequest* request,
      vm::BuildBlockResponse* reply
    ) override;

    /**
     * Parse a block coming from AvalancheGo.
     * Fails if the block is invalid.
     */
    Status ParseBlock(
      ServerContext* context,
      const vm::ParseBlockRequest* request,
      vm::ParseBlockResponse* reply
    ) override;

    /**
     * Get a block asked by AvalancheGo.
     * Can answer four difference block statuses:
     * STATUS_UNSPECIFIED, STATUS_PROCESSING, STATUS_REJECTED and STATUS_ACCEPTED.
     */
    Status GetBlock(
      ServerContext* context,
      const vm::GetBlockRequest* request,
      vm::GetBlockResponse* reply
    ) override;

    /// Set the preferred block according to the gRPC client request.
    Status SetPreference(
      ServerContext* context,
      const vm::SetPreferenceRequest* request,
      google::protobuf::Empty* reply
    ) override;

    /// Ping AvalancheGo to check if connection is still alive.
    Status Health(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::HealthResponse* reply
    ) override {
      Utils::logToFile("Health called!!");
      return Status::OK;
    }

    /// Show the blockchain's version.
    Status Version(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::VersionResponse* reply
    ) override;

    /**
     * NOT IMPLEMENTED.
     * AvalancheGo function for node <-> node communication, we're using P2P instead.
     */
    Status AppRequest(
      ServerContext* context,
      const vm::AppRequestMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("AppRequest called!!");
      return Status::OK;
    }

    /**
     * NOT IMPLEMENTED.
     * AvalancheGo function for node <-> node communication, we're using P2P instead.
     */
    Status AppRequestFailed(
      ServerContext* context,
      const vm::AppRequestFailedMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("AppRequestFailed called!!");
      return Status::OK;
    }

    /**
     * NOT IMPLEMENTED.
     * AvalancheGo function for node <-> node communication, we're using P2P instead.
     */
    Status AppResponse(
      ServerContext* context,
      const vm::AppResponseMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("AppResponse called!!");
      return Status::OK;
    }

    /**
     * NOT IMPLEMENTED.
     * AvalancheGo function for node <-> node communication, we're using P2P instead.
     */
    Status AppGossip(
      ServerContext* context,
      const vm::AppGossipMsg* request,
      google::protobuf::Empty* reply
    ) override;

    /**
     * NOT IMPLEMENTED.
     * AvalancheGo function for node <-> node communication, we're using P2P instead.
     */
    Status Gather(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::GatherResponse* reply
    ) override {
      Utils::logToFile("Gather called!!");
      return Status::OK;
    }

    /// Verify a block from the gRPC client request.
    Status BlockVerify(
      ServerContext* context,
      const vm::BlockVerifyRequest* request,
      vm::BlockVerifyResponse* reply
    ) override;

    /// Accept a block from the gRPC client request.
    Status BlockAccept(
      ServerContext* context,
      const vm::BlockAcceptRequest* request,
      google::protobuf::Empty* reply
    ) override;

    /// Reject a block from the gRPC client request.
    Status BlockReject(
      ServerContext* context,
      const vm::BlockRejectRequest* request,
      google::protobuf::Empty* reply
    ) override;

    /**
     * NOT IMPLEMENTED.
     * AvalancheGo function for different Subnet <-> Subnet communication.
     */
    Status CrossChainAppRequest(
      ServerContext* context,
      const vm::CrossChainAppRequestMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("CrossChainAppRequestMsg called!!!");
      return Status::OK;
    }

    /**
     * NOT IMPLEMENTED.
     * AvalancheGo function for different Subnet <-> Subnet communication.
     */
    Status CrossChainAppRequestFailed(
      ServerContext* context,
      const vm::CrossChainAppRequestFailedMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("CrossChainAppRequestFailedMsg called!!!");
      return Status::OK;
    }

    /**
     * NOT IMPLEMENTED.
     * AvalancheGo function for different Subnet <-> Subnet communication.
     */
    Status CrossChainAppResponse(
      ServerContext* context,
      const vm::CrossChainAppResponseMsg* request,
      google::protobuf::Empty* reply
    ) override {
      Utils::logToFile("CrossChainAppResponseMsg called!!!");
      return Status::OK;
    }

    /// Get the ancestors of the gRPC client's requested block.
    Status GetAncestors(
      ServerContext* context,
      const vm::GetAncestorsRequest* request,
      vm::GetAncestorsResponse* reply
    ) override;

    /// Same as `ParseBlock()` but batched.
    Status BatchedParseBlock(
      ServerContext* context,
      const vm::BatchedParseBlockRequest* request,
      vm::BatchedParseBlockResponse* reply
    ) override;

    /**
     * NOT IMPLEMENTED.
     * No docs from Ava Labs, we don't know what this does.
     */
    Status VerifyHeightIndex(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::VerifyHeightIndexResponse* reply
    ) override;

    /**
     * NOT IMPLEMENTED.
     * No docs from Ava Labs, we don't know what this does.
     * We suppose it could be getting a block's hash (?) based on the gRPC
     * client's requested block height, but nothing conclusive.
     */
    Status GetBlockIDAtHeight(
      ServerContext* context,
      const vm::GetBlockIDAtHeightRequest* request,
      vm::GetBlockIDAtHeightResponse* reply
    ) override {
      Utils::logToFile("GetBlockIDAtHeight called!!");
      return Status::OK;
    }

    /**
     * NOT IMPLEMENTED.
     * This and the functions below are related to state syncing.
     * Instead of downloading all the blocks of a given chain and syncing them
     * orderly, AvalancheGo provides a way for syncing the innser state of the
     * chain (user balance, contract variables, etc.) without requiring all of
     * this work. They call it "StateSync".
     * The reason those are not implemented is lack of documentation from
     * Ava Labs themselves on how those functions should work in normal
     * conditions in order to avoid consensus problems.
     * Seriously, we even contacted them and all we got was radio silence,
     * not only for those functions but for other structures coming from them.
     */
    Status StateSyncEnabled(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::StateSyncEnabledResponse* reply
    ) override;

    /// NOT IMPLEMENTED. See `StateSyncEnabled()`.
    Status GetOngoingSyncStateSummary(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::GetOngoingSyncStateSummaryResponse* reply
    ) override {
      Utils::logToFile("GetOngoingSyncStateSummary called!!");
      return Status::OK;
    }

    /// NOT IMPLEMENTED. See `StateSyncEnabled()`.
    Status GetLastStateSummary(
      ServerContext* context,
      const google::protobuf::Empty* request,
      vm::GetLastStateSummaryResponse* reply
    ) override {
      Utils::logToFile("GetLastStateSummary called!!");
      return Status::OK;
    }

    /// NOT IMPLEMENTED. See `StateSyncEnabled()`.
    Status ParseStateSummary(
      ServerContext* context,
      const vm::ParseStateSummaryRequest* request,
      vm::ParseStateSummaryResponse* reply
    ) override {
      Utils::logToFile("ParseStateSummary called!!");
      return Status::OK;
    }

    /// NOT IMPLEMENTED. See `StateSyncEnabled()`.
    Status GetStateSummary(
      ServerContext* context,
      const vm::GetStateSummaryRequest* request,
      vm::GetStateSummaryResponse* reply
    ) override {
      Utils::logToFile("GetStateSummary called!!");
      return Status::OK;
    }
};

#endif  // GRPCSERVER_H
