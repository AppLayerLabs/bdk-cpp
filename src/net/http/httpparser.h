#ifndef HTTPBASE_H
#define HTTPBASE_H

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

#include "../utils/utils.h"
#include "jsonrpc/methods.h"
#include "jsonrpc/encoding.h"
#include "jsonrpc/decoding.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Forward declaration!
// httparser never access any of these members only passes them around
// It is preferable to use forward declarations
class State;
class Storage;
namespace P2P {
  class ManagerNormal;
}

/**
 * Parse a std::string JSON-RPC request body into a std::string JSON-RPC response body.
 * Handles all requests and errors.
 * @param body The request body
 * @param state Pointer to the state
 * @param storage Pointer to the storaeg
 * @param p2p Pointer to the p2p manager
 * @return The response body
 */
std::string parseJsonRpcRequest(
  const std::string& body,
  const std::unique_ptr<State>& state,
  const std::unique_ptr<Storage>& storage,
  const std::unique_ptr<P2P::ManagerNormal>& p2p
);

/**
 * Produce an HTTP response for the given request.
 * The type of the response object depends on the contents of the request,
 * so the interface requires the caller to pass a generic lambda to receive the response.
 * @param docroot The root directory of the endpoint.
 * @param req The request to handle.
 * @param send TODO: we're missing details on this, Allocator, Body, the function itself and where it's used
 * @param unique_ptr reference to the state.
 */
template<class Body, class Allocator, class Send> void handle_request(
  beast::string_view docroot,
  http::request<Body, http::basic_fields<Allocator>>&& req,
  Send&& send, const std::unique_ptr<State>& state, const std::unique_ptr<Storage>& storage,
  const std::unique_ptr<P2P::ManagerNormal>& p2p
) {
  // Returns a bad request response
  const auto bad_request = [&req](beast::string_view why){
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
  };

  // Returns a not found response
  const auto not_found = [&req](beast::string_view target){
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(target) + "' was not found.";
    res.prepare_payload();
    return res;
  };

  // Returns a server error response
  const auto server_error = [&req](beast::string_view what) {
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

  std::string answer = parseJsonRpcRequest(request, state, storage, p2p);
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

#endif  // HTTPBASE_H
