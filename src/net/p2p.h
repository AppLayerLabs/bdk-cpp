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

// List of known P2P commands.
const std::map<std::string, std::pair<std::string, bool>> p2pcmds {
  {"info",                          {"0000", false} },
  {"sendTransaction",               {"0001", true} },
  {"sendBulkTransaction",           {"0002", true} },
  {"requestBlockByNumber",          {"0003", true} },
  {"requestBlockByHash",            {"0004", true} },
  {"requestBlockRange",             {"0005", true} },
  {"newBestBlock",                  {"0006", true} },
  {"sendValidatorTransaction",      {"0007", true} },
  {"sendBulkValidatorTransaction",  {"0008", true} },
  {"requestValidatorTxs",           {"0009", false} },
  {"getConnectedNodes",             {"000a", false} },
};

// TODO: neither P2PMsg nor P2PRes are considering the "0x" prefix - should they?
// Helper class for abstracting P2P messages.
class P2PMsg {
  private:
    std::string msg;
  public:
    P2PMsg(
      std::string id, std::vector<std::variant<uint64_t, uint256_t, std::string>> args = {}
    );
    std::string dump() { return this->msg; }
};

// Helper class for abstracting P2P responses.
class P2PRes {
  private:
    std::string res;
  public:
    P2PRes(std::string data, std::shared_ptr<ChainHead> ch);
    std::string dump() { return this->res; }
};

// The P2P node itself.
class P2P : public std::enable_shared_from_this<P2P> {
  private:
    std::vector<std::thread> ioc_threads;
    net::io_context ioc;
    // TODO: I dunno if we can use just one websocket for everything but we could try?
    websocket::stream<beast::tcp_stream> c_ws;
    std::shared_ptr<websocket::stream<beast::tcp_stream>> s_ws;
    beast::flat_buffer buf;
    tcp::resolver resolver;
    tcp::acceptor acceptor;
    std::vector<std::string> seedNodes; // TODO: peer discovery? we need another list for connected nodes
    std::shared_ptr<ChainHead> ch;
    // This thing goes so fast, we actually need to give some sort of
    // delay/cooldown between actions so it doesn't choke on itself with
    // throws/segfaults/cancelled operations/etc.
    // 2 milliseconds would be a good safe spot.
    // 1.5 milliseconds seems to be the limit, at least from what I've tested.
    // TODO: cooldown is not being used! It really should
    unsigned short cooldown = 2000; // in microseconds (1000 = 1ms)
    void wait() {
      std::this_thread::sleep_for(std::chrono::microseconds(cooldown));
    }

    // Set timeout for operation and prepare for connecting to another node.
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

    // Prepare for connecting to another node.
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

    // Prepare for reading something after accepting the handshake.
    void on_handshake(beast::error_code ec);

    // Prepare for starting the server.
    void on_accept(beast::error_code ec, tcp::socket socket);

    // Prepare for accepting a handshake.
    void on_start();

    // Prepare for listening to a message after writing one.
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

    // Prepare for writing a message based on what was read.
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

  public:
    // Constructor.
    P2P(const std::string host, const unsigned short port, std::shared_ptr<ChainHead> ch)
      : resolver(net::make_strand(ioc)), acceptor(net::make_strand(ioc)), c_ws(net::make_strand(ioc))
    {
      beast::error_code ec;
      tcp::endpoint ep{net::ip::make_address(host), port};
      acceptor.open(ep.protocol(), ec); // Open acceptor
      if (ec) { p2p_fail(ec, "open"); return; }
      acceptor.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
      if (ec) { p2p_fail(ec, "set_option"); return; }
      acceptor.bind(ep, ec); // Bind to server address
      if (ec) { p2p_fail(ec, "bind"); return; }
      acceptor.listen(net::socket_base::max_listen_connections, ec); // Listen for connections
      if (ec) { p2p_fail(ec, "listen"); return; }
      Utils::logToFile("P2P: listening on " + host + ":" + std::to_string(port));
      try {
        acceptor.async_accept(net::make_strand(ioc), beast::bind_front_handler(
          &P2P::on_accept, shared_from_this()
        ));
      } catch (std::exception &e) { p2p_fail(ec, e.what()); return; }
      ioc_threads.emplace_back([this]{ ioc.run(); }); // Always comes *after* async
      //json j = Utils::readConfigFile();
      //this->seedNodes = j["seedNodes"].get<std::vector<std::string>>();
    }

    // Connect to another node in a given host and port.
    void resolve(std::string host, std::string port);

    // Same as resolve() but specifically for nodes in the seedNodes list.
    void resolveSeedNode();

    // Write a message to another node.
    void write(std::string msg);

    // Read a message from another node.
    void read();
};

#endif  // P2P_H
