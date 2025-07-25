/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "httpclient.h"

#include "../utils/dynamicexception.h"

HTTPSyncClient::HTTPSyncClient(const std::string& host, const std::string& port)
  : host(host), port(port), resolver(ioc), stream(ioc) { this->connect(); }

// TODO: either close() shouldn't be throwing, or the dtor shouldn't be calling it
HTTPSyncClient::~HTTPSyncClient() { this->close(); }

void HTTPSyncClient::connect() {
  boost::system::error_code ec;
  auto const results = resolver.resolve(host, port, ec);
  if (ec) throw DynamicException("Error while resolving the HTTP Client: " + ec.message());
  stream.connect(results, ec);
  if (ec) throw DynamicException("Error while connecting the HTTP Client: " + ec.message());
}

void HTTPSyncClient::close() {
  boost::system::error_code ec;
  stream.socket().shutdown(tcp::socket::shutdown_both, ec);
  if (ec) throw DynamicException("Error while closing the HTTP Client: " + ec.message());
}

std::string HTTPSyncClient::makeHTTPRequest(const std::string& reqBody) {
  namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

  boost::system::error_code ec;
  // Set up an HTTP POST/GET request message
  http::request<http::string_body> req{ http::verb::post , "/", 11};

  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(http::field::accept, "application/json");
  req.set(http::field::content_type, "application/json");
  req.body() = reqBody;
  req.prepare_payload();

  // Send the HTTP request to the remote host
  http::write(stream, req, ec);
  if (ec) throw DynamicException("Error while writing the HTTP request: " + ec.message());

  boost::beast::flat_buffer buffer;
  // Declare a container to hold the response
  http::response_parser<http::dynamic_body> parser;
  parser.body_limit((std::numeric_limits<std::uint64_t>::max)());
  // Receive the HTTP response
  http::read(stream, buffer, parser, ec);
  if (ec) throw DynamicException("Error while reading the HTTP response: " + ec.message() + " " + std::to_string(ec.value()));

  // Write only the body answer to output
  const auto& body = parser.get().body();
  return {
    boost::asio::buffers_begin(body.data()),
    boost::asio::buffers_end(body.data())
  };
}

