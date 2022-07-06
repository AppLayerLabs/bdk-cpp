#include "grpcserver.h"
#include "subnet.h"
#include "utils.h"

Status VMServiceImplementation::Initialize(
  ServerContext* context,
  const vm::InitializeRequest* request,
  vm::InitializeResponse* reply
) {
  subnet.initialize(request, reply);
  return Status::OK;
}

Status VMServiceImplementation::SetState(
  ServerContext* context,
  const vm::SetStateRequest* request,
  vm::SetStateResponse* reply
) {
  subnet.setState(request, reply);
  return Status::OK;
}

