#ifndef MAIN_H
#define MAIN_H

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

Block genesis("0000000000000000000000000000000000000000000000000000000000000000",
  1648317800,
  0, // 0 tx's
  0,
  ""
);

class VMServiceImplementation;

void startServer(std::shared_ptr<VMServiceImplementation> service_pointer);

// Logic and data behind the server's behavior.
class VMServiceImplementation final : public vmproto::VM::Service, public std::enable_shared_from_this<VMServiceImplementation> {
  std::string dbName;
  bool initialized = false;
  Database blocksDb; // Key -> blockHash()
                      // Value -> json block
 
  Database confirmedTxs;
  Database accountsDb; // Key -> underscored user address "0x..."
                        // Value -> balance in wei.


  std::vector<dev::eth::TransactionBase> mempool;

  Status Initialize(ServerContext* context, const vmproto::InitializeRequest* request,
                  vmproto::InitializeResponse* reply) override {
    Utils::logToFile("Initialized Called");
    initialized = true;
    std::string jsonRequest;
    google::protobuf::util::JsonOptions options;
    google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
    Utils::logToFile(jsonRequest);
    // Open database with the nodeID as folder name.
    json req = json::parse(jsonRequest);
    dbName = req["nodeId"];
    blocksDb.setAndOpenDB(dbName + "-blocks");
    accountsDb.setAndOpenDB(dbName + "-balances");
    confirmedTxs.setAndOpenDB(dbName + "-txs");
    if (blocksDb.isEmpty()) {
      blocksDb.putKeyValue(genesis.blockHash(), genesis.serializeToString());
      blocksDb.putKeyValue("latest", genesis.serializeToString());
    }
    if(accountsDb.isEmpty()) {
      accountsDb.putKeyValue("0x24ff7cb43b80a5fb44c5ddeb5510d2a4b62ae26d", "10000000000000000000000000000000000000");
    }
    Block bestBlock(blocksDb.getKeyValue("latest"));
    Utils::logToFile(blocksDb.getKeyValue("latest"));

    auto hash = bestBlock.blockHash();
    Utils::logToFile(std::string("size: ") + boost::lexical_cast<std::string>(hash.size()));
    Utils::logToFile(bestBlock.blockHash());
    reply->set_last_accepted_id(Utils::hashToBytes(bestBlock.blockHash()));
    reply->set_last_accepted_parent_id(Utils::hashToBytes(bestBlock.prevBlockHash()));
    reply->set_status(1);
    reply->set_height(1);
    // bytes -> last block bytes.
    reply->set_bytes(bestBlock.serializeToString()); 
    reply->set_timestamp(Utils::secondsToGoTimeStamp(boost::lexical_cast<uint64_t>(bestBlock.timestamp())));
    std::string jsonAnswer;
    google::protobuf::util::MessageToJsonString(*reply, &jsonAnswer, options);
    Utils::logToFile(std::string("jsonAnswer:" + jsonAnswer));

    boost::thread p(&startServer, shared_from_this());
    p.detach();
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
    if (initialized) {
      blocksDb.cleanCloseDB();
      accountsDb.cleanCloseDB();
    }
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
    Block pastBlock(blocksDb.getKeyValue("latest"));
    const auto p1 = std::chrono::system_clock::now();
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
    auto nextHeight = pastBlock.nHeight() + 1;
    Block newBestBlock(pastBlock.blockHash(),
                       boost::lexical_cast<dev::u256>(now),
                       0,
                       nextHeight,
                       "");
    for (auto tx : mempool) {
      newBestBlock.addTx(tx);
      confirmedTxs.putKeyValue(dev::toHex(tx.sha3()), dev::toHex(tx.rlp()));
    }

    reply->set_id(Utils::hashToBytes(newBestBlock.blockHash()));
    reply->set_parent_id(Utils::hashToBytes(newBestBlock.prevBlockHash()));
    reply->set_bytes(newBestBlock.serializeToString());
    reply->set_height(boost::lexical_cast<uint64_t>(newBestBlock.nHeight()));
    reply->set_timestamp(Utils::secondsToGoTimeStamp(boost::lexical_cast<uint64_t>(newBestBlock.timestamp())));

    blocksDb.deleteKeyValue("latest");
    blocksDb.putKeyValue(newBestBlock.blockHash(), newBestBlock.serializeToString());
    blocksDb.putKeyValue("latest", newBestBlock.serializeToString());
    Utils::logToFile(request->DebugString());
    Utils::logToFile(reply->DebugString());
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
  
  public:
  void processRPCMessage(std::string message) {
    Utils::logToFile(message);
    return;
  }
};

template<
    class Body, class Allocator,
    class Send>
void
handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send,
    std::shared_ptr<VMServiceImplementation> service_pointer)
{
    // Returns a bad request response
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    std::string request = req.body();

    // Respond to OPTIONS.
    // there is absolute no fkin documentation over this
    // someone plz write some :pray:
    if (req.method() == http::verb::options) {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::access_control_allow_origin, "*");
        res.set(http::field::access_control_allow_methods, "POST, GET");
        res.set(http::field::access_control_allow_headers, "content-type");
        res.set(http::field::accept_encoding, "deflate");
        res.set(http::field::accept_language, "en-US");
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }
    // Respond to POST
    service_pointer->processRPCMessage(request);
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return send(std::move(res));
}

template<class Stream>
struct send_lambda
{
    Stream& stream_;
    bool& close_;
    beast::error_code& ec_;

    explicit
    send_lambda(
        Stream& stream,
        bool& close,
        beast::error_code& ec)
        : stream_(stream)
        , close_(close)
        , ec_(ec)
    {
    }

    template<bool isRequest, class Body, class Fields>
    void
    operator()(http::message<isRequest, Body, Fields>&& msg) const
    {
        // Determine if we should close the connection after
        close_ = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::write(stream_, sr, ec_);
    }
};

void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

void
do_session(
    tcp::socket& socket,
    std::shared_ptr<std::string const> const& doc_root,
    std::shared_ptr<VMServiceImplementation> service_pointer)
{
    bool close = false;
    beast::error_code ec;

    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    // This lambda is used to send messages
    send_lambda<tcp::socket> lambda{socket, close, ec};

    for(;;)
    {
        // Read a request
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if(ec == http::error::end_of_stream)
            break;
        if(ec)
            return fail(ec, "read");

        // Send the response
        handle_request(*doc_root, std::move(req), lambda, service_pointer);
        if(ec)
            return fail(ec, "write");
        if(close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    // Send a TCP shutdown
    socket.shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}

void startServer(std::shared_ptr<VMServiceImplementation> service_pointer)
{
    try
    {
        // Check command line arguments.
        auto const address = net::ip::make_address("0.0.0.0");
        auto const port = 80;
        auto const doc_root = std::make_shared<std::string>("/");

        // The io_context is required for all I/O
        net::io_context ioc{1};

        // The acceptor receives incoming connections
        tcp::acceptor acceptor{ioc, {address, port}};
        for(;;)
        {
            // This will receive the new connection
            tcp::socket socket{ioc};

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread{std::bind(
                &do_session,
                std::move(socket),
                doc_root,
                service_pointer)}.detach();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
}

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
  // responsible for shutting down the server for this call to ever return
  server->Wait();
}

#endif // MAIN_H