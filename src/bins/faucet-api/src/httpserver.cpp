/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "httpserver.h"
namespace Faucet {
  bool HTTPServer::run() {
    // Create and launch a listening port
    const boost::asio::ip::address address = net::ip::make_address("0.0.0.0");
    auto docroot = std::make_shared<const std::string>(".");
    this->listener_ = std::make_shared<HTTPListener>(
      this->ioc_, tcp::endpoint{address, this->port_}, docroot, this->faucet_
    );
    this->listener_->start();

    // Run the I/O service on the requested number of threads (4)
    std::vector<std::thread> v;
    v.reserve(4 - 1);
    for (int i = 4 - 1; i > 0; i--) v.emplace_back([&]{ this->ioc_.run(); });
    LOGINFO(std::string("HTTP Server Started at port: ") + std::to_string(port_));
    this->ioc_.run();

    // If we get here, it means we got a SIGINT or SIGTERM. Block until all the threads exit
    for (std::thread& t : v) t.join();
    LOGINFO("HTTP Server Stopped");
    return true;
  }

}

