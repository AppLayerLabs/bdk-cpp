#include "grpcserver.h"
#include "subnet.h"

Status VMServiceImplementation::Initialize(ServerContext* context, const vm::InitializeRequest* request, vm::InitializeResponse* reply) {
  subnet.initialize(request, reply);
  return Status::OK;
}