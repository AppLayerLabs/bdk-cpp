/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "client.h"

namespace P2P {
  void ClientFactory::createClientSession(const boost::asio::ip::address &address, const unsigned short &port) {
    tcp::socket socket(io_context_);
    auto session = std::make_shared<Session>(std::move(socket), ConnectionType::OUTBOUND, manager_, address, port);
    session->run();
  }

  bool ClientFactory::run() {
    Logger::logToDebug(LogType::INFO, Log::P2PClientFactory, __func__,
                      "Starting P2P Client Factory "
    );

    // Restart is needed to .run() the ioc again, otherwise it returns instantly.
    io_context_.restart();
    std::vector<std::thread> v;
    v.reserve(this->threadCount_ - 1);

    for (auto i = this->threadCount_ - 1; i > 0; --i) { v.emplace_back([this] { this->io_context_.run(); }); }
    io_context_.run();

    for (auto &t: v) t.join(); // Wait for all threads to exit
    return true;
  }

  bool ClientFactory::start() {
    if (this->executor_.valid()) {
      Logger::logToDebug(LogType::ERROR, Log::P2PClientFactory, __func__, "P2P Client Factory already started.");
      return false;
    }
    this->executor_ = std::async(std::launch::async, &ClientFactory::run, this);
    return true;
  }

  bool ClientFactory::stop() {
    if (!this->executor_.valid()) {
      Logger::logToDebug(LogType::ERROR, Log::P2PClientFactory, __func__, "P2P Client Factory not started.");
      return false;
    }
    this->io_context_.stop();
    this->executor_.get();
    return true;
  }

  void ClientFactory::connectToServer(const boost::asio::ip::address &address, const unsigned short &port) {
    boost::asio::post(this->connectorStrand_, std::bind(&ClientFactory::createClientSession, this, address, port));
  }
}
