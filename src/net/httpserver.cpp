#include "httpserver.h"

template<class Body, class Allocator, class Send> void handle_request(
  beast::string_view doc_root, http::request<Body, http::basic_fields<Allocator>>&& req,
  Send&& send, Subnet &subnet
) {
  // Returns a bad request response
  auto const bad_request = [&req](beast::string_view why){
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
  };

  // Returns a not found response
  auto const not_found = [&req](beast::string_view target){
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
  if (req.method() != http::verb::post && req.method() != http::verb::options) {
    return send(bad_request("Unknown HTTP-method"));
  }

  // Request path must be absolute and not contain ".."
  if (
    req.target().empty() || req.target()[0] != '/' ||
    req.target().find("..") != beast::string_view::npos
  ) {
    return send(bad_request("Illegal request-target"));
  }

  // Respond to OPTIONS, Metamask requests it
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

  std::string request = req.body();

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

void fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

/**
 * We need to be executing within a strand to perform async operations
 * on the I/O objects in this session. Although not strictly necessary
 * for single-threaded contexts, this example code is written to be
 * thread-safe by default.
 */
void http_session::run() {
  net::dispatch(stream_.get_executor(), beast::bind_front_handler(
    &http_session::do_read,this->shared_from_this()
  ));
}

void http_session::do_read() {
  parser_.emplace();  // Construct a new parser for each message
  parser_->body_limit(10000); // Apply a reasonable limit to the body size in bytes to prevent abuse
  stream_.expires_after(std::chrono::seconds(30)); // Set a reasonable timeout
  // Read a request using the parser-oriented interface
  http::async_read(stream_, buffer_, *parser_, beast::bind_front_handler(
    &http_session::on_read,shared_from_this()
  ));
}

void http_session::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec == http::error::end_of_stream) return do_close(); // This means the other side closed the connection
  if (ec) return fail(ec, "read");
  handle_request(*doc_root_, parser_->release(), queue_, this->subnet); // Send the response
  if(! queue_.is_full()) do_read(); // If queue still has free space, try to pipeline another request
}

void http_session::on_write(bool close, beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) return fail(ec, "write");
  // This means we should close the connection, usually because
  // the response indicated the "Connection: close" semantic.
  if (close) return do_close();
  if (queue_.on_write()) do_read(); // Inform the queue that a write was completed and read another request
}

void http_session::do_close() {
  // Send a TCP shutdown
  beast::error_code ec;
  stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
  // At this point the connection is closed gracefully
}

listener::listener(
  net::io_context& ioc, tcp::endpoint endpoint,
  std::shared_ptr<std::string const> const& doc_root, Subnet &subnet
) : ioc_(ioc),acceptor_(net::make_strand(ioc)),doc_root_(doc_root), subnet(subnet) {
  beast::error_code ec;
  acceptor_.open(endpoint.protocol(), ec);  // Open the acceptor
  if (ec) { fail(ec, "open"); return; }
  acceptor_.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
  if (ec) { fail(ec, "set_option"); return; }
  acceptor_.bind(endpoint, ec); // Bind to the server address
  if (ec) { fail(ec, "bind"); return; }
  acceptor_.listen(net::socket_base::max_listen_connections, ec); // Start listening for connections
  if (ec) { fail(ec, "listen"); return; }
}

/**
 * We need to be executing within a strand to perform async operations
 * on the I/O objects in this session. Although not strictly necessary
 * for single-threaded contexts, this example code is written to be
 * thread-safe by default.
 */
void listener::run() {
  net::dispatch(acceptor_.get_executor(), beast::bind_front_handler(
    &listener::do_accept,this->shared_from_this()
  ));
}

// The new connection gets its own strand
void listener::do_accept() {
  acceptor_.async_accept(net::make_strand(ioc_), beast::bind_front_handler(
    &listener::on_accept,shared_from_this()
  ));
}

void listener::on_accept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    fail(ec, "accept");
  } else {
    std::make_shared<http_session>(std::move(socket),doc_root_, subnet)->run(); // Create the http session and run it
  }
  do_accept(); // Accept another connection
}


void HTTPServer::run() {
  // Create and launch a listening port
  auto const address = net::ip::make_address("0.0.0.0");
  // Get a random number between 25000 and 30000
  std::random_device rd; // obtain a random number from hardware
  std::mt19937 gen(rd()); // seed the generator
  std::uniform_int_distribution<> distr(25000, 30000);
  unsigned short port = distr(gen);
  auto const doc_root = std::make_shared<std::string>(".");
  this->_listener = std::make_shared<listener>(
    ioc, tcp::endpoint{address, port}, doc_root, this->subnet
  );
  this->_listener->run();

  // Run the I/O service on the requested number of threads (4)
  std::vector<std::thread> v;
  v.reserve(4 - 1);
  for (auto i = 4 - 1; i > 0; --i) v.emplace_back([&]{ this->ioc.run(); });
  Utils::LogPrint(Log::httpServer, __func__, std::string("HTTP Server Started at port: ") + std::to_string(port));
  ioc.run();

  // If we get here, it means we got a SIGINT or SIGTERM. Block until all the threads exit
  for (auto& t : v) t.join();
  this->stopped = true;
  Utils::LogPrint(Log::httpServer, __func__, "HTTP Server Stopped");
  return;
}

