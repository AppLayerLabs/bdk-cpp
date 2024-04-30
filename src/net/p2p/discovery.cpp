/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "managerbase.h"

namespace P2P {
  void DiscoveryWorker::refreshRequestedNodes() {
    std::unique_lock lock(this->requestedNodesMutex_);
    for (auto it = this->requestedNodes_.begin(); it != this->requestedNodes_.end();) {
      if (std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
      ).count() - it->second > 10) {
        it = this->requestedNodes_.erase(it);
      } else {
        ++it;
      }
    }
  }

  std::pair<
    std::unordered_set<NodeID, SafeHash>, std::unordered_set<NodeID, SafeHash>
  > DiscoveryWorker::listConnectedNodes() {
    std::pair<std::unordered_set<NodeID, SafeHash>, std::unordered_set<NodeID ,SafeHash>> connectedNodes;
    std::shared_lock<std::shared_mutex> requestedNodesLock(this->requestedNodesMutex_);
    std::shared_lock<std::shared_mutex> sessionsLock(this->manager_.sessionsMutex_);
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
    Logger::logToDebug(LogType::INFO, Log::P2PDiscoveryWorker, __func__, "Discovery thread started minConnections: "
                        + std::to_string(this->manager_.minConnections()) + " maxConnections: " + std::to_string(this->manager_.maxConnections()));
    uint64_t lastLogged = 0;
    while (!this->stopWorker_) {
      // Check if we reached connection limit
      uint64_t sessionSize;
      {
        std::shared_lock<std::shared_mutex> lock(this->manager_.sessionsMutex_);
        sessionSize = this->manager_.sessions_.size();
      }

      if (lastLogged != sessionSize) {
        Logger::logToDebug(LogType::INFO, Log::P2PDiscoveryWorker, __func__, "DiscoveryWorker current sessionSize: " + std::to_string(sessionSize));
        lastLogged = sessionSize;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (sessionSize >= this->manager_.maxConnections()) {
         Logger::logToDebug(LogType::INFO, Log::P2PDiscoveryWorker, __func__, "Max connections reached, sleeping...");
         std::this_thread::sleep_for(std::chrono::seconds(10));
         continue;
      }

      // Refresh and get the list of requested nodes
      this->refreshRequestedNodes();
      auto [connectedDiscoveries, connectedNormals] = this->listConnectedNodes();
      if (this->stopWorker_) return true;

      /// Keep switching between discovery and normal nodes
      if (!discoveryPass) {
        // Ask each found discovery node for their peer list,
        // connect to said peer, and add them to the list of requested nodes
        for (const auto& nodeId : connectedDiscoveries) {
          // Request nodes from discovery node
          auto nodeList = this->getConnectedNodes(nodeId);
          if (this->stopWorker_) return true;

          // Connect to all found nodes
          for (const auto& [foundNodeId, foundNodeInfo] : nodeList) {
            if (this->stopWorker_) return true;
            this->connectToNode(foundNodeId, foundNodeInfo);
          }
          if (this->stopWorker_) return true;

          // Add requested node to list of requested nodes, but only if the list is has at least minConnections and we have at least minConnections
          if (nodeList.size() >= this->manager_.minConnections() && sessionSize >= this->manager_.minConnections()) {
            std::unique_lock lock(this->requestedNodesMutex_);
            this->requestedNodes_[nodeId] = std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count();
          }
        }
        discoveryPass = true;
      } else {
        // Ask each found normal node for their peer list
        // Connect to said peer, and add them to the list of requested nodes
        for (const auto& nodeId : connectedNormals) {
          // Request nodes from normal node
          auto nodeList = this->getConnectedNodes(nodeId);
          if (this->stopWorker_) return true;

          // Connect to all found nodes.
          for (const auto& [foundNodeId, foundNodeInfo] : nodeList) {
            if (this->stopWorker_) return true;
            this->connectToNode(foundNodeId, foundNodeInfo);
          }
          if (this->stopWorker_) return true;

          // Add requested node to list of requested nodes, but only if we have at least minConnections
          if (sessionSize >= this->manager_.minConnections()) {
            std::unique_lock lock(this->requestedNodesMutex_);
            this->requestedNodes_[nodeId] = std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count();
          }
        }
        discoveryPass = false;
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

