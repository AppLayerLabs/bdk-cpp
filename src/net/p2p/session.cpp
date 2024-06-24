/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "session.h"
#include "managerbase.h"
#include <functional>

namespace P2P {

  std::string Session::getLogicalLocation() const { return this->logSrc_; }

  Session::Session(tcp::socket &&socket,
                    ConnectionType connectionType,
                    ManagerBase& manager)
      : socket_(std::move(socket)),
        address_(socket_.remote_endpoint().address()),
        port_(socket_.remote_endpoint().port()),
        connectionType_(connectionType),
        manager_(manager),
        strand_(socket_.get_executor())
  {
    if (connectionType == ConnectionType::OUTBOUND) {
      throw DynamicException("Session: Invalid connection type.");
    }

    setLogSrc();
  }

  Session::Session(tcp::socket &&socket,
                    ConnectionType connectionType,
                    ManagerBase& manager,
                    const net::ip::address& address,
                    unsigned short port)
      : socket_(std::move(socket)),
        address_(address),
        port_(port),
        connectionType_(connectionType),
        manager_(manager),
        strand_(socket_.get_executor())
  {
    if (connectionType == ConnectionType::INBOUND) {
      throw DynamicException("Session: Invalid connection type.");
    }

    // OUTBOUND sessions are registered before they are created with the fresh, unconnected socket,
    //   to avoid multiple Sessions being created before any of them completes the connection.
    // The registered flag must be set so that if this Session is closed, it will callback ManagerBase
    //   to remove it from the map.
    this->registered_ = true;

    // OUTBOUND sessions have the NodeID set in them from the start, since they don't need the
    //   handshake to know what the remote server listen port is. (i.e. port_ == serverPort_
    //   must always hold for a handshaked outbound Session to be valid).
    this->nodeId_ = {this->address_, this->port_};
    setLogSrc();
  }

  void Session::setLogSrc() {
    std::string connectionTypeStr;
    if (this->connectionType_ == ConnectionType::INBOUND) {
      connectionTypeStr = "I";
    } else {
      connectionTypeStr = "O";
    }
    if (this->nodeId_.second > 0) {
      this->logSrc_ = manager_.getLogicalLocation() + "(" + toString(this->nodeId_) + "," + connectionTypeStr + ")";
    } else {
      this->logSrc_ = manager_.getLogicalLocation() + "(" + this->addressAndPortStr() + "," + connectionTypeStr + ",*)";
    }
  }

  void Session::handle_error(const std::string& func, const boost::system::error_code& ec) {
    // Closed and unregistered sessions are defunct objects that do not have an effect on anything, so nothing to log.
    if (this->closed_ || this->unregistered_) {
      return;
    }

    // Log the Session failure
    if (this->nodeId_.second > 0) {
      // nodeId_ is valid
      LOGDEBUG("Peer connection " + toString(this->nodeId_) + " error (" +
               func + ", " + std::to_string(ec.value()) + "): " + ec.message());
    } else {
      // nodeId_ is not resolved, so use the next best thing (addressAndPortStr)
      LOGDEBUG("Peer connection (INBOUND non-handshaked) (" + this->addressAndPortStr() + ") error (" +
                func + ", " + std::to_string(ec.value()) + "): " + ec.message());
    }

    // Close the socket.
    this->close("Session::handle_error: " + ec.message());
  }

  void Session::do_connect() {
    // Create a resolver with the strand's executor
    // (resolver(this->socket_.get_executor()) would also work since do_connect()
    //  should already be synchronized by strand_)
    net::ip::tcp::resolver resolver(this->strand_);

    auto endpoints = resolver.resolve({this->address_, this->port_});

    // Open socket to set options
    boost::system::error_code open_ec;
    this->socket_.open(endpoints->endpoint().protocol(), open_ec);
    if (open_ec) {
      LOGERROR("Error opening client P2P TCP socket: " + open_ec.message());
      this->close("Session::do_connect() open():" + open_ec.message());
      return;
    }
    boost::system::error_code opt_ec;
    // Make sockets go away immediately when closed.
    this->socket_.set_option(net::socket_base::linger(true, 0), opt_ec);
    if (opt_ec) {
      LOGERROR("Error tring to set up SO_LINGER for a P2P client socket: " + opt_ec.message());
      this->close("Session::do_connect() set_option(linger): " + opt_ec.message());
      return;
    }
    // Turn off Nagle
    this->socket_.set_option(net::ip::tcp::no_delay(true), opt_ec);
    if (opt_ec) {
      LOGERROR("Error trying to set up TCP_NODELAY for a P2P client socket: " + opt_ec.message());
      this->close("Session::do_connect() set_option(nodelay): " + opt_ec.message());
      return;
    }

    net::async_connect(this->socket_, endpoints, net::bind_executor(
      this->strand_, std::bind_front(
        &Session::on_connect, shared_from_this()
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
      this->strand_, std::bind_front(
        &Session::read_handshake, shared_from_this()
      )
    ));
  }

  void Session::read_handshake(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    net::async_read(this->socket_, net::buffer(this->inboundHandshake_, 3), net::bind_executor(
      this->strand_, std::bind_front(
        &Session::finish_handshake, shared_from_this()
      )
    ));
  }

  void Session::finish_handshake(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    if (this->inboundHandshake_.size() != 3) {
      LOGDEBUG("Invalid handshake size from " + this->addressAndPortStr());
      this->close("Invalid handshake size");
      return;
    }
    this->type_ = (!this->inboundHandshake_[0]) ? NodeType::NORMAL_NODE : NodeType::DISCOVERY_NODE;
    this->serverPort_ = Utils::bytesToUint16(Utils::create_view_span(this->inboundHandshake_, 1, 2));

    // Ensure that the server does not inform us a port number that is actually different from the listen port number we
    //   actually connected to, which could cause problems, since we already registered this OUTBOUND Session to port_.
    // This only happens if there's a bug in the (remote) node.
    if (connectionType_ == ConnectionType::OUTBOUND && (this->port_ != this->serverPort_)) {
      LOGDEBUG("Invalid handshake from " + this->addressAndPortStr() +
               ": OUTBOUND session port (" + std::to_string(this->port_) +
               ") != handshake server port (" + std::to_string(this->serverPort_) + ")");
      // registered_ is false here, so this close() will not try to deregister the session (which would
      //   be catastrophic for when two peers simultaneously connect to each other, as the handshaked
      //   Node ID is the same for both, and this would close the *other*, registered session).
      this->close("finish_handshake invalid handshake port");
      return;
    }

    // Resolving the NodeID now is only relevant to INBOUND, since for OUTBOUND we know the
    //   NodeID in the constructor.
    if (connectionType_ == ConnectionType::INBOUND) {
      this->nodeId_ = {this->address_, this->serverPort_};
      setLogSrc();
      LOGTRACE("INBOUND handshaked connection port " + std::to_string(this->port_) +
               " to listen port " + std::to_string(this->serverPort_));
    }

    // This mutex is acquired here because we want to set doneHandshake_ simultaneously with
    //   generating a "Peer Connected" event at the P2P Manager. This allows the P2P Manager
    //   to figure out whether a Session that replaces this one should generate a "Peer
    //   Connected" event or not by inspecting the bool return value from notifyUnregistered().
    std::unique_lock lock(this->stateMutex_);

    this->doneHandshake_ = true;

    // If, for any reason, this Session object has already been actively unregistered by the manager,
    //   then do not flip it back to a registered state.
    if (this->unregistered_) {
      lock.unlock();
      // registered_ is false here, so the implied close() here will not try to deregister the session.
      this->close("finish_handshake already unregistered");
      return;
    }

    // Notify the P2P Manager that a handshake was completed for a session.
    // For OUTBOUND sessions, the Manager knows there's no registration to be done since
    //   those are registered upon creation, but it still wants to log "Peer Connected".
    // For INBOUND sessions, it will attempt to register them, and in that case this call
    //   can return false if the registration failed (which is expected in some cases of
    //   e.g. double connection attempts).
    if (!this->manager_.sessionHandshaked(shared_from_this())) {

      // Handle having failed to register this session that has just completed its handshake.

      // If it was a failed OUTBOUND, post a socket close and return.
      if (connectionType_ == ConnectionType::OUTBOUND) {
        // OUTBOUND only returns false from sessionHandshaked() if the code is bugged elsewhere, so this
        //   branch should not normally execute (leave it here as defense).
        lock.unlock();
        // registered_ is false here, so the implied close() here will not try to deregister the session.
        this->close("finish_handshake OUTBOUND sessionHandshaked failure (should never happen)");
        return;
      }

      // It was a failed INBOUND.

      // This is a defunct (failed-registration) INBOUND TCP connection that we can't immediately close,
      //   otherwise that creates an extraneous "Peer Disconnected" event at the other side.
      // That's not bad per se, but it makes it more difficult to write tests, as you can no longer
      //   assume our Session registrations are "stable"; they would flicker out and back into existence
      //   during a connection replacement process, which is not what we want.
      //
      // So, what we do here, is set up a 10-second timer:
      // - In production, the node software does not actually care if a session dies and is
      //   immediately reestablished (a "replacement" that does not replace), so there's nothing
      //   really at stake in production environments other than (potentially) extra logging.
      //   So there's no reason we need to wait for any "minimum" amount of time; the timer can
      //   be small (that is, 10s).
      // - In any case, 10s is pretty good for the modern internet, so it is likely that this
      //   timer will actually "never" trigger (aside from bugs, ...)
      // - 10s is plenty for localhost testing.
      // - 10s is enough time to tolerate a stale TCP connection unnecessarily spending resources.
      // - any close/do_close that happens first will replace this timer; correct nodes should
      //   close this from their end much faster than 10s.

      LOGXTRACE("INBOUND session failed to register, waiting replacement at remote w/ 10s timeout");

      // Create a shared pointer to a steady timer
      auto timer = std::make_shared<net::steady_timer>(strand_, std::chrono::seconds(10));

      // Post the handler to be executed after the specified duration, binding the shared pointer to keep it alive
      timer->async_wait(boost::asio::bind_executor(
        strand_,
        [self = shared_from_this(), timer](const boost::system::error_code& timer_ec) mutable {
          if (!timer_ec) {
            self->do_close("Failed-to-register INBOUND Session 10s timer expired");
          } else {
            GLOGDEBUG("Failed-to-register INBOUND Session 10s timer error: " + timer_ec.message());
          }
        }
      ));

      // Fallthrough to do_read_hreader() below, which allows our INBOUND session to benefit
      //   from socket read errors and close early. registered_ is not set, which discards
      //   all received data (as it should, as the session is dead).
    } else {
      // Handshake completion was OK, so we know that we are registered
      this->registered_ = true; // OUTBOUND already set this to true, but this is relevant to INBOUND
    }
    lock.unlock();

    // Start reading messages from the peer
    this->do_read_header();
  }

  void Session::do_read_header() {
    inboundHeader_.fill(0x00);
    net::async_read(this->socket_, net::buffer(this->inboundHeader_), net::bind_executor(
      this->strand_, std::bind_front(
        &Session::on_read_header, shared_from_this()
      )
    ));
  }

  void Session::on_read_header(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }
    uint64_t messageSize = Utils::bytesToUint64(this->inboundHeader_);
    if (messageSize > this->maxMessageSize_) {
      LOGWARNING("Peer " + toString(nodeId_) + " message too large: " + std::to_string(messageSize) +
                 ", max: " + std::to_string(this->maxMessageSize_) + ", closing session");
      this->close("message too large");
      return;
    }
    this->do_read_message(messageSize);
  }

  void Session::do_read_message(const uint64_t& messageSize) {
    this->inboundMessage_ = std::make_shared<Message>();
    net::dynamic_vector_buffer readBuffer(this->inboundMessage_->rawMessage_);
    auto mutableBuffer = readBuffer.prepare(messageSize);
    net::async_read(this->socket_, mutableBuffer, net::bind_executor(this->strand_, std::bind_front(
      &Session::on_read_message, shared_from_this()
    )));
  }

  void Session::on_read_message(boost::system::error_code ec, std::size_t) {
    if (ec) { this->handle_error(__func__, ec); return; }

    // Call back the manager with a new message (if this Session is not already unregistered,
    //   or if it was never registered which is a corner case of duplicate session replacement)
    std::unique_lock lock(this->stateMutex_);
    if (this->registered_) {
      this->manager_.incomingMessage(*this, this->inboundMessage_);
    }
    lock.unlock();

    this->inboundMessage_ = nullptr;
    this->do_read_header();
  }

  void Session::do_write_header() {
    // Nothing to do, someone called us by mistake.
    if (this->outboundMessage_ == nullptr) return;
    this->outboundHeader_ = Utils::uint64ToBytes(this->outboundMessage_->rawMessage_.size());
    net::async_write(this->socket_, net::buffer(this->outboundHeader_), net::bind_executor(
      this->strand_, std::bind_front(
        &Session::on_write_header, shared_from_this()
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
    ), net::bind_executor(this->strand_, std::bind_front(
      &Session::on_write_message, shared_from_this()
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
      net::post(net::bind_executor(this->strand_, std::bind_front(&Session::write_handshake, shared_from_this())));
    } else {
      LOGTRACE("Connecting to " + this->addressAndPortStr() + " (outbound)");
      net::post(net::bind_executor(this->strand_, std::bind_front(&Session::do_connect, shared_from_this())));
    }
  }

  void Session::close(std::string&& reason) {
    // This net::dispatch (vs. net::post) should immediately call do_close() if the caller
    //   is already a handler that is holding strand_, otherwise it will post the handler
    //   (when close() is called from ManagerBase, for example).
    net::dispatch(
      net::bind_executor(
        strand_,
        [self = shared_from_this(), reasonStr = std::move(reason)]() mutable {
          self->do_close(reasonStr);
        }
      )
    );
  }

  void Session::do_close(const std::string& reason) {
    // This can only be called once per Session object
    if (closed_) {
      return;
    }
    this->closed_ = true;

    std::string peerStr;
    if (this->nodeId_.second > 0) {
      peerStr = toString(nodeId_);
    } else {
      peerStr = this->addressAndPortStr() + " (INBOUND non-handshaked)";
    }

    LOGTRACE("Closing peer connection: " + peerStr + " (reason: " + reason + ")");

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

    // Ensure that all closed sessions are always unregistered before their strand_ will run
    //   any other handler after this do_close() one.
    std::unique_lock lock(this->stateMutex_);
    if (this->registered_) {
      // This callback carries a few subtleties:
      // - If this session is not registered, that is, it never was registered in the first place, or if it
      //   was registered once then got unregistered from the Manager side, then, in either case, we do not
      //   call the manager to notify of a session closure. The logic here is that sessionClosed() means
      //   the Manager is being called to unregister a session. If we know there is no registration, then
      //   there is no reason to do so: if the session never was registered, then there was no "Peer Connected"
      //   event raised at the Manager interface, so there is no need for the Manager to know that now there is
      //   no longer a connection and it should raise a "Peer Disconnected".
      // - For OUTBOUND sessions, which are registered on creation, there's the additional detail that the
      //   "Peer Connected" event is always generated (for both INBOUND and OUTBOUND) during handshake completion,
      //   actually. For INBOUND, doneHandshake_ and registered_ are flipped to true at the same time, but for
      //   OUTBOUND, doneHandshake_ is flipped to true later, during handshake completion. And so, though we
      //   want to notify the Manager that a non-handshaked OUTBOUND session is closed so it can remove it from
      //   the sessions_ map, we also want to send, along with the notification, the doneHandshake_ status of
      //   the Session, so that when it is `false`, it can know that it should not generate a "Peer Disconnected"
      //   event, because it never generated a `Peer Connected` one in the first place.
      // - Note that unregistered_ cannot ever be true when registered_ is true, so the unregistered_ flag
      //   is guaranteed to be false at this point (because both registered_ and unregistered_ are set
      //   inside the stateMutex_ within notifyUnregistered(), which is the only place unregistered_ is set
      //   to true). This means sessionClosed() is never called for a defunct Session that is being replaced.
      this->manager_.sessionClosed(*this, this->doneHandshake_);
      this->registered_ = false;
    }
    lock.unlock();
  }

  void Session::write(const std::shared_ptr<const Message>& message) {
    std::unique_lock lock(this->writeQueueMutex_);
    if (this->outboundMessage_ == nullptr) {
      this->outboundMessage_ = message;
      net::post(this->strand_, std::bind_front(&Session::do_write_header, shared_from_this()));
    } else {
      this->outboundMessages_.push_back(message);
    }
  }

  bool Session::notifyUnregistered() {
    // IMPORTANT: The caller to this method should NOT be holding ManagerBase::sessionsMutex_,
    //            otherwise this WILL deadlock.
    //            (And there is no actual valid reason why the caller should be holding it).
    std::unique_lock lock(this->stateMutex_);
    this->registered_ = false;
    this->unregistered_ = true;
    bool handshaked = this->doneHandshake_;
    lock.unlock();
    close("notifyUnregistered");
    return handshaked;
  }
}

