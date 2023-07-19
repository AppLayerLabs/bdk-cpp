#include "blockchain.h"

Blockchain::Blockchain(std::string blockchainPath) :
  options(std::make_unique<Options>(Options::fromFile(blockchainPath))),
  db(std::make_unique<DB>(blockchainPath + "/database")),
  storage(std::make_unique<Storage>(db, options)),
  rdpos(std::make_unique<rdPoS>(db, storage, p2p, options, state)),
  state(std::make_unique<State>(db, storage, rdpos, p2p, options)),
  p2p(std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), rdpos, options, storage, state)),
  http(std::make_unique<HTTPServer>(state, storage, p2p, options)),
  syncer(std::make_unique<Syncer>(*this))
{}

void Blockchain::start() { p2p->start(); http->start(); syncer->start(); }

void Blockchain::stop() { syncer->stop(); http->stop(); p2p->stop(); }

const std::atomic<bool>& Blockchain::isSynced() const { return this->syncer->isSynced(); }

void Syncer::updateCurrentlyConnectedNodes() {
  // Get the list of currently connected nodes
  std::vector<P2P::NodeID> connectedNodes = blockchain.p2p->getSessionsIDs();
  while (connectedNodes.size() < blockchain.p2p->minConnections() && !this->stopSyncer) {
    Utils::logToDebug(Log::syncer, __func__,
      "Waiting for discoveryWorker to connect to more nodes, currently connected to: "
      + std::to_string(connectedNodes.size())
    );
    // If we have less than the minimum number of connections,
    // wait for a bit for discoveryWorker to kick in and connect to more nodes
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connectedNodes = blockchain.p2p->getSessionsIDs();
  }

  // Update information of already connected nodes
  for (auto& [nodeId, nodeInfo] : this->currentlyConnectedNodes) {
    // If node is not connected, remove it from the list
    if (std::find(connectedNodes.begin(), connectedNodes.end(), nodeId) == connectedNodes.end()) {
      this->currentlyConnectedNodes.erase(nodeId);
      continue;
    }
    // If node is connected, update its information
    auto newNodeInfo = blockchain.p2p->requestNodeInfo(nodeId);
    // If node is not responding, remove it from the list
    if (newNodeInfo == P2P::NodeInfo()) {
      this->currentlyConnectedNodes.erase(nodeId);
      continue;
    }
    // If node is responding, update its information
    this->currentlyConnectedNodes[nodeId] = newNodeInfo;
  }

  // Add new nodes to the list
  for (auto& nodeId : connectedNodes) {
    if (!this->currentlyConnectedNodes.contains(nodeId)) {
      auto newNodeInfo = blockchain.p2p->requestNodeInfo(nodeId);
      if (newNodeInfo != P2P::NodeInfo()) {
        this->currentlyConnectedNodes[nodeId] = newNodeInfo;
      }
    }
  }
}

bool Syncer::checkLatestBlock() { return (this->latestBlock != this->blockchain.storage->latest()); }

void Syncer::doSync() {
  // TODO: Fully implement Sync
  this->latestBlock = blockchain.storage->latest();
  // Get the list of currently connected nodes and their current height
  this->updateCurrentlyConnectedNodes();
  std::pair<P2P::NodeID, uint64_t> highestNode = {P2P::NodeID(), 0};

  // Get the highest node.
  for (auto& [nodeId, nodeInfo] : this->currentlyConnectedNodes) {
    if (nodeInfo.latestBlockHeight > highestNode.second) {
      highestNode = {nodeId, nodeInfo.latestBlockHeight};
    }
  }

  // Sync from the best node.
  if (highestNode.second > this->latestBlock->getNHeight()) {
    // TODO: currently we are starting all the nodes from genesis (0)
  }

  this->latestBlock = blockchain.storage->latest();
  synced = true;
  return;
}

void Syncer::doValidatorBlock() {
  // TODO: Improve this somehow.
  // Wait until we have enough transactions in the rdpos mempool.
  while (this->blockchain.rdpos->getMempool().size() < rdPoS::minValidators * 2) {
    if (this->stopSyncer) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Wait until we have at least one transaction in the state mempool.
  while (this->blockchain.state->getMempoolSize() < 1) {
    if (this->stopSyncer) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Create the block.
  if (this->stopSyncer) return;
  auto mempool = this->blockchain.rdpos->getMempool();
  auto randomList = this->blockchain.rdpos->getRandomList();

  // Order the transactions in the proper manner.
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomnessTxs;
  uint64_t i = 1;
  while (randomHashTxs.size() != rdPoS::minValidators) {
    for (const auto [txHash, tx]: mempool) {
      if (this->stopSyncer) return;
      if (tx.getFrom() == randomList[i]) {
        if (tx.getFunctor() == Hex::toBytes("0xcfffe746")) {
          randomHashTxs.emplace_back(tx);
          i++;
          break;
        }
      }
    }
  }
  i = 1;
  while (randomnessTxs.size() != rdPoS::minValidators) {
    for (const auto [txHash, tx]: mempool) {
      if (tx.getFrom() == randomList[i]) {
        if (tx.getFunctor() == Hex::toBytes("0x6fc5a2d6")) {
          randomnessTxs.emplace_back(tx);
          i++;
          break;
        }
      }
    }
  }
  if (this->stopSyncer) return;

  // Create the block and append to all chains, we can use any storage for latest block.
  const std::shared_ptr<const Block> latestBlock = this->blockchain.storage->latest();
  Block block(latestBlock->hash(), latestBlock->getTimestamp(), latestBlock->getNHeight() + 1);

  // Append transactions towards block.
  for (const auto& tx: randomHashTxs) block.appendTxValidator(tx);
  for (const auto& tx: randomnessTxs) block.appendTxValidator(tx);
  if (this->stopSyncer) return;

  // Add transactions from state, sign, validate and process the block.
  this->blockchain.state->fillBlockWithTransactions(block);
  this->blockchain.rdpos->signBlock(block);
  if (!this->blockchain.state->validateNextBlock(block)) {
    Utils::logToDebug(Log::syncer, __func__, "Block is not valid!");
    throw std::runtime_error("Block is not valid!");
  }
  if (this->stopSyncer) return;
  Hash latestBlockHash = block.hash();
  this->blockchain.state->processNextBlock(std::move(block));
  if (this->blockchain.storage->latest()->hash() != latestBlockHash) {
    Utils::logToDebug(Log::syncer, __func__, "Block is not valid!");
    throw std::runtime_error("Block is not valid!");
  }

  // Broadcast the block through P2P
  if (this->stopSyncer) return;
  this->blockchain.p2p->broadcastBlock(this->blockchain.storage->latest());
  return;
}

void Syncer::doValidatorTx() {
  ; // There is nothing to do, validatorLoop will wait for the next block.
}

void Syncer::validatorLoop() {
  Utils::logToDebug(Log::syncer, __func__, "Starting validator loop.");
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->blockchain.options->getValidatorPrivKey())));
  this->blockchain.rdpos->startrdPoSWorker();
  while (!this->stopSyncer) {
    this->latestBlock = this->blockchain.storage->latest();
    // Check if validator is within the current validator list.
    const auto currentRandomList = this->blockchain.rdpos->getRandomList();
    bool isBlockCreator = false;
    if (currentRandomList[0] == me) {
      isBlockCreator = true;
      this->doValidatorBlock();
    }

    if (this->stopSyncer) return;
    if (!isBlockCreator) this->doValidatorTx();

    while (!this->checkLatestBlock() && !this->stopSyncer) {
      // Wait for next block to be created.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  return;
}

void Syncer::nonValidatorLoop() {
  // TODO: Improve tx broadcasting and syncing
  while (!this->stopSyncer) std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return;
}

bool Syncer::syncerLoop() {
  Utils::safePrint("Starting OrbiterSDK Node...");
  Utils::logToDebug(Log::syncer, __func__, "Starting syncer loop.");
  // Connect to all seed nodes from the config and start the discoveryThread.
  auto discoveryNodeList = this->blockchain.options->getDiscoveryNodes();
  for (const auto &[ipAddress, port]: discoveryNodeList) {
    this->blockchain.p2p->connectToServer(ipAddress, port);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  this->blockchain.p2p->startDiscovery();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Sync the node with the network.
  this->doSync();
  if (this->stopSyncer) return false;
  Utils::safePrint("Synced with the network, starting the node.");
  if (this->blockchain.options->getIsValidator()) {
    this->validatorLoop();
  } else {
    this->nonValidatorLoop();
  }
  return true;
}

void Syncer::start() {
  if (!this->syncerLoopFuture.valid()) {
    this->syncerLoopFuture = std::async(std::launch::async, &Syncer::syncerLoop, this);
  }
  return;
}

void Syncer::stop() {
  this->stopSyncer = true;
  this->blockchain.rdpos->stoprdPoSWorker(); // Stop the rdPoS worker.
  if (this->syncerLoopFuture.valid()) this->syncerLoopFuture.wait();
  return;
}

