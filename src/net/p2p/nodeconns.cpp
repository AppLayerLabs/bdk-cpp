/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "nodeconns.h"
#include "../../core/blockchain.h"

void P2P::NodeConns::refresh() {
  // Get the list of currently connected nodes
  std::vector<P2P::NodeID> connectedNodes = this->manager_.getSessionsIDs();
  //while (connectedNodes.size() < this->manager_.minConnections() && !this->blockchain_.getSyncer().isStopped()) {
  // TODO: Syncer::stopSyncer_ doesn't exist anymore, this needs to be replaced
  // with either another flag that makes sense, or some other logic that stops
  // the function in case of general shutdown so it doesn't hang forever
  while (connectedNodes.size() < this->manager_.minConnections()) {
    Logger::logToDebug(LogType::INFO, Log::nodeConns, __func__,
      "Waiting for discoveryWorker to connect to more nodes, currently connected to: "
      + std::to_string(connectedNodes.size())
    );
    // If we have less than the minimum number of connections,
    // wait for a bit for discoveryWorker to kick in and connect to more nodes
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connectedNodes = this->manager_.getSessionsIDs();
  }

  // Update information of already connected nodes
  auto it = this->connected_.begin();
  while (it != this->connected_.end()) {
    const auto& nodeId = it->first;
    if (std::find(connectedNodes.begin(), connectedNodes.end(), nodeId) == connectedNodes.end()) {
      it = this->connected_.erase(it); // Node not connected, remove it from list
    } else {
      auto newNodeInfo = this->manager_.requestNodeInfo(nodeId);
      if (newNodeInfo == P2P::NodeInfo()) {
        it = this->connected_.erase(it); // Node not responding to info request, remove it from list
      } else {
        it->second = newNodeInfo; // Save node response to info request and iterate to next
        it++;
      }
    }
  }

  // Add new nodes to the list
  for (const auto& nodeId : connectedNodes) {
    if (!this->connected_.contains(nodeId)) {
      auto newNodeInfo = this->manager_.requestNodeInfo(nodeId);
      if (newNodeInfo != P2P::NodeInfo()) {
        this->connected_[nodeId] = newNodeInfo;
      }
    }
  }
}

