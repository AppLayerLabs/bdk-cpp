#include "httpserver.h"
#include "subnet.h"

#include "httpserver.h"
#include "utils.h"

template<class Body, class Allocator, class Send>
void handle_request(beast::string_view doc_root, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, Subnet &subnet)
{
  // Returns a bad request response
  auto const bad_request =
  [&req](beast::string_view why)
  {
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
  };

  // Returns a not found response
  auto const not_found =
  [&req](beast::string_view target)
  {
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(target) + "' was not found.";
    res.prepare_payload();
    return res;
  };

  // Returns a server error response
  auto const server_error =
  [&req](beast::string_view what)
  {
    http::response<http::string_body> res{http::status::internal_server_error, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + std::string(what) + "'";
    res.prepare_payload();
    return res;
  };

  // Make sure we can handle the method
  if (req.method() != http::verb::post && req.method() != http::verb::options)
    return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
  if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Respond to OPTIONS, metamask requests it.
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

void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

void http_session::run()
{
  // We need to be executing within a strand to perform async operations
  // on the I/O objects in this session. Although not strictly necessary
  // for single-threaded contexts, this example code is written to be
  // thread-safe by default.
  net::dispatch(stream_.get_executor(),beast::bind_front_handler(&http_session::do_read,this->shared_from_this()));
}

void http_session::do_read()
{
  // Construct a new parser for each message
  parser_.emplace();
  // Apply a reasonable limit to the allowed size
  // of the body in bytes to prevent abuse.
  parser_->body_limit(10000);
  // Set the timeout.
  stream_.expires_after(std::chrono::seconds(30));
  // Read a request using the parser-oriented interface
  http::async_read(stream_,buffer_,*parser_,beast::bind_front_handler(&http_session::on_read,shared_from_this()));
}

void http_session::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
  boost::ignore_unused(bytes_transferred);
  // This means they closed the connection
  if(ec == http::error::end_of_stream)
    return do_close();

  if(ec)
    return fail(ec, "read");

  // Send the response
  handle_request(*doc_root_, parser_->release(), queue_, this->subnet);

  // If we aren't at the queue limit, try to pipeline another request
  if(! queue_.is_full())
    do_read();
}

void http_session::on_write(bool close, beast::error_code ec, std::size_t bytes_transferred)
{
  boost::ignore_unused(bytes_transferred);
  if(ec)
    return fail(ec, "write");
  if(close)
  {
    // This means we should close the connection, usually because
    // the response indicated the "Connection: close" semantic.
    return do_close();
  }
    // Inform the queue that a write completed
  if(queue_.on_write())
  {
    // Read another request
    do_read();
  }
}

void http_session::do_close()
{
  // Send a TCP shutdown
  beast::error_code ec;
  stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
  // At this point the connection is closed gracefully
}

listener::listener(net::io_context& ioc,tcp::endpoint endpoint,std::shared_ptr<std::string const> const& doc_root, Subnet &subnet) : ioc_(ioc),acceptor_(net::make_strand(ioc)),doc_root_(doc_root), subnet(subnet)
{
  beast::error_code ec;
  // Open the acceptor
  acceptor_.open(endpoint.protocol(), ec);
  if(ec)
  {
    fail(ec, "open");
    return;
  }

  // Allow address reuse
  acceptor_.set_option(net::socket_base::reuse_address(true), ec);
  if(ec)
  {
    fail(ec, "set_option");
    return;
  }
  
  // Bind to the server address
  acceptor_.bind(endpoint, ec);
  if(ec)
  {
    fail(ec, "bind");
    return;
  }

  // Start listening for connections
  acceptor_.listen(net::socket_base::max_listen_connections, ec);
  
  if(ec)
  {
    fail(ec, "listen");
    return;
  }
}

void listener::run()
{
  // We need to be executing within a strand to perform async operations
  // on the I/O objects in this session. Although not strictly necessary
  // for single-threaded contexts, this example code is written to be
  // thread-safe by default.
  net::dispatch(acceptor_.get_executor(),beast::bind_front_handler(&listener::do_accept,this->shared_from_this()));
}

void listener::do_accept()
{
  // The new connection gets its own strand
  acceptor_.async_accept(net::make_strand(ioc_),beast::bind_front_handler(&listener::on_accept,shared_from_this()));
}

void listener::on_accept(beast::error_code ec, tcp::socket socket)
{
  if(ec)
  {
    fail(ec, "accept");
  } else {
    // Create the http session and run it
    std::make_shared<http_session>(std::move(socket),doc_root_, subnet)->run();
  }
  // Accept another connection
  do_accept();
}


void HTTPServer::run() {
  // Create and launch a listening port
  auto const address = net::ip::make_address("0.0.0.0");
  unsigned short port = 30000;
  auto const doc_root = std::make_shared<std::string>(".");
  this->_listener = std::make_shared<listener>(ioc,
        tcp::endpoint{address, port},
        doc_root,
        this->subnet);

  this->_listener->run();

  // Run the I/O service on the requested number of threads (4)

  std::vector<std::thread> v;
  v.reserve(4 - 1);
  for(auto i = 4 - 1; i > 0; --i)
      v.emplace_back(
      [&]
      {
          this->ioc.run();
      });

  Utils::LogPrint(Log::httpServer, __func__, "HTTP Server Started");
  ioc.run();

  // (If we get here, it means we got a SIGINT or SIGTERM)
  // Block until all the threads exit
  for(auto& t : v)
    t.join();

  this->stopped = true;

  Utils::LogPrint(Log::httpServer, __func__, "HTTP Server Stopped");
  return;
}

void HTTPServer::stop() {
  ioc.stop();
}

bool HTTPServer::isRunning() {
  return !this->stopped;
}