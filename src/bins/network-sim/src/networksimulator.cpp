#include "networksimulator.h"

NetworkSimulator::NetworkSimulator(
  const PrivKey& chainOwnerPrivKey,
  const uint64_t& chainId,
  const uint64_t& packetSize,
  const uint64_t& packetCount,
  const uint256_t& initNativeBalance,
  const uint256_t& txNativeBalance,
  const uint64_t& workerThreads,
  const std::vector<std::pair<net::ip::address_v4, uint16_t>>& httpEndpoints
) : chainOwnerPrivKey_(chainOwnerPrivKey),
    chainId_(chainId),
    packetSize_(packetSize),
    packetCount_(packetCount),
    initNativeBalance_(initNativeBalance),
    txNativeBalance_(txNativeBalance),
    workerThreads_(workerThreads),
    httpEndpoints_(httpEndpoints) {}


void NetworkSimulator::setup() {
  std::cout << "Setting up the network simulator..." << std::endl;
  std::cout << "Creating accounts for each worker..." << std::endl;
  for (uint64_t i = 0; i < workerThreads_; i++) {
    std::vector<WorkerAccount> accounts;
    for (uint64_t j = 0; j < packetSize_; j++) {
      Utils::randBytes(32);
      accounts.emplace_back(PrivKey(Utils::randBytes(32)));
    }
    accounts_.emplace_back(accounts);
  }

  std::cout << "Accounts created!" << std::endl;

  std::cout << "Creating the necessary transactions from the chain owner to the workers accounts..." << std::endl;
  std::vector<std::vector<std::string>> packets;

  // Using only the chainOwnerAccount for now is extremely inefficient as only one transaction (one nonce) can be created at a time from each account.
  // We must prepare packets in multiples of 2 using the accounts_ themselves.
  // For this we need to calculate the balance for each packet (initNativeBalance_ * packetSize_ * workerThreads_) and then
  // Half the value for each packet
  // Example:
  // Init balance: 1000000000000000000
  // 100 Total accounts
  // First packet  TxValue:  1000000000000000000:   ChainOwner -> Account001
  // Second Packet TxValue:  500000000000000000:    ChainOwner -> Account002 | Account001 -> Account003
  // Third Packet: TxValue:  250000000000000000:    ChainOwner -> Account004 | Account001 -> Account005 | Account002 -> Account006 | Account003 -> Account007
  // Fourth Packet: TxValue: 125000000000000000:    ChainOwner -> Account008 | Account001 -> Account009 | Account002 -> Account010 | Account003 -> Account011
  //                                             |  Account004 -> Account012 | Account005 -> Account013 | Account006 -> Account014 | Account007 -> Account015
  // And so on... Until we fill all 100 accounts

  {
    // Reference wrapper so we can modify the nonce of the accounts within the accounts_ vector
    // each of the vector inside accounts_ will be passed to a SimulatorWorker instance when run() is called
    std::vector<std::reference_wrapper<WorkerAccount>> allAccounts;
    for (auto& accounts : accounts_) {
      for (auto& account : accounts) {
        allAccounts.emplace_back(account);
      }
    }
    std::vector<WorkerAccount> chainOwnerAccount = { WorkerAccount(this->chainOwnerPrivKey_) };
    uint256_t txValue = initNativeBalance_ * packetSize_ * workerThreads_;
    std::vector<std::reference_wrapper<WorkerAccount>> createTxAccounts;
    createTxAccounts.emplace_back(chainOwnerAccount[0]);

    uint64_t currentTo = 0;
    while (createTxAccounts.size() < allAccounts.size()) {
      std::vector<std::string> packet;
      auto currentCreateTxAccounts = createTxAccounts;
      for (auto& account : currentCreateTxAccounts) {
        if (currentTo >= allAccounts.size()) {
          break;
        }
        packet.emplace_back(
          makeRequestMethod("eth_sendRawTransaction",
            json::array({Hex::fromBytes(
              TxBlock(
                allAccounts[currentTo].get().address,
                account.get().address,
                {},
                808080,
                account.get().nonce,
                txValue,
                1000000000,
                1000000000,
                21000,
                account.get().privKey).rlpSerialize()
              ,true).forRPC()})));
        createTxAccounts.emplace_back(allAccounts[currentTo]);
        ++account.get().nonce;
        ++currentTo;
      }
      txValue = txValue / 2;
      packets.emplace_back(packet);
    }
  }

  std::cout << "Transactions created!" << std::endl;
  std::cout << "Sending the transactions to the HTTP endpoints..." << std::endl;
  HTTPSyncClient client(
    httpEndpoints_[0].first.to_string(),
    std::to_string(httpEndpoints_[0].second)
  );
  uint64_t totalPackets = 0;

  for (const auto& packet : packets) {
    totalPackets += packet.size();
  }

  std::cout << "Sending setup transactions (total of " << totalPackets << " txs)..." << std::endl;
  for (const auto& packet : packets) {
    auto startTime = std::chrono::high_resolution_clock::now();
    std::cout << "Sending " << packet.size() << " txs..." << std::endl;
    auto packetHash = SimulatorWorker::sendTransactions(packet, client);
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    std::cout << "Confirming " << packetHash.size() << " txs..." << std::endl;
    SimulatorWorker::confirmTransactions(packetHash, client);
    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << " ms" << std::endl;
  }
  std::cout << "Setup complete! Dumping privkeys to privkeys.txt" << std::endl;
  // Write to "privkeys.txt" file one line per hex private key
  std::ofstream privKeysFile("privkeys.txt");
  for (auto& accounts : accounts_) {
    for (auto& account : accounts) {
      privKeysFile << account.privKey.hex() << std::endl;
    }
  }
  privKeysFile.close();
}

void NetworkSimulator::run() {
  std::vector<std::unique_ptr<SimulatorWorker>> workers;
  std::cout << "Creating worker threads..." << std::endl;
  for (uint64_t i = 0; i < workerThreads_; i++) {
    workers.emplace_back(std::make_unique<SimulatorWorker>(
        httpEndpoints_[i],
        accounts_[i],
        chainId_,
        txNativeBalance_
        )
    );
  }

  std::cout << "Starting worker threads..." << std::endl;
  for (auto& worker : workers) {
    worker->run();
  }

  uint64_t totalSentPackets = 0;
  while (totalSentPackets < this->packetCount_) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    uint64_t totalTransactionCreationTime = 0;
    uint64_t totalTransactionSendTime = 0;
    uint64_t totalTransactionConfirmTime = 0;
    for (uint64_t i = 0; i < workerThreads_; i++) {
      std::cout << "Worker " << i << " - Create: " << workers[i]->getCreateTransactionTime() << " ms, Send: " << workers[i]->getSendTransactionTime() << " ms, Confirm: " << workers[i]->getConfirmTransactionTime() << " ms" << std::endl;
      totalTransactionCreationTime += workers[i]->getCreateTransactionTime();
      totalTransactionSendTime += workers[i]->getSendTransactionTime();
      totalTransactionConfirmTime += workers[i]->getConfirmTransactionTime();
      totalSentPackets += workers[i]->getTotalSentPackets();
    }
    std::cout << "Average - Create: " << totalTransactionCreationTime / workerThreads_ << " ms, Send: " << totalTransactionSendTime / workerThreads_ << " ms, Confirm: " << totalTransactionConfirmTime / workerThreads_ << " ms" << std::endl;
    std::cout << "Total transactions sent: " << totalSentPackets * packetSize_ << std::endl;
  }

  std::cout << "Packet count reached! Stopping worker threads..." << std::endl;
  for (auto& worker : workers) {
    worker->stop();
  }
}