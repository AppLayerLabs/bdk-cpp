/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "nodeconns.h"
#include "managernormal.h"
#include "../../core/blockchain.h"

void P2P::NodeConns::forceRefresh() {

  // forceRefresh() reduces the interval between a peer node having a TCP connection to us and it appearing
  //   in the NodeConns peer tracking data structure (nodeInfo_), since it actually requests the NodeInfo
  //   from the remote nodes immediately; this may be faster than waiting ~100ms for it to appear organically
  //   via an incomingInfo() callback.
  // It is also useful when the caller wants to ensure that we have the latest NodeInfo from all peers.

  // Get the list of currently connected nodes
  std::vector<P2P::NodeID> connectedNodes = this->manager_.getSessionsIDs();

  // Synchronous requests are made outside the lock, so these helpers are needed
  std::vector<P2P::NodeID> nodesToCheck;
  std::map<P2P::NodeID, P2P::NodeInfo> updatedNodeInfo;

  {
    std::scoped_lock lock(this->stateMutex_);
    auto it = this->nodeInfo_.begin();
    while (it != this->nodeInfo_.end()) {
      const auto& nodeId = it->first;
      if (std::find(connectedNodes.begin(), connectedNodes.end(), nodeId) == connectedNodes.end()) {
        it = this->nodeInfo_.erase(it);
        nodeInfoTime_.erase(nodeId);
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
    updatedNodeInfo[nodeId] = this->manager_.requestNodeInfo(nodeId); // Will return P2P::NodeInfo() on failure
  }

  {
    std::scoped_lock lock(this->stateMutex_);
    for (const auto& [nodeId, newNodeInfo] : updatedNodeInfo) {
      if (newNodeInfo == P2P::NodeInfo()) {
        this->nodeInfo_.erase(nodeId);
        this->nodeInfoTime_.erase(nodeId);
      } else {
        this->nodeInfo_[nodeId] = newNodeInfo;
        this->nodeInfoTime_[nodeId] = Utils::getCurrentTimeMillisSinceEpoch(); // Good enough; postpones some timeouts
      }
    }
  }
}

void P2P::NodeConns::incomingInfo(const P2P::NodeID& sender, const P2P::NodeInfo& info) {
  std::scoped_lock lock(this->stateMutex_);
  this->nodeInfo_[sender] = info;
  this->nodeInfoTime_[sender] = Utils::getCurrentTimeMillisSinceEpoch();
}

std::unordered_map<P2P::NodeID, P2P::NodeInfo, SafeHash> P2P::NodeConns::getConnected() {
  std::scoped_lock lock(this->stateMutex_);
  return this->nodeInfo_;
}

std::unordered_map<P2P::NodeID, P2P::NodeType, SafeHash> P2P::NodeConns::getConnectedWithNodeType() {
  std::unordered_map<NodeID, NodeType, SafeHash> nodesToNodeType;
  std::scoped_lock lock(this->stateMutex_);
  for (const auto& node : nodeInfo_) {
    nodesToNodeType[node.first] = P2P::NodeType::NORMAL_NODE; // FIXME/REVIEW: are we dealing here exclusively with full nodes?
  }
  return nodesToNodeType;
}

std::optional<P2P::NodeInfo> P2P::NodeConns::getNodeInfo(const P2P::NodeID& nodeId) {
  std::scoped_lock lock(this->stateMutex_);
  auto it = this->nodeInfo_.find(nodeId);
  if (it == this->nodeInfo_.end()) return {};
  return it->second;
}

void P2P::NodeConns::loop() {

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
        const auto& nodeId = it->first;
        auto timeIt = this->nodeInfoTime_.find(nodeId);
        if (timeIt == this->nodeInfoTime_.end()) {
          it = this->nodeInfo_.erase(it); // never happens
        } else {
          uint64_t currentTimeMillis = Utils::getCurrentTimeMillisSinceEpoch();
          if (currentTimeMillis - timeIt->second >= 10000) {
            // 10 seconds elapsed since last update: remove the nodeInfo
            it = this->nodeInfo_.erase(it); // Node not responding to info request, remove it from list
            this->nodeInfoTime_.erase(timeIt);
          } else {
            ++it;
          }
        }
      }
    }
  }
}

void P2P::NodeConns::start() {
  if (!this->loopFuture_.valid()) {
    this->loopFuture_ = std::async(std::launch::async, &P2P::NodeConns::loop, this);
  }
}

void P2P::NodeConns::stop() {
  if (this->loopFuture_.valid()) {
    this->stop_ = true;
    this->loopFuture_.wait();
    this->loopFuture_.get();
  }
}
