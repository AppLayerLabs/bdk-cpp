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
      /// Not a server, it will not call do_connect().
      throw DynamicException("Session: Invalid connection type.");
    }

    setLogSrc();
  }

  /// Construct a session with the given socket (Used by the client)
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
      /// Not a client, it will try to write handshake without connecting.
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

      // Now, this is a bit tricky:
      // When you get "false" from sessionHandshaked(), it means the registration failed. But if we are
      //   an INBOUND connection that was not eligible for replacing an OUTBOUND one (due to our nodeid vs.
      //   the remote node id), and the node is not just shutting down, we need to WAIT for the other side
      //   to close the connection, otherwise we send a TCP RST which risks causing the remote OUTBOUND
      //   being closed before the INBOUND has a chance to replace it, generating an extraneous
      //   "Peer Disconnected" event instead of the proper transparent socket/session replacement.
      //
      //   The remaining question is -- should we create a timer to close this session sometime later,
      //   or just leave it active?
      //   During shutdown, disconnect()s should be explicitly called, so that's covered.
      //   And what if the other side just goes away? Then in that case this side get an RST or
      //   timeout in this one.
      //   If not, we are left here with a dangling TCP connection that is awaiting cooperation
      //   from the other endpoint.

      //   If we are OUTBOUND, then we can afford to close.
      //   But OUTBOUND only returns false from sessionHandshaked() if we detected a bug, so this is
      //   also a bit of a no-op.
      if (connectionType_ == ConnectionType::OUTBOUND) {
        lock.unlock();
        // registered_ is false here, so the implied close() here will not try to deregister the session.
        this->close("finish_handshake sessionHandshaked failure");
        return;
      }

      // FIXME/TODO: Here we know we are a defunct INBOUND session, and ideally we'd set up a timer
      //   to clean it up if the other side is not going to do it for us.

      // OK, so what we are going to do here, is to NOT set the registered flag.
      // We set the handshaked flag above. which is correct: the session is handshaked but NOT registered,
      //  because it is a defunct INBOUND that needs to wait for the session/socket replacement process to
      //  be completed from the remote end.
      // But, we still want to start reading headers/messages, because we want to at least get an error
      //  when the connection IS closed from the other end. This could have been done in some other way,
      //  but that's what we are doing here, for now.
      // The registered_ flag being set to false as you read messages should be enough to discard them and
      //  avoid sending them to the application -- if the session/socket is not registered, then the
      //  communication from this session/socket is all irrelevant. That was already imposed by the
      //  connection replacement logic, so we are just coasting on that here.
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
          self->do_close(std::move(reasonStr));
        }
      )
    );
  }

  void Session::do_close(std::string reason) {
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
      // - Also for OUTBOUND sessions, we send the value of the unregistered flag, which is only set when the
      //   Manager itself has unregistered the session from its own side, which is a thing it only needs to do
      //   when it is transparently replacing this OUTBOUND session with an equivalent INBOUND session with the
      //   same remote peer, to solve the simultaneous double connection attempt between two TCP endpoints. If
      //   sessionClosed() receives a (doneHandshake == unregistered_ == false) callback, then it knows that's
      //   not the case and that what happened is that the OUTBOUND session in question just failed to connect,
      //   which elicits a "Failed to connect to remote peer: xxx" event at its interface.
      this->manager_.sessionClosed(*this, this->doneHandshake_, this->unregistered_);
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

