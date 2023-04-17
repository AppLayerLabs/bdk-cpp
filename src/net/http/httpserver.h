#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httpparser.h"
#include "httplistener.h"

/// Abstraction of an HTTP server.
class HTTPServer {
  private:
    /// Reference pointer to the blockchain's state.
    const std::unique_ptr<State>& state;

    /// Reference pointer to the blockchain's storage.
    const std::unique_ptr<Storage>& storage;

    /// Reference pointer to the P2P connection manager.
    const std::unique_ptr<P2P::ManagerNormal>& p2p;

    /// Reference pointer to the options singleton.
    const std::unique_ptr<Options>& options;

    /// Provides core I/O functionality ({x} = max threads the object can use).
    net::io_context ioc{4};

    /// Pointer to the HTTP listener.
    std::shared_ptr<HTTPListener> listener;

    /// The port where the server is running.
    const unsigned short port;

    /// The run function (effectively starts the server).
    bool run();

    /// Future for the run function so we know when it should stop.
    std::future<bool> runFuture;

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
    ) : state(state), storage(storage), p2p(p2p), options(options), port(options->getHttpPort())
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
    bool running() { return this->runFuture.valid(); }
};

#endif  // HTTPSERVER_H
