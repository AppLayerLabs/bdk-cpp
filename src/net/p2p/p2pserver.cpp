#include "p2pserver.h"
#include "p2pmanagerbase.h"

namespace P2P {

  void ServerSession::run() {
    net::dispatch(ws_.get_executor(), beast::bind_front_handler(
      &ServerSession::on_run, shared_from_this()
    ));
  }

  void ServerSession::stop() {}

  void ServerSession::handleError(const std::string& func, const beast::error_code& ec) {
    if (!this->closed_) {
      Utils::logToDebug(Log::P2PServer, __func__, "Server Error Code: " + std::to_string(ec.value()) + " message: " + ec.message());
      this->manager_.unregisterSession(shared_from_this());
      this->closed_ = true;
    }
  }

  void ServerSession::on_run() {
    // Set suggested timeout settings for the websocket
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    ws_.set_option(websocket::stream_base::decorator([this](websocket::response_type& res){
      res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");
      res.set("X-Node-Id", this->manager_.nodeId().hex().get()); // Add the custom header
      res.set("X-Node-Type", std::to_string(this->manager_.nodeType()));
      res.set("X-Node-ServerPort", std::to_string(this->manager_.serverPort()));
    }));
    // Accept the websocket handshake

    http::async_read(ws_.next_layer(), buffer_, upgrade_request_, beast::bind_front_handler(&ServerSession::accept, shared_from_this()));

  }

  void ServerSession::accept(beast::error_code ec, size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) { handleError(__func__, ec); return; }

    try {
      this->hostNodeId_ = Hash(Hex::toBytes(std::string(this->upgrade_request_["X-Node-Id"])));
      } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PServer, __func__, "ServerSession: X-Node-Id header is not valid from: " + this->host_ + " at " + std::to_string(this->port_));
    }

    std::string nodeTypeStr = std::string(this->upgrade_request_["X-Node-Type"]);
    if (nodeTypeStr.size() == 1) {
      if(!std::isdigit(nodeTypeStr[0])) {
        Utils::logToDebug(Log::P2PServer, __func__, "ServerSession: X-Node-Type header is not a valid digit from " + this->hostNodeId_.hex().get());
        return;
      }
      this->hostType_ = (NodeType)std::stoi(nodeTypeStr);
    } else {
      Utils::logToDebug(Log::P2PServer, __func__, "ServerSession: X-Node-Type header is not valid from " + this->hostNodeId_.hex().get());
      return;
    }

    std::string serverPortStr = std::string(this->upgrade_request_["X-Node-ServerPort"]);
    if (serverPortStr.size() > 0) {
      if(!std::all_of(serverPortStr.begin(), serverPortStr.end(), ::isdigit)) {
        Utils::logToDebug(Log::P2PServer, __func__, "ServerSession: X-Node-ServerPort header is not a valid digit from " + this->hostNodeId_.hex().get());
        return;
      }
      this->hostServerPort_ = std::stoi(serverPortStr);
    } else {
      Utils::logToDebug(Log::P2PServer, __func__, "ServerSession: X-Node-ServerPort header is not valid from " + this->hostNodeId_.hex().get());
      return;
    }
    
    buffer_.consume(buffer_.size());
    Utils::logToFile("Server: async_accept");
    ws_.async_accept(upgrade_request_, beast::bind_front_handler(&ServerSession::on_accept, shared_from_this()));
  }

  void ServerSession::on_accept(beast::error_code ec) {
    if (ec) { handleError(__func__, ec); return; }


    this->address_ = ws_.next_layer().socket().remote_endpoint().address();
    if (!this->manager_.registerSession(shared_from_this())) {
      return;
    }

    read();
  }

  void ServerSession::read() {
    ws_.async_read(buffer_, beast::bind_front_handler(&ServerSession::on_read, shared_from_this()));
  }

  void ServerSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    //std::cout << "Request received!" << std::endl;
    if (ec) { handleError(__func__, ec); return; }

    try {
      if (buffer_.size() >= 11) {
        Message message(boost::beast::buffers_to_string(buffer_.data()));
        /// TODO *URGENT*: Change this to a thread pool. spawning threads is too utterly expensive, specially when the requesting node can try to DDoS us.
        std::thread t(&ManagerBase::handleMessage, &this->manager_, shared_from_this(), message);
        t.detach();
        buffer_.consume(buffer_.size());
      } else {
        Utils::logToDebug(Log::P2PServer, __func__, "Message too short: " + this->hostNodeId_.hex().get() + " too short");
      }
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PServer, __func__, "ServerSession exception from: " + this->hostNodeId_.hex().get() + " " + std::string(e.what()));
    }
    read();
  }

    //std::cout << "Received server: " << boost::beast::buffers_to_string(buffer_.data()) << std::endl;

  void ServerSession::write(const Message& response) {
    if (ws_.is_open()) { // Check if the stream is open, before commiting to it.
      // Copy string to buffer
      writeLock_.lock();
      auto buffer = net::buffer(response.raw());
      // Write to the socket
      ws_.async_write(buffer, beast::bind_front_handler(
        &ServerSession::on_write, shared_from_this()
      ));
    }
  }

  void ServerSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    writeLock_.unlock();
    if (ec) { handleError(__func__, ec); return; }
  }

  void ServerSession::close() {
    // Close the WebSocket connection
    this->closed_ = true;
    ws_.async_close(websocket::close_code::normal, beast::bind_front_handler(
      &ServerSession::on_close, shared_from_this()
    ));
  }

  void ServerSession::on_close(beast::error_code ec) {
    if (ec) { handleError(__func__, ec); return; }
    // If we get here then the connection is closed gracefully
    Utils::logToFile("Server Session disconnected: " + host_ + ":" + std::to_string(port_));
  }

  void Server::listener::run() {
    accept();
  }

  void Server::listener::accept() {
    // The new connection gets its own strand
    acceptor_.async_accept(net::make_strand(ioc_), beast::bind_front_handler(
      &listener::on_accept, shared_from_this()
    ));
  }

  void Server::listener::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
      Utils::logToDebug(Log::P2PServerListener, __func__, "Server listener error: " + ec.message());
      return; // Close the listener regardless of the error
    } else {
      std::make_shared<ServerSession>(std::move(socket), this->manager_)->run();
    }
   accept();
  }

  void Server::listener::stop() {
    // Cancel is not available under windows systems
    #ifdef __MINGW32__
    #else
    acceptor_.cancel(); // Cancel the acceptor.
    #endif
    acceptor_.close(); // Close the acceptor.
  }

  void Server::stop() {
    if (!this->isRunning()) {
      Utils::logToDebug(Log::P2PServer, __func__, "Server is not running");
      return;
    }
    this->ioc.stop();
  }

  void Server::start() {
    if (this->isRunning()) {
      Utils::logToDebug(Log::P2PServer, __func__, "Server is already running");
      return;
    }
    Utils::logToDebug(Log::P2PServer, __func__, "Starting server on " + this->address.to_string() + ":" + std::to_string(this->port));
    // Restart is needed to .run() the ioc again, otherwise it returns instantly.

    ioc.restart();
    std::make_shared<listener>(
      ioc, tcp::endpoint{this->address, this->port}, this->manager_
    )->run();

    std::vector<std::thread> v;
    v.reserve(this->threads - 1);

    for (auto i = this->threads - 1; i > 0; --i) { v.emplace_back([this]{ ioc.run(); }); }
    this->isRunning_ = true;
    ioc.run();

    for (auto& t : v) t.join(); // Wait for all threads to exit
    this->isRunning_ = false;
  }

};