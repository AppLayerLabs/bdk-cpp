#ifndef NETWORKSIMULATOR_H
#define NETWORKSIMULATOR_H

#include "httpclient.h"
#include "common.h"
#include "simulatorworker.h"

/**
 * Network Simulator class
 * Used to manage and coordinate the workers on the network simulator
 */
class NetworkSimulator {
  private:
    const PrivKey chainOwnerPrivKey_;
    const uint64_t chainId_;
    const uint64_t packetSize_;
    const uint64_t packetCount_;
    const uint256_t initNativeBalance_;
    const uint256_t txNativeBalance_;
    const uint64_t workerThreads_;
    const std::vector<std::pair<net::ip::address_v4, uint16_t>> httpEndpoints_;

    std::vector<std::vector<WorkerAccount>> accounts_; // Vector of accounts for each worker

  public:

    NetworkSimulator(
      const PrivKey& chainOwnerPrivKey,
      const uint64_t& chainId,
      const uint64_t& packetSize,
      const uint64_t& packetCount,
      const uint256_t& initNativeBalance,
      const uint256_t& txNativeBalance,
      const uint64_t& workerThreads,
      const std::vector<std::pair<net::ip::address_v4, uint16_t>>& httpEndpoints
    );
    NetworkSimulator(const NetworkSimulator& other) = delete;
    NetworkSimulator& operator=(const NetworkSimulator& other) = delete;
    NetworkSimulator(NetworkSimulator&& other) = delete;
    NetworkSimulator& operator=(NetworkSimulator&& other) = delete;

    void setup();
    void run();
};

#endif // NETWORKSIMULATOR_H