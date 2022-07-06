#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sstream>
#include <fstream>
#include <chrono>
#include <csignal>

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/message.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/thread.hpp>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class Subnet;

namespace HTTPServer {
  template<class Body, class Allocator,class Send> void handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send,
    Subnet& subnet
  );

  template<class Stream> struct send_lambda {
    Stream& stream_;
    bool& close_;
    beast::error_code& ec_;

    explicit send_lambda(
      Stream& stream,
      bool& close,
      beast::error_code& ec
    ) : stream_(stream),
        close_(close),
        ec_(ec)
    {}

    template<bool isRequest, class Body, class Fields> void operator()(
      http::message<isRequest, Body, Fields>&& msg
    ) const {
      close_ = msg.need_eof(); // Determine if we should close the connection at the end

      /**
       * We need the serializer here because the serializer requires
       * a non-const file_body, and the message oriented version of
       * http::write only works with const messages.
       */
      http::serializer<isRequest, Body, Fields> sr{msg};
      http::write(stream_, sr, ec_);
    }
  };

  void fail(beast::error_code ec, char const* what);
  void do_session(
    tcp::socket& socket,
    std::shared_ptr<std::string const> const& doc_root,
    Subnet& subnet
  );
  void startServer(Subnet& subnet);

};

#endif // HTTPSERVER_H
