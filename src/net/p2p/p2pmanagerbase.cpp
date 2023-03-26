#include "p2pmanagerbase.h"

namespace P2P {
  ManagerBase::ManagerBase(const boost::asio::ip::address& hostIp, unsigned short hostPort, NodeType nodeType, unsigned int maxConnections) : 
    nodeId_(Hash::random()),
    hostIp_(hostIp), 
    hostPort_(hostPort), 
    p2pserver_(std::make_shared<Server>(hostIp_, hostPort_, 2, *this)),
    nodeType_(nodeType),
    maxConnections_(maxConnections),
    discoveryWorker(std::make_unique<DiscoveryWorker>(*this))
  {}
  
  void ManagerBase::startServer() {
    if(this->p2pserver_->start()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void ManagerBase::startDiscovery() {
    this->discoveryWorker->start();
  }

  void ManagerBase::stopDiscovery() {
    this->discoveryWorker->stop();
  }

  void ManagerBase::connectToServer(const std::string &host, const unsigned short &port) {
    if (this->hostIp_.to_string() == host && this->hostPort_ == port) {
      Utils::logToDebug(Log::P2PManager, __func__, "Cannot connect to self");
      return;
    }
    std::thread clientThread([&, host, port] {
      net::io_context ioc;
      auto client = std::make_shared<ClientSession>(ioc, host, port, *this);
      client->run();
      ioc.run();
      Utils::logToFile("ClientSession thread exitted");
    });
    clientThread.detach();
  }
  
  std::shared_ptr<Request> ManagerBase::sendMessageTo(const Hash& nodeId, const Message& message) {
    std::unique_lock lockSession(sessionsMutex);
    std::unique_lock lockRequests(requestsMutex);
    if(!sessions_.contains(nodeId)) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + nodeId.hex().get());
      return nullptr;
    }
    auto session = sessions_[nodeId];
    /// We can only request ping, info and requestNode to discovery nodes
    if (session->hostType() == NodeType::DISCOVERY_NODE && (message.command() != CommandType::Ping &&
        message.command() != CommandType::Info &&
        message.command() != CommandType::RequestNodes)) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session is discovery, cannot send message");
      return nullptr;
    }
    requests_[message.id()] = std::make_shared<Request>(message.command(), message.id(), session->hostNodeId());
    session->write(message);
    return requests_[message.id()];
  }

  void ManagerBase::answerSession(std::shared_ptr<BaseSession>& session, const Message& message) {
    session->write(message);
  }
  
  bool ManagerBase::registerSession(std::shared_ptr<BaseSession> session) {
    std::unique_lock lock(sessionsMutex);
    if(sessions_.contains(session->hostNodeId())) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session already exists for " + session->hostNodeId().hex().get() + " at " + session->address().to_string());
      return false;
    } 
    Utils::logToDebug(Log::P2PManager, __func__, "Registering " + std::string((session->connectionType() == ConnectionType::CLIENT) ? "Client" : "Server") + " session for " + session->hostNodeId().hex().get() + " at " + session->address().to_string());
    sessions_[session->hostNodeId()] = session;
    return true;
  }
  
  bool ManagerBase::unregisterSession(std::shared_ptr<BaseSession> session) {
    std::unique_lock lock(sessionsMutex);
    if(!sessions_.contains(session->hostNodeId())) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + session->hostNodeId().hex().get() + " at " + session->address().to_string());
      return false;
    } 

    Utils::logToDebug(Log::P2PManager, __func__, "Unregistering session for " + session->hostNodeId().hex().get() + " at " + session->address().to_string());
    sessions_.erase(session->hostNodeId());
    return true;
  }
  
  bool ManagerBase::disconnectSession(const Hash& nodeId) {
    std::unique_lock lock(sessionsMutex);
    if(!sessions_.contains(nodeId)) {
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
    for(auto& [key, value] : sessions_) {
      ret.push_back(key);
    }
    return ret;
  }

  void ManagerBase::ping(const Hash& nodeId) {
    auto request = RequestEncoder::ping();
    Utils::logToFile("Pinging " + nodeId.hex().get());
    auto requestPtr = sendMessageTo(nodeId, request);
    if (requestPtr == nullptr)
      throw std::runtime_error("Failed to send ping to " + nodeId.hex().get() + ".)");
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
                        "Request to " + nodeId.hex().get() + " failed with error: " + e.what());
      return {};
    }
  }

  void ManagerBase::stop() {
    this->discoveryWorker->stop();
    std::unique_lock lock(sessionsMutex);
    Utils::logToDebug(Log::P2PManager, __func__, "Stopping P2PManager");
    for(auto& [key, value] : sessions_) {
      value->close();
    }
    sessions_.clear();
    p2pserver_->stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}; // namespace P2P