/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

/// Class for an HTTP sync client.
class HTTPSyncClient {
  private:
    const std::string host; ///< The host the client will connect to.
    const std::string port; ///< The port the client will connect to.
    boost::asio::io_context ioc;  ///< I/O context object for the client.
    tcp::resolver resolver; ///< TCP/IP resolver.
    beast::tcp_stream stream; ///< Buffer stream.

  public:
    /**
     * Constructor.
     * @param host The host the client will connect to.
     * @param port The port the client will connect to.
     */
    HTTPSyncClient(const std::string& host, const std::string& port);

    ~HTTPSyncClient(); ///< Destructor.
    HTTPSyncClient(const HTTPSyncClient&) = delete; ///< Copy constructor (deleted, Rule of Zero).
    HTTPSyncClient& operator=(const HTTPSyncClient&) = delete; ///< Copy assignment operator. (deleted, Rule of Zero).
    HTTPSyncClient(HTTPSyncClient&&) = delete; ///< Move constructor. (deleted, Rule of Zero).
    HTTPSyncClient& operator=(HTTPSyncClient&&) = delete; ///< Move assignment operator. (deleted, Rule of Zero).

    void connect(); ///< Start the connection.

    void close(); ///< Close the connection.

    /**
     * Perform an HTTP request to the endpoint.
     * @param reqBody The request body.
     * @return A string containing the result of the request.
     */
    std::string makeHTTPRequest(const std::string& reqBody);
};

#endif // HTTPCLIENT_H
