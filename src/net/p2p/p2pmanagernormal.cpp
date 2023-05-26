#include "p2pmanagernormal.h"
#include "../core/rdpos.h"
#include "../core/storage.h"
#include "../core/state.h"

namespace P2P {
  void ManagerNormal::broadcastMessage(const Message& message) {
    // TODO: Improve "broadcast" ordering:
    // All broadcast requests currently has to pass through broadcastMessage
    // In order to check if the messagas was previously received.
    // Currently, it is uniquely locking for every single request.
    // This is utterly inefficient, and should be improved.

    // TODO: **URGENT** Sometimes things segfault on this line
    // With the following error: __pthread_mutex_lock: Assertion `mutex->__data.__owner == 0' failed.
    // This means that the mutex doesn't exists anymore
    // Somehow the BaseSessiono is calling back ManagerNormal after is has been destroyed!
    if (this->closed_) return;
    {
      std::unique_lock broadcastLock(broadcastMutex);
      if (broadcastedMessages_[message.id().toUint64()] > 0) {
        Utils::logToDebug(Log::P2PManager, __func__,
          "Message " + message.id().hex().get() + " already broadcasted, skipping."
        );
        return;
      } else {
        broadcastedMessages_[message.id().toUint64()]++;
      }
    }
    // ManagerNormal::broadcastMessage doesn't change sessions_ map
    std::shared_lock sessionsLock(sessionsMutex);
    Utils::logToDebug(Log::P2PManager, __func__,
      "Broadcasting message " + message.id().hex().get() + " to all nodes. "
    );
    for (const auto& session : this->sessions_) {
      if (session.second->hostType() == NodeType::NORMAL_NODE) session.second->write(message);
    }
  }

  void ManagerNormal::handleMessage(
    std::shared_ptr<BaseSession> session, const Message message
  ) {
    if (this->closed_) return;
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
        Utils::logToDebug(
          Log::P2PParser, __func__, "Invalid message type from "
          + session->hostNodeId().hex().get() + ", closing session."
        );
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void ManagerNormal::handleRequest(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    switch (message.command()) {
      case Ping:
        handlePingRequest(session, message);
        break;
      case Info:
        handleInfoRequest(session, message);
        break;
      case RequestNodes:
        handleRequestNodesRequest(session, message);
        break;
      case RequestValidatorTxs:
        handleTxValidatorRequest(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__,
          "Invalid Request Command Type: " + std::to_string(message.command())
          + " from: " + session->hostNodeId().hex().get() + ", closing session."
        );
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void ManagerNormal::handleAnswer(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    switch (message.command()) {
      case Ping:
        handlePingAnswer(session, message);
        break;
      case Info:
        handleInfoAnswer(session, message);
        break;
      case RequestNodes:
        handleRequestNodesAnswer(session, message);
        break;
      case RequestValidatorTxs:
        handleTxValidatorAnswer(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__,
          "Invalid Answer Command Type from "
          + session->hostNodeId().hex().get() + ", closing session."
        );
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void ManagerNormal::handleBroadcast(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    if (this->closed_) return;
    {
      std::shared_lock broadcastLock(broadcastMutex);
      auto it = broadcastedMessages_.find(message.id().toUint64());
      if (it != broadcastedMessages_.end()) {
        if (it->second > 0) {
          Utils::logToDebug(Log::P2PManager, __func__,
            "Already broadcasted message " + message.id().hex().get() +
            " to all nodes. Skipping broadcast."
          );
          return;
        }
      }
    }
    switch (message.command()) {
      case BroadcastValidatorTx:
        handleTxValidatorBroadcast(session, message);
        break;
      case BroadcastTx:
        handleTxBroadcast(session, message);
        break;
      case BroadcastBlock:
        handleBlockBroadcast(session, message);
        break;
      default:
        Utils::logToDebug(Log::P2PParser, __func__,
          "Invalid Broadcast Command Type from "
          + session->hostNodeId().hex().get() + ", closing session."
        );
        this->disconnectSession(session->hostNodeId());
        break;
    }
  }

  void ManagerNormal::handlePingRequest(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    if (!RequestDecoder::ping(message)) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Invalid ping request from "
        + session->hostNodeId().hex().get() + " closing session."
      );
      this->disconnectSession(session->hostNodeId());
      return;
    }
    this->answerSession(session, AnswerEncoder::ping(message));
  }

  void ManagerNormal::handleInfoRequest(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    RequestDecoder::info(message);
    this->answerSession(session, AnswerEncoder::info(
      message, this->storage_->latest(), this->options
    ));
  }

  void ManagerNormal::handleRequestNodesRequest(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    if (!RequestDecoder::requestNodes(message)) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Invalid requestNodes request, closing session."
      );
      this->disconnectSession(session->hostNodeId());
      return;
    }

    std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> nodes;
    {
      std::shared_lock lock(sessionsMutex);
      for (const auto& session : this->sessions_) {
        nodes[session.second->hostNodeId()] = std::make_tuple(
          session.second->hostType(), session.second->address(), session.second->hostServerPort()
        );
      }
    }
    this->answerSession(session, AnswerEncoder::requestNodes(message, nodes));
  }

  void ManagerNormal::handleTxValidatorRequest(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    if (!RequestDecoder::requestValidatorTxs(message)) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Invalid requestValidatorTxs request, closing session."
      );
      this->disconnectSession(session->hostNodeId());
      return;
    }
    this->answerSession(session, AnswerEncoder::requestValidatorTxs(message, this->rdpos_->getMempool()));
  }

  void ManagerNormal::handlePingAnswer(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Answer to invalid request from " + session->hostNodeId().hex().get()
      );
      this->disconnectSession(session->hostNodeId());
      return;
    }
    requests_[message.id()]->setAnswer(message);
  }

  void ManagerNormal::handleInfoAnswer(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Answer to invalid request from " + session->hostNodeId().hex().get()
      );
      this->disconnectSession(session->hostNodeId());
      return;
    }
    requests_[message.id()]->setAnswer(message);
  }

  void ManagerNormal::handleRequestNodesAnswer(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Answer to invalid request from " + session->hostNodeId().hex().get()
      );
      this->disconnectSession(session->hostNodeId());
      return;
    }
    requests_[message.id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxValidatorAnswer(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    std::unique_lock lock(requestsMutex);
    if (!requests_.contains(message.id())) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Answer to invalid request from " + session->hostNodeId().hex().get()
      );
      this->disconnectSession(session->hostNodeId());
      return;
    }
    requests_[message.id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxValidatorBroadcast(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    try {
      auto tx = BroadcastDecoder::broadcastValidatorTx(message, this->options->getChainID());
      if (this->state_->addValidatorTx(tx)) this->broadcastMessage(message);
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Invalid txValidatorBroadcast from " + session->hostNodeId().hex().get()
        + ", error: " + e.what() + " closing session."
      );
      this->disconnectSession(session->hostNodeId());
    }
  }

  void ManagerNormal::handleTxBroadcast(
    std::shared_ptr<BaseSession>& session, const Message& message
  ) {
    try {
      auto tx = BroadcastDecoder::broadcastTx(message, this->options->getChainID());
      if (!this->state_->addTx(std::move(tx))) this->broadcastMessage(message);
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Invalid txBroadcast from " + session->hostNodeId().hex().get()
        + ", error: " + e.what() + " closing session."
      );
      this->disconnectSession(session->hostNodeId());
    }
  }

  void ManagerNormal::handleBlockBroadcast(
    std::shared_ptr<BaseSession> &session, const P2P::Message &message
  ) {
    // We require a lock here because validateNextBlock **throws** if the block is invalid.
    // The reason for locking because for that a processNextBlock race condition can occur,
    // making the same block be accepted, and then rejected, disconnecting the node.
    bool rebroadcast = false;
    try {
      auto block = BroadcastDecoder::broadcastBlock(message, this->options->getChainID());
      std::unique_lock lock(this->blockBroadcastMutex);
      if (this->storage_->blockExists(block.hash())) {
        // If the block is latest()->getNHeight() - 1, we should still rebroadcast it
        if (this->storage_->latest()->getNHeight() - 1 == block.getNHeight()) rebroadcast = true;
        return;
      }
      if (this->state_->validateNextBlock(block)) {
        this->state_->processNextBlock(std::move(block));
        this->broadcastMessage(message);
        rebroadcast = true;
      }
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Invalid blockBroadcast from " + session->hostNodeId().hex().get()
        + ", error: " + e.what() + " closing session."
      );
      this->disconnectSession(session->hostNodeId());
    }
    if (rebroadcast) this->broadcastMessage(message);
  }

  // TODO: Both ping and requestNodes is a blocking call on .wait()
  // Somehow change to wait_for.
  std::vector<TxValidator> ManagerNormal::requestValidatorTxs(const Hash& nodeId) {
    auto request = RequestEncoder::requestValidatorTxs();
    Utils::logToFile("Requesting nodes from " + nodeId.hex().get());
    auto requestPtr = sendMessageTo(nodeId, request);
    if (requestPtr == nullptr) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Request to " + nodeId.hex().get() + " failed."
      );
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Request to " + nodeId.hex().get() + " timed out."
      );
      return {};
    }
    try {
      return AnswerDecoder::requestValidatorTxs(answer.get(), this->options->getChainID());
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Request to " + nodeId.hex().get() + " failed with error: " + e.what()
      );
      return {};
    }
  }

  NodeInfo ManagerNormal::requestNodeInfo(const Hash& nodeId) {
    auto request = RequestEncoder::info(this->storage_->latest(), this->options);
    Utils::logToFile("Requesting nodes from " + nodeId.hex().get());
    auto requestPtr = sendMessageTo(nodeId, request);
    if (requestPtr == nullptr) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Request to " + nodeId.hex().get() + " failed."
      );
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Request to " + nodeId.hex().get() + " timed out."
      );
      return {};
    }
    try {
      return AnswerDecoder::info(answer.get());
    } catch (std::exception &e) {
      Utils::logToDebug(Log::P2PParser, __func__,
        "Request to " + nodeId.hex().get() + " failed with error: " + e.what()
      );
      return {};
    }
  }

  void ManagerNormal::broadcastTxValidator(const TxValidator& tx) {
    auto broadcast = BroadcastEncoder::broadcastValidatorTx(tx);
    this->broadcastMessage(broadcast);
    return;
  }

  void ManagerNormal::broadcastTxBlock(const TxBlock &txBlock) {
    auto broadcast = BroadcastEncoder::broadcastTx(txBlock);
    this->broadcastMessage(broadcast);
    return;
  }

  void ManagerNormal::broadcastBlock(const std::shared_ptr<const Block> block) {
    auto broadcast = BroadcastEncoder::broadcastBlock(block);
    this->broadcastMessage(broadcast);
    return;
  }
};

