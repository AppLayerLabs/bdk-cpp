#ifndef P2PSERVER_H
#define P2PSERVER_H


#include <thread>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../utils/utils.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


// Forward declaration
class P2PManager;

class ServerSession : public std::enable_shared_from_this<ServerSession> {
  private:
    const std::shared_ptr<P2PManager> manager_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_; 
    beast::flat_buffer answerBuffer_;
  public:

  //using SessionBase::SessionBase;
  ServerSession(tcp::socket&& socket, const std::shared_ptr<P2PManager> manager) : ws_(std::move(socket)), manager_(manager) {}

  void run();
  void stop();
  void on_run();
  void on_accept(beast::error_code ec);
  void read();
  void on_read(beast::error_code ec, std::size_t bytes_transferred);
  void write(const std::string& message);
  void on_write(beast::error_code ec, std::size_t bytes_transferred);
  
};

class P2PServer : public std::enable_shared_from_this<P2PServer>  {
  class listener : public std::enable_shared_from_this<listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<P2PManager> manager_;
    public:
      listener(net::io_context& ioc, tcp::endpoint endpoint, const std::shared_ptr<P2PManager> manager) : ioc_(ioc), acceptor_(ioc), manager_(manager) {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec); // Open the acceptor
        if (ec) { p2p_fail_server(__func__, ec, "open"); return; }
        acceptor_.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
        if (ec) { p2p_fail_server(__func__, ec, "set_option"); return; }
        acceptor_.bind(endpoint, ec); // Bind to the server address
        if (ec) { p2p_fail_server(__func__, ec, "bind"); return; }
        acceptor_.listen(net::socket_base::max_listen_connections, ec); // Start listening
        if (ec) { p2p_fail_server(__func__, ec, "listen"); return; }
      }

      void run();
      void stop();

    private:

      void accept();
      void on_accept(beast::error_code ec, tcp::socket socket);
  };

  private: 
    const std::shared_ptr<P2PManager> manager_;
    net::io_context ioc;
    const boost::asio::ip::address address;
    const unsigned short port;
    const unsigned int threads;

  public:
    P2PServer(std::string address, unsigned short port, unsigned int threads, const std::shared_ptr<P2PManager> manager)
     : address(boost::asio::ip::make_address(address)), port(port), threads(threads), manager_(manager) {};

    void start();
    void stop();

};

#endif