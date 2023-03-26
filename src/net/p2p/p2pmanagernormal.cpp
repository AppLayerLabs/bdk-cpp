#include "p2pmanagernormal.h"
#include "../core/rdpos.h"

namespace P2P {

  void ManagerNormal::broadcastMessage(const Message& message) {
    std::unique_lock broadcastLock(broadcastMutex);
    if (broadcastedMessages_[message.id().toUint64()] > 0) {
      Utils::logToDebug(Log::P2PManager, __func__, "Already broadcasted message " + message.id().hex().get() + " to all nodes. Skipping broadcast.");
      return;
    }

    std::unique_lock sessionsLock(sessionsMutex);
    Utils::logToDebug(Log::P2PManager, __func__, "Broadcasting message " + message.id().hex().get() + " to all nodes. ");
    for (const auto& session : this->sessions_) {
      if (session.second->hostType() == NodeType::NORMAL_NODE) {
        session.second->write(message);
      }
    }
    broadcastedMessages_[message.id().toUint64()] = ++broadcastedMessages_[message.id().toUint64()];
  }

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
        Utils::logToDebug(Log::P2PParser, __func__, "Invalid Request Command Type: " + std::to_string(message.command()) + " from: " + session->hostNodeId().hex().get() + ", closing session.");
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
      case BroadcastValidatorTx:
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

    this->answerSession(session, AnswerEncoder::requestValidatorTxs(message, this->rdpos_->getMempool()));
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
      if (this->rdpos_->addValidatorTx(tx)) {
        this->broadcastMessage(message);
      }
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__, "Invalid txValidatorBroadcast from " + session->hostNodeId().hex().get() + ", error: " + e.what() + " closing session.");
      this->disconnectSession(session->hostNodeId());
    }
  }

  // TODO: Both ping and requestNodes is a blocking call on .wait()
  // Somehow change to wait_for.
  std::vector<TxValidator> ManagerNormal::requestValidatorTxs(const Hash& nodeId) {
    auto request = RequestEncoder::requestValidatorTxs();
    Utils::logToFile("Requesting nodes from " + nodeId.hex().get());
    auto requestPtr = sendMessageTo(nodeId, request);
    if (requestPtr == nullptr) {
      Utils::logToDebug(Log::P2PParser, __func__, "Request to " + nodeId.hex().get() + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      Utils::logToDebug(Log::P2PParser, __func__, "Request to " + nodeId.hex().get() + " timed out.");
      return {};
    }
    try {
      return AnswerDecoder::requestValidatorTxs(answer.get());
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__,
                        "Request to " + nodeId.hex().get() + " failed with error: " + e.what());
      return {};
    }
  }

  void ManagerNormal::broadcastTxValidator(const TxValidator& tx) {
    auto broadcast = BroadcastEncoder::broadcastValidatorTx(tx);
    this->broadcastMessage(broadcast);
    return;
  }
};