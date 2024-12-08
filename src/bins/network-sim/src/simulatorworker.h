#ifndef SIMULATORWORKER_H
#define SIMULATORWORKER_H

#include "httpclient.h"
#include "common.h"
#include <atomic>

/**
 * Worker implementation for the network simulator
 * Constructor automatically connects to the HTTP endpoint
 * Execution pattern when running the worker:
 * 1 - Create accounts_.size() transactions with the packet_ size
 * 2 - Send all transactions from packet_ to the HTTP endpoint and store the hashes in packetHashes_
 * 3 - Wait for all transactions to be confirmed
 * 4 - Empty the packet_ and packetHashes_ vectors and repeat
 */
class SimulatorWorker {
  private:
    HTTPSyncClient client_; ///< HTTP sync client object.
    const std::pair<net::ip::address_v4, uint16_t> httpEndpoint_; ///< HTTP endpoint to be used.
    std::vector<WorkerAccount> accounts_; ///< List of accounts to be used when creating transactions.
    const uint64_t chainId_;  ///< Chain ID to be used when creating transactions.
    const uint256_t txNativeBalance_; ///< Balance to be used when creating transactions.
    std::vector<std::string> packet_; ///< Transactions (serialized) to be sent.
    std::vector<std::pair<Hash, bool>> packetHashes_; ///< Hashes of the transactions and their confirmation status.
    std::atomic<uint64_t> createTransactionTime_; ///< Maximum time spent to create a transaction.
    std::atomic<uint64_t> sendTransactionTime_; ///< Maximum time spent to send a transaction.
    std::atomic<uint64_t> confirmTransactionTime_; ///< Maximum time spent to confirm a transaction.
    std::atomic<uint64_t> totalSentPackets_; ///< Total number of packets sent in the network simulator.
    std::atomic<bool> stop_;  ///< Flag to stop the network simulator.
    std::thread workerThread_; ///< Thread to run the network simulator worker.

    void work(); ///< Worker function for the network simulator.

  public:
    /**
     * Constructor.
     * @param httpEndpoint HTTP endpoint to be used.
     * @param accounts List of accounts to be used when creating transactions.
     * @param chainId Chain ID to be used when creating transactions.
     * @param txNativeBalance Balance to be used when creating transactions.
     */
    SimulatorWorker(
      const std::pair<net::ip::address_v4, uint16_t>& httpEndpoint,
      const std::vector<WorkerAccount>& accounts, const uint64_t& chainId, const uint256_t& txNativeBalance
    );

    SimulatorWorker(const SimulatorWorker& other) = delete; ///< Copy constructor (deleted, Rule of Zero)
    SimulatorWorker(SimulatorWorker&& other) = delete; ///< Move constructor (deleted, Rule of Zero)
    SimulatorWorker& operator=(const SimulatorWorker& other) = delete; ///< Copy assignment operator (deleted, Rule of Zero)
    SimulatorWorker& operator=(SimulatorWorker&& other) = delete; ///< Move assignment operator (deleted, Rule of Zero)

    void run(); ///< Run loop as described in the class description.
    void stop(); ///< Stop the worker

    /**
     * Create a number of transactions based on the number of accounts.
     * @param accounts List of accounts to be used when creating transactions.
     * @param chainId Chain ID to be used when creating transactions.
     * @param txNativeBalance Balance to be used when creating transactions.
     * @param to The address to be used when creating the transactions.
     */
    static std::vector<std::string> createTransactions(
      std::vector<WorkerAccount>& accounts, const uint256_t& txNativeBalance,
      const uint64_t& chainId, const Address& to
    );

    /**
     * Send transactions to the HTTP endpoint
     * @param packet The packet to send.
     * @param client Reference to the HTTP sync client.
     */
    static std::vector<std::pair<Hash, bool>> sendTransactions(
      const std::vector<std::string>& packet, HTTPSyncClient& client
    );

    /**
     * Wait for all transactions to be confirmed.
     * @param packetHashes The list of packet hashes to confirm.
     * @param client Reference to the HTTP sync client.
     */
    static void confirmTransactions(
      std::vector<std::pair<Hash, bool>>& packetHashes, HTTPSyncClient& client
    );

    ///@{
    /** Getter. */
    inline uint64_t getCreateTransactionTime() const { return createTransactionTime_; }
    inline uint64_t getSendTransactionTime() const { return sendTransactionTime_; }
    inline uint64_t getConfirmTransactionTime() const { return confirmTransactionTime_; }
    inline uint64_t getTotalSentPackets() const { return totalSentPackets_; }
    ///@}
};

#endif // SIMULATORWORKER_H
