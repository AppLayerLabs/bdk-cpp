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
// But if you want to experiment with larger thread counts, it's just a matter of tweaking the constants below.

/// Concurrency in processing incoming connection requests in the TCP listen socket, with handshake for session.
/// All operations are async and light-weight, so there's no hard requirement to have more than one thread.
/// It is also not a performance bottleneck to have many threads here, as they're all waiting most of the time.
/// If anything, having only one thread makes thread debugging simpler.
#define P2P_LISTEN_SOCKET_THREADS 1

/// Concurrency in processing accepted connections in the TCP client socket, with handshake for session.
/// All operations are async and light-weight, so there's no hard requirement to have more than one thread.
/// It is also not a performance bottleneck to have many threads here, as they're all waiting most of the time.
/// If anything, having only one thread makes thread debugging simpler.
#define P2P_CLIENT_SOCKET_THREADS 1

/// Size of the P2P engine's thread pool which processes actual network messages.
/// hardware_concurrency() counts Intel Hyperthreading (and disregards whatever other threads we may have in
///   the system) so that is already a quite elevated number. Hardcoded numbers like 2, 3 or 4 would already
///   be sufficient, probably.
#define P2P_NET_THREADS_DEFAULT (std::min(4u, std::thread::hardware_concurrency()))

namespace P2P {

  std::atomic<int> ManagerBase::instanceIdGen_(0);

  std::atomic<int> ManagerBase::netThreads_(P2P_NET_THREADS_DEFAULT);

  void ManagerBase::setNetThreads(int netThreads) {
    SLOGINFO("P2P_NET_THREADS set to " + std::to_string(netThreads) + " (was " + std::to_string(netThreads_) + ")");
    netThreads_ = netThreads;
  }

  ManagerBase::ManagerBase(
    const net::ip::address& hostIp, NodeType nodeType, const Options& options,
    const unsigned int& minConnections, const unsigned int& maxConnections
  ) : serverPort_(options.getP2PPort()), nodeType_(nodeType), options_(options),
    minConnections_(minConnections), maxConnections_(maxConnections),
    server_(hostIp, options.getP2PPort(), P2P_LISTEN_SOCKET_THREADS, *this),
    clientfactory_(*this, P2P_CLIENT_SOCKET_THREADS),
    discoveryWorker_(*this),
    instanceIdStr_("#" + std::to_string(instanceIdGen_++))
  {};

  bool ManagerBase::registerSessionInternal(const std::shared_ptr<Session>& session) {
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
      LOGERROR("Session already exists at " + toString(session->hostNodeId()));
      return false;
    }
    LOGINFO("Registering session at " + toString(session->hostNodeId()));
    sessions_.insert({session->hostNodeId(), session});
    return true;
  }

  bool ManagerBase::unregisterSessionInternal(const std::shared_ptr<Session> &session) {
    std::unique_lock lockSession(this->sessionsMutex_); // ManagerBase::unregisterSessionInternal can change sessions_ map.
    if (!this->started_) {
      return false;
    }
    if (!sessions_.contains(session->hostNodeId())) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      LOGERROR("Session does not exist at " + toString(session->hostNodeId()));
      return false;
    }
    sessions_.erase(session->hostNodeId());
    return true;
  }

  bool ManagerBase::disconnectSessionInternal(const NodeID& nodeId) {
    std::unique_lock lockSession(this->sessionsMutex_); // ManagerBase::disconnectSessionInternal can change sessions_ map.
    if (!this->started_) {
      return false;
    }
    if (!sessions_.contains(nodeId)) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      LOGERROR("Session does not exist at " + toString(nodeId));
      return false;
    }
    LOGINFO("Disconnecting session at " + toString(nodeId));
    // Get a copy of the pointer
    sessions_[nodeId]->close();
    sessions_.erase(nodeId);
    return true;
  }

  std::shared_ptr<Request> ManagerBase::sendRequestTo(const NodeID &nodeId, const std::shared_ptr<const Message>& message) {
    if (!this->started_) return nullptr;
    std::shared_lock<std::shared_mutex> lockSession(this->sessionsMutex_); // ManagerBase::sendRequestTo doesn't change sessions_ map.
    if(!sessions_.contains(nodeId)) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      LOGERROR("Session does not exist at " + toString(nodeId));
      return nullptr;
    }
    auto session = sessions_[nodeId];
    // We can only request ping, info and requestNode to discovery nodes
    if (session->hostType() == NodeType::DISCOVERY_NODE &&
      (message->command() == CommandType::Info || message->command() == CommandType::RequestValidatorTxs)
    ) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      LOGDEBUG("Session is discovery, cannot send message");
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
    std::shared_lock lockSession(this->sessionsMutex_);
    if (!this->started_) return;
    auto it = sessions_.find(nodeId);
    if (it == sessions_.end()) {
      LOGERROR("Cannot find session for " + toString(nodeId));
      return;
    }
    it->second->write(message);
  }

  void ManagerBase::start() {
    std::scoped_lock lock(this->stateMutex_);
    if (this->started_) return;
    LOGINFO("Starting " + std::to_string(ManagerBase::netThreads_) + " P2P worker threads; default: " +
            std::to_string(P2P_NET_THREADS_DEFAULT) + "; CPU: " + std::to_string(std::thread::hardware_concurrency()));
    this->threadPool_ = std::make_unique<BS::thread_pool_light>(ManagerBase::netThreads_);
    this->server_.start();
    this->clientfactory_.start();
    this->started_ = true;
  }

  void ManagerBase::stop() {
    // Only stop if started
    {
      std::scoped_lock lock(this->stateMutex_);
      if (!this->started_ || this->stopping_) return;
      this->stopping_ = true;
    }
    // Close all our peer connections
    {
      std::unique_lock lock(this->sessionsMutex_);
      for (auto it = sessions_.begin(); it != sessions_.end();) {
        std::weak_ptr<Session> session = std::weak_ptr(it->second);
        it = sessions_.erase(it);
        if (auto sessionPtr = session.lock()) sessionPtr->close();
      }
    }
    // We can't call any server/client stop() method while holding a write lock to the stateMutex_,
    //   since the stateMutex_ is locked by the network/session threads to do ManagerBase::asyncHandleMessage()
    //   which also does lock it (to access the threadPool_). If those threads are locked out due to a write
    //   lock, we can't join() them here.
    // The stopping_ flag will protect against multiple threads calling this method for whatever reason.
    // NOTE: The shared_lock here may be unnecessary.
    {
      std::shared_lock lock(this->stateMutex_);
      this->server_.stop();
      this->clientfactory_.stop();
    }
    // Finally, collect the threadpool and mark the P2P engine as fully stopped
    {
      std::scoped_lock lock (this->stateMutex_);

      // This needs to be inside the write lock. NOTE: It may be a better idea to have an extra mutex
      //   that is exclusive to threadPool_.
      this->threadPool_.reset();

      this->started_ = false;
      this->stopping_ = false;
    }
  }

  void ManagerBase::asyncHandleMessage(const NodeID &nodeId, const std::shared_ptr<const Message> message) {
    std::shared_lock lock(this->stateMutex_);
    if (this->threadPool_) {
      this->threadPool_->push_task(&ManagerBase::handleMessage, this, nodeId, message);
    }
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

  bool ManagerBase::registerSession(const std::shared_ptr<Session> &session) {
    return this->registerSessionInternal(session);
  }

  bool ManagerBase::unregisterSession(const std::shared_ptr<Session> &session) {
    return this->unregisterSessionInternal(session);
  }

  bool ManagerBase::disconnectSession(const NodeID& nodeId) {
    return this->disconnectSessionInternal(nodeId);
  }

  void ManagerBase::connectToServer(const boost::asio::ip::address& address, uint16_t port) {
    if (!this->started_) return;
    if (address == this->server_.getLocalAddress() && port == this->serverPort_) return; /// Cannot connect to itself.
    {
      std::shared_lock<std::shared_mutex> lock(this->sessionsMutex_);
      if (this->sessions_.contains({address, port})) return; // Node is already connected
    }
    this->clientfactory_.connectToServer(address, port);
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
      LOGERROR("Request to " + toString(nodeId) + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2));
    if (status == std::future_status::timeout) {
      LOGERROR("Request to " + toString(nodeId) + " timed out.");
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestNodes(*answerPtr);
    } catch (std::exception &e) {
      LOGERROR("Request to " + toString(nodeId) + " failed with error: " + e.what());
      return {};
    }
  }
}
