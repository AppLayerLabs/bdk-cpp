#ifndef P2P_H
#define P2P_H

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

/**
 * Abstraction of an asynchronous P2P node.
 * Adapted from Boost Beast client and server async examples.
 */

// Report a failure.
// TODO: join this and fail() from httpserver
void p2p_fail(beast::error_code ec, char const* what);

// Client side of the P2P node.
class P2PClient : public std::enable_shared_from_this<P2PClient> {
  std::vector<std::thread> ioc_threads;
  net::io_context ioc;
  tcp::resolver resolver_;
  websocket::stream<beast::tcp_stream> ws_;
  beast::flat_buffer buffer_;
  std::string host_;

  public:
    // Constructor.
    P2PClient() : resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc)) {}

    // Initialize the I/O context and resolve a given host and port.
    void resolve(std::string host, std::string port);

    // Connect to a given host and port.
    void connect(tcp::resolver::results_type results);

    // Perform a handshake to the connected host.
    void handshake(std::string host, std::string port);

    // Send a message to the connected host.
    void write(std::string msg);

    // Read a message from the connected host into the buffer.
    void read();

    // Close the websocket connection.
    void stop();

  private:
    // Set timeout for operation and prepare for connecting to looked up host.
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

    // Prepare for connectiong to a given host.
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

    // Prepare for sending a message to the connected host.
    void on_handshake(beast::error_code ec);

    // Prepare for reading a message from the connected host into the buffer.
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

    // Display the message in the buffer, clear it, wait a second and write again.
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    // Warn that the connection was closed.
    // If we get here then the connection is closed gracefully.
    void on_stop(beast::error_code ec);
};

// Server side of the P2P node.
class P2PServer : public std::enable_shared_from_this<P2PServer> {
  websocket::stream<beast::tcp_stream> ws_;
  beast::flat_buffer buffer_;

  public:
    // Take ownership of the socket
    explicit P2PServer(tcp::socket&& socket) : ws_(std::move(socket)) {}

    // Get on the correct executor. We need to execute within a strand
    // to perform async operations on I/O objects.
    // Although not strictly necessary for single-threaded contexts,
    // this code is written to be thread-safe by default.
    void start();

    // Accept the websocket handshake.
    void accept();

    // Read a message into the buffer.
    void read();

    // Write a message to the origin.
    void write(std::string msg);

  private:
    // Prepare for accepting a handshake from a given origin.
    void on_start();

    // Prepare for reading something after accepting the handshake.
    void on_accept(beast::error_code ec);

    // Prepare for writing a message to the origin.
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    // Clear the buffer and do another read.
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
};

// Accepts incoming connections and launches the P2PServer
// TODO: join this with P2PServer?
class P2PListener : public std::enable_shared_from_this<P2PListener> {
  std::vector<std::thread> ioc_threads;
  net::io_context ioc;
  tcp::acceptor acceptor_;

  public:
    P2PListener(tcp::endpoint endpoint) : acceptor_(ioc) {
      beast::error_code ec;
      acceptor_.open(endpoint.protocol(), ec); // Open acceptor
      if (ec) { p2p_fail(ec, "open"); return; }
      acceptor_.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
      if (ec) { p2p_fail(ec, "set_option"); return; }
      acceptor_.bind(endpoint, ec); // Bind to server address
      if (ec) { p2p_fail(ec, "bind"); return; }
      acceptor_.listen(net::socket_base::max_listen_connections, ec); // Listen for connections
      if (ec) { p2p_fail(ec, "listen"); return; }
    }

    // Accept an incoming connection into its own strand
    void start() {
      //std::cout << "Listener: running" << std::endl;
      acceptor_.async_accept(net::make_strand(ioc), beast::bind_front_handler(
        &P2PListener::on_accept, shared_from_this()
      ));
      ioc_threads.emplace_back([this]{ ioc.run(); }); // Always comes *after* async
    }

  private:
    // Create the P2PServer, run it and accept another connection
    void on_accept(beast::error_code ec, tcp::socket socket) {
      //std::cout << "Listener: connection accepted" << std::endl;
      if (ec) {
        p2p_fail(ec, "listener_accept");
      } else {
        std::make_shared<P2PServer>(std::move(socket))->start();
      }
      acceptor_.async_accept(net::make_strand(ioc), beast::bind_front_handler(
        &P2PListener::on_accept, shared_from_this()
      ));
    }
};

#endif  // P2P_H
