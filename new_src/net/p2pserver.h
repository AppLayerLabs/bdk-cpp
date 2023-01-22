#ifndef P2PSERVER_H
#define P2PSERVER_H

#include <thread>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "p2pmanager.h"
#include "p2pencoding.h"
#include "../utils/utils.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace tcp = boost::asio::ip::tcp;

/// Abstraction of a P2P server session/connection.
class P2PServerSession : public std::enable_shared_from_this<P2PServerSession> {
  private:
    /// Pointer to the P2P manager.
    const std::shared_ptr<P2PManager> mgr;

    /// Websocket object that maintains the connection and data transfers.
    websocket::stream<beast::tcp_stream> ws;

    /// Data buffer for read operations.
    beast::flat_buffer readBuf;

    /// Data buffer for write operations.
    beast::flat_buffer writeBuf;

  public:
    /**
     * Constructor.
     * @param sock The socket to take ownership of.
     * @param mgr Pointer to the P2P manager.
     */
    P2PServerSession(tcp::socket&& sock, const std::shared_ptr<P2PManager>& mgr)
      : ws(std::move(sock)), mgr(mgr) { ws.binary(true); }

    /// Get the host address of the remote endpoint.
    const boost::asio::ip::address host() {
      return ws.next_layer().socket().remote_endpoint().address();
    }

    /// Get the port of the remote endpoint.
    const unsigned short port() {
      return ws.next_layer().socket().remote_endpoint().port();
    }

    /// Dispatch a connection to its own strand and start the session.
    void start();

    /// Stop the session. TODO: why? Is there even a need for this?
    void stop();

    /**
     * Callback for the dispatch operation.
     * Listens for a handshake and accepts it when it arrives.
     */
    void on_start();

    /**
     * Read a message from the buffer.
     * Keeps on listening asynchronously until the message arrives from the endpoint.
     */
    void read();

    /**
     * Callback for the read operation (after a message is successfully read).
     * Automatically parses the message, calls `write()` to answer it, and
     * calls `read()` again to keep on listening to the next message.
     * @param ec The error code to parse.
     * @param bytes The number of bytes that were read.
     */
    void on_read(beast::error_code ec, std::size_t bytes);

    /**
     * Write a message to the buffer and send it to the endpoint.
     * @param data The data string to write and send.
     */
    void write(const P2PMsg& msg);

    /**
     * Callback for the write operation (after a message is successfully written).
     * @param ec The error code to parse.
     * @param bytes The number of bytes that were written.
     */
    void on_write(beast::error_code ec, std::size_t bytes);
};

/// Abstraction of a connection listener/dispatcher.
class P2PListener : public std::enable_shared_from_this<P2PListener> {
  private:
    /// I/O context object.
    net::io_context& ioc;

    /// TCP acceptor for the incoming connections.
    tcp::acceptor acceptor;

    /// Pointer to the P2P manager.
    const std::shared_ptr<P2PManager> mgr;

    /**
     * Accept an incoming connection.
     * The new connection gets its own strand.
     */
    void accept();

    /**
     * Callback for the accept operation.
     * Creates a new server session for the connection, dispatches it and
     * listens to the next one.
     * @param ec The error code to parse.
     * @param sock The socket to take ownership of.
     */
    void on_accept(beast::error_code ec, tcp::socket sock);

  public:
    /**
     * Constructor.
     * @param ioc The I/O context object to strand for the connection.
     * @param ep The endpoint to open the acceptor in for listening.
     * @param mgr Pointer to the P2P manager.
     */
    P2PListener(
      net::io_context& ioc, tcp::endpoint ep,
      const std::shared_ptr<P2PManager>& mgr
    ) : ioc(ioc), mgr(mgr) {
      beast::error_code ec;
      acceptor.open(ep.protocol(), ec); // Open the acceptor
      if (ec) { fail("P2PListener", __func__, ec, "open"); return; }
      acceptor.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
      if (ec) { fail("P2PListener", __func__, ec, "set_option"); return; }
      acceptor.bind(ep, ec); // Bind to the server address
      if (ec) { fail("P2PListener", __func__, ec, "bind"); return; }
      acceptor.listen(net::socket_base::max_listen_connections, ec); // Start listening
      if (ec) { fail("P2PListener", __func__, ec, "listen"); return; }
    }

    /// Start the listener.
    void start();

    /// Stop the listener.
    void stop();
};

/// Abstraction of the server side of a P2P connection.
class P2PServer : public std::enable_shared_from_this<P2PServer> {
  private:
    /// Pointer to the P2P manager.
    const std::shared_ptr<P2PManager> mgr;

    /// Object for core I/O functionality.
    net::io_context ioc;

    /// The host address where the server is.
    const boost::asio::ip::address host;

    /// The port where the server is.
    const unsigned short port;

    /// The number of threads that the server is running in.
    const unsigned int threads;

  public:
    /**
     * Constructor.
     * @param host The host address to open the server in.
     * @param The port to open the server in.
     * @param threads The number of threads to use.
     * @param mgr Pointer to the P2P manager.
     */
    P2PServer(
      const std::string& host, const unsigned short& port,
      const unsigned int& threads, const std::shared_ptr<P2PManager>& mgr
    ) : host(boost::asio::ip::make_address(host)),
        port(port), threads(threads), mgr(mgr) {}

    /// Start the server.
    void start();

    /// Stop the server.
    void stop();
};

#endif  // P2PSERVER_H
