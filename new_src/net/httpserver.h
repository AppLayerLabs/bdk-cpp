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

#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

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

#include "../core/blockchain.h"
#include "../utils/utils.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace tcp = boost::asio::ip::tcp;

template<class Body, class Allocator, class Send> void handle_request(
  beast::string_view docroot,
  http::request<Body, http::basic_fields<Allocator>>&& req,
  Send&& send, Blockchain& blockchain
);

class HTTPQueue {
  private:
    unsigned int limit = 8;
    struct work { virtual ~work() = default; virtual void operator()() = 0; };
    HTTPSession& session;
    std::vector<std::unique_ptr<work>> items;

  public:
    HTTPQueue(HTTPSession& session) : session(session) {
      static_assert(this->limit > 0, "queue limit must be positive");
      items.reserve(this->limit);
    }

    bool full() { return this->items.size() >= this->limit; }

    bool on_write() {
      BOOST_ASSERT(!this->items.empty());
      bool wasFull = this->full();
      this->items.erase(this->items.begin());
      if (!this->items.empty()) (*this->items.front())();
      return wasFull;
    }

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

class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
  private:
    beast::tcp_stream stream;
    beast::flat_buffer buf;
    std::shared_ptr<const std::string> docroot;
    HTTPQueue queue;
    boost::optional<http::request_parser<http::string_body>> parser;
    Blockchain& blockchain;
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes);
    void on_write(bool close, beast::error_code ec, std::size_t bytes);
    void do_close();

  public:
    HTTPSession(
      tcp::socket&& sock,
      std::shared_ptr<const std::string>& docroot,
      Blockchain& blockchain
    ) : stream(std::move(sock)), docroot(docroot), queue(*this), blockchain(blockchain)
    {}
    void start();
};

class HTTPListener : public std::enable_shared_from_this<HTTPListener> {
  private:
    net::io_context& ioc;
    tcp::acceptor acceptor;
    std::shared_ptr<const std::string> docroot;
    Blockchain& blockchain;
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket sock);

  public:
    HTTPListener(
      net::io_context& ioc, tcp::endpoint ep,
      std::shared_ptr<const std::string>& docroot,
      Blockchain& blockchain
    );
    void start();
};

class HTTPServer {
  private:
    Blockchain& blockchain;
    net::io_context ioc{4};
    std::shared_ptr<HTTPListener> listener;
    bool stopped = false;
    const unsigned short port;
  public:
    HTTPServer(Blockchain& blockchain, const unsigned short port)
      : blockchain(blockchain), port(port) {}
    void start();
    void stop() { this->ioc.stop(); }
    bool running() { return !this->stopped; }

};

#endif  // HTTPSERVER_H
