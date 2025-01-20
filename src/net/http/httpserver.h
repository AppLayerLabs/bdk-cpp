/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httplistener.h" // httpsession.h -> httpparser.h

#include "noderpcinterface.h"

#include "../utils/logger.h"

/// HTTP server for a Blockchain node
class HTTPServer : public Log::LogicalLocationProvider {
  private:
    /// Reference to the actual implementor of the RPC interface
    NodeRPCInterface& rpc_;

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

    /// Instance ID for logging
    std::string instanceId_;

  public:
    /**
     * Constructor. Does NOT automatically start the server.
     */
    HTTPServer(const unsigned short port, NodeRPCInterface& rpc, std::string instanceId = "")
      : port_(port), rpc_(rpc), instanceId_(instanceId)
    {
    }

    std::string getLogicalLocation() const override; ///< Get log location from the P2P engine

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
    bool running() const { return this->runFuture_.valid(); }
};

#endif  // HTTPSERVER_H
