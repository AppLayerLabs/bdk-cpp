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
  std::copy(request->subnet_id().begin(), request->subnet_id().end(), std::back_inserter(this->initParams.subnetId));
  std::copy(request->chain_id().begin(), request->chain_id().end(), std::back_inserter(this->initParams.chainId));
  std::copy(request->node_id().begin(), request->node_id().end(), std::back_inserter(this->initParams.nodeId));
  std::copy(request->x_chain_id().begin(), request->x_chain_id().end(), std::back_inserter(this->initParams.xChainId));
  std::copy(request->avax_asset_id().begin(), request->avax_asset_id().end(), std::back_inserter(this->initParams.avaxAssetId));
  std::copy(request->genesis_bytes().begin(), request->genesis_bytes().end(), std::back_inserter(this->initParams.genesisBytes));
  std::copy(request->upgrade_bytes().begin(), request->upgrade_bytes().end(), std::back_inserter(this->initParams.upgradeBytes));
  std::copy(request->config_bytes().begin(), request->config_bytes().end(), std::back_inserter(this->initParams.configBytes));
  bool hasDBServer = false;
  for (int i = 0; i < request->db_servers_size(); i++) {
    auto db_server = request->db_servers(i);
    DBServer db_Server;
    db_Server.host = db_server.server_addr();
    db_Server.version = db_server.version();
    this->initParams.dbServers.push_back(db_Server);
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
  
  // Initialize the gRPC client, to be used for DB and other services.

}