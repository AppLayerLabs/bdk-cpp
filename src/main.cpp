#include "main.h"
#include "grpcserver.h"

void Subnet::start()
{
  std::string server_address("0.0.0.0:50051");
  //VMServiceImplementation service(shared_from_this());
  service = std::make_shared<VMServiceImplementation>(shared_from_this());

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(service.get());
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "1|12|tcp|" << server_address << "|grpc\n"<< std::flush;
  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return
  server->Wait();
}

int main() {

  std::signal(SIGINT, SIG_IGN);
  std::shared_ptr<Subnet> subnet = std::make_shared<Subnet>();
  subnet->start();
  Utils::logToFile("returned");
  return 0;
}


