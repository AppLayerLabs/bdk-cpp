/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "server.h"
#include "managerbase.h"

namespace P2P {

  std::string ServerListener::getLogicalLocation() const { return manager_.getLogicalLocation(); }

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
    LOGINFO("New connection.");
    if (ec) {
      LOGERROR("Error accepting connection: " + ec.message());
      /// TODO: Handle error
      return;
    } else {
      std::make_shared<Session>(std::move(socket), ConnectionType::INBOUND, this->manager_)->run();
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
      LOGERROR("Failed to cancel acceptor operations: " + ec.message());
      return;
    }
    acceptor_.close(ec); // Close the acceptor.
    if (ec) {
      LOGERROR("Failed to close acceptor: " + ec.message());
      return;
    }
  }

  std::string Server::getLogicalLocation() const { return manager_.getLogicalLocation(); }

  bool Server::run() {
    try {
      LOGINFO("Starting server on " + this->localAddress_.to_string() + ":" + std::to_string(this->localPort_));

      // Restart is needed to .run() the ioc again, otherwise it returns instantly.
      io_context_.restart();
      LOGDEBUG("Starting listener.");
      this->listener_ = std::make_shared<ServerListener>(
        io_context_, tcp::endpoint{this->localAddress_, this->localPort_}, this->manager_
      );
      this->listener_->run();
      LOGDEBUG("Listener started.");

      std::vector<std::thread> v;
      v.reserve(this->threadCount_ - 1);

      LOGDEBUG("Starting " + std::to_string(this->threadCount_) + " threads.");
      for (auto i = this->threadCount_ - 1; i > 0; --i) { v.emplace_back([this] { this->io_context_.run(); }); }
      io_context_.run();
      for (auto &t: v) t.join(); // Wait for all threads to exit
      LOGDEBUG("All threads stopped.");
    } catch ( std::exception &e ) {
      LOGERROR("Exception: " + std::string(e.what()));
      return false;
    }
    return true;
  }

  bool Server::start() {
    if (this->executor_.valid()) {
      LOGERROR("Server already started.");
      return false;
    }
    this->executor_ = std::async(std::launch::async, &Server::run, this);
    return true;
  }

  bool Server::stop() {
    if (!this->executor_.valid()) {
      LOGERROR("Server not started.");
      return false;
    }
    this->io_context_.stop();
    this->executor_.get();
    return true;
  }

  bool Server::isRunning() const { return this->executor_.valid(); }
}

