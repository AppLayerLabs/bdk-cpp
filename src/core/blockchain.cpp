/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "blockchain.h"

Blockchain::Blockchain(const std::string& blockchainPath) :
  options_(Options::fromFile(blockchainPath)),
  db_(std::get<0>(DumpManager::getBestStateDBPath(options_))),
  storage_(options_),
  state_(db_, storage_, p2p_, std::get<1>(DumpManager::getBestStateDBPath(options_)), options_),
  p2p_(boost::asio::ip::address::from_string("127.0.0.1"), options_, storage_, state_),
  http_(state_, storage_, p2p_, options_),
  syncer_(p2p_, storage_, state_),
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

  // Do initial sync
  // TODO: This may fail to bring the node up to date if we have poor connectivity at this point.
  this->syncer_.sync();

  // if node is a Validator, start the consensus loop
  this->consensus_.start();
}

void Blockchain::stop() {
  this->consensus_.stop();
  this->http_.stop();
  this->p2p_.stop();
}

bool Syncer::sync(int tries) {
  // NOTE: This is a synchronous operation that's (currently) run during note boot only, in the caller (main) thread.
  // TODO: Detect out-of-sync after the intial synchronization on node boot and resynchronize.

  Utils::safePrint("Syncing with other nodes in the network...");
  Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Syncing with other nodes in the network...");

  // Synchronously get the first list of currently connected nodes and their current height
  this->p2p_.getNodeConns().forceRefresh();
  std::pair<P2P::NodeID, uint64_t> highestNode = {P2P::NodeID(), 0};

  // Loop downloading blocks until we are synchronized
  while (true) {

    // P2P is running, so we are getting updated NodeInfos via NodeConns.
    // Get the node with the highest block height available for download.
    auto connected = this->p2p_.getNodeConns().getConnected();
    if (connected.size() == 0) {
      // No one to download blocks from. For now, this means synchronization is complete by definition.
      Utils::safePrint("Syncer quitting due to no peer connections.");
      Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Syncer quitting due to no peer connections.");
      break;
    }
    for (auto& [nodeId, nodeInfo] : connected) {
      if (nodeInfo.latestBlockHeight() > highestNode.second) highestNode = {nodeId, nodeInfo.latestBlockHeight()};
    }
    Utils::safePrint("Latest known block height is " + std::to_string(highestNode.second));
    Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Latest known block height is " + std::to_string(highestNode.second));

    auto currentNHeight = this->storage_.latest()->getNHeight();

    // If synced, quit sync loop.
    if (highestNode.second <= currentNHeight) {
      break;
    }

    auto downloadNHeight = currentNHeight + 1;

    // NOTE: Possible optimizatons:
    // - Parallel download of different blocks from multiple nodes
    // - Retry slow/failed downloads
    // - Deprioritize download from slow/failed nodes

    // Currently, fetch the next block from a node that is the best node (has the highest block height)
    Utils::safePrint("Downloading block " + std::to_string(downloadNHeight) + " from " + toString(highestNode.first));
    Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Downloading block " + std::to_string(downloadNHeight) + " from " + toString(highestNode.first));

    // Request the next block we need from the chosen peer
    std::optional<FinalizedBlock> result = this->p2p_.requestBlock(highestNode.first, downloadNHeight);

    // If the request failed, retry it (unless we set a finite number of tries and we've just run out of them)
    if (!result) {
      if (tries > 0) {
        if (--tries == 0) return false;
      }
      continue;
    }

    // Validate and connect the block
    try {
      FinalizedBlock& block = result.value();
      if (block.getNHeight() != downloadNHeight) {
        throw DynamicException("Peer sent block with wrong height " + std::to_string(block.getNHeight())
                                + " instead of " + std::to_string(downloadNHeight));
      }
      this->state_.processNextBlock(std::move(block)); // This call validates the block first (throws exception if the block invalid)
      Utils::safePrint("Processed block " + std::to_string(downloadNHeight) + " from " + toString(highestNode.first));
      Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Processed block " + std::to_string(downloadNHeight)
                          + " from " + toString(highestNode.first));
    } catch (std::exception &e) {
      Logger::logToDebug(LogType::ERROR, Log::P2PParser, __func__,
                         "Invalid RequestBlock Answer from " + toString(highestNode.first) +
                         " , error: " + e.what() + " closing session.");
      this->p2p_.disconnectSession(highestNode.first);
    }
  }

  this->synced_ = true;
  Utils::safePrint("Synced with the network; my latest block height: " + std::to_string(this->storage_.latest()->getNHeight()));
  Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Synced with the network; my latest block height: " + std::to_string(this->storage_.latest()->getNHeight()));
  return true;
}

