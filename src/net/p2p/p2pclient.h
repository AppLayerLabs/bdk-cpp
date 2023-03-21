#ifndef P2PCLIENT_H
#define P2PCLIENT_H

#include <memory>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../../utils/utils.h"
#include "p2pbase.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Forward declaration.

namespace P2P {

  class ManagerBase;
  class Message;

  class ClientSession : public BaseSession, public std::enable_shared_from_this<ClientSession> {
    private:
      tcp::resolver resolver_;
      beast::flat_buffer receiveBuffer_;
      std::mutex writeLock_;
      boost::beast::websocket::response_type req_;

    public:
      ClientSession(net::io_context& ioc, const std::string& host, const unsigned short& port, ManagerBase& manager) :
        BaseSession(ioc, manager, host, port, ConnectionType::CLIENT), resolver_(net::make_strand(ioc)) {}

      void run() override;
      void stop() override;
      void resolve();
      void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
      void connect(tcp::resolver::results_type& results);
      void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
      void handshake(const std::string& host);
      void read() override;
      void on_read(beast::error_code ec, std::size_t bytes_transferred) override;
      void write(const Message& data) override;
      void on_write(beast::error_code ec, std::size_t bytes_transferred) override;
      void close() override;
      void on_close(beast::error_code ec) override;

      void handleError(const std::string& func, const beast::error_code& ec);
  };

}

#endif