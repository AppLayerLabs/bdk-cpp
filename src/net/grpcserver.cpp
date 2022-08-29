#include "grpcserver.h"

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
  subnet.blockRequest(context, reply);
  return Status::OK;
}

Status VMServiceImplementation::ParseBlock(
  ServerContext* context,
  const vm::ParseBlockRequest* request,
  vm::ParseBlockResponse* reply
) {
  Utils::logToFile("Parse block called!");
  if (!subnet.parseBlock(context, request->bytes(), reply)) {
    return Status::CANCELLED;
  }
  return Status::OK;
}

// VerifyHeightIndex not supported.
Status VMServiceImplementation::VerifyHeightIndex(
  ServerContext* context,
  const google::protobuf::Empty* request,
  vm::VerifyHeightIndexResponse* reply
) {
  Utils::logToFile("VerifyHeightIndex called!");
  reply->set_err(1);
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
  Utils::logToFile(Utils::bytesToHex(request->id()));
  subnet.setPreference(context, request);
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

Status VMServiceImplementation::Shutdown(
  ServerContext* context,
  const google::protobuf::Empty* request,
  google::protobuf::Empty* reply
) {
  Utils::logToFile("Shutdown called!!");
  subnet.stop();
  std::thread t(&Subnet::shutdownServer, &subnet);
  // Detach thread so we can return this function before gRPC server closes.
  t.detach();
  return Status::OK;
}

Status VMServiceImplementation::GetBlock(
  ServerContext* context,
  const vm::GetBlockRequest* request,
  vm::GetBlockResponse* reply
) {
  subnet.getBlock(context, request, reply);
  return Status::OK;
}

Status VMServiceImplementation::GetAncestors(
  ServerContext* context,
  const vm::GetAncestorsRequest* request,
  vm::GetAncestorsResponse* reply
) { 
  Utils::logToFile("GetAncestors called!!");
  subnet.getAncestors(context, request, reply);
  return Status::OK;
}

// TODO in order to fully enable multi node support: 
// https://github.com/ava-labs/avalanchego/blob/master/vms/proposervm/README.md
// https://github.com/ava-labs/avalanchego/blob/master/vms/README.md

// When BlockVerify is called, it verifies if the block is valid as the next block of the chain.
// If it is valid, It is placed in chainTip as processing.
Status VMServiceImplementation::BlockVerify(
  ServerContext* context,
  const vm::BlockVerifyRequest* request,
  vm::BlockVerifyResponse* reply
) {
  Utils::logToFile("BlockVerify called!!");


  auto block = subnet.verifyBlock(request->bytes());

  if (block != nullptr) {
    Block block(request->bytes());
    auto timestamp = reply->mutable_timestamp();
    timestamp->set_seconds(block.timestamp() / 1000000000);
    timestamp->set_nanos(block.timestamp() % 1000000000);
    Utils::logToFile("BlockVerify success, block is valid");
    return Status::OK;
  }

  return Status::CANCELLED;
}

Status VMServiceImplementation::BlockAccept(
  ServerContext* context,
  const vm::BlockAcceptRequest* request,
  google::protobuf::Empty *reply
) {
  Utils::logToFile(std::string("BlockAccept called!! ") + Utils::bytesToHex(request->id()));
  if (!subnet.acceptBlock(request->id())) {
    return Status::CANCELLED;
  }
  return Status::OK;
}

Status VMServiceImplementation::BlockReject(
  ServerContext* context,
  const vm::BlockRejectRequest* request,
  google::protobuf::Empty *reply
) {
  Utils::logToFile("BlockReject called!!");
  return Status::OK;
}

Status VMServiceImplementation::BatchedParseBlock(
  ServerContext* context,
  const vm::BatchedParseBlockRequest* request,
  vm::BatchedParseBlockResponse* reply
) {
  Utils::logToFile("BatchedParseBlock called!!");
  for (uint64_t i = 0; i < request->request_size(); ++i) {
    auto response = reply->add_response();
    if (!subnet.parseBlock(context, request->request(i), response)) {
      return Status::CANCELLED;
    }
  }
  return Status::OK;
}