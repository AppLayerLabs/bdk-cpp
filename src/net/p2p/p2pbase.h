#ifndef P2PBASE_H
#define P2PBASE_H

#include <iostream>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/lexical_cast.hpp>

#include "p2pencoding.h"

#include "../../libs/BS_thread_pool_light.hpp"
#include "../../utils/strings.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Forward declaration

namespace P2P {
  class ManagerBase;

  class BaseSession {
    protected:
      websocket::stream<beast::tcp_stream> ws_;
      ManagerBase& manager_;
      const std::unique_ptr<BS::thread_pool_light>& threadPool;
      const std::string host_;                 // Target IP/hostname
      const unsigned short port_;              // Target port
      boost::asio::ip::address address_;       // Target address
      Hash hostNodeId_;                 // Host node ID, got from handshake (hex), stored in bytes.
      unsigned short hostServerPort_;    // Host P2P Server Port, got from handshake.
      NodeType hostType_;                      // Host node type, got from handshake.
      bool closed_ = false;
      const ConnectionType connectionType_;

    public:
      // Used by ClientSession
      BaseSession(
        net::io_context& ioc, ManagerBase& manager, const std::string& host,
        unsigned short port, ConnectionType connectionType,
        const std::unique_ptr<BS::thread_pool_light>& threadPool
      ) : ws_(net::make_strand(ioc)), manager_(manager), host_(host),
        port_(port), connectionType_(connectionType), threadPool(threadPool)
      { ws_.binary(true); }

      // Used by ServerSession
      BaseSession(
        tcp::socket&& socket, ManagerBase& manager, ConnectionType connectionType,
        const std::unique_ptr<BS::thread_pool_light>& threadPool
      ) : ws_(std::move(socket)), manager_(manager),
        host_(ws_.next_layer().socket().remote_endpoint().address().to_string()),
        port_(ws_.next_layer().socket().remote_endpoint().port()),
        connectionType_(connectionType), threadPool(threadPool)
      { ws_.binary(true); }

      virtual void run() {}
      virtual void stop() {}
      virtual void read() {}
      virtual void on_read(beast::error_code ec, std::size_t bytes_transferred) {}
      virtual void write(const Message& message) {}
      virtual void on_write(beast::error_code ec, std::size_t bytes_transferred) {}
      virtual void close() {}
      virtual void on_close(beast::error_code ec) {}

      const std::string& host() const { return host_; }
      const boost::asio::ip::address address() { return address_; }
      const unsigned short& port() const { return port_; }
      const std::pair<boost::asio::ip::address, unsigned short> addressAndPort() { return std::make_pair(address_, port_); }
      const Hash& hostNodeId() const { return hostNodeId_; }
      const ConnectionType& connectionType() const { return connectionType_; }
      const NodeType& hostType() const { return hostType_; }
      const unsigned short& hostServerPort() const { return hostServerPort_; }
  };

};

#endif
