#include "p2pmanagerbase.h"

namespace P2P {
  void DiscoveryWorker::refreshRequestedNodes() {
    std::unique_lock lock(this->requestedNodesMutex);
    for (auto it = this->requestedNodes.begin(); it != this->requestedNodes.end();) {
      if (std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count() - it->second > 60
      ) {
        this->requestedNodes.erase(it++);
      } else {
        it++;
      }
    }
  }

  std::pair<
    std::unordered_set<Hash, SafeHash>,std::unordered_set<Hash, SafeHash>
  > DiscoveryWorker::listConnectedNodes() {
    std::pair<std::unordered_set<Hash, SafeHash>,std::unordered_set<Hash,SafeHash>> connectedNodes;
    std::shared_lock requestedNodesLock(this->requestedNodesMutex);
    std::shared_lock sessionsLock(this->manager.sessionsMutex);
    for (const auto& [nodeId, session] : this->manager.sessions_) {
      // Skip nodes that were already requested in the last 60 seconds
      if (this->requestedNodes.contains(nodeId)) continue;
      if (session->hostType() == NodeType::DISCOVERY_NODE) {
        connectedNodes.first.insert(nodeId);
      } else if (session->hostType() == NodeType::NORMAL_NODE) {
        connectedNodes.second.insert(nodeId);
      }
    }
    return connectedNodes;
  }

  std::unordered_map<
    Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash
  > DiscoveryWorker::getConnectedNodes(const Hash& nodeId) {
    return this->manager.requestNodes(nodeId);
  }

  void DiscoveryWorker::connectToNode(
    const Hash& nodeId, const std::tuple<NodeType, boost::asio::ip::address,
    unsigned short>& nodeInfo
  ) {
    const auto& [nodeType, nodeIp, nodePort] = nodeInfo;
    if (nodeType == NodeType::DISCOVERY_NODE) return; // Do not connect to new discovery nodes
    {
      std::shared_lock(this->manager.sessionsMutex);
      if (this->manager.sessions_.contains(nodeId)) return; // Node is already connected
    }
    this->manager.connectToServer(nodeIp.to_string(), nodePort);
  }

  bool DiscoveryWorker::discoverLoop() {
    bool discoveryPass = false;

    Utils::logToDebug(Log::P2PDiscoveryWorker, __func__, "Discovery thread started");
    while (!this->stopWorker) {
      // Check if we reached connection limit
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::shared_lock lock(this->manager.sessionsMutex);
        if (this->manager.sessions_.size() >= this->manager.minConnections()) {
          // If we don't have at least 11 connections, we don't sleep discovery.
          // This is to make sure that local_testnet can quickly start up a new
          // network, but still sleep discovery if the minimum is reached.
          lock.unlock();
          Utils::logToDebug(Log::P2PDiscoveryWorker, __func__, "Min connections reached, sleeping");
          std::this_thread::sleep_for(std::chrono::seconds(5)); // Only 1 second because we still want to reach maxConnections
          lock.lock();
        } else if (this->manager.sessions_.size() >= this->manager.maxConnections()) {
          lock.unlock();
          Utils::logToDebug(Log::P2PDiscoveryWorker, __func__, "Max connections reached, sleeping");
          std::this_thread::sleep_for(std::chrono::seconds(60));
          continue;
        }
      }

      // Refresh and get the list of requested nodes
      this->refreshRequestedNodes();
      auto connectedNodes = this->listConnectedNodes();
      if (this->stopWorker) return true;

      if (!discoveryPass) {
        // Ask each found discovery node for their peer list,
        // connect to said peer, and add them to the list of requested nodes
        for (const auto& nodeId : connectedNodes.first) {
          // Request nodes from discovery node
          auto nodeList = this->getConnectedNodes(nodeId);
          if (this->stopWorker) return true;

          // Connect to all found nodes
          for (const auto& [nodeId, nodeInfo] : nodeList) {
            if (this->stopWorker) return true;
            this->connectToNode(nodeId, nodeInfo);
          }
          if (this->stopWorker) return true;

          // Add requested node to list of requested nodes
          std::unique_lock(this->requestedNodesMutex);
          this->requestedNodes[nodeId] = std::chrono::duration_cast<std::chrono::seconds>(
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
          if (this->stopWorker) return true;

          // Connect to all found nodes.
          for (const auto& [nodeId, nodeInfo] : nodeList) {
            if (this->stopWorker) return true;
            this->connectToNode(nodeId, nodeInfo);
          }
          if (this->stopWorker) return true;

          // Add requested node to list of requested nodes
          std::unique_lock(this->requestedNodesMutex);
          this->requestedNodes[nodeId] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
          ).count();
        }
      }
    }
    return true;
  }

  void DiscoveryWorker::start() {
    if (!this->workerFuture.valid()) {
      this->stopWorker = false;
      this->workerFuture = std::async(std::launch::async, &DiscoveryWorker::discoverLoop, this);
    }
  }

  void DiscoveryWorker::stop() {
    if (this->workerFuture.valid()) {
      this->stopWorker = true;
      this->workerFuture.get();
      std::unique_lock lock(this->requestedNodesMutex);
      this->requestedNodes.clear();
    }
  }
};

