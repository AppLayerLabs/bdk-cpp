#include "P2PServer.h"
#include "P2PManager.h"

void ServerSession::run() {
  net::dispatch(ws_.get_executor(), beast::bind_front_handler(
    &ServerSession::on_run, shared_from_this()
  ));
}

void ServerSession::stop() {}

void ServerSession::on_run() {
  // Set suggested timeout settings for the websocket
  ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

  // Set a decorator to change the Server of the handshake
  ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res){
    res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");
  }));

  // Accept the websocket handshake
  ws_.async_accept(beast::bind_front_handler(&ServerSession::on_accept, shared_from_this()));
}

void ServerSession::on_accept(beast::error_code ec) {
  if (ec.value() == 125) { p2p_fail_server(__func__, ec, "read"); return; } // Operation cancelled
  if (ec.value() == 995) { p2p_fail_server(__func__, ec, "read"); return; } // Interrupted by host
  if (ec) { return p2p_fail_server(__func__, ec, "accept"); }
  this->manager_->addClient(Connection(ws_.next_layer().socket().remote_endpoint().address(),ws_.next_layer().socket().remote_endpoint().port(),shared_from_this()));
  Utils::LogPrint(Log::P2PServer, __func__, "Client connected: " + ws_.next_layer().socket().remote_endpoint().address().to_string() +
  ":" + std::to_string(ws_.next_layer().socket().remote_endpoint().port()) + " binary: " + boost::lexical_cast<std::string>(ws_.binary()));
  read();
}

void ServerSession::read() {
  ws_.async_read(buffer_, beast::bind_front_handler(&ServerSession::on_read, shared_from_this()));
}

void ServerSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  //std::cout << "Request received!" << std::endl;
  if (ec == websocket::error::closed) { p2p_fail_server(__func__, ec, "read"); return; } // This indicates the session was closed
  if (ec.value() == 125) { p2p_fail_server(__func__, ec, "read"); return; } // Operation cancelled
  if (ec.value() == 995) { p2p_fail_server(__func__, ec, "read"); return; } // Interrupted by host
  if (ec) { p2p_fail_server(__func__, ec, "read"); }
  // Send the message for another thread to parse it.
  // TODO: make a thread pool for gods sake
  try {
    if (buffer_.size() != 0) {
      Utils::logToFile(std::string("P2PServer: received: ") + Utils::bytesToHex(boost::beast::buffers_to_string(buffer_.data())) + " size: " + std::to_string(buffer_.size()));
      P2PMessage message(boost::beast::buffers_to_string(buffer_.data()));
      std::thread t(&P2PManager::parseClientRequest, this->manager_, message, shared_from_this());
      t.detach();
      buffer_.consume(buffer_.size());
    }
  } catch (std::exception &e) {
    Utils::logToFile("P2P Server crash on_read");
  }
  read();
}

  //std::cout << "Received server: " << boost::beast::buffers_to_string(buffer_.data()) << std::endl;

void ServerSession::write(const P2PMessage& response) {
  if (ws_.is_open()) { // Check if the stream is open, before commiting to it.
    // Copy string to buffer
    answerBuffer_.consume(answerBuffer_.size());
    size_t n = boost::asio::buffer_copy(answerBuffer_.prepare(response.raw().size()), boost::asio::buffer(response.raw()));
    answerBuffer_.commit(n);
    // Write to the socket
    ws_.async_write(answerBuffer_.data(), beast::bind_front_handler(
      &ServerSession::on_write, shared_from_this()
    ));
  }
}

void ServerSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) { return p2p_fail_server(__func__, ec, "write"); }
}

void P2PServer::listener::run() {
  accept();
}

void P2PServer::listener::accept() {
  // The new connection gets its own strand
  acceptor_.async_accept(net::make_strand(ioc_), beast::bind_front_handler(
    &listener::on_accept, shared_from_this()
  ));
}

void P2PServer::listener::on_accept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    p2p_fail_server(__func__,ec, "accept");
    return; // Close the listener regardless of the error
  } else {
    std::make_shared<ServerSession>(std::move(socket), this->manager_)->run();
  }
 accept();
}

void P2PServer::listener::stop() {
  // Cancel is not available under windows systems
  #ifdef __MINGW32__
  #else
  acceptor_.cancel(); // Cancel the acceptor.
  #endif
  acceptor_.close(); // Close the acceptor.
}

void P2PServer::start() {
  Utils::logToFile("Server Starting");
  // Restart is needed to .run() the ioc again, otherwise it returns instantly.

  ioc.restart();
  std::make_shared<listener>(
    ioc, tcp::endpoint{this->address, this->port}, this->manager_
  )->run();

  std::vector<std::thread> v;
  v.reserve(this->threads - 1);

  for (auto i = this->threads - 1; i > 0; --i) { v.emplace_back([this]{ ioc.run(); }); }
  ioc.run();

  for (auto& t : v) t.join(); // Wait for all threads to exit
}