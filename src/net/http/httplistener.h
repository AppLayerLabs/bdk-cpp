#ifndef HTTPLISTENER_H
#define HTTPLISTENER_H

#include "httpparser.h"
#include "httpsession.h"

/// Helper class that accepts incoming connections and dispatches sessions.
class HTTPListener : public std::enable_shared_from_this<HTTPListener> {
  private:
    /// Provides core I/O functionality.
    net::io_context& ioc;

    /// Accepts incoming connections.
    tcp::acceptor acc;

    /// Pointer to the root directory of the endpoint.
    std::shared_ptr<const std::string> docroot;

    /// Reference to the State
    const std::unique_ptr<State>& state;

    /// Reference to the Storage
    const std::unique_ptr<Storage>& storage;

    /// Reference to the P2P manager.
    const std::unique_ptr<P2P::ManagerNormal>& p2p;

    /**
     * Accept an incoming connection from the endpoint.
     * The new connection gets its own strand.
     */
    void do_accept();

    /**
     * Callback to create a new HTTP session from the accepted incoming connection.
     * Also automatically listens to another session when finished dispatching.
     * @param ec The error code to parse.
     * @param sock The socket to use for creating the HTTP session.
     */
    void on_accept(beast::error_code ec, tcp::socket sock);

  public:
    /**
     * Constructor.
     * @param ioc Reference to the core I/O functionality object.
     * @param ep The endpoint (host and port) to listen to.
     * @param Pointer to the root directory of the endpoint.
     * @param unique_ptr reference to the state.
     */
    HTTPListener(
      net::io_context& ioc, tcp::endpoint ep,
      std::shared_ptr<const std::string>& docroot,
      const std::unique_ptr<State>& state,
      const std::unique_ptr<Storage>& storage,
      const std::unique_ptr<P2P::ManagerNormal>& p2p
    );

    /// Start accepting incoming connections.
    void start();
};

#endif  // HTTPLISTENER_H
