#include "httpserver.h"
#include "subnet.h"

template<class Body, class Allocator, class Send> void HTTPServer::handle_request(
  beast::string_view doc_root,
  http::request<Body, http::basic_fields<Allocator>>&& req,
  Send&& send,
  Subnet &subnet
) {
  // Returns a bad request response
  auto const bad_request = [&req](beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
  };

  // Returns a not found response
  auto const not_found = [&req](beast::string_view target) {
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(target) + "' was not found.";
    res.prepare_payload();
    return res;
  };

  // Returns a server error response
  auto const server_error = [&req](beast::string_view what) {
    http::response<http::string_body> res{http::status::internal_server_error, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + std::string(what) + "'";
    res.prepare_payload();
    return res;
  };

  // Make sure we can handle the method

  // Request path must be absolute and not contain "..".
  if (
    req.target().empty() ||
    req.target()[0] != '/' ||
    req.target().find("..") != beast::string_view::npos
  ) {
    return send(bad_request("Illegal request-target"));
  }

  std::string request = req.body();

  /**
   * Respond to OPTIONS.
   * there is absolute no fkin documentation over this
   * someone plz write some :pray:
   */
  if (req.method() == http::verb::options) {
    http::response<http::empty_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::access_control_allow_origin, "*");
    res.set(http::field::access_control_allow_methods, "POST, GET");
    res.set(http::field::access_control_allow_headers, "content-type");
    res.set(http::field::accept_encoding, "deflate");
    res.set(http::field::accept_language, "en-US");
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
  }

  // Respond to POST
  std::string answer = subnet.processRPCMessage(request);
  http::response<http::string_body> res{http::status::ok, req.version()};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::access_control_allow_origin, "*");
  res.set(http::field::access_control_allow_methods, "POST, GET");
  res.set(http::field::access_control_allow_headers, "content-type");
  res.set(http::field::content_type, "application/json");
  res.set(http::field::connection, "keep-alive");
  res.set(http::field::strict_transport_security, "max-age=0");
  res.set(http::field::vary, "Origin");
  res.set(http::field::access_control_allow_credentials, "true");
  res.body() = answer;
  res.keep_alive(req.keep_alive());
  res.prepare_payload();
  return send(std::move(res));
}

void HTTPServer::do_session(
  tcp::socket& socket,
  std::shared_ptr<std::string const> const& doc_root,
  Subnet &subnet
) {
  bool close = false;
  beast::error_code ec;
  beast::flat_buffer buffer;  // This buffer is required to persist across reads.
  send_lambda<tcp::socket> lambda{socket, close, ec}; // This lambda is used to send messages.
  for (;;) {
    // Read a request
    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if (ec == http::error::end_of_stream) break;
    if (ec) return fail(ec, "read");

    // Send the response
    handle_request(*doc_root, std::move(req), lambda, subnet);
    if (ec) return fail(ec, "write");

    /**
     * This means we should close the connection, usually because
     * the response indicated the "Connection: close" semantic.
     */
    if (close) break;
  }

  /**
   * Send a TCP shutdown.
   * At this point the connection should be closed gracefully.
   */
  socket.shutdown(tcp::socket::shutdown_send, ec);
}

void HTTPServer::fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

// Acceptor pointer
std::unique_ptr<tcp::acceptor> acceptor;

void HTTPServer::startServer(Subnet &subnet) {
  try {
    // Check command line arguments.
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = 30000;
    auto const doc_root = std::make_shared<std::string>("/");
    net::io_context ioc{1}; // io_context is required for all I/O.
    acceptor = std::make_unique<tcp::acceptor>(ioc); // The acceptor receives incoming connections.
    auto ep = boost::asio::ip::tcp::endpoint(address, port);
    acceptor->open(ep.protocol());
    acceptor->bind(ep);
    acceptor->listen();
    for (;;) {
      tcp::socket socket{ioc};  // Receive the new connection
      acceptor->accept(socket);  // Block until we get a connection
      if(subnet.isShutdown()) {
        break;
      }
      // Launch the session in a thread, transferring ownership of the socket.
      std::thread{std::bind(
        &do_session, std::move(socket), doc_root, std::ref(subnet)
      )}.detach();
    }
  } catch (const std::exception& e) {
    Utils::logToFile(std::string("HTTP Error: ") + e.what());
    return;
  }
}

void HTTPServer::shutdownServer() {
  if (acceptor->is_open()) {
    Utils::logToFile("Trying to shutdown HTTP server");
    acceptor->cancel();
    acceptor->close();
    Utils::logToFile("HTTP server shutdown");
  }
  Utils::LogPrint(Log::subnet, __func__, "Print 3");
}
