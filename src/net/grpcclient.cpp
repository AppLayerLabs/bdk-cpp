#include "grpcclient.h"

void VMCommClient::requestBlock() {
  Utils::logToFile("requestBlock: trying to request block");
  messenger::NotifyRequest req;
  req.set_message(0);
  messenger::NotifyResponse reply;
  ClientContext context;
  this->lock.lock();;
  Status status = messenger_stub_->Notify(&context, req, &reply);
  if (status.ok()) {
    this->lock.unlock();
    return;
  } else {
    this->lock.unlock();;
    Utils::LogPrint(Log::grpcClient, __func__, "requestBlock: RPC failed");
    return;
  }
}


