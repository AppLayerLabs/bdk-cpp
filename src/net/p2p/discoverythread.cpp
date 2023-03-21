#include "p2pmanagerbase.h"

namespace P2P {

  void ManagerBase::handleDiscoveryStop() {
    this->discoveryThreadRunning_ = false;
  }

  void ManagerBase::discoveryThread() {
    this->discoveryThreadRunning_ = true;
    bool foundNodesToConnect = false;
    bool discoveryPass = false;

    std::unordered_set <Hash, SafeHash> nodesRequested;
    Utils::logToDebug(Log::P2PManager, __func__, "Discovery thread started");
    while (!this->discoveryThreadStopFlag_) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      
      std::unordered_set<Hash, SafeHash> connectedNormalNodes;
      std::unordered_set<Hash, SafeHash> connectedDiscoveryNodes;
      {
        std::unique_lock lock(this->sessionsMutex);
        if (this->sessions_.size() >= this->maxConnections_) {
          Utils::logToDebug(Log::P2PManager, __func__, "Max nodes reached, skipping discovery");
          std::this_thread::sleep_for(std::chrono::seconds(60));
          nodesRequested.clear();
          continue;
        }
        for (auto& session : this->sessions_) {
          // Skip nodes that were already requested.
          if (nodesRequested.contains(session.first)) continue;
          if (session.second->hostType() == NodeType::NORMAL_NODE)
            connectedNormalNodes.insert(session.first);
          else if (session.second->hostType() == NodeType::DISCOVERY_NODE)
            connectedDiscoveryNodes.insert(session.first);
        }
      }

      std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> newNodes;

      if (this->discoveryThreadStopFlag_) break;

      // Give priority to discovery nodes for the first pass of discovery.
      if (connectedDiscoveryNodes.size() == 0 || discoveryPass) {
        Utils::logToDebug(Log::P2PManager, __func__, "No discovery nodes found, requesting from other normal nodes");
        // Give 10 seconds to request nodes and wait for response.
        auto startTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        // Request node list to each node.
        // Time limit for requesting is 10 seconds.
        // After node is requested, wait for 1 second for response, if no response, skip node and move on.
        // Nodes that were requested previously are skipped.
        for (auto& node : connectedNormalNodes) {
          auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
          if (now - startTime > 10) break;
          if (this->discoveryThreadStopFlag_) break;
          if (!nodesRequested.contains(node)) {
            nodesRequested.insert(node);
            auto request = RequestEncoder::requestNodes();
            auto requestPtr = sendMessageTo(node, request);
            auto answer = requestPtr->answerFuture();
            auto status = answer.wait_for(std::chrono::seconds(1));
            if (status == std::future_status::ready) {
              auto nodes = AnswerDecoder::requestNodes(answer.get());
              for (auto& [key, value] : nodes) {
                // Skip nodes that we already have connection to
                if (connectedNormalNodes.contains(key) || connectedDiscoveryNodes.contains(key)) continue;
                newNodes[key] = value;
              }
            }
          }
        }
      } else {
        Utils::logToDebug(Log::P2PManager, __func__, "Requesting nodes from discovery nodes");
        // Give 10 seconds to request nodes and wait for response.
        auto startTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        Utils::logToDebug(Log::P2PManager, __func__, "Discovery node found, requesting from discovery nodes");
        // Request node list to each node.
        // Same as above, but with discovery nodes instead.
        for (auto& node : connectedDiscoveryNodes) {
          auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
          if (now - startTime > 10) break;
          if (this->discoveryThreadStopFlag_) break;
          if (!nodesRequested.contains(node)) {
            nodesRequested.insert(node);
            auto request = RequestEncoder::requestNodes();
            auto requestPtr = sendMessageTo(node, request);
            auto answer = requestPtr->answerFuture();
            auto status = answer.wait_for(std::chrono::seconds(1));
            if (status == std::future_status::ready) {
              auto nodes = AnswerDecoder::requestNodes(answer.get());
              for (auto& [key, value] : nodes) {
                // Skip nodes that we already have connection to
                if (connectedNormalNodes.contains(key) || connectedDiscoveryNodes.contains(key)) continue;
                newNodes[key] = value;
              }
            }
          }
        }
        if (newNodes.size() > 0) discoveryPass = true;
      }
      // Try opening connections to new nodes.
      {
        Utils::logToDebug(Log::P2PManager, __func__, "Trying to connect to new nodes");
        for (auto const &node : newNodes) {
          foundNodesToConnect = true;
          Utils::logToDebug(Log::P2PManager, __func__, "Trying to connect to node: " + node.first.hex().get());
          this->connectToServer(std::get<1>(node.second).to_string(), std::get<2>(node.second));
        }
      }
    }
    handleDiscoveryStop();
  }

  void ManagerBase::startDiscovery() {
    if (!this->discoveryThreadRunning_) {
      this->discoveryThreadStopFlag_ = false;
      this->discoveryThread_ = std::thread(&ManagerBase::discoveryThread, this);
      this->discoveryThread_.detach();
    }
  }

  void ManagerBase::stopDiscovery() {
    this->discoveryThreadStopFlag_ = true;
  }

}