#include "managerbase.h"

namespace P2P {

  bool ManagerBase::registerSessionInternal(const std::shared_ptr<Session>& session) {
    if (this->closed_) {
      return false;
    }
    std::unique_lock lockSession(this->sessionsMutex_); // ManagerBase::registerSessionInternal can change sessions_ map.
    // The NodeID of a session is made by the host IP and his server port.
    // That means, it is possible for us to receive a inbound connection for someone that we already have a outbound connection.
    // In this case, we will keep the oldest connection alive and close the new one.
    // The other endpoint will also see that we already have a connection and will close the new one.
    if (sessions_.contains(session->hostNodeId())) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PManager, __func__, "Session already exists at " +
                        session->hostNodeId().first.to_string() + ":" + std::to_string(session->hostNodeId().second));
      return false;
    }
    Logger::logToDebug(LogType::INFO, Log::P2PManager, __func__, "Registering session at " +
                      session->hostNodeId().first.to_string() + ":" + std::to_string(session->hostNodeId().second));
    sessions_.insert({session->hostNodeId(), session});
    return true;
  }

  bool ManagerBase::unregisterSessionInternal(const std::shared_ptr<Session> &session) {
    if (this->closed_) {
      return false;
    }
    std::unique_lock lockSession(this->sessionsMutex_); // ManagerBase::unregisterSessionInternal can change sessions_ map.
    if (!sessions_.contains(session->hostNodeId())) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PManager, __func__, "Session does not exist at " +
                        session->hostNodeId().first.to_string() + ":" + std::to_string(session->hostNodeId().second));
      return false;
    }
    sessions_.erase(session->hostNodeId());
    return true;
  }

  bool ManagerBase::disconnectSessionInternal(const NodeID& nodeId) {
    std::unique_lock lockSession(this->sessionsMutex_); // ManagerBase::disconnectSessionInternal can change sessions_ map.
    if (!sessions_.contains(nodeId)) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PManager, __func__, "Session does not exist at " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second));
      return false;
    }
    Logger::logToDebug(LogType::INFO, Log::P2PManager, __func__, "Disconnecting session at " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second));
    // Get a copy of the pointer
    sessions_[nodeId]->close();
    sessions_.erase(nodeId);
    return true;
  }

  std::shared_ptr<Request> ManagerBase::sendRequestTo(const NodeID &nodeId, const std::shared_ptr<const Message>& message) {
    if (this->closed_) return nullptr;
    std::shared_lock lockSession(this->sessionsMutex_); // ManagerBase::sendRequestTo doesn't change sessions_ map.
    if(!sessions_.contains(nodeId)) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PManager, __func__, "Session does not exist at " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second));
      return nullptr;
    }
    auto session = sessions_[nodeId];
    // We can only request ping, info and requestNode to discovery nodes
    if (session->hostType() == NodeType::DISCOVERY_NODE && (message->command() == CommandType::Info ||
                                                            message->command() == CommandType::RequestValidatorTxs)) {
      lockSession.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::INFO, Log::P2PManager, __func__, "Session is discovery, cannot send message");
      return nullptr;
    }
    std::unique_lock lockRequests(this->requestsMutex_);
    requests_[message->id()] = std::make_shared<Request>(message->command(), message->id(), session->hostNodeId(), message);
    session->write(message);
    return requests_[message->id()];
  }

  void ManagerBase::answerSession(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message) {
    if (this->closed_) return;
    std::shared_lock<std::shared_mutex> lockSession(this->sessionsMutex_); // ManagerBase::answerSession doesn't change sessions_ map.
                                                                           // But we still need to be sure that the session io_context doesn't get deleted while we are using it.
    if (auto ptr = session.lock()) {
      ptr->write(message);
    } else {
      Logger::logToDebug(LogType::ERROR, Log::P2PManager, __func__, "Session is no longer valid");
    }
  }

  void ManagerBase::start() {
    this->closed_ = false;
    this->server_->start();
    this->clientfactory_->start();
  }

  void ManagerBase::stop() {
    this->closed_ = true;
    {
      std::unique_lock lock(this->sessionsMutex_);
      for (auto it = sessions_.begin(); it != sessions_.end();) {
        std::weak_ptr<Session> session = std::weak_ptr(it->second);
        it = sessions_.erase(it);
        if (auto sessionPtr = session.lock()) sessionPtr->close();
      }
    }
    this->server_->stop();
    this->clientfactory_->stop();
  }

  std::vector<NodeID> ManagerBase::getSessionsIDs() const {
    std::vector<NodeID> nodes;
    std::shared_lock lock(this->sessionsMutex_);
    for (auto& session : sessions_) {
      nodes.push_back(session.first);
    }
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
    if (this->closed_) return;
    if (address == this->server_->getLocalAddress() && port == this->serverPort_) return; /// Cannot connect to itself.

    {
      std::shared_lock(this->sessionsMutex_);
      if (this->sessions_.contains({address, port})) {
        return; // Node is already connected
      }
    }
    this->clientfactory_->connectToServer(address, port);
  }

  void ManagerBase::ping(const NodeID& nodeId) {
    auto request = std::make_shared<const Message>(RequestEncoder::ping());
    Utils::logToFile("Pinging " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second));
    auto requestPtr = sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) throw std::runtime_error(
          "Failed to send ping to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second)
      );
    requestPtr->answerFuture().wait();
  }

  // TODO: Both ping and requestNodes is a blocking call on .wait()
  // Somehow change to wait_for.
  std::unordered_map<NodeID, NodeType, SafeHash> ManagerBase::requestNodes(const NodeID& nodeId) {
    auto request = std::make_shared<const Message>(RequestEncoder::requestNodes());
    Utils::logToFile("Requesting nodes from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second));
    auto requestPtr = sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__, "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2));
    if (status == std::future_status::timeout) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__, "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " timed out.");
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestNodes(*answerPtr);
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " failed with error: " + e.what()
      );
      return {};
    }
  }

}