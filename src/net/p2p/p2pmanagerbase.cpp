#include "p2pmanagerbase.h"

namespace P2P {
  ManagerBase::ManagerBase(
    const boost::asio::ip::address& hostIp,
    NodeType nodeType, unsigned int maxConnections, const std::unique_ptr<Options>& options
  ) :
    options(options),
    nodeId_(Hash::random()),
    hostIp_(hostIp),
    hostPort_(options->getWsPort()),
    threadPool(std::make_unique<BS::thread_pool_light>(std::thread::hardware_concurrency() * 4)),
    p2pserver_(std::make_shared<Server>(hostIp_, hostPort_, 2, *this, this->threadPool)),
    nodeType_(nodeType),
    maxConnections_(maxConnections),
    discoveryWorker(std::make_unique<DiscoveryWorker>(*this))
  {}

  void ManagerBase::startServer() {
    this->closed_ = false;
    if (this->p2pserver_->start()) std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  void ManagerBase::startDiscovery() { this->discoveryWorker->start(); }

  void ManagerBase::stopDiscovery() { this->discoveryWorker->stop(); }

  void ManagerBase::connectToServer(const std::string &host, const unsigned short &port) {
    if (this->closed_) return;
    if (this->hostIp_.to_string() == host && this->hostPort_ == port) {
      Utils::logToDebug(Log::P2PManager, __func__, "Cannot connect to self");
      return;
    }
    if (clientSessionsCount >= this->maxConnections_) {
      Utils::logToDebug(Log::P2PManager, __func__, "Cannot connect to more than " + std::to_string(maxConnections_) + " clients");
      return;
    }
    // TODO: Move clientThread to a thread pool somehow (currently it gets stuck on .run() if you do)
    // WARNING:
    // If messing around with clientThread/ClientSessions, make SURE to delete the clientSession
    // From the sessions_ map in the ClientSession destructor.
    // Otherwise it will segfault trying to access the deleted object (ioc) in the ClientSession destructor.
    std::thread clientThread([&, host, port] {
      net::io_context ioc;
      auto client = std::make_shared<ClientSession>(ioc, host, port, *this, this->threadPool);
      client->run();
      ioc.run();
      Utils::logToFile("ClientSession thread exitted");
      ioc.stop();
      // TODO: Calling unregisterSession here causes a deadlock.
      // unregisterSession(client);
    });
    clientThread.detach();
  }

  std::shared_ptr<Request> ManagerBase::sendMessageTo(const Hash& nodeId, const Message& message) {
    if (this->closed_) return nullptr;
    std::shared_lock lockSession(sessionsMutex); // ManagerBase::sendMessageTo doesn't change sessions_ map.
    if(!sessions_.contains(nodeId)) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + nodeId.hex().get());
      return nullptr;
    }
    auto session = sessions_[nodeId];
    // We can only request ping, info and requestNode to discovery nodes
    if (session->hostType() == NodeType::DISCOVERY_NODE && (message.command() == CommandType::Info ||
        message.command() == CommandType::RequestValidatorTxs)) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session is discovery, cannot send message");
      return nullptr;
    }
    std::unique_lock lockRequests(requestsMutex);
    requests_[message.id()] = std::make_shared<Request>(message.command(), message.id(), session->hostNodeId());
    session->write(message);
    return requests_[message.id()];
  }

  void ManagerBase::answerSession(std::shared_ptr<BaseSession>& session, const Message& message) {
    if (this->closed_) return;
    session->write(message);
  }

  bool ManagerBase::registerSession(std::shared_ptr<BaseSession> session) {
    if (this->closed_) return false;
    std::unique_lock lock(sessionsMutex);
    if (sessions_.contains(session->hostNodeId())) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session already exists for " + session->hostNodeId().hex().get() + " at " + session->address().to_string());
      return false;
    }
    if (session->connectionType() == ConnectionType::CLIENT) clientSessionsCount++;
    Utils::logToDebug(Log::P2PManager, __func__, "Registering " + std::string((session->connectionType() == ConnectionType::CLIENT) ? "Client" : "Server") + " session for " + session->hostNodeId().hex().get() + " at " + session->address().to_string());
    sessions_[session->hostNodeId()] = session;
    return true;
  }

  bool ManagerBase::unregisterSession(std::shared_ptr<BaseSession> session) {
    std::unique_lock lock(sessionsMutex);
    if (!sessions_.contains(session->hostNodeId())) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + session->hostNodeId().hex().get() + " at " + session->address().to_string());
      return false;
    }
    if (session->connectionType() == ConnectionType::CLIENT) clientSessionsCount--;
    Utils::logToDebug(Log::P2PManager, __func__, "Unregistering session for " + session->hostNodeId().hex().get() + " at " + session->address().to_string());
    sessions_.erase(session->hostNodeId());
    return true;
  }

  bool ManagerBase::disconnectSession(const Hash& nodeId) {
    std::unique_lock lock(sessionsMutex);
    if (!sessions_.contains(nodeId)) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + nodeId.hex().get());
      return false;
    }
    Utils::logToDebug(Log::P2PManager, __func__, "Disconnecting session for " + nodeId.hex().get());
    // Get a copy of the pointer
    sessions_[nodeId]->close();
    sessions_.erase(nodeId);
    return true;
  }

  std::vector<Hash> ManagerBase::getSessionsIDs() {
    std::vector<Hash> ret;
    std::shared_lock lock(sessionsMutex);
    for (auto& [key, value] : sessions_) ret.push_back(key);
    return ret;
  }

  void ManagerBase::ping(const Hash& nodeId) {
    auto request = RequestEncoder::ping();
    Utils::logToFile("Pinging " + nodeId.hex().get());
    auto requestPtr = sendMessageTo(nodeId, request);
    if (requestPtr == nullptr) throw std::runtime_error(
      "Failed to send ping to " + nodeId.hex().get() + ".)"
    );
    requestPtr->answerFuture().wait();
  }

  // TODO: Both ping and requestNodes is a blocking call on .wait()
  // Somehow change to wait_for.
  std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> ManagerBase::requestNodes(const Hash& nodeId) {
    auto request = RequestEncoder::requestNodes();
    Utils::logToFile("Requesting nodes from " + nodeId.hex().get());
    auto requestPtr = sendMessageTo(nodeId, request);
    if (requestPtr == nullptr) {
      Utils::logToDebug(Log::P2PParser, __func__, "Request to " + nodeId.hex().get() + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2));
    if (status == std::future_status::timeout) {
      Utils::logToDebug(Log::P2PParser, __func__, "Request to " + nodeId.hex().get() + " timed out.");
      return {};
    }
    try {
      return AnswerDecoder::requestNodes(answer.get());
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Request to " + nodeId.hex().get() + " failed with error: " + e.what()
      );
      return {};
    }
  }

  void ManagerBase::stop() {
    this->closed_ = true;
    this->discoveryWorker->stop();
    std::unique_lock lock(sessionsMutex);
    for (auto it = sessions_.begin(); it != sessions_.end();) {
      std::weak_ptr<BaseSession> session = std::weak_ptr(it->second);
      it = sessions_.erase(it);
      auto sessionPtr = session.lock();
      if (sessionPtr) sessionPtr->close();
    }
    this->threadPool->wait_for_tasks();
    Utils::logToDebug(Log::P2PManager, __func__, "Stopping P2PManager");
    sessions_.clear();
    p2pserver_->stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
};

