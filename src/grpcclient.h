#ifndef GRPC_CLIENT
#define GRPC_CLIENT

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
#include "../proto/aliasreader.grpc.pb.h"
#include "../proto/appsender.grpc.pb.h"
#include "../proto/keystore.grpc.pb.h"
#include "../proto/messenger.grpc.pb.h"
#include "../proto/sharedmemory.grpc.pb.h"
#include "db.h"

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


using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;


class VMCommClient : public std::enable_shared_from_this<VMCommClient> {

    public:
    explicit VMCommClient(std::shared_ptr<Channel> channel)
        : aliasreader_stub_(aliasreader::AliasReader::NewStub(channel)),
          appsender_stub_(appsender::AppSender::NewStub(channel)),
          keystore_stub_(keystore::Keystore::NewStub(channel)),
          messenger_stub_(messenger::Messenger::NewStub(channel)),
          sharedmemory_stub_(sharedmemory::SharedMemory::NewStub(channel)) {}
      
    private: 
    std::unique_ptr<aliasreader::AliasReader::Stub> aliasreader_stub_;
    std::unique_ptr<appsender::AppSender::Stub> appsender_stub_;
    std::unique_ptr<keystore::Keystore::Stub> keystore_stub_;
    std::unique_ptr<messenger::Messenger::Stub> messenger_stub_;
    std::unique_ptr<sharedmemory::SharedMemory::Stub> sharedmemory_stub_;
  };


#endif // GRPC_CLIENT.