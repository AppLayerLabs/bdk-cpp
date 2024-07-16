/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "blockchain.h"

Blockchain::Blockchain(const std::string& blockchainPath) :
  options_(Options::fromFile(blockchainPath)),
  db_(std::get<0>(DumpManager::getBestStateDBPath(options_))),
  storage_(p2p_.getLogicalLocation(), options_),
  state_(db_, storage_, p2p_, std::get<1>(DumpManager::getBestStateDBPath(options_)), options_),
  p2p_(options_.getP2PIp(), options_, storage_, state_),
  http_(state_, storage_, p2p_, options_),
  syncer_(p2p_, storage_, state_),
  consensus_(state_, p2p_, storage_, options_)
{}

void Blockchain::start() {
  // Initialize necessary modules
  LOGINFOP("Starting BDK Node...");
  this->p2p_.start();
  this->http_.start();

  // Connect to all seed nodes from the config and start the discoveryThread.
  auto discoveryNodeList = this->options_.getDiscoveryNodes();
  for (const auto &[ipAddress, port]: discoveryNodeList) this->p2p_.connectToServer(ipAddress, port);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  this->p2p_.startDiscovery();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Do initial sync
  this->syncer_.sync(100, 20000000); // up to 100 blocks per request, 20MB limit, default connection timeout & retry count

  // After Syncing, start the DumpWorker.
  this->state_.dumpStartWorker();

  // if node is a Validator, start the consensus loop
  this->consensus_.start();
}

void Blockchain::stop() {
  this->consensus_.stop();
  this->http_.stop();
  this->p2p_.stop();
}

bool Syncer::sync(uint64_t blocksPerRequest, uint64_t bytesPerRequestLimit, int waitForPeersSecs, int tries) {

  // Make sure we are requesting at least one block per request.
  if (blocksPerRequest == 0) blocksPerRequest = 1;

  // NOTE: This is a synchronous operation that's (currently) run during note boot only, in the caller (main) thread.
  // TODO: Detect out-of-sync after the intial synchronization on node boot and resynchronize.

  LOGINFOP("Syncing with other nodes in the network...");

  // Synchronously get the first list of currently connected nodes and their current height
  this->p2p_.getNodeConns().forceRefresh();
  std::pair<P2P::NodeID, uint64_t> highestNode = {P2P::NodeID(), 0};

  // Loop downloading blocks until we are synchronized
  while (true) {
    // P2P is running, so we are getting updated NodeInfos via NodeConns.
    // Get the node with the highest block height available for download.
    auto connected = this->p2p_.getNodeConns().getConnected();
    if (connected.size() == 0) {
      // No one to download blocks from.
      // While we don't exhaust the waiting-for-a-connection timeout, sleep and try again later.
      if (waitForPeersSecs-- > 0) {
        LOGINFOP("Syncer waiting for peer connections (" + std::to_string(waitForPeersSecs) + "s left) ...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      }
      // We have timed out waiting for peers, so synchronization is complete.
      LOGINFOP("Syncer quitting due to no peer connections.");
      break;
    }
    for (auto& [nodeId, nodeInfo] : connected) {
      if (nodeInfo.latestBlockHeight() > highestNode.second) highestNode = {nodeId, nodeInfo.latestBlockHeight()};
    }
    LOGINFOP("Latest known block height is " + std::to_string(highestNode.second));

    auto currentNHeight = this->storage_.latest()->getNHeight();

    // If synced, quit sync loop.
    if (highestNode.second <= currentNHeight) break;

    auto downloadNHeight = currentNHeight + 1;
    auto downloadNHeightEnd = downloadNHeight + blocksPerRequest - 1;

    // NOTE: Possible optimizatons:
    // - Parallel download of different blocks or block ranges from multiple nodes
    // - Retry slow/failed downloads
    // - Deprioritize download from slow/failed nodes

    // Currently, fetch the next batch of block froms a node that is the best node (has the highest block height)
    LOGINFOP("Requesting blocks [" + std::to_string(downloadNHeight) + ","  + std::to_string(downloadNHeightEnd)
              + "] (" + std::to_string(bytesPerRequestLimit) + " bytes limit) from " + toString(highestNode.first));

    // Request the next block we need from the chosen peer
    std::vector<FinalizedBlock> result = this->p2p_.requestBlock(
      highestNode.first, downloadNHeight, downloadNHeightEnd, bytesPerRequestLimit
    );

    // If the request failed, retry it (unless we set a finite number of tries and we've just run out of them)
    if (result.size() == 0) {
      bool shouldRetry = (tries > 0);
      if (shouldRetry) {
        tries--; LOGWARNINGP("Blocks request failed (" + std::to_string(tries) + " tries left)");
      }
      if (shouldRetry && tries == 0) return false;
      LOGWARNINGP("Blocks request failed, restarting sync");
      continue;
    }

    // Validate and connect the blocks
    try {
      for (auto& block : result) {
        // Blocks in the response must be all a contiguous range
        if (block.getNHeight() != downloadNHeight) {
          throw DynamicException("Peer sent block with wrong height " + std::to_string(block.getNHeight())
                                  + " instead of " + std::to_string(downloadNHeight));
        }
        // This call validates the block first (throws exception if the block invalid).
        // Note that the "result" vector's element data is being consumed (moved) by this call.
        this->state_.processNextBlock(std::move(block));
        LOGINFOP("Processed block " + std::to_string(downloadNHeight) + " from " + toString(highestNode.first));
        ++downloadNHeight;
      }
    } catch (std::exception &e) {
      LOGERROR("Invalid RequestBlock Answer from " + toString(highestNode.first) +
                         " , error: " + e.what() + " closing session.");
      this->p2p_.disconnectSession(highestNode.first);
    }
  }

  this->synced_ = true;
  LOGINFOP("Synced with the network; my latest block height: " + std::to_string(this->storage_.latest()->getNHeight()));
  return true;
}

