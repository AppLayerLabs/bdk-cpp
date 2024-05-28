/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managerbase.h"

// NOTE (Threading): A relatively low net worker thread count is something you want, in general:
// - It encourages writing good network messaging handlers;
// - It encourages writing dedicated thread pools elsewhere in the stack to do heavy processing of
//   messages *after* they are received (you should never have to do heavy computation in io threads);
// - It avoids having networked unit tests with a massive number of threads to debug;
// - Even debugging a single node (with e.g. gdb: thread info, bt, ...) is now much simpler;
// - Having less threads in general reduces the probability that we need to worry about having
//   thread scheduling & context switching bottlenecks of any sort;
// - Having multiple threads helps to hide some kinds of bugs, making them harder to reproduce.
// But if you want to experiment with larger thread counts, it's just a matter of tweaking the constant below.

/// Size of the P2P engine's thread pool
#define P2P_NET_THREADS_DEFAULT (std::min(4u, std::thread::hardware_concurrency()))

namespace P2P {

  ManagerBase::Net::Net(ManagerBase &manager, int netThreads)
  : manager_(manager),
    io_context_(),
    work_guard_(boost::asio::make_work_guard(io_context_)),
    threadPool_(netThreads),
    connectorStrand_(io_context_.get_executor()),
    acceptorStrand_(io_context_.get_executor()),
    acceptor_(acceptorStrand_)
  {
    auto endpoint = tcp::endpoint{manager_.serverLocalAddress_, manager_.serverPort_};
    try {
      this->acceptor_.open(endpoint.protocol()); // Open the acceptor
      this->acceptor_.set_option(net::socket_base::reuse_address(true)); // Allow address reuse
      this->acceptor_.bind(endpoint); // Bind to the server address
      this->acceptor_.listen(net::socket_base::max_listen_connections); // Start listening
    } catch (const std::exception& e) {
      throw DynamicException(std::string("Error opening listen socket: ") + e.what());
    }

    doAccept(); // enqueue first TCP inbound connection request handler

    for (int i = 0; i < netThreads; ++i) { // put the thread pool to work
      boost::asio::post(this->threadPool_, [this] { this->io_context_.run(); });
    }
  }

  ManagerBase::Net::~Net() {
    work_guard_.reset();
    io_context_.stop();
    threadPool_.join();
  }

  void ManagerBase::Net::connect(const boost::asio::ip::address& address, uint16_t port) {
    boost::asio::post(this->connectorStrand_, std::bind(&ManagerBase::Net::handleOutbound, this, address, port));
  }

  void ManagerBase::Net::doAccept() {
    this->acceptor_.async_accept(
      net::bind_executor(
        this->acceptorStrand_,
        boost::beast::bind_front_handler(&ManagerBase::Net::handleInbound, this)
      )
    );
  }

  void ManagerBase::Net::handleInbound(boost::system::error_code ec, net::ip::tcp::socket socket) {
    if (ec) {
      // Make sure doAccept() is called again so we keep accepting connections.
      LOGDEBUG("Error accepting new connection: " + ec.message());
    } else {
      std::make_shared<Session>(std::move(socket), ConnectionType::INBOUND, manager_)->run();
    }
    this->doAccept();
  }

  void ManagerBase::Net::handleOutbound(const boost::asio::ip::address &address, const unsigned short &port) {
    // async_connect() is actually done inside the Session object
    tcp::socket socket(this->io_context_);
    auto session = std::make_shared<Session>(std::move(socket), ConnectionType::OUTBOUND, manager_, address, port);
    session->run();
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
    )
  {};

  std::shared_ptr<Request> ManagerBase::sendRequestTo(const NodeID &nodeId, const std::shared_ptr<const Message>& message) {
    if (!this->started_) return nullptr;
    std::shared_ptr<Session> session;
    {
      std::shared_lock<std::shared_mutex> lockSession(this->sessionsMutex_); // ManagerBase::sendRequestTo doesn't change sessions_ map.
      auto it = sessions_.find(nodeId);
      if (it == sessions_.end()) {
        lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
        LOGDEBUG("Peer not connected: " + toString(nodeId));
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
    if (!this->started_) return;
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
    std::scoped_lock lock(this->stateMutex_);
    if (this->started_) return;
    LOGINFO("Starting " + std::to_string(ManagerBase::netThreads_) + " P2P worker threads; default: " +
            std::to_string(P2P_NET_THREADS_DEFAULT) + "; CPU: " + std::to_string(std::thread::hardware_concurrency()));

    // Attempt to start the network engine.
    // Can throw a DynamicException on error (e.g. error opening TCP listen port).
    this->net_ = std::make_unique<Net>(*this, ManagerBase::netThreads_);

    this->started_ = true;
  }

  void ManagerBase::stop() {
    std::scoped_lock lock(this->stateMutex_);
    if (!this->started_) return;

    // Ensure all peer sockets are closed and unregister all peer connections
    {
      std::unique_lock lock(this->sessionsMutex_);
      for (auto it = sessions_.begin(); it != sessions_.end();) {
        std::weak_ptr<Session> session = std::weak_ptr(it->second);
        it = sessions_.erase(it);
        if (auto sessionPtr = session.lock()) sessionPtr->close();
      }
    }

    // Completely stop and destroy the network engine
    this->net_.reset();

    this->started_ = false;
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

  // NOTE: Lifetime of a P2P connection:
  // - socket connection (encapsulated in a not-yet-handshaked Session object)
  // - handshake process
  // - registerSessionInternal: Session (useful, handshaked BDK peer socket connection) registration
  // - disconnectSessionInternal: socket disconnection + Session deregistration (simultaneous)

  bool ManagerBase::registerSession(const std::shared_ptr<Session> &session) {
    {
      std::unique_lock lockSession(this->sessionsMutex_); // ManagerBase::registerSessionInternal can change sessions_ map.
      if (!this->started_) {
        return false;
      }
      // The NodeID of a session is made by the host IP and his server port.
      // That means, it is possible for us to receive a inbound connection for someone that we already have a outbound connection.
      // In this case, we will keep the oldest connection alive and close the new one.
      // The other endpoint will also see that we already have a connection and will close the new one.
      if (sessions_.contains(session->hostNodeId())) {
        lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
        LOGTRACE("Peer already connected: " + toString(session->hostNodeId()));
        return false;
      }
      // Register the session (peer socket connection)
      sessions_.insert({session->hostNodeId(), session});
    }
    LOGINFO("Connected peer: " + toString(session->hostNodeId()));
    return true;
  }

  bool ManagerBase::disconnectSession(const NodeID& nodeId) {
    if (!this->started_) {
      return false;
    }
    std::shared_ptr<Session> session;
    {
      std::unique_lock lockSession(this->sessionsMutex_); // ManagerBase::disconnectSessionInternal can change sessions_ map.
      auto it = sessions_.find(nodeId);
      if (it == sessions_.end()) {
        lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
        LOGTRACE("Peer not connected: " + toString(nodeId));
        return false;
      }
      session = it->second;
    }
    // Ensure Session (socket) is closed (caller is counting on this)
    try {
      session->close();
    } catch ( const std::exception& e ) {
      LOGTRACE("Exception attempting to close socket to " + toString(nodeId) + ": " + e.what());
    }
    // Unregister the Session (peer socket connection)
    {
      std::unique_lock lockSession(this->sessionsMutex_); // ManagerBase::disconnectSessionInternal can change sessions_ map.
      sessions_.erase(nodeId);
    }
    LOGINFO("Disconnected peer: " + toString(nodeId));
    return true;
  }

  void ManagerBase::connectToServer(const boost::asio::ip::address& address, uint16_t port) {
    if (!this->started_) return;
    if (address == this->serverLocalAddress_ && port == this->serverPort_) return; /// Cannot connect to itself.
    {
      std::shared_lock<std::shared_mutex> lock(this->sessionsMutex_);
      if (this->sessions_.contains({address, port})) return; // Node is already connected
    }
    this->net_->connect(address, port);
  }

  void ManagerBase::ping(const NodeID& nodeId) {
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
  std::unordered_map<NodeID, NodeType, SafeHash> ManagerBase::requestNodes(const NodeID& nodeId) {
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
}
