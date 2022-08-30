#ifndef GRPC_CLIENT_H
#define GRPC_CLIENT_H

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
#include "../core/db.h"
#include "../core/transaction.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

class VMCommClient : public std::enable_shared_from_this<VMCommClient> {
  public:
    explicit VMCommClient(std::shared_ptr<Channel> channel, const std::vector<std::string> &nodeList, std::shared_mutex &nodeListLock) :
      aliasreader_stub_(aliasreader::AliasReader::NewStub(channel)),
      appsender_stub_(appsender::AppSender::NewStub(channel)),
      keystore_stub_(keystore::Keystore::NewStub(channel)),
      messenger_stub_(messenger::Messenger::NewStub(channel)),
      sharedmemory_stub_(sharedmemory::SharedMemory::NewStub(channel)),
      nodeList(nodeList),
      nodeListLock(nodeListLock) {}

  void requestBlock();
  // Copy because we don't actually know if mempool is keeping the transaction alive.
  // Subnet may receive a block that clears a given tx from mempool, making a reference to that tx null.
  void relayTransaction(const Tx::Base tx);
  private: 
    // TODO: having nodeList here as a refenrence is not ideal.
    // We should create a new class (Relayer) that actively relay transactions and messages to the network.
    const std::vector<std::string>& nodeList;
    std::shared_mutex &nodeListLock;
    std::unique_ptr<aliasreader::AliasReader::Stub> aliasreader_stub_;
    std::unique_ptr<appsender::AppSender::Stub> appsender_stub_;
    std::unique_ptr<keystore::Keystore::Stub> keystore_stub_;
    std::unique_ptr<messenger::Messenger::Stub> messenger_stub_;
    std::unique_ptr<sharedmemory::SharedMemory::Stub> sharedmemory_stub_;
    std::mutex lock;
};

#endif // GRPC_CLIENT_H
