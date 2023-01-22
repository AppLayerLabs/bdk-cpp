#ifndef P2PCLIENT_H
#define P2PCLIENT_H

#include <memory>
#include <mutex>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "p2pencoding.h"
#include "p2pmanager.h"
#include "../utils/utils.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace tcp = boost::asio::ip::tcp;

class P2PClient : public std::enable_shared_from_this<P2PClient> {
  private:
    const std::shared_ptr<P2PManager> mgr;
    tcp::resolver resolver;
    websocket::stream<beast::tcp_stream> ws;
    beast::flat_buffer buf;
    std::mutex writeLock;

  public:
    const std::string host;
    const unsigned short port;

    P2PClient(
      net::io_context& ioc, const std::string& host,
      const unsigned short& port, const std::shared_ptr<P2PManager>& mgr
    ) : resolver(net::make_strand(ioc)), ws(net::make_strand(ioc)),
      host(host), port(port), mgr(mgr)
    { ws.binary(true); }

    void start();
    void stop();
    void resolve();
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void connect(tcp::resolver::results_type& results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void handshake(const std::string& host);
    void on_handshake(beast::error_code ec);
    void read();
    void on_read(beast::error_code ec, std::size_t bytes);
    void write(const P2PMessage data);
    void on_write(beast::error_code ec, std::size_t bytes);
};

#endif  // P2PCLIENT_H
