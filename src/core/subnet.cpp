#include "subnet.h"

void Subnet::start() {
  /**
   * When starting the binary, the first thing to setup is the gRPC server,
   * as the AvalancheGo Daemon node will be waiting for the gRPC server
   * to answer on the terminal.
   */
  // Get a random number between 50000 and 60000
  std::random_device rd; // obtain a random number from hardware
  std::mt19937 gen(rd()); // seed the generator
  std::uniform_int_distribution<> distr(50000, 60000);
  unsigned short port = distr(gen);
  Utils::LogPrint(Log::subnet, __func__, std::string("Starting subnet at port: ") + std::to_string(port));
  std::string server_address(std::string("0.0.0.0:") + std::to_string(port));
  grpcServer = std::make_shared<VMServiceImplementation>(*this);
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  /**
   * Register "service" as the instance through which we'll communicate with
   * clients. In this case it corresponds to a *synchronous* service.
   */
  builder.RegisterService(grpcServer.get());

  /**
   * Assemble the server and send the address of the gRPC server
   * to the AvalancheGo Daemon.
   */
  server = builder.BuildAndStart();
  std::cout << "1|16|tcp|" << server_address << "|grpc\n" << std::flush;

  /**
   * Wait for the server to shutdown. Note that some other thread must be
   * responsible for shutting down the server for this call to ever return.
   */
  Utils::LogPrint(Log::subnet, __func__, "Startup Done");
  server->Wait();
  Utils::LogPrint(Log::subnet, __func__, "Server Thread Returning...");
  return;
}

void Subnet::stop() {
  if (this->initialized) {
    Utils::LogPrint(Log::subnet, __func__, "Stopping subnet...");
    this->shutdown = true;
    // Dump State and ChainHead from memory to the database, then close it
    this->chainHead->dumpToDB();
    Utils::LogPrint(Log::subnet, __func__, "chainHead saved to DB");
    this->headState->saveState(this->dbServer);
    Utils::LogPrint(Log::subnet, __func__, "headState saved to DB");
    this->dbServer->close();
    Utils::LogPrint(Log::subnet, __func__, "DB closed successfully");
    this->httpServer->stop();
    for (;;) {
      if (!this->httpServer->isRunning()) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Utils::LogPrint(Log::subnet, __func__, "HTTP Server stopped");
    // Sleep for 2 seconds and wait for Server shutdown answer
    Utils::LogPrint(Log::subnet, __func__, "Waiting for Server to shutdown...");
    // Subnet::stop() is called from the gRPC Server, so we cannot stop the server here.
    // A thread is created and detached there calling the function below.
  }
  return;
}


void Subnet::shutdownServer() {
  if (this->initialized) {
    if (this->shutdown) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      server->Shutdown();
    }
  }
}

void Subnet::initialize(const vm::InitializeRequest* request, vm::InitializeResponse* reply) {
  Utils::logToFile("Initialize");
  /**
   * The initialization request is made by the AvalancheGo Daemon.
   * See vm.proto for more information.
   */
  if (this->initialized) {  // Subnet was already initialized. This is not allowed.
    Utils::LogPrint(Log::subnet, __func__, "Subnet already initialized");
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Subnet already initialized")
    );
  } else {
    this->initialized = true;
  }

  std::string jsonRequest;
  google::protobuf::util::JsonOptions options;
  google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
  Utils::logToFile(jsonRequest);

  this->initParams.networkId = request->network_id();
  this->initParams.subnetId = request->subnet_id();
  this->initParams.chainId = request->chain_id();
  this->initParams.nodeId = request->node_id();
  this->initParams.xChainId = request->x_chain_id();
  this->initParams.avaxAssetId = request->avax_asset_id();
  this->initParams.genesisBytes = request->genesis_bytes();
  this->initParams.upgradeBytes = request->upgrade_bytes();
  this->initParams.configBytes = request->config_bytes();

  bool hasDBServer = false;
  for (int i = 0; i < request->db_servers_size(); i++) {
    auto db_server = request->db_servers(i);
    this->initParams.dbServers.emplace_back(db_server.server_addr(), db_server.version());
    hasDBServer = true;
  }

  this->initParams.gRPCServerAddress = request->server_addr();

  /**
   * Initialize the DB to get information about the subnet there.
   * We are assuming that we are NOT running inside a sandbox.
   */
  dbServer = std::make_shared<DBService>(this->initParams.nodeId);

  // Initialize the gRPC client to communicate back with AvalancheGo.
  grpcClient = std::make_shared<VMCommClient>(grpc::CreateChannel(this->initParams.gRPCServerAddress, grpc::InsecureChannelCredentials()), this->connectedNodes, this->connectedNodesLock);

  // Initialize the State and ChainHead.
  #if !IS_LOCAL_TESTS
    this->headState = std::make_shared<State>(this->dbServer, this->grpcClient);
  #else
    this->headState = std::make_shared<State>(this->dbServer);
  #endif
  this->chainHead = std::make_shared<ChainHead>(this->dbServer);
  this->chainTip = std::make_shared<ChainTip>();
  this->blockManager = std::make_shared<BlockManager>(this->dbServer);

  // Parse the latest block to answer AvalancheGo.
  auto latestBlock = chainHead->latest();
  reply->set_last_accepted_id(latestBlock->getBlockHash().get());
  reply->set_last_accepted_parent_id(latestBlock->prevBlockHash().get());
  reply->set_height(latestBlock->nHeight());
  reply->set_bytes(latestBlock->serializeToBytes(false));
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(latestBlock->timestamp() / 1000000000);
  timestamp->set_nanos(latestBlock->timestamp() % 1000000000);

  // Start the HTTP Server.
  Utils::logToFile("Starting HTTP");
  this->httpServer = std::make_unique<HTTPServer>(*this);
  std::thread httpServerThread = std::thread([&]
  {
    this->httpServer->run();
  });
  httpServerThread.detach();
  Utils::logToFile("HTTP Started");
  std::string jsonReply;
  google::protobuf::util::MessageToJsonString(*reply, &jsonReply, options);
  Utils::logToFile(jsonReply);
}

void Subnet::setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply) {
  /**
   * See vm.proto and https://github.com/ava-labs/avalanchego/blob/master/snow/engine/snowman/bootstrap/bootstrapper.go#L111
   * for more information about the SetState request.
   */
  auto bestBlock = chainHead->latest();
  reply->set_last_accepted_id(bestBlock->getBlockHash().get());
  reply->set_last_accepted_parent_id(bestBlock->prevBlockHash().get());
  reply->set_height(bestBlock->nHeight());
  reply->set_bytes(bestBlock->serializeToBytes(false));
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(bestBlock->timestamp() / 1000000000);
  timestamp->set_nanos(bestBlock->timestamp() % 1000000000);
}

bool Subnet::blockRequest(ServerContext* context, vm::BuildBlockResponse* reply) {
  auto newBlock = this->headState->createNewBlock(this->chainHead, this->chainTip);
  if (newBlock == nullptr) {
    Utils::LogPrint(Log::subnet, __func__, "Could not create new block");
    return false;
  }
  // Answer back avalanchego.
  Utils::LogPrint(Log::subnet, __func__, "Trying to answer AvalancheGo");
  Utils::LogPrint(Log::subnet, __func__, std::string("New block created: ") + Utils::bytesToHex(newBlock->getBlockHash().get()));
  reply->set_id(newBlock->getBlockHash().get());
  reply->set_parent_id(newBlock->prevBlockHash().get());
  reply->set_height(newBlock->nHeight());
  reply->set_bytes(newBlock->serializeToBytes(false));
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(newBlock->timestamp() / 1000000000);
  timestamp->set_nanos(newBlock->timestamp() % 1000000000);
  Utils::LogPrint(Log::subnet, __func__, "New block broadcasted, but not enforced.");
  return true;
}

bool Subnet::parseBlock(ServerContext* context, const std::string& blockBytes, vm::ParseBlockResponse* reply) {
  try {
    // Check if block already exists on chain head or chain tip
    Hash blockHash = Utils::sha3(blockBytes);
    bool onHead = chainHead->exists(blockHash);
    bool onTip = chainTip->exists(blockHash);
    if (onHead || onTip) {
      auto block = ((onHead) ? chainHead->getBlock(blockHash) : chainTip->getBlock(blockHash));
      reply->set_id(block->getBlockHash().get());
      reply->set_parent_id(block->prevBlockHash().get());
      reply->set_status(BlockStatus::Accepted);
      reply->set_height(block->nHeight());
      auto timestamp = reply->mutable_timestamp();
      timestamp->set_seconds(block->timestamp() / 1000000000);
      timestamp->set_nanos(block->timestamp() % 1000000000);
      Utils::LogPrint(Log::subnet, __func__, std::string("Block ") + std::to_string(block->nHeight()) + "already exists, returning Accepted");
      return true;
    }

    // Build block and get latest accepted block as reference
    auto block = std::make_shared<Block>(blockBytes, false);
    auto latestBlock = chainHead->latest();

    // Parse block
    reply->set_id(block->getBlockHash().get());
    reply->set_parent_id(block->prevBlockHash().get());
    reply->set_height(block->nHeight());
    if (block->nHeight() <= latestBlock->nHeight()) {
      reply->set_status(BlockStatus::Rejected);
      Utils::LogPrint(Log::subnet, __func__, std::string("Block: ") + Utils::bytesToHex(block->getBlockHash().get()) + " is not higher than latest block, returning Rejected, raw byte");
    // https://github.com/ava-labs/avalanchego/blob/master/vms/README.md#processing-blocks
    } else if (block->nHeight() > latestBlock->nHeight()) {
      // We don't know anything about a future block, so we just say we are processing it.
      Utils::LogPrint(Log::subnet, __func__, std::string("Block: " + Utils::bytesToHex(block->getBlockHash().get())) + " is higher than latest block, returning Unknown");
      reply->set_status(BlockStatus::Processing);
      this->chainTip->processBlock(block);
    }
    auto timestamp = reply->mutable_timestamp();
    timestamp->set_seconds(block->timestamp() / 1000000000);
    timestamp->set_nanos(block->timestamp() % 1000000000);
    Utils::LogPrint(Log::subnet, __func__, "Block is valid");
  } catch (std::exception &e) {
    Utils::LogPrint(Log::subnet, __func__, std::string("Error parsing block") + e.what());
    return false;
  }
  return true;
}

void Subnet::getBlock(ServerContext* context, const vm::GetBlockRequest* request, vm::GetBlockResponse* reply) {
  Hash blockHash(request->id());
  if (chainHead->exists(blockHash)) {
    auto block = chainHead->getBlock(blockHash);
    reply->set_parent_id(block->prevBlockHash().get());
    reply->set_bytes(block->serializeToBytes(false));
    reply->set_status(BlockStatus::Accepted);
    reply->set_height(block->nHeight());
    auto timestamp = reply->mutable_timestamp();
    timestamp->set_seconds(block->timestamp() / 1000000000);
    timestamp->set_nanos(block->timestamp() % 1000000000);
    Utils::LogPrint(Log::subnet, __func__, "Block found in chainHead: " + Utils::bytesToHex(block->serializeToBytes(false)));
    return;
  } else if (chainTip->exists(blockHash)) {
    auto block = chainTip->getBlock(blockHash);
    reply->set_parent_id(block->prevBlockHash().get());
    reply->set_bytes(block->serializeToBytes(false));
    reply->set_status(chainTip->getBlockStatus(blockHash));
    reply->set_height(block->nHeight());
    auto timestamp = reply->mutable_timestamp();
    timestamp->set_seconds(block->timestamp() / 1000000000);
    timestamp->set_nanos(block->timestamp() % 1000000000);
    Utils::LogPrint(Log::subnet, __func__, "Block found in chainTip: " + Utils::bytesToHex(block->serializeToBytes(false)));
    return;
  }

  Utils::LogPrint(Log::subnet, __func__, "Block " + Utils::bytesToHex(request->id()) + " does not exist");
  reply->set_status(BlockStatus::Unknown);
  reply->set_err(2); // https://github.com/ava-labs/avalanchego/blob/559ce151a6b6f28d8115e0189627d8deaf00d9fb/vms/rpcchainvm/errors.go#L21
  return;
}

bool Subnet::getAncestors(ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply) {
  Hash blockHash(request->blk_id());
  Utils::LogPrint(Log::subnet, __func__,
    std::string("getAncestors of: ") + Utils::bytesToHex(blockHash.get())
    + " with depth: " + std::to_string(request->max_blocks_num())
    + " up to " + std::to_string(request->max_blocks_size())
    + " bytes and/or for " + std::to_string(request->max_blocks_retrival_time()) + " nanosseconds"
  );
  if (!chainHead->exists(blockHash)) return false;
  auto headBlock = chainHead->getBlock(blockHash);
  auto bestBlock = chainHead->latest();
  uint64_t depth = request->max_blocks_num();
  uint64_t maxSize = request->max_blocks_size(); // Bytes
  uint64_t maxTime = request->max_blocks_retrival_time(); // Nanosseconds

  // Depth can be actually higher than chain height, so we need to set it to the chain height.
  if (depth > bestBlock->nHeight()) {
    Utils::LogPrint(Log::subnet, __func__, "Depth is higher than chain height, setting depth to chain height");
    depth = bestBlock->nHeight();
  }
  auto timeStart = std::chrono::system_clock::now();
  for (uint64_t index = (headBlock->nHeight()); index >= (headBlock->nHeight() - depth) && index <= headBlock->nHeight(); --index) {
    auto block = chainHead->getBlock(index);
    reply->add_blks_bytes(block->serializeToBytes(false));
    auto timeEnd = std::chrono::system_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::nanoseconds>(timeEnd - timeStart).count();
    if (reply->blks_bytes().size() > maxSize || timeDiff > maxTime) {
      Utils::LogPrint(Log::subnet, __func__, "Max block byte size reached or time ran out");
      return false;
    }
  }
  Utils::LogPrint(Log::subnet, __func__, "Ancestors found, answering...");
  return true;
}

void Subnet::setPreference(ServerContext* context, const vm::SetPreferenceRequest* request) {
  this->chainTip->setPreference(Hash(request->id()));
  return;
}

const std::shared_ptr<const Block> Subnet::verifyBlock(const std::string &blockBytes) {
  auto block = std::make_shared<Block>(blockBytes, false);
  // Check if block can be attached to top of the chain.
  if (!this->headState->validateNewBlock(block, this->chainHead)) {
    return nullptr;
  }

  // Add block to processing.
  this->chainTip->processBlock(block);

  return this->chainTip->getBlock(block->getBlockHash());
}

bool Subnet::acceptBlock(const Hash &blockHash) {
  // The block submitted from Accept is located in the chainTip, stored from when
  // avalancheGo submitted verifyBlock.
  Utils::LogPrint(Log::subnet, __func__, "Getting block: " + Utils::bytesToHex(blockHash.get()) + " from chainTip");
  uint64_t blockHeight = 0;
  {
    auto block = this->chainTip->getBlock(blockHash);
    if (block == nullptr) {
      Utils::LogPrint(Log::subnet, __func__, "Block not found");
      return false;
    }

    // Check if block is processing...
    if (!this->chainTip->isProcessing(blockHash)) {
      Utils::LogPrint(Log::subnet, __func__, "Block is not processing");
      return false;
    }
    blockHeight = block->nHeight();
  } // scope because auto block is going to be deleted and chainTip->accept prefers that its block be unique as it's *moved* into chainHead.

  // Accept block in chainTip, move it to chainState and after being processed,
  // finally move it to chainHead.
  Utils::LogPrint(Log::subnet, __func__, "Processing block: " + Utils::bytesToHex(blockHash.get()));
  bool ret = this->chainTip->accept(blockHash, this->headState, this->chainHead);
  Utils::LogPrint(Log::subnet, __func__, "Block " + Utils::bytesToHex(blockHash.get()) + ", height: " + boost::lexical_cast<std::string>(blockHeight) + " accepted");
  return ret;
}

void Subnet::rejectBlock(const Hash &blockHash) {
  this->chainTip->reject(blockHash);
}

void Subnet::validateTransaction(const Tx::Base &&tx) {
  this->headState->validateTransactionForRPC(std::move(tx), false);
}

void Subnet::connectNode(const std::string &nodeId) {
  this->connectedNodesLock.lock();
  Utils::LogPrint(Log::subnet, __func__, "Connecting node: " + Utils::bytesToHex(nodeId));
  this->connectedNodes.emplace_back(nodeId);
  this->connectedNodesLock.unlock();
}

void Subnet::disconnectNode(const std::string &nodeId) {
  this->connectedNodesLock.lock();
  for (uint64_t i = 0; i < this->connectedNodes.size();  ++i) {
    if (this->connectedNodes[i] == nodeId) {
        this->connectedNodes.erase(this->connectedNodes.begin() + i);
      break;
    }
  }
  this->connectedNodesLock.unlock();
}
