#include "subnet.h"

// TODO: maybe Utils::LogPrint() each step here? stop() is doing it, maybe start() should too just in case
void Subnet::start() {
  /**
   * When starting the binary, the first thing to setup is the gRPC server,
   * as the AvalancheGo Daemon node will be waiting for the gRPC server
   * to answer on the terminal.
   */
  std::string server_address("0.0.0.0:50051");
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
  // std::unique_ptr<Server> server(builder.BuildAndStart());
  server = builder.BuildAndStart();
  std::cout << "1|15|tcp|" << server_address << "|grpc\n" << std::flush;

  /**
   * Wait for the server to shutdown. Note that some other thread must be
   * responsible for shutting down the server for this call to ever return.
   */
  server->Wait();
  return;
}

void Subnet::stop() {
  Utils::LogPrint(Log::subnet, __func__, "Stopping subnet...");
  this->shutdown = true;
  // Dump State and ChainHead from memory to the database.
  if (this->initialized) {
    this->chainHead->dumpToDB();
    Utils::LogPrint(Log::subnet, __func__, "chainHead saved to DB...");
    this->headState->saveState(this->dbServer);
    Utils::LogPrint(Log::subnet, __func__, "headState saved to DB...");
  }
  this->dbServer->close();

  // Kill HTTP Server if is still running;
  HTTPServer::shutdownServer();
  // Sleep for 2 seconds and wait for Server shutdown answer.
  boost::this_thread::sleep_for(boost::chrono::seconds(2));
  Utils::LogPrint(Log::subnet, __func__, "Shutdown Done");
  server->Shutdown();
  return;
}

void Subnet::initialize(const vm::InitializeRequest* request, vm::InitializeResponse* reply) {
  /**
   * The initialization request is made by the AvalancheGo Daemon.
   * See vm.proto for more information.
   */
  if (this->initialized) {
    Utils::LogPrint(Log::subnet, __func__, "Subnet already initialized.");
    throw ""; // Subnet was already initialized. This shouldn't be allowed.
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
  this->headState = std::make_unique<State>(this->dbServer);
  this->chainHead = std::make_unique<ChainHead>(this->dbServer);

  // Parse the latest block to answer AvalancheGo.
  Block latestBlock = chainHead->latest();
  reply->set_last_accepted_id(latestBlock.getBlockHash());
  reply->set_last_accepted_parent_id(latestBlock.prevBlockHash());
  reply->set_height(latestBlock.nHeight());
  reply->set_bytes(latestBlock.serializeToBytes());
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(latestBlock.timestamp() / 1000000000);
  timestamp->set_nanos(latestBlock.timestamp() % 1000000000);

  // Start the HTTP Server.
  HTTPServer::startServer(*this);
  std::string jsonReply;
  google::protobuf::util::MessageToJsonString(*reply, &jsonReply, options);
  Utils::logToFile(jsonReply);
}

void Subnet::setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply) {
  /**
   * See vm.proto and https://github.com/ava-labs/avalanchego/blob/master/snow/engine/snowman/bootstrap/bootstrapper.go#L111
   * for more information about the SetState request.
   */
  // TODO: DO NOT READ FROM DB DIRECTLY
  Block bestBlock = chainHead->latest();
  reply->set_last_accepted_id(bestBlock.getBlockHash());
  reply->set_last_accepted_parent_id(bestBlock.prevBlockHash());
  reply->set_height(bestBlock.nHeight());
  reply->set_bytes(bestBlock.serializeToBytes());
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(bestBlock.timestamp() / 1000000000);
  timestamp->set_nanos(bestBlock.timestamp() % 1000000000);
}

