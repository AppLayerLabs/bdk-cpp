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

#include "../core/chainHead.h"
#include "../utils/utils.h"

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
void p2p_fail(beast::error_code ec, char const* what);

// Information about the P2P client and/or server.
class P2PInfo {
  private:
    std::shared_ptr<ChainHead> ch;
    std::string version;  // e.g. "0.0.0.1"
    uint64_t epoch;       // from system, in nanosseconds
    uint64_t nHeight;     // most recent block height
    Hash nHash;           // most recent block hash
    uint64_t nodes = 0;   // number of connected nodes

  public:
    // TODO: update nodes on connect
    P2PInfo(std::string version_, std::shared_ptr<ChainHead> ch_)
      : version(version_), ch(ch_) {}

    void addNode() { this->nodes++; }
    void delNode() { this->nodes--; }

    std::string dump() {
      epoch = std::time(NULL);
      nHeight = this->ch->latest()->nHeight();
      nHash = this->ch->latest()->getBlockHash();
      return std::string("Version: " + this->version +
        "\nEpoch: " + std::to_string(this->epoch) +
        "\nnHeight: " + std::to_string(this->nHeight) +
        "\nnHash: " + this->nHash.hex() +
        "\nConnected nodes: " + std::to_string(this->nodes)
      );
    }
};

// Client side of the P2P node.
class P2PClient : public std::enable_shared_from_this<P2PClient> {
  std::vector<std::thread> ioc_threads;
  net::io_context ioc;
  tcp::resolver resolver_;
  websocket::stream<beast::tcp_stream> ws_;
  beast::flat_buffer buffer_;
  std::string host_;
  P2PInfo info_;
  std::vector<std::string> seedNodes;
  const std::vector<std::string> cmds {
    "info"
  };

  public:
    // Constructor.
    P2PClient(P2PInfo info) : resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc)), info_(info) {
      json j = Utils::readConfigFile();
      this->seedNodes = j["seedNodes"].get<std::vector<std::string>>();
    }

    // Initialize the I/O context and resolve a given host and port.
    // Call this function to connect to a server.
    void resolve(std::string host, std::string port);

    // Same as resolve() but specifically handles the seedNodes list.
    void resolveSeedNode();

    // Connect to a given host and port.
    void connect(tcp::resolver::results_type results);

    // Perform a handshake to the connected host.
    void handshake(std::string host, std::string port);

    // Send a message to the connected host.
    void write(std::string msg);

    // Read a message from the connected host into the buffer.
    void read();

    // Parse a given command.
    std::string parse(std::string cmd);

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
};

// Server side of the P2P node.
class P2PServer : public std::enable_shared_from_this<P2PServer> {
  std::vector<std::thread> ioc_threads;
  net::io_context ioc;
  tcp::acceptor acceptor_;
  std::shared_ptr<websocket::stream<beast::tcp_stream>> ws_;
  beast::flat_buffer buffer_;
  P2PInfo info_;
  const std::vector<std::string> cmds {
    "info"
  };

  public:
    P2PServer(tcp::endpoint ep, P2PInfo info)
    : acceptor_(ioc), info_(info) {
      beast::error_code ec;
      acceptor_.open(ep.protocol(), ec); // Open acceptor
      if (ec) { p2p_fail(ec, "open"); return; }
      acceptor_.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
      if (ec) { p2p_fail(ec, "set_option"); return; }
      acceptor_.bind(ep, ec); // Bind to server address
      if (ec) { p2p_fail(ec, "bind"); return; }
      acceptor_.listen(net::socket_base::max_listen_connections, ec); // Listen for connections
      if (ec) { p2p_fail(ec, "listen"); return; }
    }

    // Start accepting connections.
    void accept();

    // Read a message into the buffer.
    void read();

    // Write a message to the origin.
    void write(std::string msg);

    // Parse a given command.
    std::string parse(std::string cmd);

  private:
    // Prepare for starting the server.
    void on_accept(beast::error_code ec, tcp::socket socket);

    // Prepare for accepting a handshake.
    void on_start();

    // Prepare for reading something after accepting the handshake.
    void on_handshake(beast::error_code ec);

    // Prepare for writing a message to the origin.
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    // Clear the buffer and do another read.
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
};

// The P2P node itself.
class P2PNode : public std::enable_shared_from_this<P2PNode> {
  private:
    std::shared_ptr<P2PServer> p2ps;
    std::shared_ptr<P2PClient> p2pc;
    // This thing goes so fast, we actually need to give some sort of
    // delay/cooldown between actions so it doesn't choke on itself with
    // throws/segfaults/cancelled operations/etc.
    // 2 milliseconds would be a good safe spot.
    // 1.5 milliseconds seems to be the limit, at least from what I've tested.
    unsigned short cooldown = 2000; // in microseconds (1000 = 1ms)
    void wait() {
      std::this_thread::sleep_for(std::chrono::microseconds(cooldown));
    }

  public:
    P2PNode(const std::string s_host, const unsigned short s_port, std::shared_ptr<ChainHead> ch) {
      // TODO: un-hardcode versions
      this->p2ps = std::make_shared<P2PServer>(
        tcp::endpoint{net::ip::make_address(s_host), s_port}, P2PInfo("0.0.0.1s", ch)
      );
      this->p2pc = std::make_shared<P2PClient>(P2PInfo("0.0.0.1c", ch));
      Utils::logToFile("P2P node running on " + s_host + ":" + std::to_string(s_port));
      this->p2ps->accept(); this->wait();
      this->p2pc->resolveSeedNode(); this->wait();
    }
};

#endif  // P2P_H
