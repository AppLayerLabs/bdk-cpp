/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef GRPCCLIENT_H
#define GRPCCLIENT_H

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <shared_mutex>

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

#include "../../proto/aliasreader.grpc.pb.h"
#include "../../proto/appsender.grpc.pb.h"
#include "../../proto/keystore.grpc.pb.h"
#include "../../proto/messenger.grpc.pb.h"
#include "../../proto/sharedmemory.grpc.pb.h"

#include "../utils/db.h"
#include "../utils/tx.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

/// Abstraction of the client side of the gRPC protocol.
class gRPCClient : public std::enable_shared_from_this<gRPCClient> {
  private:
    // TODO: having nodes here as a reference is not ideal.
    // We should create a new class (Relayer) that actively relay transactions and messages to the network.
    // USE P2P INSTEAD OF GRPCCLIENT
    /// List of node IDs connected through AvalancheGo.
    const std::vector<std::string> nodes_;

    /// Stub for .proto gRPC Client.
    std::unique_ptr<aliasreader::AliasReader::Stub> aliasreaderStub_;

    /// Stub for .proto gRPC Client.
    std::unique_ptr<appsender::AppSender::Stub> appsenderStub_;

    /// Stub for .proto gRPC Client.
    std::unique_ptr<keystore::Keystore::Stub> keystoreStub_;

    /// Stub for .proto gRPC Client.
    std::unique_ptr<messenger::Messenger::Stub> messengerStub_;

    /// Stub for .proto gRPC Client.
    std::unique_ptr<sharedmemory::SharedMemory::Stub> sharedmemoryStub_;

    /// Mutex for managing read/write access to the stubs.
    std::mutex lock_;

  public:
    /**
     * Constructor.
     * @param channel Pointer to a gRPC channel. See the [gRPC docs](https://grpc.io/docs/what-is-grpc/core-concepts/#channels) for more info.
     * @param nodes List of nodes connected through AvalancheGo.
     */
    gRPCClient(
      const std::shared_ptr<Channel> channel,
      const std::vector<std::string>& nodes
    ) : nodes_(nodes),
      aliasreaderStub_(aliasreader::AliasReader::NewStub(channel)),
      appsenderStub_(appsender::AppSender::NewStub(channel)),
      keystoreStub_(keystore::Keystore::NewStub(channel)),
      messengerStub_(messenger::Messenger::NewStub(channel)),
      sharedmemoryStub_(sharedmemory::SharedMemory::NewStub(channel))
    {}

    /**
     * Request a block to AvalancheGo.
     * AvalancheGo will call BuildBlock() after that.
     */
    void requestBlock();
};

#endif  // GRPCCLIENT_H
