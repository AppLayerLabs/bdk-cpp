/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "blockchain.h"

Blockchain::Blockchain(const std::string& blockchainPath) :
  options_(Options::fromFile(blockchainPath)),
  db_(blockchainPath + "/database"),
  storage_(db_, options_),
  state_(db_, storage_, p2p_, options_),
  p2p_(boost::asio::ip::address::from_string("127.0.0.1"), options_, storage_, state_),
  http_(state_, storage_, p2p_, options_),
  nodeConns_(*this),
  syncer_(*this),
  broadcaster_(*this)
{}

void Blockchain::start() { p2p_.start(); http_.start(); syncer_.start(); }

void Blockchain::stop() { syncer_.stop(); http_.stop(); p2p_.stop(); }

const std::atomic<bool>& Blockchain::isSynced() const { return this->syncer_.isSynced(); }

// TODO: Fully implement Sync
void Syncer::doSync() {
  // Get the list of currently connected nodes and their current height
  this->blockchain_.getNodeConns().refresh();
  std::pair<P2P::NodeID, uint64_t> highestNode = {P2P::NodeID(), 0};

  // Get the highest node.
  for (auto& [nodeId, nodeInfo] : this->blockchain_.getNodeConns().getConnected()) {
    if (nodeInfo.latestBlockHeight > highestNode.second) {
      highestNode = {nodeId, nodeInfo.latestBlockHeight};
    }
  }

  // Sync from the best node.
  if (highestNode.second > this->blockchain_.storage_.latest()->getNHeight()) {
    // TODO: currently we are starting all the nodes from genesis (0)
  }

  this->synced_ = true;
}

bool Syncer::syncerLoop() {
  Utils::safePrint("Starting OrbiterSDK Node...");
  Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Starting syncer loop.");
  // Connect to all seed nodes from the config and start the discoveryThread.
  auto discoveryNodeList = this->blockchain_.options_.getDiscoveryNodes();
  for (const auto &[ipAddress, port]: discoveryNodeList) {
    this->blockchain_.p2p_.connectToServer(ipAddress, port);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  this->blockchain_.p2p_.startDiscovery();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Sync the node with the network.
  this->doSync();
  if (this->stopSyncer_) return false;
  Utils::safePrint("Synced with the network, starting the node.");
  if (this->blockchain_.options_.getIsValidator()) {
    this->blockchain_.getBroadcaster().validatorLoop();
  } else {
    this->blockchain_.getBroadcaster().nonValidatorLoop();
  }
  return true;
}

void Syncer::start() {
  if (!this->syncerLoopFuture_.valid()) {
    this->syncerLoopFuture_ = std::async(std::launch::async, &Syncer::syncerLoop, this);
  }
}

void Syncer::stop() {
  this->stopSyncer_ = true;
  this->blockchain_.state_.rdposStopWorker(); // Stop the rdPoS worker.
  if (this->syncerLoopFuture_.valid()) this->syncerLoopFuture_.wait();
}

