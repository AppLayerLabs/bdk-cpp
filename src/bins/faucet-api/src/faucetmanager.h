/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef FAUCETMANAGER_H
#define FAUCETMANAGER_H

#include "httpserver.h"
#include "net/http/httpclient.h"
#include "utils/utils.h"
#include "utils/tx.h"
#include "net/http/httpclient.h"
#include <boost/algorithm/string.hpp>
#include "libs/BS_thread_pool_light.hpp"
#include <shared_mutex>
#include <mutex>

/// Helper struct that abstracts a worker account.
struct WorkerAccount {
  const PrivKey privKey;  ///< Private key of the account.
  const Address address;  ///< Address of the account.
  uint256_t nonce;  ///< Current nonce of the account.
  std::mutex inUse_;  ///< Mutex for when the account is in use.

  /**
   * Constructor.
   * @param privKey Private key of the account.
   */
  explicit WorkerAccount (const PrivKey& privKey) : privKey(privKey), address(Secp256k1::toAddress(Secp256k1::toUPub(privKey))), nonce(0) {}

  /// Copy constructor.
  WorkerAccount(const WorkerAccount& other) : privKey(other.privKey), address(other.address), nonce(other.nonce) {}
};

/// Namespace for faucet-related functionalities.
namespace Faucet {
  // Forward declaration.
  class Manager;

  /**
   * Helper worker class for the faucet.
   * Consumes Manager::dripQueue_, setting it to nullptr after copying it.
   * Locks dripMutex_ to get the next list to drip to.
   */
  class FaucetWorker {
    private:
      Manager& manager_; ///< Reference to the manager.
      HTTPSyncClient client_; ///< Reference to the HTTP sync client.
      std::future<bool> runFuture_; ///< Future for the run function so we know when it should stop.
      std::atomic<bool> stop_ = false; ///< Flag that tells the worker to stop.
      bool run(); ///< Start the worker loop.

    public:
      /**
       * Constructor.
       * @param manager Reference to the manager.
       * @param httpEndpoint The endpoint to operate on.
       */
      FaucetWorker(Manager& manager, const std::pair<net::ip::address_v4, uint16_t>& httpEndpoint)
        : manager_(manager), client_(httpEndpoint.first.to_string(), std::to_string(httpEndpoint.second))
      { this->client_.connect(); }

      /// Destructor.
      ~FaucetWorker() { this->client_.close(); this->stop(); }

      FaucetWorker(const FaucetWorker& other) = delete; ///< Copy constructor (deleted, Rule of Zero).
      FaucetWorker& operator=(const FaucetWorker& other) = delete; ///< Copy assignment operator (deleted, Rule of Zero).
      FaucetWorker(FaucetWorker&& other) = delete; ///< Move constructor (deleted, Rule of Zero).
      FaucetWorker& operator=(FaucetWorker&& other) = delete; ///< Move assignment operator (deleted, Rule of Zero).

      void start(); ///< Start the worker.
      void stop(); ///< Stop the worker.
  };

  /// Faucet manager class.
  class Manager {
    private:
      FaucetWorker faucetWorker_; ///< Worker object.
      BS::thread_pool_light threadPool_;  ///< Thread pool.
      std::vector<WorkerAccount> faucetWorkers_;  ///< List of worker objects.
      const uint64_t chainId_;  ///<  CHain ID that the faucet is operating on.
      HTTPServer server_; ///< HTTP server object.
      const std::pair<net::ip::address_v4, uint16_t> httpEndpoint_; ///< HTTP endpoint to be used for the client
      const uint16_t port_; ///< Port to be used for the server
      std::mutex dripMutex_;  ///< Mutex for managing read/write access to the drip list.
      std::unique_ptr<std::vector<Address>> dripQueue_; ///< List of drip addresses to iterate on.
      std::mutex lastIndexMutex_; ///< Mutex for managing read/write access to the last index.
      uint64_t lastIndex_ = 0;  ///< Last index.
      std::shared_mutex accountsMutex_; ///< Mutex for managing read/write access to the accounts list.
      std::unordered_set<Address, SafeHash> accounts_;  ///< List of accounts.

    public:

      /**
       * Constructor.
       * @param faucetWorkers List of faucet worker objects.
       * @param chainId Chain ID that the faucet will operate on.
       * @param httpEndpoint Endpoint that the faucet will iterate on.
       * @param port Port that the faucet will operate on.
       */
      Manager(
        const std::vector<WorkerAccount>& faucetWorkers,
        const uint64_t& chainId,
        const std::pair<net::ip::address_v4, uint16_t>& httpEndpoint,
        const uint16_t& port
      ) : faucetWorkers_(faucetWorkers),
          chainId_(chainId),
          httpEndpoint_(httpEndpoint),
          port_(port),
          server_(port, *this),
          threadPool_(8),
          faucetWorker_(*this, httpEndpoint) {}

      Manager(const Manager& other) = delete; ///< Copy constructor (deleted, Rule of Zero).
      Manager& operator=(const Manager& other) = delete; ///< Copy assignment operator (deleted, Rule of Zero).
      Manager(Manager&& other) = delete; ///< Move constructor (deleted, Rule of Zero).
      Manager& operator=(Manager&& other) = delete; ///< Move assignment operator (deleted, Rule of Zero).

      /**
       * Request a drip to a given address.
       * @param address The address to drip into.
       * @return A string containing the result of the drip request.
       */
      static std::string makeDripToAddress(const Address& address);

      /**
       * Make a new "send" transaction (eth_sendRawTransaction).
       * @param account The account to be used.
       * @param txNativeBalance The transaction native balance to be used.
       * @param chainId The chain ID to be used.
       * @param to The address to send the transaction(s) to.
       * @return The resulting json string of the eth_sendRawTransaction operation.
       */
      static std::string createTransactions(
        WorkerAccount& account, const uint256_t& txNativeBalance,
        const uint64_t& chainId, const Address& to
      );

      void setup(); ///< Setup the faucet.
      void run(); ///< Run the faucet.

      /**
       * Process the next drip request in the queue for a given address.
       * @param address The address to process the drip request of.
       */
      void processDripToAddress(const Address& address);

      /**
       * Execute the drip request to a given address.
       * @param address The address to execute the drip on.
       */
      void dripToAddress(const Address& address);

      friend class FaucetWorker;
  };
};

#endif  // FAUCETMANAGER_H
