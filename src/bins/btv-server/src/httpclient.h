#ifndef HTTPASYNCCLIENT_H
#define HTTPASYNCCLIENT_H

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include "utils.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace BTVServer {
  // Forward declaration
  class Manager;
  class HTTPSemiSyncClient {
  private:
    const std::string host;
    const std::string port;
    Manager& manager;
    tcp::resolver resolver;
    beast::tcp_stream stream;
    net::strand<net::io_context::executor_type> strand_;
    std::string makeHTTPRequestInternal(const std::shared_ptr<std::string> reqBody);
    uint64_t highestBlock = 0;

  public:
    HTTPSemiSyncClient(const std::string& host, const std::string& port, net::io_context& ioc_, Manager& manager);
    ~HTTPSemiSyncClient() noexcept;
    HTTPSemiSyncClient(const HTTPSemiSyncClient&) = delete;
    HTTPSemiSyncClient& operator=(const HTTPSemiSyncClient&) = delete;
    HTTPSemiSyncClient(HTTPSemiSyncClient&&) = delete;
    HTTPSemiSyncClient& operator=(HTTPSemiSyncClient&&) = delete;

    void connect();
    void close();

    void makeHTTPRequest(std::string&& reqBody);
  };
}


#endif // HTTPASYNCCLIENT_H