/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httpparser.h"
#include "httplistener.h"


namespace Faucet {
  /// Abstraction of an HTTP server.
  class Manager;
  class HTTPServer {
  private:
    Manager& faucet_; ///< Reference to the faucet manager.

    /// Provides core I/O functionality ({x} = max threads the object can use).
    net::io_context ioc_{4};

    /// Pointer to the HTTP listener.
    std::shared_ptr<HTTPListener> listener_;

    /// The port where the server is running.
    const unsigned short port_;


    /// Future for the run function so we know when it should stop.
    std::future<bool> runFuture_;

  public:
    /// The run function (effectively starts the server).
    bool run();
    /**
     * Constructor. Does NOT automatically start the server.
     * @param state Reference pointer to the blockchain's state.
     * @param storage Reference pointer to the blockchain's storage.
     * @param p2p Reference pointer to the P2P connection manager.
     * @param options Reference pointer to the options singleton.
     */
    HTTPServer(const uint16_t& port, Manager& faucet) : port_(port), faucet_(faucet) {
      std::cout << "Starting at port: "  << port_ << std::endl;
    }

    /**
     * Destructor.
     * Automatically stops the server.
     */
    ~HTTPServer() { }

    /**
     * Check if the server is currently active and running.
     * @return `true` if the server is running, `false` otherwise.
     */
    bool running() const { return this->runFuture_.valid(); }
  };
}

#endif  // HTTPSERVER_H
