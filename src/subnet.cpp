#include "subnet.h"

void Subnet::start() {
  // When starting the binary, the first thing to setup is the gRPC server.
  // As the AvalancheGo Daemon node will be waiting for the gRPC server to answer on the terminal.
  // Start GRPC Server.
  std::string server_address("0.0.0.0:50051");
  grpcServer = std::make_shared<VMServiceImplementation>(*this);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(grpcServer.get());
  // Finally assemble the server.
  // std::unique_ptr<Server> server(builder.BuildAndStart());

  server = builder.BuildAndStart();
  std::cout << "1|15|tcp|" << server_address << "|grpc\n" << std::flush; // Send the address of the gRPC server to the AvalancheGo Daemon.
  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return
  server->Wait();
  return;
}


void Subnet::stop() {
  // Sleep for 2 seconds and wait for Server shutdown answer.
  boost::this_thread::sleep_for(boost::chrono::seconds(2));
  server->Shutdown();
  return;
}


void Subnet::initialize(const vm::InitializeRequest* request, vm::InitializeResponse* reply) {
  // See vm.proto for more information.
  // The initialization request is made by the AvalancheGo Daemon.

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

  // Initialize the DB Server to get information about the subnet there, we are assuming that we are running inside a sandbox.
  if(hasDBServer) {
    auto db_Server  = this->initParams.dbServers[0];
    dbServer = std::make_shared<DBService>(grpc::CreateChannel(db_Server.host, grpc::InsecureChannelCredentials()));
  }
  // Initialize the gRPC client to communicate back with AvalancheGo
  grpcClient = std::make_shared<VMCommClient>(grpc::CreateChannel(this->initParams.gRPCServerAddress, grpc::InsecureChannelCredentials()));

  // Initialize the State

  this->headState = std::make_unique<State>(this->dbServer);

  // Parse the latest block to answer AvalancheGo.
  auto blockStr = dbServer->get("latest", DBPrefix::blocks);
  auto latestBlock = Block(dbServer->get("latest", DBPrefix::blocks));
  reply->set_last_accepted_id(latestBlock.getBlockHash());
  reply->set_last_accepted_parent_id(Utils::uint256ToBytes(latestBlock.prevBlockHash()));
  reply->set_height(latestBlock.nHeight());
  reply->set_bytes(latestBlock.serializeToBytes());
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(latestBlock.timestamp() / 1000000000);
  timestamp->set_nanos(latestBlock.timestamp() % 1000000000); 
  Utils::logToFile(std::to_string(latestBlock.timestamp()));
  std::string jsonReply;
  google::protobuf::util::MessageToJsonString(*reply, &jsonReply, options);
  Utils::logToFile(jsonReply);
}

void Subnet::setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply) {
  // See vm.proto for more information.
  // See https://github.com/ava-labs/avalanchego/blob/master/snow/engine/snowman/bootstrap/bootstrapper.go#L111
  // for more information about the SetState request.
  // TODO: DO NOT READ FROM DB DIRECTLY
  Block bestBlock(dbServer->get("latest", DBPrefix::blocks));
  reply->set_last_accepted_id(bestBlock.getBlockHash());
  reply->set_last_accepted_parent_id(Utils::uint256ToBytes(bestBlock.prevBlockHash()));
  reply->set_height(bestBlock.nHeight());
  reply->set_bytes(bestBlock.serializeToBytes());
  auto timestamp = reply->mutable_timestamp();
  timestamp->set_seconds(bestBlock.timestamp() / 1000000000);
  timestamp->set_nanos(bestBlock.timestamp() % 1000000000); 
}