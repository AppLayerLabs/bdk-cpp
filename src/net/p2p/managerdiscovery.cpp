/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managerdiscovery.h"

namespace P2P {
  void ManagerDiscovery::handleMessage(
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
      default:
        if (auto sessionPtr = session.lock()) {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                             "Invalid message type from " + sessionPtr->hostNodeId().first.to_string() + ":" +
                             std::to_string(sessionPtr->hostNodeId().second) + " closing session."
          );
          this->disconnectSession(sessionPtr->hostNodeId());
        } else {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                             "Invalid message type from unknown session, the session ran away."
          );
        }
        break;
    }
  }

  void ManagerDiscovery::handleRequest(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
      case Ping:
        handlePingRequest(session, message);
        break;
      case RequestNodes:
        handleRequestNodesRequest(session, message);
        break;
      default:
        if (auto sessionPtr = session.lock()) {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                             "Invalid Request Command Type from " + sessionPtr->hostNodeId().first.to_string() + ":" +
                             std::to_string(sessionPtr->hostNodeId().second) + ", closing session."
          );
          this->disconnectSession(sessionPtr->hostNodeId());
        } else {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                             "Invalid Request Command Type from unknown session, closing session."
          );
        }
        break;
    }
  }

  void ManagerDiscovery::handleAnswer(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
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
        if (auto sessionPtr = session.lock()) {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                             "Invalid Answer Command Type from " + sessionPtr->hostNodeId().first.to_string() + ":" +
                             std::to_string(sessionPtr->hostNodeId().second) + ", closing session."
          );
          this->disconnectSession(sessionPtr->hostNodeId());
        } else {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                             "Invalid Answer Command Type from unknown session, closing session."
          );
        }
        break;
    }
  }

  void ManagerDiscovery::handlePingRequest(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::ping(*message)) {
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid ping request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
                           std::to_string(sessionPtr->hostNodeId().second) + " closing session."
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

  void ManagerDiscovery::handleRequestNodesRequest(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    if (!RequestDecoder::requestNodes(*message)) {
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Invalid requestNodes request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
                           std::to_string(sessionPtr->hostNodeId().second) + " closing session."
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

  void ManagerDiscovery::handlePingAnswer(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
      if (!requests_.contains(message->id())) {
        lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
        if (auto sessionPtr = session.lock()) {
          Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                             "Answer to invalid request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
                             std::to_string(sessionPtr->hostNodeId().second) + " closing session."
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

  void ManagerDiscovery::handleRequestNodesAnswer(
    std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message
  ) {
    std::unique_lock lock(this->requestsMutex_);
    if (!requests_.contains(message->id())) {
      lock.unlock(); // Unlock before calling logToDebug to avoid waiting for the lock in the logToDebug function.
      if (auto sessionPtr = session.lock()) {
        Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                           "Answer to invalid request from " + sessionPtr->hostNodeId().first.to_string() + ":" +
                           std::to_string(sessionPtr->hostNodeId().second) + " closing session."
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
};

