#include "snowmanVM.h"

void SnowmanVM::initialize(
  const vm::InitializeRequest* request, vm::InitializeResponse* reply
) {
  // TODO: this->initialized does not exist here
  //if (this->initialized) {
  //  Utils::logToDebug(Log::snowmanVM, __func__, "Already initialized");
  //  throw std::runtime_error(std::string(__func__) + ": " + std::string("Already initialized"));
  //}
  //this->initialized = true;

  // Get the required init params from AvalancheGo
  std::string jsonRequest;
  google::protobuf::util::JsonOptions options;
  google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
  this->initParams.networkId = request->network_id();
  this->initParams.subnetId = request->subnet_id();
  this->initParams.chainId = request->chain_id();
  this->initParams.nodeId = request->node_id();
  this->initParams.xChainId = request->x_chain_id();
  this->initParams.avaxAssetId = request->avax_asset_id();
  this->initParams.genesisBytes = request->genesis_bytes();
  this->initParams.upgradeBytes = request->upgrade_bytes();
  this->initParams.configBytes = request->config_bytes();
  for (int i = 0; i < request->db_servers_size(); i++) {
    auto db_server = request->db_servers(i);
    this->initParams.dbServers.emplace_back(db_server.server_addr(), db_server.version());
  }
  this->initParams.gRPCServerAddress = request->server_addr();

  // Initialize pointers to other parts of the program
  //this->db = std::make_shared<DB>(this->initParams.nodeId); // TODO: DB pointer does not exist here
  //this->state = std::make_shared<State>(this->db);  // TODO: State pointer does not exist here

  // Read the config file and parse the latest block to answer AvalancheGo
  json config = Utils::readConfigFile();
  const std::shared_ptr<const Block> latest = this->storage->latest();
  reply->set_last_accepted_id(latest->getBlockHash().get());
  reply->set_last_accepted_parent_id(latest->getPrevBlockHash().get());
  reply->set_height(latest->getNHeight());
  reply->set_bytes(latest->serializeToBytes(false));
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(latest->getTimestamp() / 1000000000);
  timestamp->set_nanos(latest->getTimestamp() % 1000000000);

  // Initialize P2P, rdPoS and HTTP server
  // TODO: P2P pointer does not exist here
  //this->p2p = std::make_shared<P2PManager>(
  //  boost::asio::ip::address::from_string("127.0.0.1"),
  //  config["p2pport"].get<unsigned short>(), 2, this->storage, *this
  //);
  //this->p2p->startServer();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  for (auto i : config["seedNodes"]) {
    std::vector<std::string> seedNode;
    boost::split(seedNode, i.get<std::string>(), boost::is_any_of(":"));
    //this->p2p->connectToServer(
    //  boost::asio::ip::address::from_string(seedNode[0]), std::stoi(seedNode[1])
    //);
  }

  // TODO: rdPoS pointer does not exist here
  //this->rdpos = std::make_shared<rdPoS>(
  //  this->db, this->storage, this->p2p,
  //  ContractAddresses::BlockManager,
  //  Address("0x0000000000000000000000000000000000000000", true),
  //  (config.contains("validatorPrivKey"))
  //    ? Hash(Utils::hexToBytes(config["validatorPrivKey"].get<std::string>())) : ""
  //);

  // TODO: HTTP pointer does not exist here
  //this->http = std::make_unique<HTTPServer>(*this, config["rpcport"].get<unsigned short>());
  //std::thread httpThread = std::thread([&]{this->http->run();});
  //httpThread.detach();
  std::string jsonReply;
  google::protobuf::util::MessageToJsonString(*reply, &jsonReply, options);
  Utils::logToFile(jsonReply);
}

bool SnowmanVM::parseBlock(
  ServerContext* context, const std::string& blockBytes, vm::ParseBlockResponse* reply
) const {
  try {
    // Check if block already exists on chain head or chain tip
    std::shared_ptr<Block> block = std::make_shared<Block>(blockBytes, false);
    Hash hash = block->getBlockHash();
    bool onStorage = this->storage->exists(hash);
    bool onMempool = this->blockExists(hash);
    if (onStorage || onMempool) {
      const std::shared_ptr<const Block> block = (onStorage)
        ? this->storage->getBlock(hash) : this->getBlock(hash);
      reply->set_id(block->getBlockHash().get());
      reply->set_parent_id(block->getPrevBlockHash().get());
      reply->set_status(BlockStatus::Accepted);
      reply->set_height(block->getNHeight());
      auto timestamp = reply->mutable_timestamp();
      timestamp->set_seconds(block->getTimestamp() / 1000000000);
      timestamp->set_nanos(block->getTimestamp() % 1000000000);
      Utils::logToDebug(Log::snowmanVM, __func__,
        std::string("Block ") + std::to_string(block->getNHeight())
        + "already exists, returning Accepted"
      );
      return true;
    }

    // Build block, parse it and get latest accepted as reference, by AvalancheGo's process:
    // https://github.com/ava-labs/avalanchego/blob/master/vms/README.md#processing-blocks
    const std::shared_ptr<const Block> latest = this->storage->latest();
    reply->set_id(block->getBlockHash().get());
    reply->set_parent_id(block->getPrevBlockHash().get());
    reply->set_height(block->getNHeight());
    if (block->getNHeight() <= latest->getNHeight()) {
      reply->set_status(BlockStatus::Rejected);
      Utils::logToDebug(Log::snowmanVM, __func__,
        std::string("Block: ") + Utils::bytesToHex(block->getBlockHash().get())
        + "(" + std::to_string(block->getNHeight()) + ") is lower than latest ("
        + std::to_string(latestBlock->getNHeight()) + "), returning Rejected"
      );
    } else if (block->getNHeight() > latest->getNHeight()) {
      // We don't know anything about a future block, so we just say we are processing it
      reply->set_status(BlockStatus::Processing);
      Utils::logToDebug(Log::snowmanVM, __func__,
        std::string("Block: ") + Utils::bytesToHex(block->getBlockHash().get())
        + "(" + std::to_string(block->getNHeight()) + ") is higher than latest ("
        + std::to_string(latestBlock->getNHeight()) + "), returning Processing"
      );
      //this->rdpos->processBlock(block);  // TODO: rdPoS pointer doesn't exist here
    }
    auto timestamp = reply->mutable_timestamp();
    timestamp->set_seconds(block->getTimestamp() / 1000000000);
    timestamp->set_nanos(block->getTimestamp() % 1000000000);
    Utils::logToDebug(Log::snowmanVM, __func__, "Block is valid");
  } catch (std::exception &e) {
    Utils::logToDebug(Log::snowmanVM, __func__,
      std::string("Error parsing block") + e.what()
    );
    return false;
  }
  return true;
}

void SnowmanVM::setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply) const {
  Utils::logToDebug(Log::snowmanVM, __func__,
    std::string("Setting State to: ") + std::to_string(request->state())
  );
  // TODO: rdPoS pointer does not exist here
  //if (request->state() == 3) this->rdpos->startValidatorThread(); // NormalOp
  const std::shared_ptr<const Block> bestBlock = this->storage->latest();
  reply->set_last_accepted_id(bestBlock->getBlockHash().get());
  reply->set_last_accepted_parent_id(bestBlock->getPrevBlockHash().get());
  reply->set_height(bestBlock->getNHeight());
  reply->set_bytes(bestBlock->serializeToBytes(false));
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(bestBlock->getTimestamp() / 1000000000);
  timestamp->set_nanos(bestBlock->getTimestamp() % 1000000000);
}

bool SnowmanVM::blockRequest(ServerContext* context, vm::BuildBlockResponse* reply) const {
  //auto newBlock = this->state->createNewBlock(); // TODO: State pointer doesn't exist here
  if (newBlock == nullptr) {
    Utils::logToDebug(Log::snowmanVM, __func__, "Could not create new block");
    return false;
  }
  Utils::logToDebug(Log::snowmanVM, __func__, "Trying to answer AvalancheGo");
  Utils::logToDebug(Log::snowmanVM, __func__, std::string("New block created: ")
    + Utils::bytesToHex(newBlock->getBlockHash().get())
  );
  reply->set_id(newBlock->getBlockHash().get());
  reply->set_parent_id(newBlock->getPrevBlockHash().get());
  reply->set_height(newBlock->getNHeight());
  reply->set_bytes(newBlock->serializeToBytes(false));
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(newBlock->getTimestamp() / 1000000000);
  timestamp->set_nanos(newBlock->getTimestamp() % 1000000000);
  Utils::logToDebug(Log::snowmanVM, __func__, "New block broadcast but not enforced");
  return true;
}

void SnowmanVM::getBlock(
  ServerContext* context, const vm::GetBlockRequest* request, vm::GetBlockResponse* reply
) const {
  Hash hash(request->id());
  if (this->storage->exists(hash)) {
    const std::shared_ptr<const Block> block = this->storage->getBlock(hash);
    reply->set_parent_id(block->getPrevBlockHash().get());
    reply->set_bytes(block->serializeToBytes(false));
    reply->set_status(BlockStatus::Accepted);
    reply->set_height(block->getNHeight());
    auto timestamp = reply->mutable_timestamp();
    timestamp->set_seconds(block->getTimestamp() / 1000000000);
    timestamp->set_nanos(block->getTimestamp() % 1000000000);
    Utils::logToDebug(Log::snowmanVM, __func__,
      "Block found in chain: " + Utils::bytesToHex(block->serializeToBytes(false))
    );
  } else if (this->blockExists(hash)) {
    const std::shared_ptr<const Block> block = this->getBlock(hash);
    reply->set_parent_id(block->getPrevBlockHash().get());
    reply->set_bytes(block->serializeToBytes(false));
    reply->set_status(this->getBlockStatus(hash));
    reply->set_height(block->getNHeight());
    auto timestamp = reply->mutable_timestamp();
    timestamp->set_seconds(block->getTimestamp() / 1000000000);
    timestamp->set_nanos(block->getTimestamp() % 1000000000);
    Utils::logToDebug(Log::snowmanVM, __func__,
      "Block found in mempool: " + Utils::bytesToHex(block->serializeToBytes(false))
    );
  } else {
    reply->set_status(BlockStatus::Unknown);
    reply->set_err(2); // https://github.com/ava-labs/avalanchego/blob/559ce151a6b6f28d8115e0189627d8deaf00d9fb/vms/rpcchainvm/errors.go#L21
    Utils::logToDebug(Log::snowmanVM, __func__,
      "Block " + Utils::bytesToHex(request->id()) + " does not exist"
    );
  }
}

bool SnowmanVM::getAncestors(
  ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply
) const {
  Utils::LogPrint(Log::snowmanVM, __func__,
    std::string("Getting ancestors of block ") + Utils::bytesToHex(hash.get())
    + " with depth of " + std::to_string(request->max_blocks_num()) + " up to "
    + std::to_string(request->max_blocks_size()) + " bytes and/or for "
    + std::to_string(request->max_blocks_retrival_time()) + " nanosseconds"
  );
  Hash hash(request->blk_id());
  if (!this->storage->exists(hash)) return false;
  const std::shared_ptr<const Block> head = this->storage->getBlock(hash);
  const std::shared_ptr<const Block> best = this->storage->latest();
  uint64_t depth = request->max_blocks_num();
  uint64_t maxSize = request->max_blocks_size(); // In bytes
  uint64_t maxTime = request->max_blocks_retrival_time(); // In nanosseconds

  // Depth can be actually higher than chain height, so we set it to chain height
  if (depth > best->nHeight()) {
    Utils::logToDebug(Log::snowmanVM, __func__,
      "Depth is higher than chain height, setting depth to chain height"
    );
    depth = best->getNHeight();
  }
  auto timeStart = std::chrono::system_clock::now();
  for (
    uint64_t index = (head->nHeight());
    index >= (head->nHeight() - depth) && index <= head->nHeight();
    index--
  ) {
    const std::shared_ptr<const Block> block = chainHead->getBlock(index);
    reply->add_blks_bytes(block->serializeToBytes(false));
    auto timeEnd = std::chrono::system_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::nanoseconds>(timeEnd - timeStart).count();
    if (reply->blks_bytes().size() > maxSize || timeDiff > maxTime) {
      Utils::logToDebug(Log::snowmanVM, __func__, "Max block byte size reached or time ran out");
      return false;
    }
  }
  Utils::logToDebug(Log::snowmanVM, __func__, "Ancestors found, replying back");
  return true;
}

void SnowmanVM::setPreference(ServerContext* context, const vm::SetPreferenceRequest* request) {
  this->setPreferredBlockHash(Hash(request->id()));
}

const BlockStatus SnowmanVM::getBlockStatus(const Hash& hash) const {
  BlockStatus ret = BlockStatus::Unknown;
  this->lock.lock();
  if (this->cachedBlockStatus.count(hash) > 0) {
    ret = this->cachedBlockStatus.find(hash)->second;
  }
  this->lock.unlock();
  return ret;
}

void SnowmanVM::setBlockStatus(const Hash& hash, const BlockStatus& status) {
  this->lock.lock();
  this->cachedBlockStatus[hash] = status;
  this->lock.unlock();
}

const std::shared_ptr<const Block> SnowmanVM::verifyBlock(const std::string bytes) const {
  // Check if block can be attached to top of the chain, if so add it to processing
  const std::shared_ptr<const Block> block = std::make_shared<Block>(blockBytes, false);
  //if (!this->state->validateNewBlock(block)) return nullptr; // TODO: State pointer doesn't exist here
  //this->rdpos->processBlock(block); // TODO: rdPoS pointer doesn't exist here
  return this->getBlock(block->getBlockHash());
}

bool SnowmanVM::acceptBlock(const Hash& hash) {
  this->lock.lock();
  auto it = this->mempool.find(hash);
  if (it == this->mempool.end()) {
    Utils::logToDebug(Log::snowmanVM, __func__, "Block not found");
    this->lock.unlock();
    return false;
  }
  if (it->second.unique()) {
    Utils::logToDebug(Log::snowmanVM, __func__, "Block is unique, moving to processNewBlock");
    //this->state->processNewBlock(std::move(it->second));  // TODO: State pointer doesn't exist here
    this->mempool.erase(hash);
  } else {
    // We have to create a copy tof the block to process it
    Utils::logToDebug(Log::snowmanVM, __func__, "Block is not unique, copying to processNewBlock");
    std::shared_ptr<Block> block = std::make_shared<Block>(*it->second);
    //this->state->processNewBlock(std::move(block)); // TODO: State pointer doesn't exist here
  }
  this->cachedBlockStatus[hash] = BlockStatus::Accepted;
  this->lock.unlock();
  return true;
}

void SnowmanVM::rejectBlock(const Hash& hash) {
  this->lock.lock();
  this->mempool.erase(hash);
  this->cachedBlockStatus[hash] = BlockStatus::Rejected;
  this->lock.unlock();
}

bool SnowmanVM::blockExists(const Hash& hash) const {
  this->lock.lock();
  bool ret = (this->mempool.count(hash) > 0);
  this->lock.unlock();
  return ret;
}

bool SnowmanVM::blockIsProcessing(const Hash& hash) const {
  bool ret = false;
  this->lock.lock();
  if (this->cachedBlockStatus.count(hash) > 0) {
    ret = (this->cachedBlockStatus.find(hash)->second == BlockStatus::Processing);
  }
  this->lock.unlock();
  return ret;
}

const std::shared_ptr<const Block> SnowmanVM::getBlock(const Hash& hash) const {
  this->lock.lock();
  auto it = this->mempool.find(hash);
  std::shared_ptr<const Block>& ret = (it != this->mempool.end()) ? it->second : nullptr;
  this->lock.unlock();
  return ret;
}

void SnowmanVM::connectNode(const std::string& id) {
  this->connectedNodesLock.lock();
  Utils::logToDebug(Log::snowmanVM, __func__,
    "Connecting node: " + Utils::bytesToHex(id)
  );
  this->connectedNodes.emplace_back(id);
  this->connectedNodesLock.unlock();
}

void SnowmanVM::disconnectNode(const std::string& id) {
  this->connectedNodesLock.lock();
  for (uint64_t i = 0; i < this->connectedNodes.size(); i++) {
    if (this->connectedNodes[i] == id) {
      Utils::logToDebug(Log::snowmanVM, __func__,
        "Disconnecting node: " + Utils::bytesToHex(id)
      );
      this->connectedNodes.erase(this->connectedNodes.begin() + i);
      break;
    }
  }
  this->connectedNodesLock.unlock();
}

