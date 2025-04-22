#include "httpclient.h"
#include "manager.h"

namespace BTVServer {
  HTTPSemiSyncClient::HTTPSemiSyncClient(const std::string& host, const std::string& port, net::io_context& ioc_, Manager& manager)
    : host(host), port(port), resolver(ioc_), stream(ioc_), strand_(ioc_.get_executor()), manager(manager) {}

  // TODO: either close() shouldn't be throwing, or the dtor shouldn't be calling it
  HTTPSemiSyncClient::~HTTPSemiSyncClient() { if (stream.socket().is_open()) this->close(); }

  void HTTPSemiSyncClient::connect() {
    boost::system::error_code ec;
    auto const results = resolver.resolve(host, port, ec);
    if (ec) throw DynamicException("Error while resolving the HTTP Client: " + ec.message());
    stream.connect(results, ec);
    if (ec) throw DynamicException("Error while connecting the HTTP Client: " + ec.message());
    Printer::safePrint("HTTPSemiSyncClient connected to " + host + ":" + port);
  }

  void HTTPSemiSyncClient::close() {
    boost::system::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec) throw DynamicException("Error while closing the HTTP Client: " + ec.message());
  }

  std::string HTTPSemiSyncClient::makeHTTPRequestInternal(const std::shared_ptr<std::string> reqBody) {
    namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

    boost::system::error_code ec;
    // Set up an HTTP POST/GET request message
    http::request<http::string_body> req{ http::verb::post , "/", 11};

    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::accept, "application/json");
    req.set(http::field::content_type, "application/json");
    req.body() = *reqBody;
    req.prepare_payload();

    // Send the HTTP request to the remote host
    http::write(stream, req, ec);
    if (ec) throw DynamicException("Error while writing the HTTP request: " + ec.message());

    boost::beast::flat_buffer buffer;
    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res, ec);
    if (ec) throw DynamicException("Error while reading the HTTP response: " + ec.message() + " " + std::to_string(ec.value()));

    // Write only the body answer to output
    return {
      boost::asio::buffers_begin(res.body().data()),
      boost::asio::buffers_end(res.body().data())
    };
  }

  void HTTPSemiSyncClient::makeHTTPRequest(std::string &&reqBody) {
    // DO NOT forget to post to the strand!!!
    // each write/read should be SEQUENTIAL
    boost::asio::post(strand_, [this, reqBodyPtr = std::make_shared<std::string>(std::move(reqBody))]() {
      auto response = this->makeHTTPRequestInternal(reqBodyPtr);
      this->manager.handleHTTPResponse(response);
    });
  }
}