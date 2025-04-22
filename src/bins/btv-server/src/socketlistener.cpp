/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "socketlistener.h"

namespace BTVServer {
  SocketListener::SocketListener(
    net::io_context& ioc, const tcp::endpoint& ep, Manager& manager
  ) : ioc_(ioc), acc_(net::make_strand(ioc)), manager_(manager)
  {
    beast::error_code ec;
    this->acc_.open(ep.protocol(), ec);  // Open the acceptor
    if (ec) { fail("SocketListener", ec, "Failed to open the acceptor"); return; }
    this->acc_.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
    if (ec) { fail("SocketListener", ec, "Failed to set address reuse"); return; }
    this->acc_.bind(ep, ec); // Bind to the server address
    if (ec) { fail("SocketListener", ec, "Failed to bind to server address"); return; }
    this->acc_.listen(net::socket_base::max_listen_connections, ec); // Start listening for connections
    if (ec) { fail("SocketListener", ec, "Failed to start listening"); return; }
  }

  void SocketListener::setup() {
    Printer::safePrint("Starting HTTP Listener at: " + this->acc_.local_endpoint().address().to_string() + ":" + std::to_string(this->acc_.local_endpoint().port()));
    this->do_accept(); // Start accepting connections
  }


  void SocketListener::do_accept() {
    this->acc_.async_accept(net::make_strand(this->ioc_), beast::bind_front_handler(
      &SocketListener::on_accept, this
    ));
  }

  void SocketListener::on_accept(beast::error_code ec, tcp::socket sock) {
    if (ec) {
      fail("SocketListener", ec, "Failed to accept connection");
    } else {
      std::make_shared<WebsocketSession>(
        std::move(sock), this->manager_
      )->start(); // Create the http session and run it
    }
    this->do_accept(); // Accept another connection
  }

  void SocketListener::start() {
    net::dispatch(this->acc_.get_executor(), beast::bind_front_handler(
      &SocketListener::setup, this
    ));
  }

  void SocketListener::close() {
    boost::system::error_code ec;
    this->acc_.close(ec);
    if (ec) {
      fail("SocketListener", ec, "Failed to close the acceptor");
    }
  }
}