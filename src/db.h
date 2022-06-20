#ifndef DB_H
#define DB_H

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
#include "../proto/rpcdb.grpc.pb.h"
#include "utils.h"

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

struct DBServer {
    std::string host;
    std::string version;
};

class DBService : public std::enable_shared_from_this<DBService> {
  private:
    std::unique_ptr<rpcdb::Database::Stub> db_stub_;
  public:
    explicit DBService(std::shared_ptr<Channel> channel)
      : db_stub_(rpcdb::Database::NewStub(channel)) {};

  bool has(std::string key);
  std::string get(std::string key);
  bool put(std::string key, std::string data);
  bool del(std::string key);
  bool close();
  bool writebatch();
};

#endif