/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef HTTPLISTENER_H
#define HTTPLISTENER_H

#include "httpparser.h"
#include "httpsession.h"
#include "noderpcinterface.h"

/// Namespace for faucet-related functionalities.
namespace Faucet {
  class Manager; // Forward declaration.
  
  /// Class for listening to, accepting and dispatching incoming connections/sessions.
  class HTTPListener : public std::enable_shared_from_this<HTTPListener> {
  private:
    NodeRPCInterface& rpc_;
    Manager& faucet_; ///< Reference to the faucet manager.
    net::io_context& ioc_; ///< Provides core I/O functionality.
    tcp::acceptor acc_; ///< Accepts incoming connections.
    const std::shared_ptr<const std::string> docroot_; ///< Pointer to the root directory of the endpoint.

    void do_accept(); ///< Accept an incoming connection from the endpoint. The new connection gets its own strand.

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
     * @param faucet Reference to the faucet manager.
     */
    HTTPListener(
      net::io_context& ioc, tcp::endpoint ep, const std::shared_ptr<const std::string>& docroot, Manager& faucet, NodeRPCInterface& rpc_
    );

    void start(); ///< Start accepting incoming connections.
  };
}
#endif  // HTTPLISTENER_H
