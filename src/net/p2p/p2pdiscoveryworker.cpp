#include "p2pmanagerbase.h"

namespace P2P {

  void DiscoveryWorker::refreshRequestedNodes() {
    std::unique_lock lock(this->requestedNodesMutex);
    for (auto it = this->requestedNodes.begin(); it != this->requestedNodes.end();) {
      if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - it->second > 60)
        this->requestedNodes.erase(it++);
      else
        ++it;
    }
  }

  std::pair<std::unordered_set<Hash, SafeHash>,std::unordered_set<Hash, SafeHash>> DiscoveryWorker::listConnectedNodes() {
    std::pair<std::unordered_set<Hash, SafeHash>,std::unordered_set<Hash,SafeHash>> connectedNodes;
    std::shared_lock requestedNodesLock(this->requestedNodesMutex);
    std::shared_lock sessionsLock(this->manager.sessionsMutex);
    for (const auto& [nodeId, session] : this->manager.sessions_) {
      if (this->requestedNodes.contains(nodeId))
        continue; /// Skip nodes that were already requested in the last 60 seconds
      if (session->hostType() == NodeType::DISCOVERY_NODE)
        connectedNodes.first.insert(nodeId);
      else if (session->hostType() == NodeType::NORMAL_NODE)
        connectedNodes.second.insert(nodeId);
    }
    return connectedNodes;
  }

  std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> DiscoveryWorker::getConnectedNodes(const Hash& nodeId) {
    return this->manager.requestNodes(nodeId);
  }

  void DiscoveryWorker::connectToNode(const Hash& nodeId, const std::tuple<NodeType, boost::asio::ip::address, unsigned short>& nodeInfo) {
    const auto& [nodeType, nodeIp, nodePort] = nodeInfo;
    if (nodeType == NodeType::DISCOVERY_NODE)
      return; /// We don't connect to new discovery nodes
    {
      std::shared_lock(this->manager.sessionsMutex);
      if (this->manager.sessions_.contains(nodeId))
        return; /// Node is already connected
    }
    this->manager.connectToServer(nodeIp.to_string(), nodePort);
  }

  /**
   * We can summarize the discovery process as follows:
   * Ask currently connected nodes to give us a list of nodes they are connected to.
   * Wait for max 5 seconds for looping the connected nodes.
   * If already asked in the last 60 seconds, skip the node.
   * Give priority to discovery nodes at the first pass.
   * Do not connect to nodes that are already connected
   * Connect to nodes that are not already connected.
   * If number of connections is over maxConnections, stop discovery.
   * As discovery nodes should be *hardcoded*, we cannot connect to other discovery nodes.
   */

  bool DiscoveryWorker::discoverLoop() {
    bool foundNodesToConnect = false;
    bool discoveryPass = false;

    Utils::logToDebug(Log::P2PDiscoveryWorker, __func__, "Discovery thread started");
    while (!this->stopWorker) {
      auto maxTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() + 5;
      // Check if we reached connection limit.
      {
        std::shared_lock lock(this->manager.sessionsMutex);
        if (this->manager.sessions_.size() >= this->manager.minConnections()) {
          /// If we don't have at least 11 connections, we don't sleep discovery.
          /// This is to make sure that local_testnet can quickly start up a new network, but still sleep discovery
          /// If the minimum is reached.
          Utils::logToDebug(Log::P2PDiscoveryWorker, __func__, "Min connections reached, sleeping");
          std::this_thread::sleep_for(std::chrono::seconds(5)); /// Only 5 seconds because we still want to reach maxConnections
        }
        if (this->manager.sessions_.size() >= this->manager.maxConnections()) {
          Utils::logToDebug(Log::P2PDiscoveryWorker, __func__, "Max connections reached, sleeping");
          std::this_thread::sleep_for(std::chrono::seconds(60));
          continue;
        } else {
          std::unique_lock lock(this->requestedNodesMutex);
        }
      }
      // Refresh list of requested nodes
      this->refreshRequestedNodes();
      // Get list of connected nodes
      auto connectedNodes = this->listConnectedNodes();
      if (this->stopWorker) return true;

      if (!discoveryPass) {
        // Ask each found discovery node for their peer list
        // Connect to said peer, and add them to the list of requested nodes
        for (const auto& nodeId : connectedNodes.first) {
          // Request nodes from discovery node
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
          this->requestedNodes[nodeId] = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
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
          this->requestedNodes[nodeId] = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
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
}