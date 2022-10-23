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

#include "../core/subnet.h"
#include "../core/utils.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class Subnet; // Forward declaration.

/**
 * Adapted from:
 *
 * Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
 * Copyright (c) 2022-2023 Sparq Network developers.
 *
 * Part of httpserver.cpp and httpserver.h is:
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * Official repository: https://github.com/boostorg/beast
 */

template<class Body, class Allocator, class Send> void handle_request(
  beast::string_view doc_root, http::request<Body, http::basic_fields<Allocator>>&& req,
  Send&& send, Subnet &subnet
);

void fail(beast::error_code ec, char const* what);

class http_session : public std::enable_shared_from_this<http_session> {
  private:
    class queue {
      private:
        enum { limit = 8 };
        struct work {
          virtual ~work() = default;
          virtual void operator()() = 0;
        };
        http_session& self_;
        std::vector<std::unique_ptr<work>> items_;

      public:
        explicit queue(http_session& self) : self_(self) {
          static_assert(limit > 0, "queue limit must be positive");
          items_.reserve(limit);
        }

        bool is_full() const { return items_.size() >= limit; }

        bool on_write() {
          BOOST_ASSERT(!items_.empty());
          auto const was_full = is_full();
          items_.erase(items_.begin());
          if (!items_.empty()) (*items_.front())();
          return was_full;
        }

        template<bool isRequest, class Body, class Fields> void operator()(
          http::message<isRequest, Body, Fields>&& msg
        ) {
          // This holds a work item
          struct work_impl : work {
            http_session& self_;
            http::message<isRequest, Body, Fields> msg_;
            work_impl(http_session& self,http::message<isRequest, Body, Fields>&& msg)
              : self_(self), msg_(std::move(msg)) {}
            void operator()() override {
              http::async_write(
                self_.stream_, msg_, beast::bind_front_handler(
                  &http_session::on_write, self_.shared_from_this(), msg_.need_eof()
                )
              );
            }
          };

          // Allocate and store the work, and if there was no previous work, start this one
          items_.push_back(boost::make_unique<work_impl>(self_, std::move(msg)));
          if (items_.size() == 1) (*items_.front())();
        }
    };

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    queue queue_;
    boost::optional<http::request_parser<http::string_body>> parser_;
    Subnet &subnet;
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred);
    void do_close();

  public:
    http_session(tcp::socket&& socket, std::shared_ptr<std::string const> const& doc_root, Subnet& subnet)
      : stream_(std::move(socket)) , doc_root_(doc_root) , queue_(*this), subnet(subnet)
    {}
    void run();
};

class listener : public std::enable_shared_from_this<listener> {
  private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<std::string const> doc_root_;
    Subnet &subnet;
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);

  public:
    listener(
      net::io_context& ioc, tcp::endpoint endpoint,
      std::shared_ptr<std::string const> const& doc_root, Subnet& subnet
    );
    void run();
};

class HTTPServer {
  private:
    Subnet& subnet;
    net::io_context ioc{4};
    std::shared_ptr<listener> _listener;
    bool stopped = false;
    const unsigned short port;

  public:
    HTTPServer(Subnet& subnet, const unsigned short port) : subnet(subnet), port(port) {}
    void run();
    void stop() { ioc.stop(); }
    bool isRunning() { return !this->stopped; }
};

#endif // HTTPSERVER_H
