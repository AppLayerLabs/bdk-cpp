#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httpparser.h"
#include "httplistener.h"

/// Abstraction of an HTTP server.
class HTTPServer {
  private:
    /// Reference to the state.
    const std::unique_ptr<State>& state;

    /// Reference to the storage.
    const std::unique_ptr<Storage>& storage;

    /// Reference to the P2P manager.
    const std::unique_ptr<P2P::ManagerNormal>& p2p;

    /// Reference to the Options
    const std::unique_ptr<Options>& options;

    /**
     * Provides core I/O functionality.
     * {x} is the maximum number of threads the object can use.
     */
    net::io_context ioc{4};

    /// Pointer to the listener.
    std::shared_ptr<HTTPListener> listener;

    /// The port where the server is running.
    const unsigned short port;

    /// The run function. (effectively starts the server)
    bool run();

    /// Future for the run function.
    std::future<bool> runFuture_;

  public:
    /**
     * Constructor.
     * @param state unique_ptr reference to the state.
     * @param port The port that the server will run at.
     */
    HTTPServer(const std::unique_ptr<State>& state,
               const std::unique_ptr<Storage>& storage,
               const std::unique_ptr<P2P::ManagerNormal>& p2p,
               const std::unique_ptr<Options>& options)
      : state(state), storage(storage), p2p(p2p), options(options), port(options->getHttpPort()) {}

    ~HTTPServer() { this->stop(); }

    void start(); ///< Start the server.
    void stop(); ///< Stop the server.

    /**
     * Check if the server is currently active and running.
     * @return `true` if the server is running, `false` otherwise.
     */
    bool running();
};

#endif  // HTTPSERVER_H
