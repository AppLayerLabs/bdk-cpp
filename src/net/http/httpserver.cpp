#include "httpserver.h"

void HTTPServer::start() {
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
  this->stopped = true;
  Utils::logToDebug(Log::httpServer, __func__, "HTTP Server Stopped");
}

void HTTPServer::stop() { this->ioc.stop(); }

bool HTTPServer::running() { return !this->stopped; }

