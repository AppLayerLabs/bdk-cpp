#include "faucetmanager.h"
template <typename T>
std::string makeRequestMethod(const std::string& method, const T& params) {
  return json({
    {"jsonrpc", "2.0"},
    {"id", 1},
    {"method", method},
    {"params", params}
  }).dump();
}


namespace Faucet {
  bool FaucetWorker::run() {
    bool log = true;
    while(!this->stop_) {
      try {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::unique_ptr<std::vector<Address>> dripQueue;
        {
          std::unique_lock lock(this->manager_.dripMutex_);
          if (this->manager_.dripQueue_ == nullptr) {
            if (log) {
              Utils::safePrint("No more addresses to drip to, sleeping for 100ms");
              log = false;
            }
            continue;
          }
          log = true;
          dripQueue = std::move(this->manager_.dripQueue_);
          // If the dripQueue is bigger than the number of accounts
          // We can only process the amount of accounts available in the Manager::faucetWorkers_.size()
          // Meaning the remaining accounts needs to be replaced back into the queue.
          if (dripQueue->size() > this->manager_.faucetWorkers_.size()) {
            this->manager_.dripQueue_ = std::make_unique<std::vector<Address>>(dripQueue->begin() + this->manager_.faucetWorkers_.size(), dripQueue->end());
            // Resize the dripQueue to the size of the number of accounts
            dripQueue->resize(this->manager_.faucetWorkers_.size());
          } else {
            this->manager_.dripQueue_ = nullptr;
          }
          Utils::safePrint("Dripping to " + std::to_string(dripQueue->size()) + " addresses");
        }

        std::vector<std::string> sendTxPackets;
        std::vector<std::pair<Hash, bool>> sendTxHashes;
        for (uint64_t i = 0; i < dripQueue->size(); ++i) {
          const auto& address = dripQueue->at(i);
          Utils::safePrint("Dripping to address: " + address.hex(true).get());
          sendTxPackets.emplace_back(this->manager_.createTransactions(
            this->manager_.faucetWorkers_[i],
            1000000000000000000,
            this->manager_.chainId_,
            address
          ));

        }

        Utils::safePrint("Sending " + std::to_string(sendTxPackets.size()) + " faucet transactions to the network");

        for (auto& tx : sendTxPackets) {
          std::this_thread::sleep_for(std::chrono::microseconds(3));
          auto response = this->client_.makeHTTPRequest(tx);
          auto json = json::parse(response);
          if (json.contains("error")) {
            throw std::runtime_error("Error while sending transactions: sent: " + tx + " received: " + json.dump());
          }
          sendTxHashes.emplace_back(Hex::toBytes(json["result"].get<std::string>()), false);
        }

        Utils::safePrint("Confirming " + std::to_string(sendTxHashes.size()) + " faucet transactions to the network");

        for (uint64_t i = 0; i < sendTxHashes.size(); ++i) {
          while (sendTxHashes[i].second == false) {
            std::this_thread::sleep_for(std::chrono::microseconds(3));
            auto response = this->client_.makeHTTPRequest(makeRequestMethod("eth_getTransactionReceipt", json::array({sendTxHashes[i].first.hex(true).get()})));
            auto json = json::parse(response);
            if (json.contains("error")) {
              throw std::runtime_error("Error while confirming transactions: sent: " + sendTxHashes[i].first.hex(true).get() + " received: " + json.dump());
            }
            if (json["result"].is_null()) {
              continue;
            }
            sendTxHashes[i].second = true;
            this->manager_.faucetWorkers_[i].nonce += 1;
          }
        }
        // Update nonce
      } catch (std::exception& e) {
        LOGERRORP(std::string("Error while processing dripToAddress: ") + e.what());
      }
    }
    return true;
  }

  void FaucetWorker::start() {
    this->stop_ = false;
    if (this->runFuture_.valid()) {
      throw std::runtime_error("FaucetWorker already running");
    }
    this->runFuture_ = std::async(std::launch::async, &FaucetWorker::run, this);
  }

  void FaucetWorker::stop() {
    if (!this->runFuture_.valid()) {
      throw std::runtime_error("FaucetWorker not running");
    }
    this->stop_ = true;
    this->runFuture_.get();
  }



  std::string Manager::makeDripToAddress(const Address& address) {
    return makeRequestMethod("dripToAddress", json::array({address.hex(true).get()}));
  }

  void Manager::setup() {
    std::cout << "Setting up the faucet manager" << std::endl;
    std::cout << "Requesting nonces from the network" << std::endl;

    for (auto& worker : this->faucetWorkers_) {
      HTTPSyncClient client(this->httpEndpoint_.first.to_string(), std::to_string(this->httpEndpoint_.second));
      client.connect();
      auto response = client.makeHTTPRequest(makeRequestMethod("eth_getTransactionCount", json::array({worker.address.hex(true).get(), "latest"})));
      auto json = json::parse(response);
      if (json.contains("error")) {
        throw std::runtime_error("Error while getting nonce: " + response);
      }
      worker.nonce = Hex(json["result"].get<std::string>()).getUint();
    }
    std::cout << "Nonces received!" << std::endl;
  }

  void Manager::run() {
    std::cout << "Running faucet service..." << std::endl;
    this->faucetWorker_.start();
    this->server_.run();
  }


  std::string Manager::createTransactions(WorkerAccount& account,
                               const uint256_t& txNativeBalance,
                               const uint64_t& chainId,
                               const Address& to) {
      return makeRequestMethod("eth_sendRawTransaction",
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
            ,true).forRPC()}));
  }

  void Manager::processDripToAddress(const Address& address) {
    // Firstly, lock the current state and check if existed, then grab the current worker account and move the index.
    std::unique_lock lock(this->dripMutex_);
    if (this->dripQueue_ == nullptr) {
      this->dripQueue_ = std::make_unique<std::vector<Address>>();
    }
    this->dripQueue_->emplace_back(address);
  }

  void Manager::dripToAddress(const Address& address) {
    this->threadPool_.push_task(&Manager::processDripToAddress, this, address);
  }
}