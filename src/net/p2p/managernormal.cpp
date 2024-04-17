/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managernormal.h"
#include "../core/rdpos.h"
#include "../core/storage.h"
#include "../core/state.h"
#include "nodeconns.h"

namespace P2P{
  void ManagerNormal::broadcastMessage(const std::shared_ptr<const Message> message) {
    if (!this->started_) return;
    {
      std::unique_lock broadcastLock(this->broadcastMutex_);
      if (broadcastedMessages_[message->id().toUint64()] > 0) {
        broadcastLock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
        Logger::logToDebug(LogType::DEBUG, Log::P2PManager, __func__,
          "Message " + message->id().hex().get() + " already broadcasted, skipping."
        );
        return;
      } else {
        broadcastedMessages_[message->id().toUint64()]++;
      }
    }
    // ManagerNormal::broadcastMessage doesn't change sessions_ map
    std::shared_lock sessionsLock(this->sessionsMutex_);
    Logger::logToDebug(LogType::INFO, Log::P2PManager, __func__,
      "Broadcasting message " + message->id().hex().get() + " to all nodes. "
    );
    for (const auto& [nodeId, session] : this->sessions_) {
      if (session->hostType() == NodeType::NORMAL_NODE) session->write(message);
    }
  }

  void ManagerNormal::notifyAllMessage(const std::shared_ptr<const Message> message) {
    // ManagerNormal::notifyAllMessage doesn't change sessions_ map
    std::shared_lock sessionsLock(this->sessionsMutex_);
    for (const auto& [nodeId, session] : this->sessions_) {
      if (session->hostType() == NodeType::NORMAL_NODE) session->write(message);
    }
  }

  void ManagerNormal::handleMessage(
    const NodeID &nodeId, const std::shared_ptr<const Message> message
  ) {
    if (!this->started_) return;
    switch (message->type()) {
      case Requesting:
        handleRequest(nodeId, message);
        break;
      case Answering:
        handleAnswer(nodeId, message);
        break;
      case Broadcasting:
        handleBroadcast(nodeId, message);
        break;
      case Notifying:
        handleNotification(nodeId, message);
        break;
      default:
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid message type from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " , closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerNormal::handleRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
      case Ping:
        handlePingRequest(nodeId, message);
        break;
      case Info:
        handleInfoRequest(nodeId, message);
        break;
      case RequestNodes:
        handleRequestNodesRequest(nodeId, message);
        break;
      case RequestValidatorTxs:
        handleTxValidatorRequest(nodeId, message);
        break;
      case RequestTxs:
        handleTxRequest(nodeId, message);
        break;
      default:
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid Request Command Type: " + std::to_string(message->command()) +
                           " from: " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                           ", closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerNormal::handleAnswer(
      const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
      case Ping:
        handlePingAnswer(nodeId, message);
        break;
      case Info:
        handleInfoAnswer(nodeId, message);
        break;
      case RequestNodes:
        handleRequestNodesAnswer(nodeId, message);
        break;
      case RequestValidatorTxs:
        handleTxValidatorAnswer(nodeId, message);
        break;
      case RequestTxs:
        handleTxAnswer(nodeId, message);
        break;
      default:
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid Answer Command Type: " + std::to_string(message->command()) +
                           " from: " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                           " , closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerNormal::handleBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    if (!this->started_) return;
    {
      std::shared_lock broadcastLock(this->broadcastMutex_);
      auto it = broadcastedMessages_.find(message->id().toUint64());
      if (it != broadcastedMessages_.end() && it->second > 0) {
        broadcastLock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
        Logger::logToDebug(LogType::DEBUG, Log::P2PManager, __func__,
          "Already broadcasted message " + message->id().hex().get() +
          " to all nodes. Skipping broadcast."
        );
        return;
      }
    }
    switch (message->command()) {
      case BroadcastValidatorTx:
        handleTxValidatorBroadcast(nodeId, message);
        break;
      case BroadcastTx:
        handleTxBroadcast(nodeId, message);
        break;
      case BroadcastBlock:
        handleBlockBroadcast(nodeId, message);
        break;
      case BroadcastInfo:
        handleInfoBroadcast(nodeId, message);
        break;
      default:
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid Broadcast Command Type: " + std::to_string(message->command()) +
                           " from: " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                           " , closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerNormal::handleNotification(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
      case NotifyInfo:
        handleInfoNotification(nodeId, message);
        break;
      default:
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid Notification Command Type: " + std::to_string(message->command()) +
                           " from: " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                           ", closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerNormal::handlePingRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::ping(*message)) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid ping request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::ping(*message)));
  }

  void ManagerNormal::handleInfoRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    RequestDecoder::info(*message);
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::info(
      *message, this->storage_.latest(), this->options_
    )));
  }

  void ManagerNormal::handleRequestNodesRequest(
      const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::requestNodes(*message)) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid requestNodes request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }

    std::unordered_map<NodeID, NodeType, SafeHash> nodes;

    {
      std::shared_lock lock(this->sessionsMutex_);
      std::transform(sessions_.begin(), sessions_.end(), std::inserter(nodes, nodes.end()),
                     [](const auto& pair){
                       return std::make_pair(pair.first, pair.second->hostType());
                     }
      );
    }
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::requestNodes(*message, nodes)));
  }

  void ManagerNormal::handleTxValidatorRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::requestValidatorTxs(*message)) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid requestValidatorTxs request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::requestValidatorTxs(*message, this->state_.rdposGetMempool())));
  }

  void ManagerNormal::handleTxRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::requestTxs(*message)) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid requestTxs request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::requestTxs(*message, this->state_.getMempool())));
  }

  void ManagerNormal::handlePingAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before doing anything else to avoid waiting for other locks.
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Answer to invalid request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleInfoAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Answer to invalid request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleRequestNodesAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Answer to invalid request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxValidatorAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Answer to invalid request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Answer to invalid request from " + nodeId.first.to_string() + ":" +
                         std::to_string(nodeId.second) + " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxValidatorBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    try {
      auto tx = BroadcastDecoder::broadcastValidatorTx(*message, this->options_.getChainID());
      if (this->state_.addValidatorTx(tx)) this->broadcastMessage(message);
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid txValidatorBroadcast from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , error: " + e.what() + " closing session.");
      this->disconnectSession(nodeId);
    }
  }

  void ManagerNormal::handleTxBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    try {
      auto tx = BroadcastDecoder::broadcastTx(*message, this->options_.getChainID());
      if (!this->state_.addTx(std::move(tx))) this->broadcastMessage(message);
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid txBroadcast from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , error: " + e.what() + " closing session.");
      this->disconnectSession(nodeId);
    }
  }

  void ManagerNormal::handleBlockBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    // We require a lock here because validateNextBlock **throws** if the block is invalid.
    // The reason for locking because for that a processNextBlock race condition can occur,
    // making the same block be accepted, and then rejected, disconnecting the node.
    bool rebroadcast = false;
    try {
      auto block = BroadcastDecoder::broadcastBlock(*message, this->options_.getChainID());
      std::unique_lock lock(this->blockBroadcastMutex_);
      if (this->storage_.blockExists(block.hash())) {
        // If the block is latest()->getNHeight() - 1, we should still rebroadcast it
        if (this->storage_.latest()->getNHeight() - 1 == block.getNHeight()) rebroadcast = true;
        return;
      }
      if (this->state_.validateNextBlock(block)) {
        this->state_.processNextBlock(std::move(block));
        rebroadcast = true;
      }
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid blockBroadcast from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , error: " + e.what() + " closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    if (rebroadcast) this->broadcastMessage(message);
  }

  void ManagerNormal::handleInfoBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    try {
      auto nodeInfo = BroadcastDecoder::broadcastInfo(*message);
      this->nodeConns_.incomingInfo(nodeId, nodeInfo);
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid infoBroadcast from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , error: " + e.what() + " closing session.");
      this->disconnectSession(nodeId);
      return;
    }
  }

  void ManagerNormal::handleInfoNotification(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    try {
      auto nodeInfo = NotificationDecoder::notifyInfo(*message);
      this->nodeConns_.incomingInfo(nodeId, nodeInfo);
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid infoNotification from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " , error: " + e.what() + " closing session.");
      this->disconnectSession(nodeId);
      return;
    }
  }

  // TODO: Both ping and requestNodes is a blocking call on .wait()
  // Somehow change to wait_for.
  std::vector<TxValidator> ManagerNormal::requestValidatorTxs(const NodeID& nodeId) {
    auto request = std::make_shared<const Message>(RequestEncoder::requestValidatorTxs());
    Utils::logToFile("Requesting nodes from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second));
    auto requestPtr = this->sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      Logger::logToDebug(LogType::WARNING, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " failed."
      );
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      Logger::logToDebug(LogType::WARNING, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " timed out."
      );
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestValidatorTxs(*answerPtr, this->options_.getChainID());
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " failed with error: " + e.what()
      );
      return {};
    }
  }

  std::vector<TxBlock> ManagerNormal::requestTxs(const NodeID& nodeId) {
    auto request = std::make_shared<const Message>(RequestEncoder::requestTxs());
    Utils::logToFile("Requesting nodes from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second));
    auto requestPtr = this->sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      Logger::logToDebug(LogType::WARNING, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " failed."
      );
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      Logger::logToDebug(LogType::WARNING, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " timed out."
      );
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestTxs(*answerPtr, this->options_.getChainID());
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " failed with error: " + e.what()
      );
      return {};
    }
  }

  NodeInfo ManagerNormal::requestNodeInfo(const NodeID& nodeId) {
    auto request = std::make_shared<const Message>(RequestEncoder::info(this->storage_.latest(), this->options_));
    Utils::logToFile("Requesting nodes from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second));
    auto requestPtr = sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      Logger::logToDebug(LogType::WARNING, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " failed."
      );
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      Logger::logToDebug(LogType::WARNING, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " timed out."
      );
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::info(*answerPtr);
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
        "Request to " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " failed with error: " + e.what()
      );
      return {};
    }
  }

  void ManagerNormal::broadcastTxValidator(const TxValidator& tx) {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastValidatorTx(tx));
    this->broadcastMessage(broadcast);
  }

  void ManagerNormal::broadcastTxBlock(const TxBlock &txBlock) {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastTx(txBlock));
    this->broadcastMessage(broadcast);
  }

  void ManagerNormal::broadcastBlock(const std::shared_ptr<const Block> block) {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastBlock(block));
    this->broadcastMessage(broadcast);
  }

  void ManagerNormal::broadcastInfo() {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastInfo(this->storage_.latest(), this->options_));
    this->broadcastMessage(broadcast);
  }

  void ManagerNormal::notifyAllInfo() {
    auto notifyall = std::make_shared<const Message>(NotificationEncoder::notifyInfo(this->storage_.latest(), this->options_));
    this->notifyAllMessage(notifyall);
  }
};

