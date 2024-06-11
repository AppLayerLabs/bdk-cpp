/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "nodeconns.h"
#include "managernormal.h"
#include "../../core/blockchain.h"

namespace P2P {
  void NodeConns::forceRefresh() {
    // forceRefresh() reduces the interval between a peer node having a TCP connection to us and it appearing
    // in the NodeConns peer tracking data structure (nodeInfo_), since it actually requests the NodeInfo
    // from the remote nodes immediately; this may be faster than waiting ~100ms for it to appear organically
    // via an incomingInfo() callback.
    // It is also useful when the caller wants to ensure that we have the latest NodeInfo from all peers.

    // Get the list of currently connected nodes, preserving the NodeType property for each
    std::vector<NodeID> connectedNormalNodes = this->manager_.getSessionsIDs(NodeType::NORMAL_NODE);
    std::vector<NodeID> connectedDiscoveryNodes = this->manager_.getSessionsIDs(NodeType::DISCOVERY_NODE);
    for (const auto& nodeId : connectedNormalNodes) nodeType_[nodeId] = NodeType::NORMAL_NODE;
    for (const auto& nodeId : connectedDiscoveryNodes) nodeType_[nodeId] = NodeType::DISCOVERY_NODE;

    // Get a combined connectedNodes vector
    std::vector<NodeID> connectedNodes;
    connectedNodes.reserve(connectedNormalNodes.size() + connectedDiscoveryNodes.size());
    connectedNodes.insert(connectedNodes.end(), connectedNormalNodes.begin(), connectedNormalNodes.end());
    connectedNodes.insert(connectedNodes.end(), connectedDiscoveryNodes.begin(), connectedDiscoveryNodes.end());

    // Synchronous requests are made outside the lock, so these helpers are needed
    std::vector<NodeID> nodesToCheck;
    std::map<NodeID, NodeInfo> updatedNodeInfo;

    {
      std::scoped_lock lock(this->stateMutex_);
      auto it = this->nodeInfo_.begin();
      while (it != this->nodeInfo_.end()) {
        const auto& nodeId = it->first;
        if (std::find(connectedNodes.begin(), connectedNodes.end(), nodeId) == connectedNodes.end()) {
          it = this->nodeInfo_.erase(it);
          nodeInfoTime_.erase(nodeId);
          nodeType_.erase(nodeId);
        } else {
          nodesToCheck.push_back(nodeId);
          ++it;
        }
      }
      for (const auto& nodeId : connectedNodes) {
        if (!this->nodeInfo_.contains(nodeId)) {
          nodesToCheck.push_back(nodeId);
        }
      }
    }

    for (const auto& nodeId : nodesToCheck) {
      updatedNodeInfo[nodeId] = this->manager_.requestNodeInfo(nodeId); // Will return NodeInfo() on failure
    }

    {
      std::scoped_lock lock(this->stateMutex_);
      for (const auto& [nodeId, newNodeInfo] : updatedNodeInfo) {
        if (newNodeInfo == NodeInfo()) {
          this->nodeInfo_.erase(nodeId);
          this->nodeInfoTime_.erase(nodeId);
          this->nodeType_.erase(nodeId);
        } else {
          this->nodeInfo_[nodeId] = newNodeInfo;
          this->nodeInfoTime_[nodeId] = Utils::getCurrentTimeMillisSinceEpoch(); // Good enough; postpones some timeouts
        }
      }
    }
  }

  void NodeConns::incomingInfo(const NodeID& sender, const NodeInfo& info, const NodeType& nodeType) {
    std::scoped_lock lock(this->stateMutex_);
    this->nodeInfo_[sender] = info;
    this->nodeInfoTime_[sender] = Utils::getCurrentTimeMillisSinceEpoch();
    this->nodeType_[sender] = nodeType;
  }

  ankerl::unordered_dense::map<NodeID, NodeInfo, SafeHash> NodeConns::getConnected() {
    std::scoped_lock lock(this->stateMutex_);
    return this->nodeInfo_;
  }

  ankerl::unordered_dense::map<NodeID, NodeType, SafeHash> NodeConns::getConnectedWithNodeType() {
    std::scoped_lock lock(this->stateMutex_);
    return this->nodeType_;
  }

  std::optional<NodeInfo> NodeConns::getNodeInfo(const NodeID& nodeId) {
    std::scoped_lock lock(this->stateMutex_);
    auto it = this->nodeInfo_.find(nodeId);
    if (it == this->nodeInfo_.end()) return {};
    return it->second;
  }

  void NodeConns::loop() {
    while (!this->stop_) {
      // work every 100ms
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // push our own current node info to all our peers
      manager_.notifyAllInfo();

      // Then, it will check for timed out peers to remove from the node connections list
      // Any entry older than 10 seconds is removed.
      {
        std::scoped_lock lock(this->stateMutex_);
        auto it = this->nodeInfo_.begin();
          while (it != this->nodeInfo_.end()) {
          const auto nodeId = it->first;
          auto timeIt = this->nodeInfoTime_.find(nodeId);
          if (timeIt == this->nodeInfoTime_.end()) { // never happens
            it = this->nodeInfo_.erase(it);
            this->nodeType_.erase(nodeId);
          } else {
            uint64_t currentTimeMillis = Utils::getCurrentTimeMillisSinceEpoch();
            if (currentTimeMillis - timeIt->second >= 10000) {
              // 10 seconds elapsed since last update: remove the nodeInfo
              it = this->nodeInfo_.erase(it); // Node not responding to info request, remove it from list
              this->nodeInfoTime_.erase(timeIt);
              this->nodeType_.erase(nodeId);
            } else {
              ++it;
            }
          }
        }
      }
    }
  }

  void NodeConns::start() {
    if (!this->loopFuture_.valid()) {
      this->loopFuture_ = std::async(std::launch::async, &NodeConns::loop, this);
    }
  }

  void NodeConns::stop() {
    if (this->loopFuture_.valid()) {
      this->stop_ = true;
      this->loopFuture_.wait();
      this->loopFuture_.get();
    }
  }
}
