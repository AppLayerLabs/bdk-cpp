#include "server.h"

namespace P2P {
  void ServerListener::do_accept() {
    this->acceptor_.async_accept(
      net::make_strand(this->io_context_),
      boost::beast::bind_front_handler(
        &ServerListener::on_accept,
        shared_from_this()
      )
    );
  }

  void ServerListener::on_accept(boost::system::error_code ec, net::ip::tcp::socket socket) {
    Utils::logToDebug(Log::P2PServer, __func__, "New connection.");
    if (ec) {
      Utils::logToDebug(Log::P2PServer, __func__, "Error accepting connection: " + ec.message());
      /// TODO: Handle error
      return;
    } else {
      std::make_shared<Session>(std::move(socket), ConnectionType::INBOUND, this->manager_, this->threadPool_)->run();
    }
    this->do_accept();
  }

  void ServerListener::run() {
    this->do_accept();
  }

  void ServerListener::stop() {
    // Cancel is not available under Windows systems
    boost::system::error_code ec;
    acceptor_.cancel(ec); // Cancel the acceptor.
    if (ec) {
      Utils::logToDebug(Log::P2PServerListener, __func__, "Failed to cancel acceptor operations: " + ec.message());
      return;
    }
    acceptor_.close(ec); // Close the acceptor.
    if (ec) {
      Utils::logToDebug(Log::P2PServerListener, __func__, "Failed to close acceptor: " + ec.message());
      return;
    }
  }

  bool Server::run() {
    try {
      Utils::logToDebug(Log::P2PServer, __func__,
                        "Starting server on " + this->localAddress_.to_string() + ":" + std::to_string(this->localPort_)
      );

      // Restart is needed to .run() the ioc again, otherwise it returns instantly.
      io_context_.restart();
      Utils::logToDebug(Log::P2PServer, __func__, "Starting listener.");
      this->listener_ = std::make_shared<ServerListener>(
          io_context_, tcp::endpoint{this->localAddress_, this->localPort_}, this->manager_, this->threadPool_
      );
      this->listener_->run();
      Utils::logToDebug(Log::P2PServer, __func__, "Listener started.");

      std::vector<std::thread> v;
      v.reserve(this->threadCount_ - 1);

      Utils::logToDebug(Log::P2PServer, __func__, "Starting " + std::to_string(this->threadCount_) + " threads.");
      for (auto i = this->threadCount_ - 1; i > 0; --i) { v.emplace_back([this] { this->io_context_.run(); }); }
      io_context_.run();

      for (auto &t: v) t.join(); // Wait for all threads to exit
      Utils::logToDebug(Log::P2PServer, __func__, "Server stopped.");
    } catch ( std::exception &e ) {
      Utils::logToDebug(Log::P2PServer, __func__, "Exception: " + std::string(e.what()));
      return false;
    }
    return true;
  }

  bool Server::start() {
    if (this->executor.valid()) {
      Utils::logToDebug(Log::P2PServer, __func__, "Server already started.");
      return false;
    }
    this->executor = std::async(std::launch::async, &Server::run, this);
    return true;
  }

  bool Server::stop() {
    if (!this->executor.valid()) {
      Utils::logToDebug(Log::P2PServer, __func__, "Server not started.");
      return false;
    }
    this->io_context_.stop();
    this->executor.get();
    return true;
  }

  bool Server::isRunning() {
    return this->executor.valid();
  }
}