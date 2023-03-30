#include "httpserver.h"

bool HTTPServer::run() {
  // Create and launch a listening port
  const boost::asio::ip::address address = net::ip::make_address("0.0.0.0");
  std::shared_ptr<const std::string> docroot = std::make_shared<const std::string>(".");
  this->listener = std::make_shared<HTTPListener>(
    this->ioc, tcp::endpoint{address, this->port}, docroot, this->state, this->storage, this->p2p
  );
  this->listener->start();

  // Run the I/O service on the requested number of threads (4)
  std::vector<std::thread> v;
  v.reserve(4 - 1);
  for (int i = 4 - 1; i > 0; i--) v.emplace_back([&]{ this->ioc.run(); });
  Utils::logToDebug(Log::httpServer, __func__,
    std::string("HTTP Server Started at port: ") + std::to_string(port)
  );
  this->ioc.run();

  // If we get here, it means we got a SIGINT or SIGTERM. Block until all the threads exit
  for (std::thread& t : v) t.join();
  Utils::logToDebug(Log::httpServer, __func__, "HTTP Server Stopped");
  return true;
}

void HTTPServer::start() {
  if (this->runFuture_.valid()) {
    Utils::logToDebug(Log::httpServer, __func__, "HTTP Server is already running");
    return;
  }
  this->runFuture_ = std::async(std::launch::async, &HTTPServer::run, this);
}

void HTTPServer::stop() {
  if (!this->runFuture_.valid()) {
    Utils::logToDebug(Log::httpServer, __func__, "HTTP Server is not running");
    return;
  }
  this->ioc.stop();
  this->runFuture_.get();
}
bool HTTPServer::running() { return this->runFuture_.valid(); }

