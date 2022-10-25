#include "p2p.h"

void p2p_fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

void P2PClient::resolve(std::string host, std::string port) {
  //std::cout << "Client: resolving host" << std::endl;
  this->resolver_.async_resolve(host, port, beast::bind_front_handler(
    &P2PClient::on_resolve, shared_from_this()
  ));
  this->ioc_threads.emplace_back([this]{ ioc.run(); }); // Always comes *after* async
}

void P2PClient::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
  //std::cout << "Client: resolved host" << std::endl;
  if (ec) return p2p_fail(ec, "resolve");
  beast::get_lowest_layer(this->ws_).expires_never();
  this->connect(results);
}

void P2PClient::connect(tcp::resolver::results_type results) {
  //std::cout << "Client: connecting to host" << std::endl;
  beast::get_lowest_layer(this->ws_).async_connect(results, beast::bind_front_handler(
    &P2PClient::on_connect, shared_from_this()
  ));
}

void P2PClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
  //std::cout << "Client: connected to host" << std::endl;
  if (ec) return p2p_fail(ec, "connect");

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
  //std::cout << "Client: giving handshake" << std::endl;
  this->ws_.async_handshake(host + ":" + port, "/", beast::bind_front_handler(
    &P2PClient::on_handshake, shared_from_this()
  ));
}

void P2PClient::on_handshake(beast::error_code ec) {
  //std::cout << "Client: handshake given" << std::endl;
  if (ec) return p2p_fail(ec, "handshake");
  this->write("info");
  this->read();
}

void P2PClient::write(std::string msg) {
  //std::cout << "Client: writing message" << std::endl;
  this->ws_.async_write(net::buffer(msg), beast::bind_front_handler(
    &P2PClient::on_write, shared_from_this()
  ));
}

void P2PClient::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  //std::cout << "Client: message written" << std::endl;
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "write");
}

void P2PClient::read() {
  //std::cout << "Client: reading message" << std::endl;
  this->ws_.async_read(buffer_, beast::bind_front_handler(
    &P2PClient::on_read, shared_from_this()
  ));
}

void P2PClient::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  //std::cout << "Client: message read" << std::endl;
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "read");
  std::stringstream ss;
  ss << beast::make_printable(buffer_.data());
  std::cout << this->parse(ss.str()) << std::endl;
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
  //std::cout << "Server: accepting connection" << std::endl;
  acceptor_.async_accept(net::make_strand(ioc), beast::bind_front_handler(
    &P2PServer::on_accept, shared_from_this()
  ));
  ioc_threads.emplace_back([this]{ ioc.run(); }); // Always comes *after* async
}

void P2PServer::on_accept(beast::error_code ec, tcp::socket socket) {
  //std::cout << "Server: connection accepted" << std::endl;
  if (ec) {
    p2p_fail(ec, "accept");
  } else {
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
  //std::cout << "Server: started" << std::endl;
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

  //std::cout << "Server: giving handshake" << std::endl;
  this->ws_->async_accept(beast::bind_front_handler(
    &P2PServer::on_handshake, shared_from_this()
  ));
}

void P2PServer::on_handshake(beast::error_code ec) {
  //std::cout << "Server: handshake given" << std::endl;
  if (ec) return p2p_fail(ec, "server_accept");
  this->write("info");
  this->read();
}

void P2PServer::read() {
  //std::cout << "Server: reading message" << std::endl;
  this->ws_->async_read(buffer_, beast::bind_front_handler(
    &P2PServer::on_read, shared_from_this())
  );
}

void P2PServer::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  //std::cout << "Server: message read" << std::endl;
  boost::ignore_unused(bytes_transferred);
  if (ec == websocket::error::closed) return;
  if (ec) p2p_fail(ec, "read");
  this->ws_->text(this->ws_->got_text());
  std::stringstream ss;
  ss << beast::make_printable(buffer_.data());
  std::cout << this->parse(ss.str()) << std::endl;
  buffer_.consume(buffer_.size());
  this->read();
}

void P2PServer::write(std::string msg) {
  //std::cout << "Server: writing message" << std::endl;
  this->ws_->async_write(boost::asio::buffer(msg), beast::bind_front_handler(
    &P2PServer::on_write, shared_from_this())
  );
}

void P2PServer::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  //std::cout << "Server: message written" << std::endl;
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "write");
  buffer_.consume(buffer_.size());
}

std::string P2PServer::parse(std::string cmd) {
  auto it = std::find(std::begin(this->cmds), std::end(this->cmds), cmd);
  if (it == std::end(this->cmds)) return "";

  std::string ret;
  if (cmd == "info") ret = this->info_.dump();
  return ret;
}

