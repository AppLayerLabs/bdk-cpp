#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <boost/algorithm/hex.hpp>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include "../core/blockchain.h"
#include "../utils/utils.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Forward declarations.
class Blockchain;
class HTTPSession;  // HTTPQueue depends on HTTPSession and vice-versa

/**
 * Produce an HTTP response for the given request.
 * The type of the response object depends on the contents of the request,
 * so the interface requires the caller to pass a generic lambda to receive the response.
 * @param docroot The root directory of the endpoint.
 * @param req The request to handle.
 * @param send TODO: we're missing details on this, Allocator, Body, the function itself and where it's used
 * @param blockchain Reference to the blockchain.
 */
template<class Body, class Allocator, class Send> void handle_request(
  beast::string_view docroot,
  http::request<Body, http::basic_fields<Allocator>>&& req,
  Send&& send, Blockchain& blockchain
);

/**
 * Helper class used for HTTP pipelining.
 * TODO: explain better what this is and what it's used for.
 */
class HTTPQueue {
  private:
    /// Maximum number of responses to queue.
    unsigned int limit = 8;

    /// Type-erased, saved work item. TODO: what is this?
    struct work { virtual ~work() = default; virtual void operator()() = 0; };

    /// Reference to the HTTP session that is handling the queue.
    HTTPSession& session;

    /// Array of pointers to work structs.
    std::vector<std::unique_ptr<work>> items;

  public:
    /**
     * Constructor.
     * @param session Reference to the HTTP session that will handle the queue.
     */
    HTTPQueue(HTTPSession& session) : session(session) {
      assert(this->limit > 0);
      items.reserve(this->limit);
    }

    /**
     * Check if the queue limit was hit.
     * @return `true` if queue is full, `false` otherwise.
     */
    bool full() { return this->items.size() >= this->limit; }

    /**
     * Callback for when a message is sent.
     * @return `true` if the caller should read a message, `false` otherwise.
     */
    bool on_write() {
      BOOST_ASSERT(!this->items.empty());
      bool wasFull = this->full();
      this->items.erase(this->items.begin());
      if (!this->items.empty()) (*this->items.front())();
      return wasFull;
    }

    /**
     * Call operator.
     * Called by the HTTP handler to send a response.
     * @param msg The message to send as a response.
     * TODO: same as `handle_request()` - also why does this have a struct inside it, why does it look similar to the work struct and why is it needed here?
     */
    template<bool isRequest, class Body, class Fields> void operator()(
      http::message<isRequest, Body, Fields>&& msg
    ) {
      // This holds a work item
      struct work_impl : work {
        HTTPSession& session;
        http::message<isRequest, Body, Fields> msg; // This msg is internal
        work_impl(HTTPSession& session, http::message<isRequest, Body, Fields>&& msg)
          : session(session), msg(std::move(msg)) {}
        void operator()() override {
          http::async_write(
            session.stream_, msg, beast::bind_front_handler(
              &HTTPSession::on_write, session.shared_from_this(), msg.need_eof()
            )
          );
        }
      };

      // Allocate and store the work, and if there was no previous work, start this one
      this->items.push_back(boost::make_unique<work_impl>(session, std::move(msg))); // This msg is from the header
      if (this->items.size() == 1) (*this->items.front())();
    }
};

/// Helper class that handles an HTTP connection session.
class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
  private:
    /// TCP/IP stream socket.
    beast::tcp_stream stream;

    /// Internal buffer to read and write from.
    beast::flat_buffer buf;

    /// Pointer to the root directory of the endpoint.
    std::shared_ptr<const std::string> docroot;

    /// Queue object that the session is responsible for.
    HTTPQueue queue;

    /**
     * HTTP/1 parser for producing a request message.
     * The parser is stored in an optional container so we can construct it
     * from scratch at the beginning of each new message.
     */
    boost::optional<http::request_parser<http::string_body>> parser;

    /// Reference to the blockchain.
    Blockchain& blockchain;

    /// Read whatever is on the internal buffer.
    void do_read();

    /**
     * Callback to handle what is read from the internal buffer.
     * Also tries to pipeline another request if the queue isn't full.
     * @param ec The error code to parse.
     * @param bytes The number of read bytes.
     */
    void on_read(beast::error_code ec, std::size_t bytes);

    /**
     * Callback to handle what is written to the internal buffer.
     * Also automatically reads another request.
     * @param ec The error code to parse.
     * @param bytes The number of written bytes.
     */
    void on_write(bool close, beast::error_code ec, std::size_t bytes);

    /// Send a TCP shutdown and close the connection.
    void do_close();

  public:
    /**
     * Constructor.
     * @param sock The socket to take ownership or.
     * @param docroot Pointer to the root directory of the endpoint.
     * @param blockchain Reference to the blockchain.
     */
    HTTPSession(
      tcp::socket&& sock,
      std::shared_ptr<const std::string>& docroot,
      Blockchain& blockchain
    ) : stream(std::move(sock)), docroot(docroot), queue(*this), blockchain(blockchain)
    {}

    /// Start the HTTP session.
    void start();
};

/// Helper class that accepts incoming connections and dispatches sessions.
class HTTPListener : public std::enable_shared_from_this<HTTPListener> {
  private:
    /// Provides core I/O functionality.
    net::io_context& ioc;

    /// Accepts incoming connections.
    tcp::acceptor acceptor;

    /// Pointer to the root directory of the endpoint.
    std::shared_ptr<const std::string> docroot;

    /// Reference to the blockchain.
    Blockchain& blockchain;

    /**
     * Accept an incoming connection from the endpoint.
     * The new connection gets its own strand.
     */
    void do_accept();

    /**
     * Callback to create a new HTTP session from the accepted incoming connection.
     * Also automatically listens to another session when finished dispatching.
     * @param ec The error code to parse.
     * @param sock The socket to use for creating the HTTP session.
     */
    void on_accept(beast::error_code ec, tcp::socket sock);

  public:
    /**
     * Constructor.
     * @param ioc Reference to the core I/O functionality object.
     * @param ep The endpoint (host and port) to listen to.
     * @param Pointer to the root directory of the endpoint.
     * @param blockchain Reference to the blockchain.
     */
    HTTPListener(
      net::io_context& ioc, tcp::endpoint ep,
      std::shared_ptr<const std::string>& docroot,
      Blockchain& blockchain
    );

    /// Start accepting incoming connections.
    void start();
};

/// Abstraction of an HTTP server.
class HTTPServer {
  private:
    /// Reference to the blockchain.
    Blockchain& blockchain;

    /**
     * Provides core I/O functionality.
     * {x} is the maximum number of threads the object can use.
     */
    net::io_context ioc{4};

    /// Pointer to the listener.
    std::shared_ptr<HTTPListener> listener;

    /// Indicates if the server is currently stopped.
    bool stopped = false;

    /// The port where the server is running.
    const unsigned short port;
  public:
    /**
     * Constructor.
     * @param blockchain Reference to the blockchain.
     * @param port The port that the server will run at.
     */
    HTTPServer(Blockchain& blockchain, const unsigned short port)
      : blockchain(blockchain), port(port) {}

    /// Start the server.
    void start();

    /// Stop the server.
    void stop() { this->ioc.stop(); }

    /**
     * Check if the server is currently active and running.
     * @return `true` if the server is running, `false` otherwise.
     */
    bool running() { return !this->stopped; }
};

#endif  // HTTPSERVER_H
