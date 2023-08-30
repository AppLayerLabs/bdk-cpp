/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "httpserver.h"

bool HTTPServer::run() {
  // Create and launch a listening port
  const boost::asio::ip::address address = net::ip::make_address("0.0.0.0");
  std::shared_ptr<const std::string> docroot = std::make_shared<const std::string>(".");
  this->listener_ = std::make_shared<HTTPListener>(
    this->ioc_, tcp::endpoint{address, this->port_}, docroot,
    this->state_, this->storage_, this->p2p_, this->options_
  );
  this->listener_->start();

  // Run the I/O service on the requested number of threads (4)
  std::vector<std::thread> v;
  v.reserve(4 - 1);
  for (int i = 4 - 1; i > 0; i--) v.emplace_back([&]{ this->ioc_.run(); });
  Logger::logToDebug(LogType::INFO, Log::httpServer, __func__,
    std::string("HTTP Server Started at port: ") + std::to_string(port_)
  );
  this->ioc_.run();

  // If we get here, it means we got a SIGINT or SIGTERM. Block until all the threads exit
  for (std::thread& t : v) t.join();
  Logger::logToDebug(LogType::INFO, Log::httpServer, __func__, "HTTP Server Stopped");
  return true;
}

void HTTPServer::start() {
  if (this->runFuture_.valid()) {
    Logger::logToDebug(LogType::ERROR, Log::httpServer, __func__, "HTTP Server is already running");
    return;
  }
  this->runFuture_ = std::async(std::launch::async, &HTTPServer::run, this);
}

void HTTPServer::stop() {
  if (!this->runFuture_.valid()) {
    Logger::logToDebug(LogType::ERROR, Log::httpServer, __func__, "HTTP Server is not running");
    return;
  }
  this->ioc_.stop();
  this->runFuture_.get();
}

