/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managernormal.h"
#include "../core/rdpos.h"
#include "../core/storage.h"
#include "../core/state.h"
#include "nodeconns.h"

namespace P2P{

  void ManagerNormal::sendMessageToAll(const std::shared_ptr<const Message> message, const std::optional<NodeID>& originalSender) {
    std::unordered_set<NodeID, SafeHash> peerMap;
    if (originalSender) {
      std::optional<NodeInfo> optionalNodeInfo = this->nodeConns_.getNodeInfo(originalSender.value());
      if (optionalNodeInfo) for (const auto& nodeId : optionalNodeInfo.value().peers()) peerMap.emplace(nodeId);
    }
    std::shared_lock sessionsLock(this->sessionsMutex_);
    for (const auto& [nodeId, session] : this->sessions_) {
      if (session->hostType() == NodeType::NORMAL_NODE && !peerMap.count(nodeId)) session->write(message);
    }
  }

  void ManagerNormal::handleMessage(
    const NodeID &nodeId, const std::shared_ptr<const Message> message
  ) {
    if (!this->started_) return;
    try {
      switch (message->type()) {
        case Requesting:
          handleRequest(nodeId, message);
          break;
        case Answering:
          handleAnswer(nodeId, message);
          break;
        case Broadcasting:
          this->broadcaster_.handleBroadcast(nodeId, message);
        case Notifying:
          handleNotification(nodeId, message);
          break;
        default:
          throw DynamicException("Invalid message type: " + std::to_string(message->command()));
          break;
      }
    } catch (std::exception const& ex) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__, "Closing session to " + toString(nodeId) + ": " + ex.what());
      this->disconnectSession(nodeId);
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
      case RequestBlock:
        handleRequestBlockRequest(nodeId, message);
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
      case RequestBlock:
        handleRequestBlockAnswer(nodeId, message);
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
      *message, this->storage_.latest(), this->nodeConns_.getConnectedWithNodeType(), this->options_
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

  void ManagerNormal::handleRequestBlockRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    uint64_t height = RequestDecoder::requestBlock(*message);
    if (this->storage_.blockExists(height)) {
      const auto& requestedBlock = *(this->storage_.getBlock(height));
      this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::requestBlock(*message, requestedBlock)));
    } else {
      // We don't have it, so send a 0-byte size serialized block back to signal it
      this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::requestBlock(*message, {})));
    }
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

  void ManagerNormal::handleRequestBlockAnswer(
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
    auto request = std::make_shared<const Message>(RequestEncoder::info(this->storage_.latest(), this->nodeConns_.getConnectedWithNodeType(), this->options_));
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

  /**
   * Request a block to a peer.
   * @param nodeId The ID of the node to request.
   * @param height The block height to request.
   * @return The requested block.
   */
  std::optional<FinalizedBlock> ManagerNormal::requestBlock(const NodeID &nodeId, const uint64_t& height) {
    auto request = std::make_shared<const Message>(RequestEncoder::requestBlock(height));
    auto requestPtr = sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      Logger::logToDebug(LogType::WARNING, Log::P2PParser, __func__,
        "RequestBlock to " + toString(nodeId) + " failed."
      );
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(10)); // 10s timeout.
    if (status == std::future_status::timeout) {
      Logger::logToDebug(LogType::WARNING, Log::P2PParser, __func__,
        "RequestBlock to " + toString(nodeId) + " timed out."
      );
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestBlock(*answerPtr, this->options_.getChainID());
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
        "RequestBlock to " + toString(nodeId) + " failed with error: " + e.what()
      );
      return {};
    }
  }

  void ManagerNormal::broadcastTxValidator(const TxValidator& tx) { this->broadcaster_.broadcastTxValidator(tx); }

  void ManagerNormal::broadcastTxBlock(const TxBlock& txBlock) { this->broadcaster_.broadcastTxBlock(txBlock); }

  void ManagerNormal::broadcastBlock(const std::shared_ptr<const FinalizedBlock>& block) { this->broadcaster_.broadcastBlock(block); }

  void ManagerNormal::broadcastInfo() { this->broadcaster_.broadcastInfo(); }

  void ManagerNormal::notifyAllInfo() {
    auto notifyall = std::make_shared<const Message>(
      NotificationEncoder::notifyInfo(
        this->storage_.latest(),
        this->nodeConns_.getConnectedWithNodeType(),
        this->options_
      )
    );
    this->sendMessageToAll(notifyall, {});
  }
};

