#include "P2PClient.h"

void P2PClient::run() {
  Utils::logToFile(std::string("Trying to resolve: ") + this->host + ":" + std::to_string(this->port));
  this->resolve();
}

void P2PClient::stop() {
  
}

void P2PClient::resolve() {
  resolver_.async_resolve(
    host,
    std::to_string(port),
    beast::bind_front_handler(
    &P2PClient::on_resolve,
    shared_from_this()));
}

void P2PClient::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
  if (ec) {
    p2p_fail_client(__func__, ec, "resolve");
  }

  this->connect(results);
}

void P2PClient::connect(tcp::resolver::results_type& results) {
  // Set the timeout for the client.
  beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

  // Make connection
  beast::get_lowest_layer(ws_).async_connect(
    results,
    beast::bind_front_handler(
    &P2PClient::on_connect,
    shared_from_this()));
}

void P2PClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
  if(ec)
    return p2p_fail_client(__func__, ec, "connect");

  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  beast::get_lowest_layer(ws_).expires_never();

  // Set suggested timeout settings for the websocket
  ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

  // Set a decorator to change the User-Agent of the handshake
  ws_.set_option(websocket::stream_base::decorator(
  [](websocket::request_type& req)
  {
    req.set(http::field::user_agent,
    std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
  }));

  host += ':' + std::to_string(ep.port());
  this->handshake(host);
}

void P2PClient::handshake(const std::string& host) {
  ws_.async_handshake(host, "/",
    beast::bind_front_handler(&P2PClient::on_handshake, shared_from_this()));
}

void P2PClient::on_handshake(beast::error_code ec) {
  if(ec)
    return p2p_fail_client(__func__, ec, "handshake");

  Utils::LogPrint(Log::P2PClient, __func__, std::string("P2PClient: connected to: ") + ws_.next_layer().socket().remote_endpoint().address().to_string() +
    ":" + std::to_string(ws_.next_layer().socket().remote_endpoint().port()));

  this->read();
}

void P2PClient::read() {
  ws_.async_read(
    receiveBuffer,
    beast::bind_front_handler(&P2PClient::on_read, shared_from_this()));
}

void P2PClient::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec)
    p2p_fail_client(__func__, ec, "read");

  this->read();
}

void P2PClient::write(const std::string& data) {
  ws_.async_write(
    net::buffer(data),
    beast::bind_front_handler(&P2PClient::on_write, shared_from_this()));
}

void P2PClient::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec)
    p2p_fail_client(__func__, ec, "write");

  this->read();
}