#include "p2pmanager.h"


namespace P2P {

  void Manager::handleMessage(std::shared_ptr<BaseSession> session, const Message message) {
    switch (message.type()) {
      case Requesting:
        handleRequest(session, message);
        break;
      case Answering:
        handleAnswer(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__, "Invalid message type from " + Hex::fromBytes(session->hostNodeId()).get() + ", closing session.");
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void Manager::handleRequest(std::shared_ptr<BaseSession>& session, const Message& message) {
    switch (message.command()) {
      case Ping:
        handlePingRequest(session, message);
        break;
      case Info:
        // handleInfoRequest(session, message);
        break;
      case RequestNodes:
        handleRequestNodesRequest(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__, "Invalid Request Command Type from " + Hex::fromBytes(session->hostNodeId()).get() + ", closing session.");
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void Manager::handleAnswer(std::shared_ptr<BaseSession>& session, const Message& message) {
    switch (message.command()) {
      case Ping:
        handlePingAnswer(session, message);
        break;
      case Info:
        // handleInfoAnswer(session, message);
        break;
      case RequestNodes:
        handleRequestNodesAnswer(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__, "Invalid Answer Command Type from " + Hex::fromBytes(session->hostNodeId()).get() + ", closing session.");
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void Manager::handlePingRequest(std::shared_ptr<BaseSession>& session, const Message& message) {
    if (!RequestDecoder::ping(message)) {
      Utils::logToDebug(Log::P2PParser, __func__, "Invalid ping request from " + Hex::fromBytes(session->hostNodeId()).get() + " closing session.");
      this->disconnectSession(session->hostNodeId());
      return;
    }
    this->answerSession(session, AnswerEncoder::ping(message));
  }

  void Manager::handleRequestNodesRequest(std::shared_ptr<BaseSession>& session, const Message& message) {
    if (!RequestDecoder::requestNodes(message)) {
      Utils::logToDebug(Log::P2PParser, __func__, "Invalid requestNodes request, closing session.");
      this->disconnectSession(session->hostNodeId());
      return;
    }

    std::vector<std::tuple<NodeType, std::string, boost::asio::ip::address, unsigned short>> nodes;
    {
      std::unique_lock lock(requestsMutex);
      for (const auto& session : this->sessions_) {
        nodes.emplace_back(std::make_tuple(session.second->hostType(),session.second->hostNodeId(),session.second->address(), session.second->hostServerPort()));
      }
    }
    this->answerSession(session, AnswerEncoder::requestNodes(message, nodes));
  }

  void Manager::handlePingAnswer(std::shared_ptr<BaseSession>& session, const Message& message) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__, "Answer to invalid request from " + Hex::fromBytes(session->hostNodeId()).get());
      this->disconnectSession(session->hostNodeId());
      return;
    }
    requests_[message.id()]->setAnswer(message);
  }

  void Manager::handleRequestNodesAnswer(std::shared_ptr<BaseSession>& session, const Message& message) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__, "Answer to invalid request from " + Hex::fromBytes(session->hostNodeId()).get());
      this->disconnectSession(session->hostNodeId());
      return;
    }
    std::cout << "Setting answer to request: " << message.id() << " " << message.size() << std::endl;
    requests_[message.id()]->setAnswer(message);
  }
};