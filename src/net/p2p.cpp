#include "p2p.h"

void p2p_fail(beast::error_code ec, char const* what) {
  Utils::logToFile(std::string("P2P: Error: ") + what + " - " + ec.message());
}

void P2P::resolve(std::string host, std::string port) {
  Utils::logToFile("P2P: resolving host: " + host + ":" + port);
  this->resolver.async_resolve(host, port, beast::bind_front_handler(
    &P2P::on_resolve, shared_from_this()
  ));
}

// TODO: iterate through the rest of the list (how to do that with async?)
void P2P::resolveSeedNode() {
  std::string ep = this->seedNodes[0];
  size_t split = ep.find(":");  // Split "host:port" -> "host", "port"
  this->resolve(ep.substr(0, split), ep.substr(split + 1));
}

void P2P::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
  if (ec) return p2p_fail(ec, "resolve");
  Utils::logToFile("P2P: resolved host, connecting");
  beast::get_lowest_layer(this->c_ws).async_connect(results,
    beast::bind_front_handler(&P2P::on_connect, shared_from_this())
  );
}

void P2P::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
  if (ec) return p2p_fail(ec, "connect");

  // Turn off timeout on tcp_stream, websocket has its own timeout system
  beast::get_lowest_layer(this->c_ws).expires_never();

  // Set suggested timeout settings for the websocket
  this->c_ws.set_option(
    websocket::stream_base::timeout::suggested(beast::role_type::client)
  );

  // Set a decorator to change the User-Agent, then perform the handshake
  this->c_ws.set_option(websocket::stream_base::decorator(
    [](websocket::request_type& req) {
      req.set(http::field::user_agent,
        std::string(BOOST_BEAST_VERSION_STRING) + " sparq-wscl"
      );
    }
  ));
  Utils::logToFile("P2P: connected to host, giving handshake");
  this->c_ws.async_handshake(
    ep.address().to_string() + ":" + std::to_string(ep.port()), "/",
    beast::bind_front_handler(&P2P::on_handshake, shared_from_this())
  );
}

void P2P::on_handshake(beast::error_code ec) {
  if (ec) return p2p_fail(ec, "handshake");
  Utils::logToFile("P2P: handshake given");
  //this->write("info");
  //this->read();
}

void P2P::on_accept(beast::error_code ec, tcp::socket socket) {
  if (ec) p2p_fail(ec, "accept"); else {
    this->s_ws = std::make_shared<websocket::stream<beast::tcp_stream>>(std::move(socket));
    Utils::logToFile("P2P: connection accepted, dispatching");
    net::dispatch(this->s_ws->get_executor(), beast::bind_front_handler(
      &P2P::on_start, shared_from_this()
    ));
  }
  this->acceptor.async_accept(net::make_strand(ioc),
    beast::bind_front_handler(&P2P::on_accept, shared_from_this())
  );
}

void P2P::on_start() {
  // Set suggested timeout settings for the websocket
  this->s_ws->set_option(
    websocket::stream_base::timeout::suggested(beast::role_type::server)
  );

  // Set a decorator to change the User-Agent, then accept the handshake
  this->s_ws->set_option(websocket::stream_base::decorator(
    [](websocket::response_type& res) {
      res.set(http::field::server,
        std::string(BOOST_BEAST_VERSION_STRING) + " sparq-wssv"
      );
    }
  ));

  Utils::logToFile("P2P: dispatched, giving handshake");
  this->s_ws->async_accept(beast::bind_front_handler(
    &P2P::on_handshake, shared_from_this()
  ));
}

void P2P::write(std::string msg) {
  Utils::logToFile("P2P: writing message: " + msg);
  //this->c_ws.async_write(boost::asio::buffer(msg), beast::bind_front_handler(
  this->c_ws.async_write(net::buffer(msg), beast::bind_front_handler(
    &P2P::on_write, shared_from_this())
  );
}

void P2P::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) return p2p_fail(ec, "write");
  Utils::logToFile("P2P: message written");
}

void P2P::read() {
  Utils::logToFile("P2P: reading message");
  this->s_ws->async_read(buf, beast::bind_front_handler(
    &P2P::on_read, shared_from_this())
  );
}

void P2P::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec == websocket::error::closed) return;
  if (ec) p2p_fail(ec, "read");
  this->s_ws->text(this->s_ws->got_text());
  std::stringstream ss;
  ss << beast::make_printable(buf.data());
  Utils::logToFile("P2P: message read: " + ss.str());
  P2PRes res(ss.str(), this->ch);
  std::string resp = res.dump();
  if (resp.empty()) resp = "Unknown or invalid command: " + ss.str();
  this->write(resp);
  buf.consume(buf.size());
  this->read();
}

