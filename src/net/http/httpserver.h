#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httpbase.h"
#include "httplistener.h"

/// Abstraction of an HTTP server.
class HTTPServer {
  private:
    /// Reference to the blockchain.
    Blockchain& blockchain;

    /**
     * Provides core I/O functionality.
     * {x} is the maximum number of threads the object can use.
     */
    net::io_context ioc{4};

    /// Pointer to the listener.
    std::shared_ptr<HTTPListener> listener;

    /// Indicates if the server is currently stopped.
    bool stopped = false;

    /// The port where the server is running.
    const unsigned short port;

  public:
    /**
     * Constructor.
     * @param blockchain Reference to the blockchain.
     * @param port The port that the server will run at.
     */
    HTTPServer(Blockchain& blockchain, const unsigned short port)
      : blockchain(blockchain), port(port) {}

    void start(); ///< Start the server.
    void stop(); ///< Stop the server.

    /**
     * Check if the server is currently active and running.
     * @return `true` if the server is running, `false` otherwise.
     */
    bool running();
};

#endif  // HTTPSERVER_H
