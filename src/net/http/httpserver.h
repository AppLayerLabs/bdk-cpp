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
     * @param state unique_ptr reference to the state.
     * @param port The port that the server will run at.
     */
    HTTPServer(const unsigned short port, const std::unique_ptr<State>& state, const std::unique_ptr<Storage>& storage, const std::unique_ptr<P2P::ManagerNormal>& p2p)
      : port(port), state(state), storage(storage), p2p(p2p) {}

    void start(); ///< Start the server.
    void stop(); ///< Stop the server.

    /**
     * Check if the server is currently active and running.
     * @return `true` if the server is running, `false` otherwise.
     */
    bool running();
};

#endif  // HTTPSERVER_H
