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

void VMCommClient::relayTransaction(const Tx::Base tx) {
  Utils::LogPrint(Log::grpcClient, __func__, "relayTransaction: trying to relay: " + Utils::bytesToHex(tx.hash()));
  appsender::SendAppGossipSpecificMsg req;
  nodeListLock.lock_shared();
  for (const auto &node : nodeList) {
    req.add_node_ids(node);
  }
  nodeListLock.unlock_shared();
  req.set_msg(std::string("") + MessagePrefix::tx + tx.rlpSerialize(true));
  google::protobuf::Empty reply;
  ClientContext context;

  std::string jsonRequest;
  google::protobuf::util::JsonOptions options;
  google::protobuf::util::MessageToJsonString(req, &jsonRequest, options);
  Utils::logToFile(jsonRequest);

  this->lock.lock();
  Status status = appsender_stub_->SendAppGossipSpecific(&context, req, &reply);
  if (status.ok()) {
    this->lock.unlock();
    Utils::LogPrint(Log::grpcClient, __func__, "relayTransaction: ok");
    return;
  } else {
    this->lock.unlock();
    Utils::LogPrint(Log::grpcClient, __func__, "relayTransaction: RPC failed");
    return;
  }
}

