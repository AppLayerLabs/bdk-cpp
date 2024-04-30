/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HTTPLISTENER_H
#define HTTPLISTENER_H

#include "httpparser.h"
#include "httpsession.h"

namespace Faucet {
  class Manager;
  /// Class for listening to, accepting and dispatching incoming connections/sessions.
  class HTTPListener : public std::enable_shared_from_this<HTTPListener> {
  private:
    Manager& faucet_; ///< Reference to the faucet manager.
    /// Provides core I/O functionality.
    net::io_context& ioc_;

    /// Accepts incoming connections.
    tcp::acceptor acc_;

    /// Pointer to the root directory of the endpoint.
    const std::shared_ptr<const std::string> docroot_;

    /// Accept an incoming connection from the endpoint. The new connection gets its own strand.
    void do_accept();

    /**
     * Callback for do_accept().
     * Automatically listens to another session when finished dispatching.
     * @param ec The error code to parse.
     * @param sock The socket to use for creating the HTTP session.
     */
    void on_accept(beast::error_code ec, tcp::socket sock);

  public:
    /**
     * Constructor.
     * @param ioc Reference to the core I/O functionality object.
     * @param ep The endpoint (host and port) to listen to.
     * @param docroot Reference pointer to the root directory of the endpoint.
     * @param state Reference pointer to the blockchain's state.
     * @param storage Reference pointer to the blockchain's storage.
     * @param p2p Reference pointer to the P2P connection manager.
     * @param options Reference pointer to the options singleton.
     */
    HTTPListener(
      net::io_context& ioc, tcp::endpoint ep, const std::shared_ptr<const std::string>& docroot, Manager& faucet
    );

    void start(); ///< Start accepting incoming connections.
  };
}
#endif  // HTTPLISTENER_H
