#include "p2pmanagerbase.h"

namespace P2P {
  ManagerBase::ManagerBase(const boost::asio::ip::address& hostIp, unsigned short hostPort, NodeType nodeType) : 
    nodeId_(Hash::random()),
    hostIp_(hostIp), 
    hostPort_(hostPort), 
    p2pserver_(std::make_shared<Server>(hostIp_, hostPort_, 2, *this)),
    nodeType_(nodeType)
  {}
  
  void ManagerBase::startServer() {
    std::thread t(&Server::start, p2pserver_);
    t.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!p2pserver_->isRunning()) {
      Utils::logToDebug(Log::P2PManager, __func__, "Server failed to start");
      throw std::runtime_error("Server failed to start");
    }
  }
  
  void ManagerBase::connectToServer(const std::string &host, const unsigned short &port) {
    std::thread clientThread([&, host, port] {
      net::io_context ioc;
      auto client = std::make_shared<ClientSession>(ioc, host, port, *this);
      client->run();
      ioc.run();
      Utils::logToFile("ClientSession thread exitted");
    });
    clientThread.detach();
  }
  
  std::shared_ptr<Request>& ManagerBase::sendMessageTo(const Hash& nodeId, const Message& message) {
    std::unique_lock lockSession(sessionsMutex);
    std::unique_lock lockRequests(requestsMutex);
    if(!sessions_.contains(nodeId)) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + nodeId.hex().get());
      throw std::runtime_error("Session does not exist for " + nodeId.hex().get());
    }
    auto session = sessions_[nodeId];
  
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
    requestPtr->answerFuture().wait();
  }

  std::vector<std::tuple<NodeType, Hash, boost::asio::ip::address, unsigned short>> ManagerBase::requestNodes(const Hash& nodeId) {
    auto request = RequestEncoder::requestNodes();
    Utils::logToFile("Requesting nodes from " + nodeId.hex().get());
    auto requestPtr = sendMessageTo(nodeId, request);
    auto answer = requestPtr->answerFuture();
    answer.wait();
    return AnswerDecoder::requestNodes(answer.get());
  }

  void ManagerBase::stop() {
    this->stopDiscovery();
    std::unique_lock lock(sessionsMutex);
    Utils::logToDebug(Log::P2PManager, __func__, "Stopping P2PManager");
    for(auto& [key, value] : sessions_) {
      value->close();
    }
    sessions_.clear();
    p2pserver_->stop();
  }
}; // namespace P2P