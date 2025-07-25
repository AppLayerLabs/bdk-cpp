/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include "socketlistener.h"


namespace BTVServer {
  /// Abstraction of an Websocket server.
  class Manager;
  class WebsocketServer {
  private:
    Manager& manager_; ///< Reference to the faucet manager.

    /// Provides core I/O functionality ({x} = max threads the object can use).
    net::io_context& ioc_;

    /// Pointer to the Websocket listener.
    SocketListener listener_;

    /// The endpoint where the server is running.
    tcp::endpoint tcpEndpoint_;

  public:
    /// The run function (effectively starts the server).
    bool setup();
    /**
     * Constructor. Does NOT automatically start the server.
     * @param state Reference pointer to the blockchain's state.
     * @param storage Reference pointer to the blockchain's storage.
     * @param p2p Reference pointer to the P2P connection manager.
     * @param options Reference pointer to the options singleton.
     */
    WebsocketServer(Manager& manager, net::io_context& ioc, const tcp::endpoint& endpoint) :
    manager_(manager),
    ioc_(ioc),
    listener_(ioc, endpoint, manager)
    {
      std::cout << "Constructing at port: "  << endpoint.port() << std::endl;
    }

    /**
     * Destructor.
     * Automatically stops the server.
     */
    ~WebsocketServer() { }

    void close();

  };
}

#endif  // WEBSOCKETSERVER_H