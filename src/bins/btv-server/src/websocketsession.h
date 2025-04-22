#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include "utils.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Forward declaration
namespace BTVServer {
  class Manager;
  class WebsocketSession : public std::enable_shared_from_this<WebsocketSession> {
  private:
    Manager& manager_;
    beast::websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_; // Must persist between reads
    net::strand<decltype(ws_.get_executor())> strand_; // Strand to post write operations to
    std::atomic_bool closed_ = false;
    std::atomic_bool registered_ = false;
    std::unique_ptr<std::string> writeMsg_;
    std::deque<std::unique_ptr<std::string>> writeQueue_;
    std::mutex writeQueueMutex_;
    uint64_t id_ = 0;

    void doAccept();
    void onAccept(beast::error_code ec);
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytes_transferred);
    void onWrite(beast::error_code ec, std::size_t bytes_transferred);
    void onClose(beast::error_code ec);
    void onError();

  public:
    WebsocketSession(tcp::socket&& ioc, Manager& manager);
    ~WebsocketSession();

    void write(const std::string& msg);
    void stop();
    void start();
    const uint64_t& getId() {
      return id_;
    }
  };
}





#endif // WEBSOCKETSESSION_H