#include "blockchain.h"

Blockchain::Blockchain(std::string blockchainPath) :
  options_(std::make_unique<Options>(Options::fromFile(blockchainPath))),
  db_(std::make_unique<DB>(blockchainPath + "/database")),
  storage_(std::make_unique<Storage>(db_, options_)),
  rdpos_(std::make_unique<rdPoS>(db_, storage_, p2p_, options_, state_)),
  state_(std::make_unique<State>(db_, storage_, rdpos_, p2p_, options_)),
  p2p_(std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), rdpos_, options_, storage_, state_)),
  http_(std::make_unique<HTTPServer>(state_, storage_, p2p_, options_)),
  syncer_(std::make_unique<Syncer>(*this))
{}

void Blockchain::start() { p2p_->start(); http_->start(); syncer_->start(); }

void Blockchain::stop() { syncer_->stop(); http_->stop(); p2p_->stop(); }

const std::atomic<bool>& Blockchain::isSynced() const { return this->syncer_->isSynced(); }

void Syncer::updateCurrentlyConnectedNodes() {
  // Get the list of currently connected nodes
  std::vector<P2P::NodeID> connectedNodes = blockchain_.p2p_->getSessionsIDs();
  while (connectedNodes.size() < blockchain_.p2p_->minConnections() && !this->stopSyncer_) {
    Logger::logToDebug(LogType::INFO, Log::syncer, __func__,
      "Waiting for discoveryWorker to connect to more nodes, currently connected to: "
      + std::to_string(connectedNodes.size())
    );
    // If we have less than the minimum number of connections,
    // wait for a bit for discoveryWorker to kick in and connect to more nodes
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connectedNodes = blockchain_.p2p_->getSessionsIDs();
  }

  // Update information of already connected nodes
  for (auto& [nodeId, nodeInfo] : this->currentlyConnectedNodes_) {
    // If node is not connected, remove it from the list
    if (std::find(connectedNodes.begin(), connectedNodes.end(), nodeId) == connectedNodes.end()) {
      this->currentlyConnectedNodes_.erase(nodeId);
      continue;
    }
    // If node is connected, update its information
    auto newNodeInfo = blockchain_.p2p_->requestNodeInfo(nodeId);
    // If node is not responding, remove it from the list
    if (newNodeInfo == P2P::NodeInfo()) {
      this->currentlyConnectedNodes_.erase(nodeId);
      continue;
    }
    // If node is responding, update its information
    this->currentlyConnectedNodes_[nodeId] = newNodeInfo;
  }

  // Add new nodes to the list
  for (auto& nodeId : connectedNodes) {
    if (!this->currentlyConnectedNodes_.contains(nodeId)) {
      auto newNodeInfo = blockchain_.p2p_->requestNodeInfo(nodeId);
      if (newNodeInfo != P2P::NodeInfo()) {
        this->currentlyConnectedNodes_[nodeId] = newNodeInfo;
      }
    }
  }
}

bool Syncer::checkLatestBlock() { return (this->latestBlock_ != this->blockchain_.storage_->latest()); }

void Syncer::doSync() {
  // TODO: Fully implement Sync
  this->latestBlock_ = blockchain_.storage_->latest();
  // Get the list of currently connected nodes and their current height
  this->updateCurrentlyConnectedNodes();
  std::pair<P2P::NodeID, uint64_t> highestNode = {P2P::NodeID(), 0};

  // Get the highest node.
  for (auto& [nodeId, nodeInfo] : this->currentlyConnectedNodes_) {
    if (nodeInfo.latestBlockHeight > highestNode.second) {
      highestNode = {nodeId, nodeInfo.latestBlockHeight};
    }
  }

  // Sync from the best node.
  if (highestNode.second > this->latestBlock_->getNHeight()) {
    // TODO: currently we are starting all the nodes from genesis (0)
  }

  this->latestBlock_ = blockchain_.storage_->latest();
  this->synced_ = true;
  return;
}

void Syncer::doValidatorBlock() {
  // TODO: Improve this somehow.
  // Wait until we have enough transactions in the rdpos mempool.
  while (this->blockchain_.rdpos_->getMempool().size() < rdPoS::minValidators * 2) {
    if (this->stopSyncer_) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Wait until we have at least one transaction in the state mempool.
  while (this->blockchain_.state_->getMempoolSize() < 1) {
    if (this->stopSyncer_) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Create the block.
  if (this->stopSyncer_) return;
  auto mempool = this->blockchain_.rdpos_->getMempool();
  auto randomList = this->blockchain_.rdpos_->getRandomList();

  // Order the transactions in the proper manner.
  std::vector<TxValidator> randomHashTxs;
  std::vector<TxValidator> randomnessTxs;
  uint64_t i = 1;
  while (randomHashTxs.size() != rdPoS::minValidators) {
    for (const auto [txHash, tx]: mempool) {
      if (this->stopSyncer_) return;
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
  if (this->stopSyncer_) return;

  // Create the block and append to all chains, we can use any storage for latest block.
  const std::shared_ptr<const Block> latestBlock = this->blockchain_.storage_->latest();
  Block block(latestBlock->hash(), latestBlock->getTimestamp(), latestBlock->getNHeight() + 1);

  // Append transactions towards block.
  for (const auto& tx: randomHashTxs) block.appendTxValidator(tx);
  for (const auto& tx: randomnessTxs) block.appendTxValidator(tx);
  if (this->stopSyncer_) return;

  // Add transactions from state, sign, validate and process the block.
  this->blockchain_.state_->fillBlockWithTransactions(block);
  this->blockchain_.rdpos_->signBlock(block);
  if (!this->blockchain_.state_->validateNextBlock(block)) {
    Logger::logToDebug(LogType::ERROR, Log::syncer, __func__, "Block is not valid!");
    throw std::runtime_error("Block is not valid!");
  }
  if (this->stopSyncer_) return;
  Hash latestBlockHash = block.hash();
  this->blockchain_.state_->processNextBlock(std::move(block));
  if (this->blockchain_.storage_->latest()->hash() != latestBlockHash) {
    Logger::logToDebug(LogType::ERROR, Log::syncer, __func__, "Block is not valid!");
    throw std::runtime_error("Block is not valid!");
  }

  // Broadcast the block through P2P
  if (this->stopSyncer_) return;
  this->blockchain_.p2p_->broadcastBlock(this->blockchain_.storage_->latest());
  return;
}

void Syncer::doValidatorTx() {
  ; // There is nothing to do, validatorLoop will wait for the next block.
}

void Syncer::validatorLoop() {
  Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Starting validator loop.");
  Validator me(Secp256k1::toAddress(Secp256k1::toUPub(this->blockchain_.options_->getValidatorPrivKey())));
  this->blockchain_.rdpos_->startrdPoSWorker();
  while (!this->stopSyncer_) {
    this->latestBlock_ = this->blockchain_.storage_->latest();
    // Check if validator is within the current validator list.
    const auto currentRandomList = this->blockchain_.rdpos_->getRandomList();
    bool isBlockCreator = false;
    if (currentRandomList[0] == me) {
      isBlockCreator = true;
      this->doValidatorBlock();
    }

    if (this->stopSyncer_) return;
    if (!isBlockCreator) this->doValidatorTx();

    while (!this->checkLatestBlock() && !this->stopSyncer_) {
      // Wait for next block to be created.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  return;
}

void Syncer::nonValidatorLoop() {
  // TODO: Improve tx broadcasting and syncing
  while (!this->stopSyncer_) std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return;
}

bool Syncer::syncerLoop() {
  Utils::safePrint("Starting OrbiterSDK Node...");
  Logger::logToDebug(LogType::INFO, Log::syncer, __func__, "Starting syncer loop.");
  // Connect to all seed nodes from the config and start the discoveryThread.
  auto discoveryNodeList = this->blockchain_.options_->getDiscoveryNodes();
  for (const auto &[ipAddress, port]: discoveryNodeList) {
    this->blockchain_.p2p_->connectToServer(ipAddress, port);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  this->blockchain_.p2p_->startDiscovery();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Sync the node with the network.
  this->doSync();
  if (this->stopSyncer_) return false;
  Utils::safePrint("Synced with the network, starting the node.");
  if (this->blockchain_.options_->getIsValidator()) {
    this->validatorLoop();
  } else {
    this->nonValidatorLoop();
  }
  return true;
}

void Syncer::start() {
  if (!this->syncerLoopFuture_.valid()) {
    this->syncerLoopFuture_ = std::async(std::launch::async, &Syncer::syncerLoop, this);
  }
  return;
}

void Syncer::stop() {
  this->stopSyncer_ = true;
  this->blockchain_.rdpos_->stoprdPoSWorker(); // Stop the rdPoS worker.
  if (this->syncerLoopFuture_.valid()) this->syncerLoopFuture_.wait();
  return;
}

