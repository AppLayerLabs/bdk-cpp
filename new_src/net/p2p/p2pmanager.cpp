#include "p2pmanager.h"

namespace P2P {
  Manager::Manager(const boost::asio::ip::address& hostIp, unsigned short hostPort, NodeType nodeType) : 
    nodeId_(Hex::fromBytes(Utils::randBytes(32), true).get()),
    hostIp_(hostIp), 
    hostPort_(hostPort), 
    p2pserver_(std::make_shared<Server>(hostIp_, hostPort_, 2, *this)),
    nodeType_(nodeType)
  {}
  
  void Manager::startServer() {
    std::thread t(&Server::start, p2pserver_);
    t.detach();
  }
  
  void Manager::connectToServer(const std::string &host, const unsigned short &port) {
    std::thread clientThread([&, host, port] {
      net::io_context ioc;
      auto client = std::make_shared<ClientSession>(ioc, host, port, *this);
      client->run();
      ioc.run();
      Utils::logToFile("ClientSession thread exitted");
    });
    clientThread.detach();
  }
  
  std::shared_ptr<Request>& Manager::sendMessageTo(std::string nodeId, const Message& message) {
    std::unique_lock lockSession(sessionsMutex);
    std::unique_lock lockRequests(requestsMutex);
    if(!sessions_.contains(nodeId)) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + nodeId);
      throw std::runtime_error("Session does not exist for " + nodeId);
    }
    auto session = sessions_[nodeId];
  
    requests_[message.id()] = std::make_shared<Request>(message.command(), message.id(), session->hostNodeId());
    session->write(message);
    return requests_[message.id()];
  }

  void Manager::answerSession(std::shared_ptr<BaseSession>& session, const Message& message) {
    session->write(message);
  }
  
  bool Manager::registerSession(std::shared_ptr<BaseSession> session) {
    std::unique_lock lock(sessionsMutex);
    if(sessions_.contains(session->hostNodeId())) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session already exists for " + session->hostNodeId() + " at " + session->address().to_string());
      return false;
    } 
    Utils::logToDebug(Log::P2PManager, __func__, "Registering client session for " + session->hostNodeId() + " at " + session->address().to_string());
    sessions_[session->hostNodeId()] = session;
    return true;
  }
  
  bool Manager::unregisterSession(std::shared_ptr<BaseSession> session) {
    std::unique_lock lock(sessionsMutex);
    if(!sessions_.contains(session->hostNodeId())) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + session->hostNodeId() + " at " + session->address().to_string());
      return false;
    } 

    Utils::logToDebug(Log::P2PManager, __func__, "Unregistering client session for " + session->hostNodeId() + " at " + session->address().to_string());
    sessions_.erase(session->hostNodeId());
    return true;
  }
  
  bool Manager::disconnectSession(const std::string& nodeId) {
    std::unique_lock lock(sessionsMutex);
    if(!sessions_.contains(nodeId)) {
      Utils::logToDebug(Log::P2PManager, __func__, "Session does not exist for " + nodeId);
      return false;
    } 
    Utils::logToDebug(Log::P2PManager, __func__, "Disconnecting client session for " + nodeId);
    // Get a copy of the pointer
    sessions_[nodeId]->close();
    sessions_.erase(nodeId);
    return true;
  }
  
  std::vector<std::string> Manager::getSessionsIDs() {
    std::vector<std::string> ret;
    std::shared_lock lock(sessionsMutex);
    for(auto& [key, value] : sessions_) {
      ret.push_back(key);
    }
    return ret;
  }

  void Manager::ping(const std::string& nodeId) {
    auto request = RequestEncoder::ping();
    Utils::logToFile("Pinging " + nodeId);
    auto requestPtr = sendMessageTo(nodeId, request);
    requestPtr->answerFuture().wait();
  }

  std::vector<std::tuple<NodeType, std::string, boost::asio::ip::address, unsigned short>> Manager::requestNodes(const std::string& nodeId) {
    auto request = RequestEncoder::requestNodes();
    Utils::logToFile("Requesting nodes from " + nodeId);
    auto requestPtr = sendMessageTo(nodeId, request);
    auto answer = requestPtr->answerFuture();
    answer.wait();
    return AnswerDecoder::requestNodes(answer.get());
  }

}; // namespace P2P