#include "httplistener.h"

HTTPListener::HTTPListener(
  net::io_context& ioc, tcp::endpoint ep,
  std::shared_ptr<const std::string>& docroot,
  Blockchain& blockchain
) : ioc(ioc), acc(net::make_strand(ioc)), docroot(docroot), blockchain(blockchain) {
  beast::error_code ec;
  this->acc.open(ep.protocol(), ec);  // Open the acceptor
  if (ec) { fail("HTTPListener", __func__, ec, "Failed to open the acceptor"); return; }
  this->acc.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
  if (ec) { fail("HTTPListener", __func__, ec, "Failed to set address reuse"); return; }
  this->acc.bind(ep, ec); // Bind to the server address
  if (ec) { fail("HTTPListener", __func__, ec, "Failed to bind to server address"); return; }
  this->acc.listen(net::socket_base::max_listen_connections, ec); // Start listening for connections
  if (ec) { fail("HTTPListener", __func__, ec, "Failed to start listening"); return; }
}

void HTTPListener::do_accept() {
  this->acc.async_accept(net::make_strand(this->ioc), beast::bind_front_handler(
    &HTTPListener::on_accept, this->shared_from_this()
  ));
}

void HTTPListener::on_accept(beast::error_code ec, tcp::socket sock) {
  if (ec) {
    fail("HTTPListener", __func__, ec, "Failed to accept connection");
  } else {
    std::make_shared<HTTPSession>(
      std::move(sock), this->docroot, this->blockchain
    )->start(); // Create the http session and run it
  }
  this->do_accept(); // Accept another connection
}

void HTTPListener::start() {
  net::dispatch(this->acc.get_executor(), beast::bind_front_handler(
    &HTTPListener::do_accept, this->shared_from_this()
  ));
}

