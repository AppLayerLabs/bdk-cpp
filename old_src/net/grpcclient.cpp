#include "grpcclient.h"

void VMCommClient::requestBlock() {
  Utils::logToFile("requestBlock: trying to request block");
  try {
    messenger::NotifyRequest req;

    req.set_message(0);

    messenger::NotifyResponse reply;

    ClientContext context;

    Utils::logToFile(Utils::bytesToHex(Utils::uint64ToBytes((uint64_t)(uintptr_t)this)));

    // this->lock.lock();

    Status status = messenger_stub_->Notify(&context, req, &reply);

  if (status.ok()) {
    this->lock.unlock();
    return;
  } else {
    this->lock.unlock();
    Utils::LogPrint(Log::grpcClient, __func__, std::string("requestBlock: RPC failed ERROR CODE ") + std::to_string(status.error_code()) + " ERROR MESSAGE: " + status.error_message());
    return;
  }
  } catch (std::exception &e) {
    Utils::LogPrint(Log::grpcClient, __func__, std::string("requestBlock: RPC failed ERROR CODE ") + std::string(e.what()));
  }
}


