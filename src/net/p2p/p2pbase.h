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

/// Namespace for everything related to %P2P (Peer-To-Peer) operations.
namespace P2P {
  // Forward declaration.
  class ManagerBase;

  /// Base class that abstracts a generic P2P connection/session.
  class BaseSession {
    protected:
      /// Websocket object for maintaining the connection.
      websocket::stream<beast::tcp_stream> ws_;

      /// Reference to the parent connection manager.
      ManagerBase& manager_;

      /// Reference pointer to a thread pool, for doing tasks in parallel.
      const std::unique_ptr<BS::thread_pool_light>& threadPool;

      /// Target IP/hostname.
      const std::string host_;

      /// Target port.
      const unsigned short port_;

      /// Target address.
      boost::asio::ip::address address_;

      /// Host node ID, got from handshake (hex), stored in bytes.
      Hash hostNodeId_;

      /// Host %P2P server port, got from handshake.
      unsigned short hostServerPort_;

      /// Host node type, got from handshake.
      NodeType hostType_;

      /// Indicates whether the session is closed or not.
      bool closed_ = false;

      /// Indicates which type of connection this session is.
      const ConnectionType connectionType_;

    public:
      /**
       * Constructor used by ClientSession.
       * @param ioc The I/O core object that will be used.
       * @param manager Reference to the parent connection manager.
       * @param host The host IP/address to connect to.
       * @param port The host port to connect to.
       * @param connectionType The type of connection that will be made.
       * @param threadPool Reference pointer to the thread pool.
       */
      BaseSession(
        net::io_context& ioc, ManagerBase& manager, const std::string& host,
        unsigned short port, ConnectionType connectionType,
        const std::unique_ptr<BS::thread_pool_light>& threadPool
      ) : ws_(net::make_strand(ioc)), manager_(manager), host_(host),
        port_(port), connectionType_(connectionType), threadPool(threadPool)
      { ws_.binary(true); }

      /**
       * Constructor used by ServerSession.
       * @param socket The socket that will be used.
       * @param manager Reference to the parent connection manager.
       * @param connectionType The type of connection that will be made.
       * @param threadPool Reference pointer to the thread pool.
       */
      BaseSession(
        tcp::socket&& socket, ManagerBase& manager, ConnectionType connectionType,
        const std::unique_ptr<BS::thread_pool_light>& threadPool
      ) : ws_(std::move(socket)), manager_(manager),
        host_(ws_.next_layer().socket().remote_endpoint().address().to_string()),
        port_(ws_.next_layer().socket().remote_endpoint().port()),
        connectionType_(connectionType), threadPool(threadPool)
      { ws_.binary(true); }

      /// Template function for startup operation. Implementation has to be overriden in child classes.
      virtual void run() {}

      /// Template function for shutdown operation. Implementation has to be overriden in child classes.
      virtual void stop() {}

      /// Template function for read operation. Implementation has to be overriden in child classes.
      virtual void read() {}

      /**
       * Template function for read callback. Implementation has to be overriden in child classes.
       * @param ec The error code to parse if necessary.
       * @param bytes_transferred The number of bytes transferred in the operation.
       */
      virtual void on_read(beast::error_code ec, std::size_t bytes_transferred) {}

      /**
       * Template function for write operation. Implementation has to be overriden in child classes.
       * @param message The mesage to write.
       */
      virtual void write(const Message& message) {}

      /**
       * Template function for write callback. Implementation has to be overriden in child classes.
       * @param ec The error code to parse if necessary.
       * @param bytes_transferred The number of bytes transferred in the operation.
       */
      virtual void on_write(beast::error_code ec, std::size_t bytes_transferred) {}

      /// Template function for closing connections. Implementation has to be overriden in child classes.
      virtual void close() {}

      /**
       * Template function for closing callback. Implementation has to be overriden in child classes.
       * @param ec The error code to parse if necessary.
       */
      virtual void on_close(beast::error_code ec) {}

      /// Getter for `host_`.
      const std::string& host() const { return host_; }

      /// Getter for `address_`.
      const boost::asio::ip::address address() { return address_; }

      /// Getter for `port_`.
      const unsigned short& port() const { return port_; }

      /// Getter for `address_` and `port_`, in form of a pair.
      const std::pair<boost::asio::ip::address, unsigned short> addressAndPort() { return std::make_pair(address_, port_); }

      /// Getter for `hostNodeId_`.
      const Hash& hostNodeId() const { return hostNodeId_; }

      /// Getter for `connectionType_`.
      const ConnectionType& connectionType() const { return connectionType_; }

      /// Getter for `hostType_`.
      const NodeType& hostType() const { return hostType_; }

      /// Getter for `hostServerPort_`.
      const unsigned short& hostServerPort() const { return hostServerPort_; }
  };
};

#endif  // P2PBASE_H
