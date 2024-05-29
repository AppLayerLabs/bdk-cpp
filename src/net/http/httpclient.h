/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/
#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/buffers_iterator.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class HTTPSyncClient {
  private:
    const std::string host;
    const std::string port;
    boost::asio::io_context ioc;
    tcp::resolver resolver;
    beast::tcp_stream stream;

  public:
    HTTPSyncClient(const std::string& host, const std::string& port);
    ~HTTPSyncClient() noexcept;
    HTTPSyncClient(const HTTPSyncClient&) = delete;
    HTTPSyncClient& operator=(const HTTPSyncClient&) = delete;
    HTTPSyncClient(HTTPSyncClient&&) = delete;
    HTTPSyncClient& operator=(HTTPSyncClient&&) = delete;

    void connect();

    void close();

    std::string makeHTTPRequest(const std::string& reqBody);
};








#endif // HTTPCLIENT_H
