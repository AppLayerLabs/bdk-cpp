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

class gRPCClient : public std::enable_shared_from_this<gRPCClient> {
  private:
    // TODO: having nodes here as a reference is not ideal.
    // We should create a new class (Relayer) that actively relay transactions and messages to the network.
    // USE P2P INSTEAD OF GRPCCLIENT
    const std::vector<std::string> nodes;
    std::unique_ptr<aliasreader::AliasReader::Stub> aliasreaderStub;
    std::unique_ptr<appsender::AppSender::Stub> appsenderStub;
    std::unique_ptr<keystore::Keystore::Stub> keystoreStub;
    std::unique_ptr<messenger::Messenger::Stub> messengerStub;
    std::unique_ptr<sharedmemory::SharedMemory::Stub> sharedmemoryStub;
    std::mutex lock;

  public:
    gRPCClient(
      const std::shared_ptr<Channel> channel,
      const std::vector<std::string>& nodes
    ) : nodes(nodes),
      aliasreaderStub(aliasreader::AliasReader::NewStub(channel)),
      appsenderStub(appsender::AppSender::NewStub(channel)),
      keystoreStub(keystore::Keystore::NewStub(channel)),
      messengerStub(messenger::Messenger::NewStub(channel)),
      sharedmemoryStub(sharedmemory::SharedMemory::NewStub(channel))
    {}

    void requestBlock();
};

#endif  // GRPCCLIENT_H
