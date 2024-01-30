/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managerbase.h"

namespace P2P {
  void DiscoveryWorker::refreshRequestedNodes() {
    std::unique_lock lock(this->requestedNodesMutex_);
    for (auto it = this->requestedNodes_.begin(); it != this->requestedNodes_.end();) {
      if (std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch()).count() - it->second > 60
          ) {
        this->requestedNodes_.erase(it++);
      } else {
        it++;
      }
    }
  }

  std::pair<
    std::unordered_set<NodeID, SafeHash>, std::unordered_set<NodeID, SafeHash>
  > DiscoveryWorker::listConnectedNodes() {
    std::pair<std::unordered_set<NodeID, SafeHash>, std::unordered_set<NodeID ,SafeHash>> connectedNodes;
    std::shared_lock requestedNodesLock(this->requestedNodesMutex_);
    std::shared_lock sessionsLock(this->manager_.sessionsMutex_);
    for (const auto& [nodeId, session] : this->manager_.sessions_) {
      // Skip nodes that were already requested in the last 60 seconds
      if (this->requestedNodes_.contains(nodeId)) continue;
      if (session->hostType() == NodeType::DISCOVERY_NODE) {
        connectedNodes.first.insert(nodeId);
      } else if (session->hostType() == NodeType::NORMAL_NODE) {
        connectedNodes.second.insert(nodeId);
      }
    }
    return connectedNodes;
  }

  std::unordered_map<NodeID, NodeType, SafeHash> DiscoveryWorker::getConnectedNodes(const NodeID& nodeId) {
    return this->manager_.requestNodes(nodeId);
  }

  void DiscoveryWorker::connectToNode(const NodeID& nodeId, NodeType nodeType) {
    const auto& [nodeIp, nodePort] = nodeId;
    if (nodeType == NodeType::DISCOVERY_NODE) return; // Do not connect to new discovery nodes
    this->manager_.connectToServer(nodeIp, nodePort);
  }

  bool DiscoveryWorker::discoverLoop() {
    bool discoveryPass = false;

    Logger::logToDebug(LogType::INFO, Log::P2PDiscoveryWorker, __func__, "Discovery thread started");
    while (!this->stopWorker_) {
      // Check if we reached connection limit
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::shared_lock lock(this->manager_.sessionsMutex_);
        if (this->manager_.sessions_.size() >= this->manager_.minConnections()) {
          // If we don't have at least 11 connections, we don't sleep discovery.
          // This is to make sure that local_testnet can quickly start up a new
          // network, but still sleep discovery if the minimum is reached.
          lock.unlock();
          Logger::logToDebug(LogType::INFO, Log::P2PDiscoveryWorker, __func__, "Min connections reached, sleeping");
          std::this_thread::sleep_for(std::chrono::seconds(5)); // Only 1 second because we still want to reach maxConnections
          lock.lock();
        } else if (this->manager_.sessions_.size() >= this->manager_.maxConnections()) {
          lock.unlock();
          Logger::logToDebug(LogType::INFO, Log::P2PDiscoveryWorker, __func__, "Max connections reached, sleeping");
          std::this_thread::sleep_for(std::chrono::seconds(60));
          continue;
        }
      }

      // Refresh and get the list of requested nodes
      this->refreshRequestedNodes();
      auto connectedNodes = this->listConnectedNodes();
      if (this->stopWorker_) return true;

      if (!discoveryPass) {
        // Ask each found discovery node for their peer list,
        // connect to said peer, and add them to the list of requested nodes
        for (const auto& nodeId : connectedNodes.first) {
          // Request nodes from discovery node
          auto nodeList = this->getConnectedNodes(nodeId);
          if (this->stopWorker_) return true;

          // Connect to all found nodes
          for (const auto& [nodeId, nodeInfo] : nodeList) {
            if (this->stopWorker_) return true;
            this->connectToNode(nodeId, nodeInfo);
          }
          if (this->stopWorker_) return true;

          // Add requested node to list of requested nodes
          std::unique_lock(this->requestedNodesMutex_);
          this->requestedNodes_[nodeId] = std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch()
          ).count();
        }
        discoveryPass = true;
      } else {
        // Ask each found normal node for their peer list
        // Connect to said peer, and add them to the list of requested nodes
        for (const auto& nodeId : connectedNodes.second) {
          // Request nodes from normal node
          auto nodeList = this->getConnectedNodes(nodeId);
          if (this->stopWorker_) return true;

          // Connect to all found nodes.
          for (const auto& [nodeId, nodeInfo] : nodeList) {
            if (this->stopWorker_) return true;
            this->connectToNode(nodeId, nodeInfo);
          }
          if (this->stopWorker_) return true;

          // Add requested node to list of requested nodes
          std::unique_lock(this->requestedNodesMutex_);
          this->requestedNodes_[nodeId] = std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch()
          ).count();
        }
      }
    }
    return true;
  }

  void DiscoveryWorker::start() {
    if (!this->workerFuture_.valid()) {
      this->stopWorker_ = false;
      this->workerFuture_ = std::async(std::launch::async, &DiscoveryWorker::discoverLoop, this);
    }
  }

  void DiscoveryWorker::stop() {
    if (this->workerFuture_.valid()) {
      this->stopWorker_ = true;
      this->workerFuture_.get();
      std::unique_lock lock(this->requestedNodesMutex_);
      this->requestedNodes_.clear();
    }
  }
};

