/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managernormal.h"
#include "../core/rdpos.h"
#include "../core/storage.h"
#include "../core/state.h"

namespace P2P{
  void ManagerNormal::broadcastMessage(const std::shared_ptr<const Message> message) {
    if (this->closed_) return;
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

  void ManagerNormal::handleMessage(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message> message
  ) {
    if (this->closed_) return;
    switch (message->type()) {
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
        if (auto sessionPtr = session.lock()) {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
            "Invalid message type from " + sessionPtr->hostNodeId().first.to_string() + ":" +
            std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
          );

          this->disconnectSession(sessionPtr->hostNodeId());
        } else {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
            "Invalid message type from unknown session, closing session."
          );
        }
        break;
    }
  }

  void ManagerNormal::handleRequest(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
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
      case RequestTxs:
        handleTxRequest(session, message);
        break;
      default:
        if (auto sessionPtr = session.lock()) {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
            "Invalid Request Command Type: " + std::to_string(message->command())
            + " from: " + sessionPtr->hostNodeId().first.to_string() + ":" +
              std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
          );
          this->disconnectSession(sessionPtr->hostNodeId());
        } else {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
            "Invalid Request Command Type: " + std::to_string(message->command())
            + " from unknown session, closing session."
          );
        }
        break;
    }
  }

  void ManagerNormal::handleAnswer(
      std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
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
      case RequestTxs:
        handleTxAnswer(session, message);
        break;
      default:
        if (auto sessionPtr = session.lock()) {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
            "Invalid Answer Command Type: " + std::to_string(message->command())
            + " from: " + sessionPtr->hostNodeId().first.to_string() + ":" +
              std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
          );
          this->disconnectSession(sessionPtr->hostNodeId());
        } else {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
            "Invalid Answer Command Type: " + std::to_string(message->command())
            + " from unknown session, closing session."
          );
        }
        break;
    }
  }

  void ManagerNormal::handleBroadcast(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    if (this->closed_) return;
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
        handleTxValidatorBroadcast(session, message);
        break;
      case BroadcastTx:
        handleTxBroadcast(session, message);
        break;
      case BroadcastBlock:
        handleBlockBroadcast(session, message);
        break;
      default:
        if (auto sessionPtr = session.lock()) {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
            "Invalid Broadcast Command Type: " + std::to_string(message->command())
            + " from: " + sessionPtr->hostNodeId().first.to_string() + ":" +
              std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
          );
          this->disconnectSession(sessionPtr->hostNodeId());
        } else {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
            "Invalid Broadcast Command Type: " + std::to_string(message->command())
            + " from unknown session, closing session."
          );
        }
        break;
    }
  }

  void ManagerNormal::handlePingRequest(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::ping(*message)) {
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid ping request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid ping request from unknown session, closing session."
        );
      }
      return;
    }
    this->answerSession(session, std::make_shared<const Message>(AnswerEncoder::ping(*message)));
  }

  void ManagerNormal::handleInfoRequest(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    RequestDecoder::info(*message);
    this->answerSession(session, std::make_shared<const Message>(AnswerEncoder::info(
      *message, this->storage_.latest(), this->options_
    )));
  }

  void ManagerNormal::handleRequestNodesRequest(
      std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::requestNodes(*message)) {
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid requestNodes request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid requestNodes request from unknown session, closing session."
        );
      }
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
    this->answerSession(session, std::make_shared<const Message>(AnswerEncoder::requestNodes(*message, nodes)));
  }

  void ManagerNormal::handleTxValidatorRequest(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::requestValidatorTxs(*message)) {
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid requestValidatorTxs request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid requestValidatorTxs request from unknown session, closing session."
        );
      }
      return;
    }
    this->answerSession(session, std::make_shared<const Message>(AnswerEncoder::requestValidatorTxs(*message, this->state_.rdposGetMempool())));
  }

  void ManagerNormal::handleTxRequest(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::requestTxs(*message)) {
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid requestTxs request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid requestTxs request from unknown session, closing session."
        );
      }
      return;
    }
    this->answerSession(session, std::make_shared<const Message>(AnswerEncoder::requestTxs(*message, this->state_.getMempool())));
  }

  void ManagerNormal::handlePingAnswer(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before doing anything else to avoid waiting for other locks.
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from unknown session, closing session."
        );
      }
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleInfoAnswer(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from unknown session, closing session."
        );
      }
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleRequestNodesAnswer(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from unknown session, closing session."
        );
      }
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxValidatorAnswer(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from unknown session, closing session."
        );
      }
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxAnswer(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Answer to invalid request from unknown session, closing session."
        );
      }
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerNormal::handleTxValidatorBroadcast(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    try {
      auto tx = BroadcastDecoder::broadcastValidatorTx(*message, this->options_.getChainID());
      if (this->state_.addValidatorTx(tx)) this->broadcastMessage(message);
    } catch (std::exception &e) {
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid txValidatorBroadcast from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , error: " + e.what() + " closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          std::string("Invalid txValidatorBroadcast from unknown session, error: ") + e.what() + " closing session."
        );
      }
    }
  }

  void ManagerNormal::handleTxBroadcast(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    try {
      auto tx = BroadcastDecoder::broadcastTx(*message, this->options_.getChainID());
      if (!this->state_.addTx(std::move(tx))) this->broadcastMessage(message);
    } catch (std::exception &e) {
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid txBroadcast from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , error: " + e.what() + " closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          std::string("Invalid txBroadcast from unknown session, error: ") + e.what() + " closing session."
        );
      }
    }
  }

  void ManagerNormal::handleBlockBroadcast(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
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
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          "Invalid blockBroadcast from " + sessionPtr->hostNodeId().first.to_string() + ":" +
          std::to_string(sessionPtr->hostNodeId().second) + " , error: " + e.what() + " closing session."
        );
        this->disconnectSession(sessionPtr->hostNodeId());
      } else {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
          std::string("Invalid blockBroadcast from unknown session, error: ") + e.what() + " closing session."
        );
      }
      return;
    }
    if (rebroadcast) this->broadcastMessage(message);
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
    return;
  }

  void ManagerNormal::broadcastTxBlock(const TxBlock &txBlock) {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastTx(txBlock));
    this->broadcastMessage(broadcast);
    return;
  }

  void ManagerNormal::broadcastBlock(const std::shared_ptr<const Block> block) {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastBlock(block));
    this->broadcastMessage(broadcast);
    return;
  }
};

