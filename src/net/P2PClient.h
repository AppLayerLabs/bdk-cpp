#ifndef P2PCLIENT_H
#define P2PCLIENT_H

#include <memory>

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

// Forward declaration.

class P2PManager;
class P2PMessage;

class P2PClient : public std::enable_shared_from_this<P2PClient> {
  private:
    const std::shared_ptr<P2PManager> manager_;
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer receiveBuffer;

  public:
    const std::string host;
    const unsigned short port;

    P2PClient(net::io_context& ioc, const std::string& address, const unsigned short& port, const std::shared_ptr<P2PManager> manager) 
      : resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc)), host(address) ,port(port), manager_(manager) { ws_.binary(true); }
      
    void run();
    void stop();
    void resolve();
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void connect(tcp::resolver::results_type& results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void handshake(const std::string& host);
    void on_handshake(beast::error_code ec);
    void read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void write(const P2PMessage& data);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
};



#endif