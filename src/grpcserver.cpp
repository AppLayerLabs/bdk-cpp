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

Status VMServiceImplementation::BuildBlock(
  ServerContext* context,
  const google::protobuf::Empty* request,
  vm::BuildBlockResponse* reply
) {
  Utils::LogPrint(Log::grpcServer, __func__, "BuildBlock: Block Requested");
  if (!subnet.blockRequest()) {
    // TODO: Handle errors
    Utils::LogPrint(Log::grpcServer, __func__, "BuildBlock: block request FAILED");
  }
  return Status::OK;
}

Status VMServiceImplementation::VerifyHeightIndex(
  ServerContext* context,
  const google::protobuf::Empty* request,
  vm::VerifyHeightIndexResponse* reply
) {
  reply->set_err(0);
  return Status::OK;
}

Status VMServiceImplementation::StateSyncEnabled(
  ServerContext* context,
  const google::protobuf::Empty* request,
  vm::StateSyncEnabledResponse* reply
) {
  reply->set_enabled(false);
  reply->set_err(0);
  return Status::OK;
}

Status VMServiceImplementation::SetPreference(
      ServerContext* context,
      const vm::SetPreferenceRequest* request,
      google::protobuf::Empty* reply
) {
  Utils::logToFile("SetPreference called!!");
  Utils::logToFile(request->DebugString());
  return Status::OK;
}

Status VMServiceImplementation::Version(
  ServerContext* context,
  const google::protobuf::Empty* request,
  vm::VersionResponse* reply
) {
  reply->set_version("0.0.1");
  return Status::OK;
}
