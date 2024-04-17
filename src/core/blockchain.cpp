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
  syncer_(p2p_.getNodeConns(), storage_),
  consensus_(state_, p2p_, storage_, options_)
{}

void Blockchain::start() {
  // Initialize necessary modules
  Utils::safePrint("Starting OrbiterSDK Node...");
  Logger::logToDebug(LogType::INFO, Log::blockchain, __func__, "Starting OrbiterSDK Node...");
  this->p2p_.start();
  this->http_.start();

  // Connect to all seed nodes from the config and start the discoveryThread.
  auto discoveryNodeList = this->options_.getDiscoveryNodes();
  for (const auto &[ipAddress, port]: discoveryNodeList) this->p2p_.connectToServer(ipAddress, port);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  this->p2p_.startDiscovery();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Do initial sync and, if node is a Validator, start the consensus loop
  this->syncer_.sync();
  this->consensus_.start();
}

void Blockchain::stop() {
  this->consensus_.stop();
  this->http_.stop();
  this->p2p_.stop();
}

void Syncer::sync() {
  // Get the list of currently connected nodes and their current height
  Utils::safePrint("Syncing with other nodes in the network...");
  Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Syncing with other nodes in the network...");
  this->nodeConns_.forceRefresh();
  std::pair<P2P::NodeID, uint64_t> highestNode = {P2P::NodeID(), 0};
  // Get the highest node.
  auto connected = this->nodeConns_.getConnected();
  for (auto& [nodeId, nodeInfo] : connected) {
    if (nodeInfo.latestBlockHeight() > highestNode.second) highestNode = {nodeId, nodeInfo.latestBlockHeight()};
  }
  // Sync from the best node.
  if (highestNode.second > this->storage_.latest()->getNHeight()) {
    // TODO: actually implement syncing - currently we are starting all the nodes from genesis (0)
  }
  this->synced_ = true; // TODO: this isn't being set back to false later, probably an oversight
  Utils::safePrint("Synced with the network");
  Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Synced with the network");
}

