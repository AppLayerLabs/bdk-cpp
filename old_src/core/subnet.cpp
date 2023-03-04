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
  std::cout << "1|20|tcp|" << server_address << "|grpc\n" << std::flush;

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

  // Initialize the State and other parts of the Subnet.
  this->headState = std::make_shared<State>(this->dbServer, this->grpcClient);

  this->chainHead = std::make_shared<ChainHead>(this->dbServer);
  this->chainTip = std::make_shared<ChainTip>();

  json config = Utils::readConfigFile();

  // Parse the latest block to answer AvalancheGo.
  auto latestBlock = chainHead->latest();
  reply->set_last_accepted_id(latestBlock->getBlockHash().get());
  reply->set_last_accepted_parent_id(latestBlock->prevBlockHash().get());
  reply->set_height(latestBlock->nHeight());
  reply->set_bytes(latestBlock->serializeToBytes(false));
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(latestBlock->timestamp() / 1000000000);
  timestamp->set_nanos(latestBlock->timestamp() % 1000000000);


  // Start the P2P Server and Clients
  Utils::logToFile("Starting P2P");
  this->p2pmanager = std::make_shared<P2PManager>(boost::asio::ip::address::from_string("127.0.0.1"), config["p2pport"].get<unsigned short>(), 2, this->chainHead, *this);
  this->p2pmanager->startServer();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  for (auto i : config["seedNodes"]) {
    std::vector<std::string> seedNode;
    boost::split(seedNode, i.get<std::string>(), boost::is_any_of(":"));
    this->p2pmanager->connectToServer(boost::asio::ip::address::from_string(seedNode[0]), std::stoi(seedNode[1]));
  }

  Utils::logToFile("Starting blockManager");
  if (config.contains("validatorPrivKey")) {
    Utils::logToFile("Validator found.");
    this->isValidator = true;
    this->blockManager = std::make_shared<BlockManager>(this->dbServer, this->chainHead, this->p2pmanager, this->grpcClient, Hash(Utils::hexToBytes(config["validatorPrivKey"].get<std::string>())), ContractAddresses::BlockManager, Address("0x0000000000000000000000000000000000000000", true));
  } else {
    this->blockManager = std::make_shared<BlockManager>(this->dbServer, this->chainHead, this->p2pmanager, this->grpcClient, ContractAddresses::BlockManager, Address("0x0000000000000000000000000000000000000000", true));
  }

  // Start the HTTP Server.
  Utils::logToFile("Starting HTTP");
  this->httpServer = std::make_unique<HTTPServer>(*this, config["rpcport"].get<unsigned short>());
  std::thread httpServerThread = std::thread([&]{this->httpServer->run();});
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
  Utils::LogPrint(Log::subnet, __func__, std::string("Setting State to: ") + std::to_string(request->state()));
  if (request->state() == 3) { // NormalOp
    this->blockManager->startValidatorThread();
  }
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
  auto newBlock = this->headState->createNewBlock(this->chainHead, this->chainTip, this->blockManager);
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
    auto block = std::make_shared<Block>(blockBytes, false);
    Hash blockHash = block->getBlockHash();
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
    auto latestBlock = chainHead->latest();

    // Parse block
    reply->set_id(block->getBlockHash().get());
    reply->set_parent_id(block->prevBlockHash().get());
    reply->set_height(block->nHeight());
    if (block->nHeight() <= latestBlock->nHeight()) {
      reply->set_status(BlockStatus::Rejected);
      Utils::LogPrint(Log::subnet, __func__, std::string("Block: ") + Utils::bytesToHex(block->getBlockHash().get()) +
       "(" + std::to_string(block->nHeight())  + ") is not higher than latest block (" + std::to_string(latestBlock->nHeight()) + "), returning Rejected");
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


// When AvalancheGo calls setPreference, it means we are synced, so we are able to create a block.
void Subnet::setPreference(ServerContext* context, const vm::SetPreferenceRequest* request) {
  this->chainTip->setPreference(Hash(request->id()));
  return;
}

const std::shared_ptr<const Block> Subnet::verifyBlock(const std::string &blockBytes) {
  auto block = std::make_shared<Block>(blockBytes, false);
  // Check if block can be attached to top of the chain.
  if (!this->headState->validateNewBlock(block, this->chainHead, this->blockManager)) {
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
  bool ret = this->chainTip->accept(blockHash, this->headState, this->chainHead, this->blockManager);
  Utils::LogPrint(Log::subnet, __func__, "Block " + Utils::bytesToHex(blockHash.get()) + ", height: " + boost::lexical_cast<std::string>(blockHeight) + " accepted");
  return ret;
}

void Subnet::rejectBlock(const Hash &blockHash) {
  this->chainTip->reject(blockHash);
}

std::pair<int, std::string> Subnet::validateTransaction(const Tx::Base &&tx) {
  auto txKnow = this->headState->getMempool().count(tx.hash());
  Utils::logToFile("validate Transaction...");
  auto ret = this->headState->validateTransactionForRPC(tx); 
  if (!txKnow) {
    Utils::logToFile("broadcasting tx...");
    // Broadcast only if tx was not previously knew.
    this->p2pmanager->broadcastTx(tx);
  }
  return ret;
}

void Subnet::validateValidatorTransaction(const Tx::Validator &tx) {
  this->blockManager->addValidatorTx(tx);
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

std::unordered_map<Hash, Tx::Validator, SafeHash> Subnet::getValidatorMempool() {
  return this->blockManager->getMempoolCopy();
}

void Subnet::test() {

  Block genesis(Hash(Utils::uint256ToBytes(0)), 1656356645000000, 0);
  genesis.finalizeBlock(PrivKey());

  std::cout << "Trying to build a block using genesis as base" << std::endl;
  auto newBlock = Block(genesis.serializeToBytes(false), false);
  std::cout << "Trying to build a db block using genesis as base" << std::endl;
  auto newDbBlock = Block(genesis.serializeToBytes(true), true);

  std::vector<Tx::Validator> validatorTxs;

  validatorTxs.push_back(Tx::Validator(Utils::hexToBytes("f86ba4cfffe746a82d9001bed16dfb4e175f0b25a05b4000f82de5fff56854d97c15ae4a599a0b80824543a00fde23b9a6298091ab5ad42adf0212e984ddf3ea9fe119c02b5878a7c9a6103ea0190d69422466efcf0938ee3b743e701b9b17ef062693c998ac6f2456c898dd07")));
  validatorTxs.push_back(Tx::Validator(Utils::hexToBytes("f86ba4cfffe746217d119c569f9ac479b2cccc6102ecbcc2b1ab6ec8ede566c758c1708fff1f7180824543a04fa3faca03ad180caec26b296af40ab53ca95b6a83ffa35556668afdaee2fb4ca04a90b194ef3edf09fbaac2d52f86b4df1c8a78ee4cf0943164eefc6a7824680d")));
  validatorTxs.push_back(Tx::Validator(Utils::hexToBytes("f86ba4cfffe746eddfbfea896a4afa5d5a6109e589a5d78a216c9fabcc69a471887836c0ddf7d280824544a05d4aa2e3bc4f371cf37d3b81d3c736c20d91759e1a5eb8bf09a3ff7c7a3b3b05a0612986c54df2abf3d59c34b2bdb897126e8e4fa94c0d19d5446828e9a14301b6")));
  validatorTxs.push_back(Tx::Validator(Utils::hexToBytes("f86ba4cfffe7465e751c94f28bc22806ec8fdb1e93e73a04d3fe03563128461dbfc36626a7c4a280824544a0d29fda797a405b0ed0abe191c4371d713c43e1329ec5e9d3f8c69e239830f558a00ecfa5b10cd2b5680ac00c32eea80741042941650f58bc363e38e682ffcc677e")));
  validatorTxs.push_back(Tx::Validator(Utils::hexToBytes("f86ba46fc5a2d6166297c102dd0b884e9a8543b695b01b9bfbeb52468f52a24dd3f078c4e669d680824543a01107efa9c543506abcaac27874e94cfd676cb277d11bc47e777148ac0cc17a70a041f00ff534a510a6b6d86661ae5375c2d64b69c37a0c1a121d97d3ad334703b7")));
  validatorTxs.push_back(Tx::Validator(Utils::hexToBytes("f86ba46fc5a2d6c683169461398660ef9f21b8537368687730ac8cceb27f86c6f23b6b96c2943f80824544a04d29e899af2fd474cf072f14a659e0f2ecde4d1444b5a2efab1748c6ad11eb2fa06844c994a3a6db99784da7744421f828586e93de506d2a5c198f6b32d9f6149d")));
  validatorTxs.push_back(Tx::Validator(Utils::hexToBytes("f86ba46fc5a2d620f2f374e100c628815ee49294e10413830fc8d29fab394351f19be8de09cf0680824544a089dd2dd366bd080f3c6a28b62cb0fd7b1f5c6c7a46376c9a5ef290e4f17b53cda07c9e21c39d8cc9969131132844aaedd2e8fb68b04a1871097af6c383d3b75288")));
  validatorTxs.push_back(Tx::Validator(Utils::hexToBytes("f86ba46fc5a2d6068478c43df0c3ac15fc0ad730bb1beb63fa15d64ed6e9386a8b6a47f62f5d6d80824543a0a3d90b420eeba9ec47ba5b18ddbcfb224a4055705c8b57324018daf5c114fad8a035d872d1e6675f47ed248234cf039732020c40f8c312336affc2b981359ff95f"))); 
  
  auto newBestBlock = std::make_shared<Block>(
    genesis.getBlockHash(),
    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(),
    genesis.nHeight() + 1
  );
  
  
  for (const auto &i : validatorTxs) {
    newBestBlock->appendValidatorTx(i);
  }

  newBestBlock->finalizeBlock(PrivKey());

  std::cout << "merkle root: " << newBestBlock->validatorMerkleRoot().hex() << std::endl;


  std::cout << "Serialized Block: " << Utils::bytesToHex(newBestBlock->serializeToBytes(false)) << std::endl;
  auto testBlock = Block(newBestBlock->serializeToBytes(false), false);

  std::cout << "newBestBlock: " << newBestBlock->getBlockHash().hex() << std::endl;
  std::cout << "testBlock: " << testBlock.getBlockHash().hex() << std::endl;
}
