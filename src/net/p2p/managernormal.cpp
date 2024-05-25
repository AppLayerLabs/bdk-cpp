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
      peerMap.emplace(originalSender.value());
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
          break;
        case Notifying:
          handleNotification(nodeId, message);
          break;
        default:
          throw DynamicException("Invalid message type: " + std::to_string(message->command()));
          break;
      }
    } catch (std::exception const& ex) {
      LOGERROR("Closing session to " + toString(nodeId) + ": " + ex.what());
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
        LOGERROR("Invalid Request Command Type: " + std::to_string(message->command()) +
                           " from: " + toString(nodeId) +
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
        LOGERROR("Invalid Answer Command Type: " + std::to_string(message->command()) +
                           " from: " + toString(nodeId) +
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
        LOGERROR("Invalid Notification Command Type: " + std::to_string(message->command()) +
                           " from: " + toString(nodeId) +
                           ", closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerNormal::handlePingRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::ping(*message)) {
      LOGERROR("Invalid ping request from " + toString(nodeId) +
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
      LOGERROR("Invalid requestNodes request from " + toString(nodeId) +
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
      LOGERROR("Invalid requestValidatorTxs request from " + toString(nodeId) +
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
      LOGERROR("Invalid requestTxs request from " + toString(nodeId) +
                         " , closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::requestTxs(*message, this->state_.getMempool())));
  }

  void ManagerNormal::handleRequestBlockRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    uint64_t height = 0, heightEnd = 0, bytesLimit = 0;
    RequestDecoder::requestBlock(*message, height, heightEnd, bytesLimit);
    std::vector<std::shared_ptr<const FinalizedBlock>> requestedBlocks;
    uint64_t bytesSpent = 0;
    for (uint64_t heightToAdd = height; heightToAdd <= heightEnd; ++heightToAdd) {
      if (!this->storage_.blockExists(heightToAdd)) break; // Stop at first block in the requested range that we don't have.
      requestedBlocks.push_back(this->storage_.getBlock(heightToAdd));
      bytesSpent += requestedBlocks.back()->getSize();
      LOGDEBUG("Uploading block " + std::to_string(heightToAdd) + " to " + toString(nodeId)
                         + " (" + std::to_string(bytesSpent) + "/" + std::to_string(bytesLimit) + " bytes)");
      if (bytesSpent >= bytesLimit) break; // bytesLimit reached so stop appending blocks to the answer
    }
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::requestBlock(*message, requestedBlocks)));
  }

  void ManagerNormal::handlePingAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before doing anything else to avoid waiting for other locks.
      LOGERROR("Answer to invalid request from " + toString(nodeId) +
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
      LOGERROR("Answer to invalid request from " + toString(nodeId) +
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
      LOGERROR("Answer to invalid request from " + toString(nodeId) +
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
      LOGERROR("Answer to invalid request from " + toString(nodeId) +
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
      LOGERROR("Answer to invalid request from " + nodeId.first.to_string() + ":" +
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
      LOGERROR("Answer to invalid request from " + nodeId.first.to_string() + ":" +
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
      NodeType nodeType;
      {
        std::shared_lock sessionsLock(this->sessionsMutex_);
        auto it = sessions_.find(nodeId);
        if (it != sessions_.end()) {
          nodeType = it->second->hostType();
        } else {
          // This actually does happen: since this message is posted to a thread pool for processing after receipt, the
          //   session with the nodeId may be gone at this point. If so, we won't have the nodeType to append to the
          //   node connection's data record at NodeConns and, anyway, if we no longer have a connection to it, then
          //   that node is no longer relevant. So we don't need to refresh it.
          return;
        }
      }
      auto nodeInfo = NotificationDecoder::notifyInfo(*message);
      this->nodeConns_.incomingInfo(nodeId, nodeInfo, nodeType);
    } catch (std::exception &e) {
      LOGERROR("Invalid infoNotification from " + toString(nodeId) +
                         " , error: " + e.what() + " closing session.");
      this->disconnectSession(nodeId);
      return;
    }
  }

  // TODO: Both ping and requestNodes is a blocking call on .wait()
  // Somehow change to wait_for.
  std::vector<TxValidator> ManagerNormal::requestValidatorTxs(const NodeID& nodeId) {
    auto request = std::make_shared<const Message>(RequestEncoder::requestValidatorTxs());
    LOGTRACE("Requesting nodes from " + toString(nodeId));
    auto requestPtr = this->sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      LOGWARNING("Request to " + toString(nodeId) + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      LOGWARNING("Request to " + toString(nodeId) + " timed out.");
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestValidatorTxs(*answerPtr, this->options_.getChainID());
    } catch (std::exception &e) {
      LOGERROR("Request to " + toString(nodeId) + " failed with error: " + e.what());
      return {};
    }
  }

  std::vector<TxBlock> ManagerNormal::requestTxs(const NodeID& nodeId) {
    auto request = std::make_shared<const Message>(RequestEncoder::requestTxs());
    LOGTRACE("Requesting nodes from " + toString(nodeId));
    auto requestPtr = this->sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      LOGWARNING("Request to " + toString(nodeId) + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      LOGWARNING("Request to " + toString(nodeId) + " timed out.");
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestTxs(*answerPtr, this->options_.getChainID());
    } catch (std::exception &e) {
      LOGERROR("Request to " + toString(nodeId) + " failed with error: " + e.what());
      return {};
    }
  }

  NodeInfo ManagerNormal::requestNodeInfo(const NodeID& nodeId) {
    auto request = std::make_shared<const Message>(RequestEncoder::info(this->storage_.latest(), this->nodeConns_.getConnectedWithNodeType(), this->options_));
    LOGTRACE("Requesting nodes from " + toString(nodeId));
    auto requestPtr = sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      LOGWARNING("Request to " + toString(nodeId) + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(2)); // 2000ms timeout.
    if (status == std::future_status::timeout) {
      LOGWARNING("Request to " + toString(nodeId) + " timed out.");
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::info(*answerPtr);
    } catch (std::exception &e) {
      LOGERROR("Request to " + toString(nodeId) + " failed with error: " + e.what());
      return {};
    }
  }

  std::vector<FinalizedBlock> ManagerNormal::requestBlock(
    const NodeID &nodeId, const uint64_t& height, const uint64_t& heightEnd, const uint64_t& bytesLimit
  ) {
    auto request = std::make_shared<const Message>(RequestEncoder::requestBlock(height, heightEnd, bytesLimit));
    auto requestPtr = sendRequestTo(nodeId, request);
    if (requestPtr == nullptr) {
      LOGWARNING("RequestBlock to " + toString(nodeId) + " failed.");
      return {};
    }
    auto answer = requestPtr->answerFuture();
    auto status = answer.wait_for(std::chrono::seconds(60)); // 60s timeout.
    if (status == std::future_status::timeout) {
      LOGWARNING("RequestBlock to " + toString(nodeId) + " timed out.");
      return {};
    }
    try {
      auto answerPtr = answer.get();
      return AnswerDecoder::requestBlock(*answerPtr, this->options_.getChainID());
    } catch (std::exception &e) {
      LOGERROR("RequestBlock to " + toString(nodeId) + " failed with error: " + e.what());
      return {};
    }
  }

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

