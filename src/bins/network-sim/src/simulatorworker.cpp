#include "simulatorworker.h"



SimulatorWorker::SimulatorWorker(
  const std::pair<net::ip::address_v4, uint16_t>& httpEndpoint,
  const std::vector<WorkerAccount>& accounts, const uint64_t& chainId, const uint256_t& txNativeBalance
) : client_(httpEndpoint.first.to_string(), std::to_string(httpEndpoint.second)),
    httpEndpoint_(httpEndpoint),
    accounts_(accounts),
    chainId_(chainId),
    txNativeBalance_(txNativeBalance) {}


/// Create a number of transactions based on the number of accounts
std::vector<std::string> SimulatorWorker::createTransactions(std::vector<WorkerAccount>& accounts,
                               const uint256_t& txNativeBalance,
                               const uint64_t& chainId,
                               const Address& to) {
  std::vector<std::string> packet;
  for (auto& account : accounts) {
    packet.emplace_back(
      makeRequestMethod("eth_sendRawTransaction",
        json::array({Hex::fromBytes(
          TxBlock(
            to,
            account.address,
            {},
            chainId,
            account.nonce,
            txNativeBalance,
            1000000000,
            1000000000,
            21000,
            account.privKey).rlpSerialize()
          ,true).forRPC()})));
    ++account.nonce;
  }
  return packet;
}

/// Send transactions to the HTTP endpoint
std::vector<std::pair<Hash, bool>> SimulatorWorker::sendTransactions(const std::vector<std::string>& packet,
                                                           HTTPSyncClient& client) {
  std::vector<std::pair<Hash, bool>> packetHashes;
  for (auto& tx : packet) {
    auto response = client.makeHTTPRequest(tx);
    auto json = json::parse(response);
    if (json.contains("error")) {
      throw std::runtime_error("Error while sending transactions: sent: " + tx + " received: " + json.dump());
    }
    packetHashes.emplace_back(Hex::toBytes(json["result"].get<std::string>()), false);
  }
  return packetHashes;
}

/// Wait for all transactions to be confirmed
void SimulatorWorker::confirmTransactions(std::vector<std::pair<Hash, bool>>& packetHashes,
                                HTTPSyncClient& client) {
  for (auto& tx : packetHashes) {
    while (tx.second == false)
    {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      auto response = client.makeHTTPRequest(makeRequestMethod("eth_getTransactionReceipt", json::array({tx.first.hex(true).get() })));
      auto json = json::parse(response);
      if (json.contains("error")) {
        throw std::runtime_error("Error while confirming transactions: sent: " + tx.first.hex(true).get() + " received: " + json.dump());
      }
      if (json["result"].is_null()) {
        tx.second = false;
      } else {
        if (json["result"]["status"] == "0x1") {
          tx.second = true;
        } else {
          throw std::runtime_error("Error while confirming transactions: sent: " + tx.first.hex(true).get() + " received: " + json.dump());
        }
      }
    }
  }
}

void SimulatorWorker::work() {
  while (!this->stop_)
  {
    auto createTxStartTime = std::chrono::high_resolution_clock::now();
    this->packet_ = createTransactions(this->accounts_, this->txNativeBalance_, this->chainId_, this->accounts_[0].address);
    auto createTxEndTime = std::chrono::high_resolution_clock::now();
    this->createTransactionTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(createTxEndTime - createTxStartTime).count();
    auto sendTxStartTime = std::chrono::high_resolution_clock::now();
    this->packetHashes_ = sendTransactions(this->packet_, this->client_);
    auto sendTxEndTime = std::chrono::high_resolution_clock::now();
    this->sendTransactionTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(sendTxEndTime - sendTxStartTime).count();
    auto confirmTxStartTime = std::chrono::high_resolution_clock::now();
    confirmTransactions(this->packetHashes_, this->client_);
    auto confirmTxEndTime = std::chrono::high_resolution_clock::now();
    this->confirmTransactionTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(confirmTxEndTime - confirmTxStartTime).count();
    this->packet_.clear();
    this->packetHashes_.clear();
    ++totalSentPackets_;
  }
}

void SimulatorWorker::stop() {
  this->stop_ = true;
  this->workerThread_.join();
}

void SimulatorWorker::run() {
  this->stop_ = false;
  this->confirmTransactionTime_ = 0;
  this->createTransactionTime_ = 0;
  this->sendTransactionTime_ = 0;
  this->workerThread_ = std::thread(&SimulatorWorker::work, this);
}
