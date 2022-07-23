#include "grpcclient.h"


void VMCommClient::requestBlock() {
  Utils::logToFile("requestBlock: trying to request block");
  messenger::NotifyRequest req;
  req.set_message(0);
  messenger::NotifyResponse reply;
  ClientContext context;
  Status status = messenger_stub_->Notify(&context, req, &reply);
  if (status.ok()) {
    return;
  } else {
    Utils::LogPrint(Log::grpcClient, __func__, "requestBlock: RPC failed");
    return;
  }
}