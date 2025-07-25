/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef NETWORKSIMULATOR_H
#define NETWORKSIMULATOR_H

#include "httpclient.h"
#include "common.h"
#include "simulatorworker.h"

/// Network Simulator class. Used to manage and coordinate the workers on the network simulator.
class NetworkSimulator {
  private:
    const PrivKey chainOwnerPrivKey_; ///< Private key of the chain owner address.
    const uint64_t chainId_;  ///< Chain ID of the network.
    const uint64_t packetSize_; ///< Network packet size.
    const uint64_t packetCount_;  ///< Network packet count.
    const uint256_t initNativeBalance_; ///< Initial native balance.
    const uint256_t txNativeBalance_; ///< Transaction native balance.
    const uint64_t workerThreads_;  ///< Number of worker threads.
    const std::vector<std::pair<net::ip::address_v4, uint16_t>> httpEndpoints_; ///< List of active endpoints in the network.
    std::vector<std::vector<WorkerAccount>> accounts_; ///< Vector of accounts for each worker

  public:
    /**
     * Constructor.
     * @param chainOwnerPrivKey Private key of the chain owner address.
     * @param chainId Chain ID of the network.
     * @param packetSize Network packet size.
     * @param packetCount Network packet count.
     * @param initNativeBalance Initial native balance.
     * @param txNativeBalance Transaction native balance.
     * @param workerThreads Number of worker threads.
     * @param httpEndpoints List of active endpoints in the network.
     */
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

    NetworkSimulator(const NetworkSimulator& other) = delete; ///< Copy constructor (deleted, Rule of Zero)
    NetworkSimulator(NetworkSimulator&& other) = delete; ///< Move constructor (deleted, Rule of Zero)
    NetworkSimulator& operator=(const NetworkSimulator& other) = delete; ///< Copy assignment operator (deleted, Rule of Zero)
    NetworkSimulator& operator=(NetworkSimulator&& other) = delete; ///< Move assignment operator (deleted, Rule of Zero)

    void setup(); ///< Setup the network simulator.
    void run(); ///< Run the network simulator.
};

#endif // NETWORKSIMULATOR_H
