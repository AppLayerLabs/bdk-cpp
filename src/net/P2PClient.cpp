#include "P2PClient.h"
#include "P2PManager.h"

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

  std::string hostStr = this->host + ':' + std::to_string(ep.port());
  this->handshake(hostStr);
}

void P2PClient::handshake(const std::string& host) {
  ws_.async_handshake(host, "/",
    beast::bind_front_handler(&P2PClient::on_handshake, shared_from_this()));
}

void P2PClient::on_handshake(beast::error_code ec) {
  if(ec)
    return p2p_fail_client(__func__, ec, "handshake");

  Utils::LogPrint(Log::P2PClient, __func__, std::string("P2PClient: connected to: ") + ws_.next_layer().socket().remote_endpoint().address().to_string() +
    ":" + std::to_string(ws_.next_layer().socket().remote_endpoint().port()) + " binary: " + boost::lexical_cast<std::string>(ws_.binary()));

  this->write(P2PRequestEncoder::info(this->manager_->chainHead, this->manager_->connectionCount()));
  this->read();
}

void P2PClient::read() {
  ws_.async_read(
    receiveBuffer,
    beast::bind_front_handler(&P2PClient::on_read, shared_from_this()));
}

void P2PClient::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec == websocket::error::closed) { p2p_fail_server(__func__, ec, "read"); return; } // This indicates the session was closed
  if (ec.value() == 125) { p2p_fail_server(__func__, ec, "read"); return; } // Operation cancelled
  if (ec.value() == 995) { p2p_fail_server(__func__, ec, "read"); return; } // Interrupted by host
  if (ec) { p2p_fail_client(__func__, ec, "read"); }

  try {
    if (receiveBuffer.size() != 0) {
      Utils::logToFile(std::string("P2PClient: received: ") + Utils::bytesToHex(boost::beast::buffers_to_string(receiveBuffer.data())) + " size: " + std::to_string(receiveBuffer.size()));
      P2PMessage message(boost::beast::buffers_to_string(receiveBuffer.data()));
      std::thread t(&P2PManager::parseServerAnswer, this->manager_, message, shared_from_this());
      t.detach();
      receiveBuffer.consume(receiveBuffer.size());
    }
  } catch (std::exception &e) {
    Utils::logToFile("P2P Server crash on_read");
  }

  this->read();
}

void P2PClient::write(const P2PMessage data) {
  auto buffer = net::buffer(data.raw());
  Utils::logToFile(std::string("P2PClient writing: ") + Utils::bytesToHex(boost::beast::buffers_to_string(buffer)));

  ws_.async_write(
    buffer,
    beast::bind_front_handler(&P2PClient::on_write, shared_from_this()));
}

void P2PClient::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec)
    p2p_fail_client(__func__, ec, "write");
}