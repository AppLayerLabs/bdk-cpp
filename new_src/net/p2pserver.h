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

class P2PServerSession : public std::enable_shared_from_this<P2PServerSession> {
  private:
    const std::shared_ptr<P2PManager> mgr;
    websocket::stream<beast::tcp_stream> ws;
    beast::flat_buffer readBuf;
    beast::flat_buffer writeBuf;

  public:
    P2PServerSession(tcp::socket&& sock, const std::shared_ptr<P2PManager>& mgr)
      : ws(std::move(sock)), mgr(mgr) { ws.binary(true); }

    const boost::asio::ip::address host() {
      return ws_.next_layer().socket().remote_endpoint().address();
    }

    const unsigned short port() {
      return ws_.next_layer().socket().remote_endpoint().port();
    }

    void start();
    void stop();
    void on_start();
    void on_read(beast::error_code ec, std::size_t bytes);
    void write(const P2PMsg& msg);
    void on_write(beast::error_code ec, std::size_t bytes);
};

class P2PListener : public std::enable_shared_from_this<P2PListener> {
  private:
    net::io_context& ioc;
    tcp::acceptor acceptor;
    const std::shared_ptr<P2PManager> mgr;
    void accept();
    void on_accept(beast::error_code ec, tcp::socket sock);

  public:
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
    void start();
    void stop();
};

class P2PServer : public std::enable_shared_from_this<P2PServer> {
  private:
    const std::shared_ptr<P2PManager> mgr;
    net::io_context ioc;
    const boost::asio::ip::address host;
    const unsigned short port;
    const unsigned int threads;

  public:
    P2PServer(
      const std::string& host, const unsigned short& port,
      const unsigned int& threads, const std::shared_ptr<P2PManager>& mgr
    ) : host(boost::asio::ip::make_address(host)),
        port(port), threads(threads), mgr(mgr) {}
    void start();
    void stop();
};

#endif  // P2PSERVER_H
