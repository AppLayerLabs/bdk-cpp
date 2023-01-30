#include "blockchain.h"

void Blockchain::start() {
  /**
   * When starting the binary, the first thing to setup is the gRPC server,
   * as the AvalancheGo daemon node will be waiting for the gRPC server
   * to answer on the terminal.
   */
  // Get a random number from hardware between 50000 and 60000 and seed the generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(50000, 60000);
  unsigned short port = distr(gen);
  Utils::LogPrint(Log::blockchain, __func__,
    std::string("Starting blockchain at port: ") + std::to_string(port)
  );
  std::string serverHost(std::string("0.0.0.0:") + std::to_string(port));
  //this->grpcServer = std::make_shared<gRPCServer>(*this); // TODO: gRPCServer pointer doesn't exist here
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(serverHost, grpc::InsecureServerCredentials());

  /**
   * Register "service" as the instance through which we'll communicate with
   * clients. In this case it corresponds to a *synchronous* service.
   */
  //builder.RegisterService(this->grpcServer.get()); // TODO: gRPCServer pointer doesn't exist here

  /**
   * Assemble the server and send the address of the gRPC server
   * to the AvalancheGo Daemon.
   */
  this->server = builder.BuildAndStart();
  std::cout << "1|20|tcp|" << serverHost << "|grpc\n" << std::flush;

  /**
   * Wait for the server to shutdown. Note that some other thread must be
   * responsible for shutting down the server for this call to ever return.
   */
  Utils::logToDebug(Log::blockchain, __func__, "Startup Done");
  this->server->Wait();
  Utils::logToDebug(Log::blockchain, __func__, "Server Thread Returning...");
}

void Blockchain::stop() {
  if (this->initialized) {
    Utils::logToDebug(Log::blockchain, __func__, "Stopping blockchain...");
    this->shutdown = true;
    // Dump State and ChainHead from memory to the database, then close it
    this->storage->saveToDB();
    Utils::logToDebug(Log::blockchain, __func__, "storage saved to DB");
    this->state->saveToDB();
    Utils::logToDebug(Log::blockchain, __func__, "state saved to DB");
    this->db->close();
    Utils::logToDebug(Log::blockchain, __func__, "DB closed successfully");
    // Signal the HTTP server to stop and wait for it
    this->httpServer->stop();
    while (true) {
      if (!this->httpServer->isRunning()) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Utils::logToDebug(Log::blockchain, __func__, "HTTP server stopped");
    // Wait for Server to shutdown.
    // This function is called from the gRPC Server, so we cannot stop the server here.
    // A thread is created and detached there calling `shutdownServer()`.
    Utils::logToDebug(Log::blockchain, __func__, "Waiting for server to shutdown...");
  }
}

std::pair<int, string> Blockchain::validateTx(const TxBlock&& tx) {
  bool hasTx = this->state->getMempool().count(tx.hash());
  auto ret = this->state->validateTxForRPC(tx);
  // Broadcast only if tx was not previously in State
  if (!hasTx) {
    Utils::logToDebug(Log::blockchain, __func__, "Broadcasting tx...");
    this->p2p->broadcastTx(tx);
  }
  return ret;
}

