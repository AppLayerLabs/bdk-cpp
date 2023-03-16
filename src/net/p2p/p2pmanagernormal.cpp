#include "p2pmanagernormal.h"
#include "../core/rdpos.h"

namespace P2P {

  void ManagerNormal::handleMessage(std::shared_ptr<BaseSession> session, const Message message) {
    switch (message.type()) {
      case Requesting:
        handleRequest(session, message);
        break;
      case Answering:
        handleAnswer(session, message);
        break;
      case Broadcasting:
        handleBroadcast(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__, "Invalid message type from " + session->hostNodeId().hex().get() + ", closing session.");
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void ManagerNormal::handleRequest(std::shared_ptr<BaseSession>& session, const Message& message) {
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
      case RequestValidatorTxs:
        handleTxValidatorRequest(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__, "Invalid Request Command Type from " + session->hostNodeId().hex().get() + ", closing session.");
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void ManagerNormal::handleAnswer(std::shared_ptr<BaseSession>& session, const Message& message) {
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
      case RequestValidatorTxs:
        handleTxValidatorAnswer(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__, "Invalid Answer Command Type from " + session->hostNodeId().hex().get() + ", closing session.");
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void ManagerNormal::handleBroadcast(std::shared_ptr<BaseSession>& session, const Message& message) {
    switch (message.command()) {
      case Ping:
        handleTxValidatorBroadcast(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__, "Invalid Broadcast Command Type from " + session->hostNodeId().hex().get() + ", closing session.");
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void ManagerNormal::handlePingRequest(std::shared_ptr<BaseSession>& session, const Message& message) {
    if (!RequestDecoder::ping(message)) {
      Utils::logToDebug(Log::P2PParser, __func__, "Invalid ping request from " + session->hostNodeId().hex().get() + " closing session.");
      this->disconnectSession(session->hostNodeId());
      return;
    }
    this->answerSession(session, AnswerEncoder::ping(message));
  }

  void ManagerNormal::handleRequestNodesRequest(std::shared_ptr<BaseSession>& session, const Message& message) {
    if (!RequestDecoder::requestNodes(message)) {
      Utils::logToDebug(Log::P2PParser, __func__, "Invalid requestNodes request, closing session.");
      this->disconnectSession(session->hostNodeId());
      return;
    }

    std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> nodes;
    {
      std::unique_lock lock(requestsMutex);
      for (const auto& session : this->sessions_) {
        nodes[session.second->hostNodeId()] = std::make_tuple(session.second->hostType(), session.second->address(), session.second->hostServerPort());
      }
    }
    this->answerSession(session, AnswerEncoder::requestNodes(message, nodes));
  }

  void ManagerNormal::handleTxValidatorRequest(std::shared_ptr<BaseSession>& session, const Message& message) {
    if (!RequestDecoder::requestValidatorTxs(message)) {
      Utils::logToDebug(Log::P2PParser, __func__, "Invalid requestValidatorTxs request, closing session.");
      this->disconnectSession(session->hostNodeId());
      return;
    }

    this->answerSession(session, AnswerEncoder::requestValidatorTxs(message, this->rdpos->getMempool()));
  }

  void ManagerNormal::handlePingAnswer(std::shared_ptr<BaseSession>& session, const Message& message) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__, "Answer to invalid request from " + session->hostNodeId().hex().get());
      this->disconnectSession(session->hostNodeId());
      return;
    }
    requests_[message.id()]->setAnswer(message);
  }

  void ManagerNormal::handleRequestNodesAnswer(std::shared_ptr<BaseSession>& session, const Message& message) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__, "Answer to invalid request from " + session->hostNodeId().hex().get());
      this->disconnectSession(session->hostNodeId());
      return;
    }
    requests_[message.id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxValidatorAnswer(std::shared_ptr<BaseSession>& session, const Message& message) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__, "Answer to invalid request from " + session->hostNodeId().hex().get());
      this->disconnectSession(session->hostNodeId());
      return;
    }
    requests_[message.id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxValidatorBroadcast(std::shared_ptr<BaseSession>& session, const Message& message) {
    // TODO: Add a filter to broadcast any message to all nodes if message was not previously know.
    try {
      auto tx = BroadcastDecoder::broadcastValidatorTx(message);
      if (this->rdpos->addValidatorTx(tx)) {
        // TODO: Broadcast to all nodes.
      }
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__, "Invalid txValidatorBroadcast from " + session->hostNodeId().hex().get() + " closing session.");
      this->disconnectSession(session->hostNodeId());
    }
  }

  // TODO: Both ping and requestNodes is a blocking call on .wait()
  // Somehow change to wait_for.
  std::vector<TxValidator> ManagerNormal::requestValidatorTxs(const Hash& nodeId) {
    auto request = RequestEncoder::requestValidatorTxs();
    Utils::logToFile("Requesting nodes from " + nodeId.hex().get());
    auto requestPtr = sendMessageTo(nodeId, request);
    auto answer = requestPtr->answerFuture();
    answer.wait();
    return AnswerDecoder::requestValidatorTxs(answer.get());
  }
};