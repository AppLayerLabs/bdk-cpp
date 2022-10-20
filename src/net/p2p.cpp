#include "p2p.h"

void p2p_fail(beast::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

void P2PClient::resolve(std::string host, std::string port) {
  this->resolver_.async_resolve(host, port, beast::bind_front_handler(
    &P2PClient::on_resolve, shared_from_this()
  ));
  this->ioc_threads.emplace_back([this]{ ioc.run(); }); // Always comes *after* async
}

void P2PClient::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
  std::cout << "Client: resolved host" << std::endl;
  if (ec) return p2p_fail(ec, "resolve");
  beast::get_lowest_layer(this->ws_).expires_never();
  this->connect(results);
}

void P2PClient::connect(tcp::resolver::results_type results) {
  beast::get_lowest_layer(this->ws_).async_connect(results, beast::bind_front_handler(
    &P2PClient::on_connect, shared_from_this()
  ));
}

void P2PClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
  std::cout << "Client: connected to host" << std::endl;
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
  this->ws_.async_handshake(host + ":" + port, "/", beast::bind_front_handler(
    &P2PClient::on_handshake, shared_from_this()
  ));
}

void P2PClient::on_handshake(beast::error_code ec) {
  std::cout << "Client: handshake given" << std::endl;
  if (ec) return p2p_fail(ec, "handshake");
  this->write("Client info:\n" + this->info_.dump());
}

void P2PClient::write(std::string msg) {
  this->ws_.async_write(net::buffer(msg), beast::bind_front_handler(
    &P2PClient::on_write, shared_from_this()
  ));
}

void P2PClient::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  std::cout << "Client: message written" << std::endl;
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "write");
  this->read();
}

void P2PClient::read() {
  this->ws_.async_read(buffer_, beast::bind_front_handler(
    &P2PClient::on_read, shared_from_this()
  ));
}

void P2PClient::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  std::cout << "Client: message read" << std::endl;
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "read");
  std::cout << beast::make_printable(buffer_.data()) << std::endl;
  buffer_.clear();
}

void P2PClient::stop() {
  this->ws_.async_close(websocket::close_code::normal, beast::bind_front_handler(
    &P2PClient::on_stop, shared_from_this()
  ));
}

void P2PClient::on_stop(beast::error_code ec) {
  std::cout << "Client: stopped" << std::endl;
  if (ec) return p2p_fail(ec, "close");
  std::cout << "Connection closed" << std::endl;
}

void P2PServer::start() {
  net::dispatch(ws_.get_executor(), beast::bind_front_handler(
    &P2PServer::on_start, shared_from_this()
  ));
}

void P2PServer::on_start() {
  std::cout << "Server: started" << std::endl;
  // Set suggested timeout settings for the websocket
  this->ws_.set_option(
    websocket::stream_base::timeout::suggested(beast::role_type::server)
  );

  // Set a decorator to change the User-Agent, then accept the handshake
  this->ws_.set_option(websocket::stream_base::decorator(
    [](websocket::response_type& res) {
      res.set(http::field::server,
        std::string(BOOST_BEAST_VERSION_STRING) + " sparq-wssv"
      );
    }
  ));
  this->accept();
}

void P2PServer::accept() {
  this->ws_.async_accept(beast::bind_front_handler(
    &P2PServer::on_accept, shared_from_this()
  ));
}

void P2PServer::on_accept(beast::error_code ec) {
  std::cout << "Server: connection accepted" << std::endl;
  if (ec) return p2p_fail(ec, "server_accept");
  this->write("Server info:\n" + this->info_.dump());
}

void P2PServer::read() {
  this->ws_.async_read(buffer_, beast::bind_front_handler(
    &P2PServer::on_read, shared_from_this())
  );
}

void P2PServer::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  std::cout << "Server: message read" << std::endl;
  boost::ignore_unused(bytes_transferred);
  if (ec == websocket::error::closed) return;
  if (ec) p2p_fail(ec, "read");
  this->ws_.text(this->ws_.got_text());
  std::cout << beast::make_printable(buffer_.data()) << std::endl;
  buffer_.clear();
}

void P2PServer::write(std::string msg) {
  this->ws_.async_write(boost::asio::buffer(msg), beast::bind_front_handler(
    &P2PServer::on_write, shared_from_this())
  );
}

void P2PServer::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  std::cout << "Server: message written" << std::endl;
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "write");
  buffer_.consume(buffer_.size());
  this->read();
}

