/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "session.h"
#include "managerbase.h"
#include <functional>

namespace P2P {

  std::string Session::getLogicalLocation() const { return manager_.getLogicalLocation(); }

  void Session::handle_error(const std::string& func, const boost::system::error_code& ec) {
    // No need to discriminate on the error code here; it is harmless to try to deregister
    //   sessions or close sockets when either of those has already been done.
    if (this->doneHandshake_) {
      // handshake was completed, so nodeId_ is valid
      if (!closed_) {
        // Avoid logging errors if the socket is tagged as being explicitly closed from our end
        LOGDEBUG("Peer connection " + toString(this->nodeId_) + " error (" +
                 func + ", " + std::to_string(ec.value()) + "): " + ec.message());
      }
      this->manager_.disconnectSession(this->nodeId_);
    } else {
      // Ensure the session/socket is going to be closed.
      // If closed_ is set, don't need to call close() since that would be a NOP.
      if (!closed_) {
        // Avoid logging errors if the socket is tagged as being explicitly closed from our end
        LOGDEBUG("Non-handshaked peer connection (" + this->addressAndPortStr() + ") error (" +
                 func + ", " + std::to_string(ec.value()) + "): " + ec.message());
        this->close();
      }
    }
  }

  void Session::do_connect() {
    // Create a resolver with the strand's executor
    // (resolver(this->socket_.get_executor()) would also work since do_connect()
    //  should already be synchronized by strand_)
    boost::asio::ip::tcp::resolver resolver(this->strand_);

    auto endpoints = resolver.resolve({this->address_, this->port_});

    // Make sockets go away immediately when closed.
    boost::system::error_code open_ec;
    this->socket_.open(endpoints->endpoint().protocol(), open_ec);
    if (open_ec) {
      GLOGERROR("Error opening client P2P TCP socket: " + open_ec.message());
      this->close();
      return;
    }
    boost::system::error_code opt_ec;
    this->socket_.set_option(net::socket_base::linger(true, 0), opt_ec);
    if (opt_ec) {
      GLOGERROR("Error tring to set up SO_LINGER for a P2P client socket: " + opt_ec.message());
      this->close();
      return;
    }

    net::async_connect(this->socket_, endpoints, net::bind_executor(
      this->strand_, std::bind(
        &Session::on_connect, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::on_connect(boost::system::error_code ec, const net::ip::tcp::endpoint&) {
    if (ec) { this->handle_error(__func__, ec); return; }
    this->write_handshake();
  }

  void Session::write_handshake() {
    this->outboundHandshake_[0] = (this->manager_.nodeType() == NodeType::NORMAL_NODE) ? 0x00 : 0x01;
    auto serverPort = Utils::uint16ToBytes(this->manager_.serverPort());
    this->outboundHandshake_[1] = serverPort[0];
    this->outboundHandshake_[2] = serverPort[1];
    net::async_write(this->socket_, net::buffer(this->outboundHandshake_, 3), net::bind_executor(
      this->strand_, std::bind(
        &Session::read_handshake, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::read_handshake(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    net::async_read(this->socket_, net::buffer(this->inboundHandshake_, 3), net::bind_executor(
      this->strand_, std::bind(
        &Session::finish_handshake, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::finish_handshake(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    if (this->inboundHandshake_.size() != 3) {
      LOGERROR("Invalid handshake size from " + this->addressAndPortStr());
      this->close();
      return;
    }
    this->type_ = (!this->inboundHandshake_[0]) ? NodeType::NORMAL_NODE : NodeType::DISCOVERY_NODE;
    this->serverPort_ = Utils::bytesToUint16(Utils::create_view_span(this->inboundHandshake_, 1, 2));
    this->doneHandshake_ = true;
    this->nodeId_ = {this->address_, this->serverPort_};
    boost::system::error_code nec;
    this->socket_.set_option(boost::asio::ip::tcp::no_delay(true), nec);
    if (nec) { this->handle_error(__func__, nec); this->close(); return; }
    if (!this->manager_.registerSession(shared_from_this())) {
      // registered_ is false here, so this close() will not try to deregister the session (which would
      //   be catastrophic for when two peers simultaneously connect to each other, as the handshaked
      //   Node ID is the same for both, and this would close the *other*, registered session).
      this->close();
      return;
    }
    this->registered_ = true;
    this->do_read_header(); // Start reading messages.
  }

  void Session::do_read_header() {
    inboundHeader_.fill(0x00);
    net::async_read(this->socket_, net::buffer(this->inboundHeader_), net::bind_executor(
      this->strand_, std::bind(
        &Session::on_read_header, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::on_read_header(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    uint64_t messageSize = Utils::bytesToUint64(this->inboundHeader_);
    if (messageSize > this->maxMessageSize_) {
      LOGWARNING("Peer " + toString(nodeId_) + " message too large: " + std::to_string(messageSize) +
                 ", max: " + std::to_string(this->maxMessageSize_) + ", closing session");
      this->close();
      return;
    }
    this->do_read_message(messageSize);
  }

  void Session::do_read_message(const uint64_t& messageSize) {
    this->inboundMessage_ = std::make_shared<Message>();
    net::dynamic_vector_buffer readBuffer(this->inboundMessage_->rawMessage_);
    auto mutableBuffer = readBuffer.prepare(messageSize);
    net::async_read(this->socket_, mutableBuffer, net::bind_executor(this->strand_, std::bind(
      &Session::on_read_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2
    )));
  }

  void Session::on_read_message(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    this->manager_.handleMessage(this->nodeId_, this->inboundMessage_);
    this->inboundMessage_ = nullptr;
    this->do_read_header();
  }

  void Session::do_write_header() {
    // Nothing to do, someone called us by mistake.
    if (this->outboundMessage_ == nullptr) return;
    this->outboundHeader_ = Utils::uint64ToBytes(this->outboundMessage_->rawMessage_.size());
    net::async_write(this->socket_, net::buffer(this->outboundHeader_), net::bind_executor(
      this->strand_, std::bind(
        &Session::on_write_header, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::on_write_header(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    this->do_write_message();
  }

  void Session::do_write_message() {
    net::async_write(this->socket_, net::buffer(
      this->outboundMessage_->rawMessage_, this->outboundMessage_->rawMessage_.size()
    ), net::bind_executor(this->strand_, std::bind(
      &Session::on_write_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2
    )));
  }

  void Session::on_write_message(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    std::unique_lock lock(this->writeQueueMutex_);
    if (this->outboundMessages_.empty()) {
      this->outboundMessage_ = nullptr;
    } else {
      this->outboundMessage_ = this->outboundMessages_.front();
      this->outboundMessages_.pop_front();
      this->do_write_header();
    }
  }

  void Session::run() {
    if (this->connectionType_ == ConnectionType::INBOUND) {
      LOGTRACE("Connecting to " + this->addressAndPortStr() + " (inbound)");
      boost::asio::dispatch(net::bind_executor(this->strand_, std::bind(&Session::write_handshake, shared_from_this())));
    } else {
      LOGTRACE("Connecting to " + this->addressAndPortStr() + " (outbound)");
      boost::asio::dispatch(net::bind_executor(this->strand_, std::bind(&Session::do_connect, shared_from_this())));
    }
  }

  void Session::close() {
    boost::asio::post(net::bind_executor(this->strand_, std::bind(&Session::do_close, shared_from_this())));
  }

  void Session::do_close() {
    // This can only be called once per Session object
    if (closed_) return;
    this->closed_ = true;

    std::string peerStr;
    if (this->doneHandshake_) {
      peerStr = toString(nodeId_);
    } else {
      peerStr = this->addressAndPortStr() + " (non-handshaked)";
    }

    LOGTRACE("Closing peer connection: " + peerStr);

    boost::system::error_code ec;

    // Attempt to cancel all pending operations.
    this->socket_.cancel(ec);
    if (ec) { LOGTRACE("Failed to cancel socket operations [" + peerStr + "]: " + ec.message()); }

    // This causes TIME_WAIT even with SO_LINGER enabled (l_onoff = 1) with a timeout of zero.
    // If we want to close a socket, then we just close it: at that point, it means we do not
    //   care about pending data. If we want useful pending data to be acknowledged, then our
    //   node protocol should ensure it in-band with its own shutdown/flush negotiation, and
    //   not rely on the TCP shutdown sequence to do it.
    //
    // Attempt to shutdown the socket.
    //this->socket_.shutdown(net::socket_base::shutdown_both, ec);
    //if (ec) { LOGTRACE("Failed to shutdown socket [" + peerStr + "]: " + ec.message()); }

    // Attempt to close the socket.
    this->socket_.close(ec);
    if (ec) { LOGTRACE("Failed to close socket [" + peerStr + "]: " + ec.message()); }

    // We have to ensure the manager is going to unregister the socket as a result of closing,
    //   since the connection is guaranteed dead at this point.
    // This is almost a recursive call, since disconnectSession() calls close() which calls do_close().
    // However, that recursive call will do nothing because we already set closed_ = true above.
    //
    // Also, this deregistration is only performed if this session has registered itself. If it is
    //   not registered, then only the socket closing above is relevant; there's nothing left to do.
    if (this->registered_) {
      this->manager_.disconnectSession(this->nodeId_);
    }
  }

  void Session::write(const std::shared_ptr<const Message>& message) {
    std::unique_lock lock(this->writeQueueMutex_);
    if (this->outboundMessage_ == nullptr) {
      this->outboundMessage_ = message;
      net::post(this->strand_, std::bind(&Session::do_write_header, shared_from_this()));
    } else {
      this->outboundMessages_.push_back(message);
    }
  }
}

