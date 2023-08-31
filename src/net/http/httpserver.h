/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httpparser.h"
#include "httplistener.h"

/// Abstraction of an HTTP server.
class HTTPServer {
  private:
    /// Reference pointer to the blockchain's state.
    const std::unique_ptr<State>& state_;

    /// Reference pointer to the blockchain's storage.
    const std::unique_ptr<Storage>& storage_;

    /// Reference pointer to the P2P connection manager.
    const std::unique_ptr<P2P::ManagerNormal>& p2p_;

    /// Reference pointer to the options singleton.
    const std::unique_ptr<Options>& options_;

    /// Provides core I/O functionality ({x} = max threads the object can use).
    net::io_context ioc_{4};

    /// Pointer to the HTTP listener.
    std::shared_ptr<HTTPListener> listener_;

    /// The port where the server is running.
    const unsigned short port_;

    /// The run function (effectively starts the server).
    bool run();

    /// Future for the run function so we know when it should stop.
    std::future<bool> runFuture_;

  public:
    /**
     * Constructor. Does NOT automatically start the server.
     * @param state Reference pointer to the blockchain's state.
     * @param storage Reference pointer to the blockchain's storage.
     * @param p2p Reference pointer to the P2P connection manager.
     * @param options Reference pointer to the options singleton.
     */
    HTTPServer(
        const std::unique_ptr<State>& state, const std::unique_ptr<Storage>& storage,
        const std::unique_ptr<P2P::ManagerNormal>& p2p, const std::unique_ptr<Options>& options
    ) : state_(state), storage_(storage), p2p_(p2p), options_(options), port_(options->getHttpPort())
    {}

    /**
     * Destructor.
     * Automatically stops the server.
     */
    ~HTTPServer() { this->stop(); }

    void start(); ///< Start the server.
    void stop(); ///< Stop the server.

    /**
     * Check if the server is currently active and running.
     * @return `true` if the server is running, `false` otherwise.
     */
    bool running() { return this->runFuture_.valid(); }
};

#endif  // HTTPSERVER_H
