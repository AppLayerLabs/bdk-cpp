#include "httpbase.h"

template<class Body, class Allocator, class Send> void handle_request(
  beast::string_view docroot,
  http::request<Body, http::basic_fields<Allocator>>&& req,
  Send&& send, Blockchain& blockchain
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

  // std::string answer = blockchain.parseRPC(request);  // This does the parsing per se
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
  // res.body() = answer;
  res.keep_alive(req.keep_alive());
  res.prepare_payload();
  return send(std::move(res));
}

