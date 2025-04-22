#include "websocketsession.h"
#include "manager.h"


namespace BTVServer {
  WebsocketSession::WebsocketSession(tcp::socket&& socket, Manager& manager)
    : manager_(manager), ws_(std::move(socket)), strand_(ws_.get_executor()) {
    auto rand = Utils::randBytes(8);
    std::memcpy(&id_, rand.data(), 8);
  }

  WebsocketSession::~WebsocketSession() {
    if (this->registered_) {
      this->manager_.removePlayer(this->id_);
    }
  }
  void WebsocketSession::doAccept() {
    // Accept the websocket handshake
    ws_.async_accept(beast::bind_front_handler(&WebsocketSession::onAccept, shared_from_this()));
  }

  void WebsocketSession::onAccept(beast::error_code ec) {
    if (ec) {
      this->onError();
      return fail("WebsocketSession", ec, "accept");
    }
    // Register the websocket session into the manager
    this->registered_ = true;
    this->manager_.registerPlayer(this->id_, weak_from_this());
    // Start reading messages from the server
    this->doRead();
  }

  void WebsocketSession::doRead() {
    // Read a message into our buffer
    ws_.async_read(buffer_, beast::bind_front_handler(&WebsocketSession::onRead, shared_from_this()));
  }

  void WebsocketSession::onRead(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
      this->onError();
      return fail("WebsocketSession", ec, "read");
    }

    // Send to manager
    manager_.handlePlayerRequest(
      weak_from_this(), boost::beast::buffers_to_string(buffer_.data())
    );
    // Clear the buffer
    buffer_.consume(buffer_.size());
    // Read again
    doRead();
  }

  void WebsocketSession::onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
      return fail("WebsocketSession", ec, "write");
    }
    std::unique_lock lock(writeQueueMutex_);
    if (writeQueue_.empty()) {
      writeMsg_.reset();
    } else {
      writeMsg_ = std::move(writeQueue_.front());
      writeQueue_.pop_front();
      ws_.async_write(net::buffer(*writeMsg_), beast::bind_front_handler(&WebsocketSession::onWrite, shared_from_this()));
    }
  }

  void WebsocketSession::onError() {
    // If it is NOT closed, close it, and set closed_ to true
    if (!this->closed_.exchange(true)) {
      Printer::safePrint("Closing the websocket session");
      ws_.async_close(websocket::close_code::normal, beast::bind_front_handler(&WebsocketSession::onClose, shared_from_this()));
    }
  }

  void WebsocketSession::onClose(beast::error_code ec) {
    if (ec) {
      return fail("WebsocketSession", ec, "close");
    }
    // Do nothing
  }

  void WebsocketSession::write(const std::string& msg) {
    // Send the message
    auto messagePtr = std::make_unique<std::string>(msg);
    std::unique_lock lock(writeQueueMutex_);
    if (this->writeMsg_ == nullptr) {
      this->writeMsg_ = std::move(messagePtr);
      ws_.async_write(net::buffer(*this->writeMsg_), beast::bind_front_handler(&WebsocketSession::onWrite, shared_from_this()));
    } else {
      writeQueue_.push_back(std::move(messagePtr));
    }
  }

  void WebsocketSession::stop() {
    // Close the WebSocket connection
    ws_.async_close(websocket::close_code::normal, beast::bind_front_handler(&WebsocketSession::onClose, shared_from_this()));
  }

  void WebsocketSession::start() {
    // Accept the websocket handshake
    doAccept();
  }
}