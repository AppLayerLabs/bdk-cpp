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


// As Subnets are meant to be run inside a sandbox, we cannot create our own DB.
// We have to use the DB that AvalancheGo provides for us through gRPC.
// It is a simple key/value store database, similar to leveldb.
// But it allows for writing in batch and reading all keys using a given prefix.
// The database structure is as follows:
// 0x0001 -- Blocks
// 0x0002 -- Transactions
// 0x0003 -- Native Balance
// 0x0004 -- ERC20 Tokens/State
// 0x0005 -- ERC721 Tokens/State

namespace DBPrefix {
  const std::string blocks = "0x0001";
  const std::string transactions = "0x0002";
  const std::string nativeBalance = "0x0003";
  const std::string erc20Tokens = "0x0004";
  const std::string erc721Tokens = "0x0005";
}

struct DBServer {
    std::string host;
    std::string version;
};

struct DBEntry {
    std::string key;
    std::string value;
};

struct DBKey {
    std::string key;
};

struct WriteBatchRequest {
  std::vector<DBEntry> puts;
  std::vector<DBKey> dels;
  int64_t id;
  bool continues;
};

class DBService : public std::enable_shared_from_this<DBService> {
  private:
    std::unique_ptr<rpcdb::Database::Stub> db_stub_;
  public:
    explicit DBService(std::shared_ptr<Channel> channel)
      : db_stub_(rpcdb::Database::NewStub(channel)) {};

  bool has(std::string key, std::string prefix = "");
  std::string get(std::string key, std::string prefix = "");
  bool put(std::string key, std::string data, std::string prefix = "");
  bool del(std::string key, std::string prefix = "");
  bool close();
  bool writeBatch(WriteBatchRequest &request, std::string prefix = "");
  // Read all keys starting with prefix
  std::vector<DBEntry> readBatch(std::string prefix);
  void testDB();
};

#endif