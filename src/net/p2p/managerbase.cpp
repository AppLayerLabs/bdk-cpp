/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managerbase.h"

/// Default size of the P2P engine's thread pool
#define P2P_NET_THREADS_DEFAULT (std::min(4u, std::thread::hardware_concurrency()))

namespace P2P {

  ManagerBase::Net::Net(ManagerBase &manager, int netThreads)
  : manager_(manager),
    work_guard_(boost::asio::make_work_guard(io_context_)),
    netThreads_(netThreads),
    threadPool_(netThreads),
    connectorStrand_(io_context_.get_executor()),
    acceptorStrand_(io_context_.get_executor()),
    acceptor_(acceptorStrand_)
  {
    // Keep all startup logic in start()
  }

  ManagerBase::Net::~Net() {
    // This can be redundant or irrelevant; since this is class instance is
    // always managed by a shared_ptr, you don't want stopping to be controlled
    // by the shared_ptr. You want to be sure it is stopped even if there are
    // handlers somehow active. This stop() here is just for completeness.
    LOGXTRACE("Net destructor calling stop()");
    try {
      this->stop();
    } catch (const std::exception& ex) {
      // This should never trigger, because we should not be destroying Net without calling stop() in the first place.
      LOGERROR(std::string("Unexpected exception thrown by stop() in Net destructor: ") + ex.what());
    }
    LOGXTRACE("Net destructor done");
  }

  // This *needs* to be done after the constructor since we use shared_from_this during startup
  void ManagerBase::Net::start() {
    if (stopped_) {
      throw DynamicException("ManagerBase::Net reuse not allowed");
    }

    LOGTRACE("Net engine starting");

    // First, run all threads. We need to be doing io_context.run() already in case something
    //   goes wrong and we need to run posted handlers (e.g. a shutdown task that is posted
    //   to the e.g. acceptor strand).
    std::string logSrc = this->getLogicalLocation();
    for (int i = 0; i < netThreads_; ++i) { // put the thread pool to work
      boost::asio::post(this->threadPool_, [this, logSrc, i] {
        LOGXTRACE("Net starting P2P worker thread " + std::to_string(i));
        try {
          this->io_context_.run();
        } catch (const std::exception& ex) {
          LOGFATAL_THROW(std::string("Unhandled exception caught by net thread runner: ") + ex.what());
        }
        LOGXTRACE("Net stopping P2P worker thread " + std::to_string(i));
      });
    }

    // Second, start the TCP listen socket in the server's listen port.
    auto endpoint = tcp::endpoint{manager_.serverLocalAddress_, manager_.serverPort_};
    LOGDEBUG("Listen socket server endpoint to bind: " + manager_.serverLocalAddress_.to_string() + ":" + std::to_string(manager_.serverPort_));
    try {
      this->acceptor_.open(endpoint.protocol()); // Open the acceptor

      // We want to avoid address reuse because it could (theoretically/rarely) cause problems.
      //
      // Enable address reuse when trying to bind to the server listen TCP endpoint.
      //this->acceptor_.set_option(net::socket_base::reuse_address(true)); // Allow address reuse

      this->acceptor_.set_option(net::socket_base::linger(true, 0)); // Make sockets go away immediately when closed.
      this->acceptor_.bind(endpoint); // Bind to the server address
      this->acceptor_.listen(net::socket_base::max_listen_connections); // Start listening
    } catch (const std::exception& e) {
      LOGFATALP_THROW(
        std::string("Error setting up TCP listen socket at [") +
        manager_.serverLocalAddress_.to_string() + ":" + std::to_string(manager_.serverPort_) +
        "]: " + e.what()
      );
    }

    // Finally, enqueue first TCP inbound connection request handler
    doAccept();

    LOGTRACE("Net engine started");
  }

  void ManagerBase::Net::stop() {
    if (stopped_) {
      LOGTRACE("Net engine already stopped, returning");
      return;
    }

    // This stopped_ = true has to be here, as this flag is read by the handlers that
    //   are going to blow up due to us closing everything below.
    stopped_ = true;

    LOGTRACE("Net engine stopping");

    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    boost::asio::post(
      this->acceptorStrand_,
      [this, &promise]() {
        // Cancel is not available under Windows systems
        boost::system::error_code ec;
        LOGXTRACE("Listen TCP socket strand shutdown starting, port: " + std::to_string(this->manager_.serverPort_));
        acceptor_.cancel(ec); // Cancel the acceptor.
        if (ec) { LOGTRACE("Failed to cancel acceptor operations: " + ec.message()); }
        acceptor_.close(ec); // Close the acceptor.
        if (ec) { LOGTRACE("Failed to close acceptor: " + ec.message()); }
        LOGXTRACE("Listen TCP socket strand shutdown complete, port: " + std::to_string(this->manager_.serverPort_));
        promise.set_value(); // Signal completion of the task
      }
    );
    LOGXTRACE("Listen TCP socket strand shutdown lambda joining...; port: " + std::to_string(this->manager_.serverPort_));
    future.wait(); // Wait for the posted task to complete
    LOGXTRACE("Listen TCP socket strand shutdown lambda joined; port: " + std::to_string(this->manager_.serverPort_));

    // Stop the IO context, which should error out anything that it is still doing, then join with the threadpool,
    //   which will ensure the io_context object has completely stopped, even if it would still be managing any
    //   kind of resource.
    work_guard_.reset();
    io_context_.stop();
    threadPool_.join();

    LOGTRACE("Net engine stopped");
  }

  void ManagerBase::Net::connect(const boost::asio::ip::address& address, uint16_t port) {
    auto self = weak_from_this(); // Weak ensures queued handlers will never hold up the Net object
    boost::asio::post(
      this->connectorStrand_,
      [self, address, port]() {
        if (auto spt = self.lock()) {
          spt->handleOutbound(address, port);
        }
      }
    );
  }

  void ManagerBase::Net::doAccept() {
    auto self = weak_from_this(); // Weak ensures queued handlers will never hold up the Net object
    this->acceptor_.async_accept(
      net::bind_executor(
        this->acceptorStrand_,
        [self](boost::system::error_code ec, net::ip::tcp::socket socket) {
          // IMPORTANT: need to call handleInbound() even if there's an error, because
          //            handleInbound() will call doAccept() again to post the handler
          //            that accepts the *next* connection attempt. That's why 'ec' is
          //            being propagated instead of handled here.
          if (auto spt = self.lock()) {
            spt->handleInbound(ec, std::move(socket));
          }
        }
      )
    );
  }

  void ManagerBase::Net::handleInbound(boost::system::error_code ec, net::ip::tcp::socket socket) {
    // If stopping, don't queue another accept handler or register a new session.
    if (stopped_) {
      return;
    }

    // If no error already, try setting the socket options
    if (!ec) {
      // Make sure socket goes away immediately when closed
      socket.set_option(net::socket_base::linger(true, 0), ec);
      if (ec) {
        LOGDEBUG("Error tring to set up SO_LINGER for a P2P server socket: " + ec.message());
        // Don't return here; make sure doAccept() is called again to keep accepting connections...
      }
    }
    if (!ec) {
      // Turn off Nagle
      socket.set_option(net::ip::tcp::no_delay(true), ec);
      if (ec) {
        LOGDEBUG("Error trying to set up TCP_NODELAY for a P2P server socket: " + ec.message());
        // Don't return here; make sure doAccept() is called again to keep accepting connections...
      }
    }

    // Check if everything went OK; if it did, then spawn a Session; otherwise the socket will be closed
    if (ec) {
      LOGDEBUG("Error accepting new connection: " + ec.message());
      // Don't return here; make sure doAccept() is called again to keep accepting connections...
    } else {
      this->manager_.trySpawnInboundSession(std::move(socket));
    }

    // Accept the next connection
    this->doAccept();
  }

  void ManagerBase::Net::handleOutbound(const boost::asio::ip::address &address, const unsigned short &port) {
    // If stopping, don't queue another accept handler or register a new session.
    if (stopped_) {
      return;
    }

    // Create a new socket to start a TCP connection to an additional remote peer.
    tcp::socket socket(this->io_context_);

    // Tries to create, register and run() a new OUTBOUND Session with the given socket.
    // If a registered Session to remote peer (address, port) already exists, does nothing.
    this->manager_.trySpawnOutboundSession(std::move(socket), address, port);
  }

  std::atomic<int> ManagerBase::instanceIdGen_(0);

  std::atomic<int> ManagerBase::netThreads_(P2P_NET_THREADS_DEFAULT);

  void ManagerBase::setNetThreads(int netThreads) {
    SLOGINFO("P2P_NET_THREADS set to " + std::to_string(netThreads) + " (was " + std::to_string(netThreads_) + ")");
    netThreads_ = netThreads;
  }

  ManagerBase::ManagerBase(
    const net::ip::address& hostIp, NodeType nodeType, const Options& options,
    const unsigned int& minConnections, const unsigned int& maxConnections
  ) : serverLocalAddress_(hostIp), serverPort_(options.getP2PPort()), nodeType_(nodeType), options_(options),
    minConnections_(minConnections), maxConnections_(maxConnections),
    discoveryWorker_(*this),
    instanceIdStr_
    (
      (instanceIdGen_++ > 0) ?
        "#" + std::to_string(instanceIdGen_) + ":" + std::to_string(options.getP2PPort()) :
        "" // omit instance info in production
    ),
    nodeId_(serverLocalAddress_, serverPort_)
  {};

  uint64_t ManagerBase::getPeerCount() const {
    std::shared_lock lock(this->sessionsMutex_);
    return this->sessions_.size();
  }

  void ManagerBase::setTesting() { if (instanceIdGen_ == 0) instanceIdGen_ = 1; }

  std::shared_ptr<Request> ManagerBase::sendRequestTo(const NodeID &nodeId, const std::shared_ptr<const Message>& message) {
    if (!this->isActive()) return nullptr;
    std::shared_ptr<Session> session;
    {
      std::shared_lock<std::shared_mutex> lockSession(this->sessionsMutex_); // ManagerBase::sendRequestTo doesn't change sessions_ map.
      auto it = sessions_.find(nodeId);
      if (it == sessions_.end()) {
        lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
        LOGXTRACE("Peer not connected: " + toString(nodeId));
        return nullptr;
      }
      session = it->second;
    }
    // We can only request ping, info and requestNode to discovery nodes
    if (session->hostType() == NodeType::DISCOVERY_NODE &&
      (message->command() == CommandType::Info || message->command() == CommandType::RequestValidatorTxs)
    ) {
      LOGDEBUG("Peer " + toString(nodeId) + " is a discovery node, cannot send request");
      return nullptr;
    }
    std::unique_lock lockRequests(this->requestsMutex_);
    requests_[message->id()] = std::make_shared<Request>(message->command(), message->id(), session->hostNodeId(), message);
    session->write(message);
    return requests_[message->id()];
  }

  // ManagerBase::answerSession doesn't change sessions_ map, but we still need to
  // be sure that the session io_context doesn't get deleted while we are using it.
  void ManagerBase::answerSession(const NodeID &nodeId, const std::shared_ptr<const Message>& message) {
    if (!this->isActive()) return;
    std::shared_ptr<Session> session;
    {
      std::shared_lock lockSession(this->sessionsMutex_);
      auto it = sessions_.find(nodeId);
      if (it == sessions_.end()) {
        lockSession.unlock();
        LOGDEBUG("Cannot send request answer to non-connected peer: " + toString(nodeId));
        return;
      }
      session = it->second;
    }
    session->write(message);
  }

  void ManagerBase::start() {
    std::scoped_lock state_lock(this->stateMutex_);
    if (this->started_) return;

    LOGINFO("Net creating " + std::to_string(ManagerBase::netThreads_) + " P2P worker threads; default: " +
            std::to_string(P2P_NET_THREADS_DEFAULT) + "; CPU: " + std::to_string(std::thread::hardware_concurrency()));

    // Attempt to start the network engine.
    // Can throw a DynamicException on error (e.g. error opening TCP listen port).
    this->net_ = std::make_shared<Net>(*this, ManagerBase::netThreads_);

    LOGDEBUG("Net starting");

    // This goes here because when we start getting the first handlers calling ManagerBase back,
    //   we want them to not error out with "not started".
    this->started_ = true;

    this->net_->start();

    LOGDEBUG("Net started");
  }

  void ManagerBase::stop() {
    std::scoped_lock state_lock(this->stateMutex_);
    if (!this->started_) return;

    LOGDEBUG("Net stopping - requesting close of all sessions");

    // Ensure all remaining peer sockets are closed and unregister all peer connections.
    // Stop sessions_ map from getting new entries (and stop any more work being sent to net_)
    //   by setting started_ = false.
    // Enqueue a request to cancel operations & close the socket on all remaining sessions.
    {
      // After acquiring this mutex and setting the flag, we know no more Session objects
      //   will be created nor registered.
      std::scoped_lock lock(this->sessionsMutex_);
      this->started_ = false;

      // Since started_ = false above (being set inside the sessionsMutex_) should
      //   absolutely guarantee that the sessions_ map will not increase in size
      //   during this mutex or after we release this mutex, it suffices to loop
      //   once and post a close() for all sessions that are still in the map.
      for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        LOGXTRACE("Posting close() to session with: " + toString(it->second->hostNodeId()) + "...");
        it->second->close("ManagerBase::stop()");
      }
    }

    LOGDEBUG("Net stopping - unregistering all sessions");

    // Ensure all remaining peer sockets are closed and unregister all peer connections.
    // Here we wait for all sessions to remove themselves from the map.
    // There is a timeout here so that if this fails, it won't tie up node shutdown.
    int triesLeft = 500; // 5s
    while (triesLeft-- > 0) {

      // Stop when the sessions_ map is found to be empty (that is, all registered Session
      //   objects have called sessionClosed() and unregistered themselves).
      // OR stop when we have waited for too long.
      {
        std::unique_lock lock(this->sessionsMutex_);
        size_t sessionsLeft = sessions_.size();
        if (sessionsLeft == 0) {
          break;
        }
        LOGDEBUG("Net stopping - sessions left: " + std::to_string(sessionsLeft) +
                  ", tries left: " + std::to_string(triesLeft));
        if (triesLeft <= 0) {
          // In case we run out of tries, the next best thing to do is to just wipe
          //   the sessions map. This does not explain why the sessions would ever not
          //   unregister themselves in time, but we are guaranteed to avoid an ASIO
          //   strand_/socket_ vs. io_context ~Session heap use after free error due
          //   to any Session destructors running *after* io_context.stop().
          LOGERROR("Net stopping - Forced clearing of sessions map on shutdown; size left: " + std::to_string(sessionsLeft));
          sessions_.clear();
          break;
        }
      }

      // Wait for a bit
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // At this point, all we need, really, is for the sessions_ map to be empty;
    //   There's no need for the actual deletion or even inactivity of Session
    //   objects, socket communication etc.
    // As long as nothing on our side holds a shared_ptr to a Session, then the
    //   Boost ASIO engine will take care of calling all ~Session() before the
    //   io_context object is destroyed, which prevents Session::strand_ and
    //   Session::socket_ destructors trying to reach a freed io_context.

    LOGDEBUG("Net stopping - stopping engine");

    // Attempt to completely stop and destroy the network engine
    this->net_->stop();

    LOGDEBUG("Net stopping - destroying engine");

    // Get a weak ptr (see below) then reset the net_ shared_ptr
    std::weak_ptr<Net> wpt = this->net_;
    this->net_.reset();

    // Get rid of the shared_ptr (it is conceivable that there can be handlers
    // active or other objects keeping the net_ instance alive after reset()).
    // This is not strictly necessary, but it is nice to show that we either
    // don't have IO handlers running, or that when they try to promote to
    // shared_ptr, they will likely fail.
    int tries = 50; // 5s
    while (true) {
      auto spt = wpt.lock();
      if (!spt) break;
      if (--tries <= 0) {
        LOGERROR("Timeout waiting for Net object to be destroyed.");
        break;
      }
      LOGDEBUG("Waiting for Net object to be destroyed (tries left: " + std::to_string(tries) +
               "); shared_ptr count: " + std::to_string(spt.use_count()));
      spt.reset();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Unless the loop above timed out, ~Net has finished successfully, meaning all Session
    //   objects and the entire Boost ASIO engine are fully gone.

    LOGDEBUG("Net engine destroyed");
  }

  bool ManagerBase::isActive() const {
    // started_ now protects the sessions_ map in stop(), so we have to sync with sessionsMutex_
    std::scoped_lock lock(this->sessionsMutex_);
    return this->started_;
  }

  std::vector<NodeID> ManagerBase::getSessionsIDs() const {
    std::vector<NodeID> nodes;
    std::shared_lock<std::shared_mutex> lock(this->sessionsMutex_);
    for (auto& [nodeId, session] : this->sessions_) nodes.push_back(nodeId);
    return nodes;
  }

  std::vector<NodeID> ManagerBase::getSessionsIDs(const NodeType& type) const {
    std::vector<NodeID> nodes;
    std::shared_lock<std::shared_mutex> lock(this->sessionsMutex_);
    for (auto& [nodeId, session] : this->sessions_) if (session->hostType() == type) nodes.push_back(nodeId);
    return nodes;
  }

  bool ManagerBase::sessionHandshaked(const std::shared_ptr<Session>& callerSession) {
    if (!this->isActive()) {
      return false;
    }

    auto& nodeId = callerSession->hostNodeId();

    // Whether the callerSession replaced another session that was in the sessions map already.
    bool replaced = false;

    // Whether the session that the callerSession replaced, that was already in the sessions map,
    //   was already handshaked at the time it was replaced here.
    bool replacedWasHandshaked = false;

    // For OUTBOUND connections, there's no registration to be done upon handshake completion, so this
    //   callback is only useful for raising a "Peer Connected" event.
    // For INBOUND connections, the handshaked session contains the correct NodeID of the remote host now,
    //   and can finally attempt registration (or maybe replace an OUTBOUND session for the same node).
    if (callerSession->connectionType() == ConnectionType::INBOUND)
    {
      std::unique_lock lockSession(this->sessionsMutex_);
      // Check if this is a duplicate connection; if not, register it and log a new peer connection.
      // The NodeID of a session is made by the host IP and his server port.
      // It is possible for a simultaneous connection attempt to happen. To understand which one to keep and which one
      //   to discard, both sides need to have a predetermined agreement, which is based on the ordering of their NodeIDs.
      // The side with the lowest NodeID will prioritize an OUTBOUND connection over an INBOUND one.
      // The side with the highest NodeID will prioritize an INBOUND connection over an OUTBOUND one.
      // (Connection to self, i.e. equal NodeIDs, must have been handled prior by preventing any connection attempt).
      if (auto it = sessions_.find(nodeId); it != sessions_.end()) {

        // Get a copy of the shared_ptr<Session>
        auto registeredSession = it->second;

        // Session replacement only has to be tested for INBOUND-replaces-OUTBOUND, because OUTBOUND
        //   connections never try to register themselves later: an OUTBOUND connection is always instantly
        //   registered on creation, and it is only created if there is no session for that NodeID yet.
        if ((nodeId < this->nodeId_) && (registeredSession->connectionType() == ConnectionType::OUTBOUND)) {

          // Replace the registered session with the new one only if the specific condition for replacement
          //   formally specified above is true. NOTE: This cannot block on any I/O.

          // Release the sessions lock while we notify the session being replaced of its forced unregistration.
          lockSession.unlock();

          replaced = true;

          // Mark the session as unregistered inside Session::stateMutex_, which will prevent the now dead
          //   session from making any subsequent callbacks to us.
          // Since the Session now knows it was unregistered, it will also take care of closing itself.
          replacedWasHandshaked = registeredSession->notifyUnregistered();

          // Unregister the existing session that will be replaced by callerSession below.
          // Since we are reacquiring the sessions mutex, we can't use 'it' anymore.
          lockSession.lock();
          sessions_.erase(nodeId);
          // Fallthrough to the registration of the new Session object below while
          //   still holding the lock (we need the replacement to be an atomic operation).

        } else {
          // There is already a session registered for the same remote host (NodeID) that the
          //   callerSession is connected to, and it is not going to replace it.
          // Keep the old registered session and discard the new one.
          lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
          LOGXTRACE("Peer already connected: " + toString(nodeId));
          return false;
        }
      }

      // Register the callerSession (peer socket connection).
      // The sessionsMutex is being used to set and read the started_ flag, which tells us
      //   whether we can still register nodes or whether we are shutting down and so the
      //   sessions_ map should not receive any more insertions.
      if (!this->started_) {
        LOGXTRACE("ManagerBase is already stopping -- cannot register session.");
        return false;
      }
      sessions_.try_emplace(nodeId, callerSession);
    }

    // If callerSession was replacing another session mapped to the same nodeId, then have to check
    //   whether a "Connected Peer" event just happened now or not, which depends on whether the
    //   old, replaced session was already handshaked or not.
    if (replaced) {
      if (replacedWasHandshaked) {
        LOGTRACE("Replaced handshaked Session to " + toString(nodeId));
        // Return to avoid fallthrough to the "Connected peer" log message, since the session that was
        //   replaced was already handshaked, meaning it already raised that event.
        return true;
      } else {
        LOGTRACE("Replaced non-handshaked Session to " + toString(nodeId));
        // Fallthrough to raise "Connected peer" event, since the session that was replaced was never
        //   handshaked, so it did not raise the peer-connection event before, which we will do now
        //   for this new (replacement) session.
      }
    }

    LOGINFO("Connected peer: " + toString(nodeId));
    return true;
  }

  bool ManagerBase::disconnectSession(const NodeID& nodeId) {
    if (!this->isActive()) {
      return false;
    }

    // Find a registered Session that matches the given NodeID to disconnect
    std::shared_ptr<Session> session;
    {
      std::shared_lock lockSession(this->sessionsMutex_);
      auto it = sessions_.find(nodeId);
      if (it == sessions_.end()) {
        lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
        LOGXTRACE("Peer not connected: " + toString(nodeId));
        return false;
      }
      session = it->second;
    }

    // Close the Session (socket). This will post a Session::do_close() handler that will eventually
    //   attempt to close the socket and the Session object; if that is successful (for example, if the Session
    //   was not already closed), we will receive a sessionClosed() callback from the Session object
    //   later, and *then* we will unregister it.
    session->close("ManagerBase::disconnectSession(nodeId:" + toString(nodeId) + ")");

    // The peer is only considered to be disconnected when its session is closed
    LOGTRACE("Called disconnectSession for peer: " + toString(nodeId));

    return true;
  }

  // This should only be called by the Session object that is notifying its manager of the
  //   Session's closure/teardown, and should only ever be called by sessions that are in the
  //   registered_ == true state.
  void ManagerBase::sessionClosed(const Session& callerSession, bool wasHandshaked) {

    // Important: do not quit this callback if started_ == false, since this is also called to empty out the sessions_
    //   map during ManagerBase::stop().

    auto& nodeId = callerSession.hostNodeId();

    std::unique_lock lockSession(this->sessionsMutex_);

    // Locate the Session that we already have registered in the sessions_ map
    auto it = sessions_.find(nodeId);
    if (it == sessions_.end()) {
      // There is no session to unregister.
      lockSession.unlock();
      LOGXTRACE("Peer not connected: " + toString(nodeId));
      return;
    }

    // Replaced sessions never call sessionClosed(), because they are unregistered_, so this is being called
    //   by an actual session that is registered_, so we are removing that registration.
    // That is, *(it->second) is guaranteed to be the same object as callerSession here.
    sessions_.erase(it);

    lockSession.unlock();

    // Raise the appropriate event and/or debug log as appropriate
    if (wasHandshaked) {
      LOGINFO("Disconnected peer: " + toString(nodeId));
    } else {
      LOGINFO("Failed to connect: " + toString(nodeId));
    }
  }

  void ManagerBase::connectToServer(const boost::asio::ip::address& address, uint16_t port) {
    if (address == this->serverLocalAddress_ && port == this->serverPort_) {
      return; // Filter connect-to-self requests (these are coming from the discovery worker?)
    }
    if (!this->isActive()) {
      return;
    }
    if (address.is_unspecified() || port == 0) {
      LOGERROR("Unexpected error: Address is unspecified or port is zero.");
      return;
    }
    // Don't post an outbound connection to a remote peer for which we already have a registered connection.
    std::shared_lock<std::shared_mutex> lock(this->sessionsMutex_);
    if (this->sessions_.contains({address, port})) {
      return;
    }
    lock.unlock();
    this->net_->connect(address, port);
  }

  void ManagerBase::ping(const NodeID& nodeId) {
    if (!this->isActive()) {
      LOGTRACE("Skipping ping to " + toString(nodeId) + ": Net engine not active.");
      return;
    }
    auto request = std::make_shared<const Message>(RequestEncoder::ping());
    LOGTRACE("Pinging " + toString(nodeId));
    auto requestPtr = sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) throw DynamicException(
      "Failed to send ping to " + toString(nodeId)
    );
    requestPtr->answerFuture().wait();
  }

  // TODO: Both ping and requestNodes is a blocking call on .wait()
  // Somehow change to wait_for.
  boost::unordered_flat_map<NodeID, NodeType, SafeHash> ManagerBase::requestNodes(const NodeID& nodeId) {
    if (!this->isActive()) {
      LOGTRACE("Skipping request to " + toString(nodeId) + ": Net engine not active.");
      return {};
    }
    auto request = std::make_shared<const Message>(RequestEncoder::requestNodes());
    LOGTRACE("Requesting nodes from " + toString(nodeId));
    auto requestPtr = sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      LOGDEBUG("Request to " + toString(nodeId) + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2));
    if (status == std::future_status::timeout) {
      LOGDEBUG("Request to " + toString(nodeId) + " timed out.");
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestNodes(*answerPtr);
    } catch (std::exception &e) {
      LOGDEBUG("Request to " + toString(nodeId) + " failed with error: " + e.what());
      return {};
    }
  }

  // Template Method pattern: Session calls this non-virtual method that can do something before it dispatches
  //   to handleMessage, which is defined by the subclass that specializes ManagerBase.
  void ManagerBase::incomingMessage(const Session& callerSession, const std::shared_ptr<const Message> message) {
    auto& nodeId = callerSession.hostNodeId();
    // Dispatch the incoming message to the derived class for receipt.
    handleMessage(nodeId, message);
  }

  void ManagerBase::trySpawnOutboundSession(tcp::socket&& socket, const boost::asio::ip::address &address, const unsigned short &port) {

    NodeID nodeId({address, port});

    // Sync posting the handler, checking started_ etc. with ManagerBase::stop() using the sessions mutex.
    std::unique_lock lock(this->sessionsMutex_);

    // If we are not started (e.g. ManagerBase::stop() called), nothing will be done.
    // Do not create a Session object!
    // This check needs to be inside the sessionsMutex_
    if (!this->started_) {
      return;
    }

    // If we already have a session object mapped to that NodeID, then there is no reason to start
    //   connecting to that remote peer right now.
    if (this->sessions_.contains(nodeId)) {
      return;
    }

    // Since it is OUTBOUND, it knows it has already registered itself
    auto session = std::make_shared<Session>(std::move(socket), ConnectionType::OUTBOUND, *this, address, port);

    // Register the session to avoid simultaneous OUTBOUND connection attempts to the same remote node.
    this->sessions_.try_emplace(nodeId, session);

    // Run the session to post a handler to start the outbound connection process.
    // It is probably better to have asio::post inside the sessions mutex to sync with ManagerBase::stop().
    session->run();
  }

  void ManagerBase::trySpawnInboundSession(tcp::socket&& socket) {

    // Sync posting the handler, checking started_ etc. with ManagerBase::stop() using the sessions mutex.
    std::unique_lock lock(this->sessionsMutex_);

    // If we are not started (e.g. ManagerBase::stop() called), nothing will be done.
    // Do not create a Session object!
    // This check needs to be inside the sessionsMutex_
    if (!this->started_) {
      return;
    }

    // Start and run a session.
    // It is probably better to have asio::post inside the sessions mutex to sync with ManagerBase::stop().
    std::make_shared<Session>(std::move(socket), ConnectionType::INBOUND, *this)->run();
  }
}
