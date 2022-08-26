#include "subnet.h"
#include "chainTip.h"

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
  std::cout << "1|15|tcp|" << server_address << "|grpc\n" << std::flush;

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
  grpcClient = std::make_shared<VMCommClient>(grpc::CreateChannel(this->initParams.gRPCServerAddress, grpc::InsecureChannelCredentials()));

  // Initialize the State and ChainHead.
  #if !IS_LOCAL_TESTS
    this->headState = std::make_unique<State>(this->dbServer, this->grpcClient);
  #else
    this->headState = std::make_unique<State>(this->dbServer);
  #endif
  this->chainHead = std::make_unique<ChainHead>(this->dbServer);
  this->chainTip = std::make_unique<ChainTip>();

  // Parse the latest block to answer AvalancheGo.
  auto latestBlock = chainHead->latest();
  reply->set_last_accepted_id(latestBlock->getBlockHash());
  reply->set_last_accepted_parent_id(latestBlock->prevBlockHash());
  reply->set_height(latestBlock->nHeight());
  reply->set_bytes(latestBlock->serializeToBytes());
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
  reply->set_last_accepted_id(bestBlock->getBlockHash());
  reply->set_last_accepted_parent_id(bestBlock->prevBlockHash());
  reply->set_height(bestBlock->nHeight());
  reply->set_bytes(bestBlock->serializeToBytes());
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(bestBlock->timestamp() / 1000000000);
  timestamp->set_nanos(bestBlock->timestamp() % 1000000000);
}

void Subnet::blockRequest(ServerContext* context, vm::BuildBlockResponse* reply) {
  auto newBlock = this->headState->createNewBlock(this->chainHead, this->chainTip);
  if (newBlock == nullptr) {
    Utils::LogPrint(Log::subnet, __func__, "Could not create new block");
    // TODO: handle not being able to create a block.
    throw;
  }
  // Answer back avalanchego.
  Utils::LogPrint(Log::subnet, __func__, "Trying to answer AvalancheGo");
  Utils::LogPrint(Log::subnet, __func__, std::string("New block created: ") + Utils::bytesToHex(newBlock->getBlockHash()));
  reply->set_id(newBlock->getBlockHash());
  reply->set_parent_id(newBlock->prevBlockHash());
  reply->set_height(newBlock->nHeight());
  reply->set_bytes(newBlock->serializeToBytes());
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(newBlock->timestamp() / 1000000000);
  timestamp->set_nanos(newBlock->timestamp() % 1000000000);
  Utils::LogPrint(Log::subnet, __func__, "New block broadcasted, but not enforced.");
  return;
}


bool Subnet::parseBlock(ServerContext* context, const vm::ParseBlockRequest* request, vm::ParseBlockResponse* reply) {
  try {
    auto block = std::make_shared<Block>(request->bytes());

    // Check if block already exists...
    if (chainHead->exists(block->getBlockHash())) {
      reply->set_id(block->getBlockHash());
      reply->set_parent_id(block->prevBlockHash());
      reply->set_status(BlockStatus::Accepted);
      reply->set_height(block->nHeight());
      auto timestamp = reply->mutable_timestamp();
      timestamp->set_seconds(block->timestamp() / 1000000000);
      timestamp->set_nanos(block->timestamp() % 1000000000);
      Utils::LogPrint(Log::subnet, __func__, std::string("Block ") + std::to_string(block->nHeight()) + "already exists, returning Accepted");
      return true;
    }

    // Check if block is on chainTip.
    // TODO.

    // Get latest accepted block as reference.
    auto latestBlock = chainHead->latest();

    // Parse block.
    reply->set_id(block->getBlockHash());
    reply->set_parent_id(block->prevBlockHash());
    reply->set_height(block->nHeight());
    if (block->nHeight() <= latestBlock->nHeight()) {
      reply->set_status(BlockStatus::Rejected);
      Utils::LogPrint(Log::subnet, __func__, std::string("Block: ") + Utils::bytesToHex(block->getBlockHash()) + " is not higher than latest block, returning Rejected, raw byte");
    // https://github.com/ava-labs/avalanchego/blob/master/vms/README.md#processing-blocks
    } else if (block->nHeight() > latestBlock->nHeight()) {
      // We don't know anything about a future block, so we just say we are processing it.
      Utils::LogPrint(Log::subnet, __func__, std::string("Block: " + Utils::bytesToHex(block->getBlockHash())) + " is higher than latest block, returning Unknown");
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
  if (!chainHead->exists(request->id())) {
    Utils::LogPrint(Log::subnet, __func__, "Block does not exist");
    reply->set_status(BlockStatus::Unknown);
    reply->set_err(2); // https://github.com/ava-labs/avalanchego/blob/559ce151a6b6f28d8115e0189627d8deaf00d9fb/vms/rpcchainvm/errors.go#L21
    return;
  }

  auto block = chainHead->getBlock(request->id());

  reply->set_parent_id(block->prevBlockHash());
  reply->set_bytes(block->serializeToBytes());
  reply->set_status(BlockStatus::Accepted);
  reply->set_height(block->nHeight());
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(block->timestamp() / 1000000000);
  timestamp->set_nanos(block->timestamp() % 1000000000);
  Utils::LogPrint(Log::subnet, __func__, "Block found");
  return;
}

void Subnet::getAncestors(ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply) {
  // TODO: check vm.proto and implement max_blocks_size/max_blocks_retrival_time
  Utils::LogPrint(Log::subnet, __func__, std::string("getAncestors of: ") + Utils::bytesToHex(request->blk_id()) + " with depth: " + std::to_string(request->max_blocks_num()));
  if (!chainHead->exists(request->blk_id())) {
    return;
  }
  auto headBlock = chainHead->getBlock(request->blk_id());
  uint64_t depth = request->max_blocks_num();

  for (uint64_t index = (headBlock->nHeight() - 1); index >= (headBlock->nHeight() - depth); --index) {
    auto block = chainHead->getBlock(index);
    reply->add_blks_bytes(block->serializeToBytes());
  }
  return;
}

void Subnet::setPreference(ServerContext* context, const vm::SetPreferenceRequest* request) {
  this->chainTip->setPreference(request->id());
  return;
}

const std::shared_ptr<const Block> Subnet::verifyBlock(const std::string &blockBytes) {
  Block block(blockBytes);
  // Check if block can be attached to top of the chain.
  if (!this->headState->validateNewBlock(block, this->chainHead)) {
    return nullptr;
  }

  // Add block to processing.
  this->chainTip->processBlock(std::make_shared<Block>(block));
  
  return this->chainTip->getBlock(block.getBlockHash());
}

bool Subnet::acceptBlock(const std::string &blockHash) {
  // The block submitted from Accept is located in the chainTip, stored from when
  // avalancheGo submitted verifyBlock.
  Utils::LogPrint(Log::subnet, __func__, "Getting block: " + Utils::bytesToHex(blockHash) + " from chainTip");
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

  // Accept block in state first then append to chainHead.
  // After block being accept on chainTip, it will be erased from it.
  Utils::LogPrint(Log::subnet, __func__, "Processing block: " + Utils::bytesToHex(blockHash));
  this->headState->processNewBlock(block, this->chainHead);
  Utils::LogPrint(Log::subnet, __func__, "Accepting block: " + Utils::bytesToHex(blockHash));
  this->chainTip->accept(block->getBlockHash());
  Utils::LogPrint(Log::subnet, __func__, "Block " + Utils::bytesToHex(block->getBlockHash()) + ", height: " + boost::lexical_cast<std::string>(block->nHeight()) + " accepted");
  return true;
}