/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managerdiscovery.h"

namespace P2P {
  void ManagerDiscovery::handleMessage(
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
      default:
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid message type from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                           " closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerDiscovery::handleRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
      case Ping:
        handlePingRequest(nodeId, message);
        break;
      case RequestNodes:
        handleRequestNodesRequest(nodeId, message);
        break;
      default:
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid Request Command Type from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                           ", closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerDiscovery::handleAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
      case Ping:
        handlePingAnswer(nodeId, message);
        break;
      case Info:
        break;
      case RequestNodes:
        handleRequestNodesAnswer(nodeId, message);
        break;
      default:
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid Answer Command Type from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                           ", closing session.");
        this->disconnectSession(nodeId);
        break;
    }
  }

  void ManagerDiscovery::handlePingRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::ping(*message)) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid ping request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::ping(*message)));
  }

  void ManagerDiscovery::handleRequestNodesRequest(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::requestNodes(*message)) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid requestNodes request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) + " closing session.");
      this->disconnectSession(nodeId);
      return;
    }

    std::unordered_map<NodeID, NodeType, SafeHash> nodes;

    {
      std::shared_lock lock(this->sessionsMutex_);
      std::transform(sessions_.begin(), sessions_.end(),
        std::inserter(nodes, nodes.end()), [](const auto& pair){
          return std::make_pair(pair.first, pair.second->hostType());
        }
      );
    }
    this->answerSession(nodeId, std::make_shared<const Message>(AnswerEncoder::requestNodes(*message, nodes)));
  }

  void ManagerDiscovery::handlePingAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Answer to invalid request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }

  void ManagerDiscovery::handleRequestNodesAnswer(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Answer to invalid request from " + nodeId.first.to_string() + ":" + std::to_string(nodeId.second) +
                         " closing session.");
      this->disconnectSession(nodeId);
      return;
    }
    requests_[message->id()]->setAnswer(message);
  }
};

