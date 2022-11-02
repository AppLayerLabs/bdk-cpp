#include "p2p.h"

void p2p_fail(beast::error_code ec, char const* what) {
  Utils::logToFile(std::string("P2P FAIL:") + ec.message());
}

void P2PClient::resolve(std::string host, std::string port) {
  Utils::logToFile("P2PClient: resolving host");
  this->resolver_.async_resolve(host, port, beast::bind_front_handler(
    &P2PClient::on_resolve, shared_from_this()
  ));
  this->ioc_threads.emplace_back([this]{ ioc.run(); }); // Always comes *after* async
}

void P2PClient::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
  if (ec) return p2p_fail(ec, "resolve");
  Utils::logToFile("P2PClient: resolved host");
  beast::get_lowest_layer(this->ws_).expires_never();
  this->connect(results);
}

void P2PClient::connect(tcp::resolver::results_type results) {
  Utils::logToFile("P2PClient: connecting to host");
  beast::get_lowest_layer(this->ws_).async_connect(results, beast::bind_front_handler(
    &P2PClient::on_connect, shared_from_this()
  ));
}

void P2PClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
  if (ec) return p2p_fail(ec, "connect");
  Utils::logToFile("P2PClient: connected to host");

  // Turn off timeout on tcp_stream, websocket has its own timeout system
  beast::get_lowest_layer(this->ws_).expires_never();

  // Set suggested timeout settings for the websocket
  this->ws_.set_option(
    websocket::stream_base::timeout::suggested(beast::role_type::client)
  );

  // Set a decorator to change the User-Agent, then perform the handshake
  this->ws_.set_option(websocket::stream_base::decorator(
    [](websocket::request_type& req) {
      req.set(http::field::user_agent,
        std::string(BOOST_BEAST_VERSION_STRING) + " sparq-wscl"
      );
    }
  ));
  this->handshake(host_, std::to_string(ep.port()));
}

void P2PClient::handshake(std::string host, std::string port) {
  Utils::logToFile("P2PClient: giving handshake");
  this->ws_.async_handshake(host + ":" + port, "/", beast::bind_front_handler(
    &P2PClient::on_handshake, shared_from_this()
  ));
}

void P2PClient::on_handshake(beast::error_code ec) {
  if (ec) return p2p_fail(ec, "handshake");
  Utils::logToFile("P2PClient: handshake given");
  this->write("info");
  this->read();
}

void P2PClient::write(std::string msg) {
  Utils::logToFile("P2PClient: writing message: " + msg);
  this->ws_.async_write(net::buffer(msg), beast::bind_front_handler(
    &P2PClient::on_write, shared_from_this()
  ));
}

void P2PClient::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "write");
  Utils::logToFile("P2PClient: message written");
}

void P2PClient::read() {
  Utils::logToFile("P2PClient: reading message");
  this->ws_.async_read(buffer_, beast::bind_front_handler(
    &P2PClient::on_read, shared_from_this()
  ));
}

void P2PClient::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "read");
  std::stringstream ss;
  ss << beast::make_printable(buffer_.data());
  Utils::logToFile("P2PClient: message read: " + ss.str());
  std::string resp = this->parse(ss.str());
  if (resp.empty()) resp = "Unknown command: " + ss.str();
  this->write(resp);
  buffer_.consume(buffer_.size());
  this->read();
}

std::string P2PClient::parse(std::string cmd) {
  auto it = std::find(std::begin(this->cmds), std::end(this->cmds), cmd);
  if (it == std::end(this->cmds)) return "";

  std::string ret;
  if (cmd == "info") ret = this->info_.dump();
  return ret;
}

void P2PServer::accept() {
  Utils::logToFile("P2PServer: accepting connection");
  try {
    acceptor_.async_accept(net::make_strand(ioc), beast::bind_front_handler(
      &P2PServer::on_accept, shared_from_this()
    ));
  } catch (std::exception &e) {
    Utils::logToFile("P2P ERROR");
    Utils::logToFile(std::string("Error: ") + e.what());
  }
  ioc_threads.emplace_back([this]{ ioc.run(); }); // Always comes *after* async
}

void P2PServer::on_accept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    p2p_fail(ec, "accept");
  } else {
    Utils::logToFile("P2PServer: connection accepted");
    this->ws_ = std::make_shared<websocket::stream<beast::tcp_stream>>(std::move(socket));
    net::dispatch(this->ws_->get_executor(), beast::bind_front_handler(
      &P2PServer::on_start, shared_from_this()
    ));
  }
  acceptor_.async_accept(net::make_strand(ioc), beast::bind_front_handler(
    &P2PServer::on_accept, shared_from_this()
  ));
}

void P2PServer::on_start() {
  Utils::logToFile("P2PServer: started");
  // Set suggested timeout settings for the websocket
  this->ws_->set_option(
    websocket::stream_base::timeout::suggested(beast::role_type::server)
  );

  // Set a decorator to change the User-Agent, then accept the handshake
  this->ws_->set_option(websocket::stream_base::decorator(
    [](websocket::response_type& res) {
      res.set(http::field::server,
        std::string(BOOST_BEAST_VERSION_STRING) + " sparq-wssv"
      );
    }
  ));

  Utils::logToFile("P2PServer: giving handshake");
  this->ws_->async_accept(beast::bind_front_handler(
    &P2PServer::on_handshake, shared_from_this()
  ));
}

void P2PServer::on_handshake(beast::error_code ec) {
  if (ec) return p2p_fail(ec, "server_accept");
  Utils::logToFile("P2PServer: handshake given");
  this->write("info");
  this->read();
}

void P2PServer::read() {
  Utils::logToFile("P2PServer: reading message");
  this->ws_->async_read(buffer_, beast::bind_front_handler(
    &P2PServer::on_read, shared_from_this())
  );
}

void P2PServer::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec == websocket::error::closed) return;
  if (ec) p2p_fail(ec, "read");
  this->ws_->text(this->ws_->got_text());
  std::stringstream ss;
  ss << beast::make_printable(buffer_.data());
  Utils::logToFile("P2PServer: message read: " + ss.str());
  std::string resp = this->parse(ss.str());
  if (resp.empty()) resp = "Unknown command: " + ss.str();
  this->write(resp);
  buffer_.consume(buffer_.size());
  this->read();
}

void P2PServer::write(std::string msg) {
  Utils::logToFile("P2PServer: writing message: " + msg);
  this->ws_->async_write(boost::asio::buffer(msg), beast::bind_front_handler(
    &P2PServer::on_write, shared_from_this())
  );
}

void P2PServer::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "write");
  Utils::logToFile("P2PServer: message written");
  buffer_.consume(buffer_.size());
}

std::string P2PServer::parse(std::string cmd) {
  auto it = std::find(std::begin(this->cmds), std::end(this->cmds), cmd);
  if (it == std::end(this->cmds)) return "";

  std::string ret;
  if (cmd == "info") ret = this->info_.dump();
  return ret;
}

