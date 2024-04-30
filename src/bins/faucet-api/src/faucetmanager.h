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

struct WorkerAccount {
  const PrivKey privKey;
  const Address address;
  uint256_t nonce;
  std::mutex inUse_;
  explicit WorkerAccount (const PrivKey& privKey) : privKey(privKey), address(Secp256k1::toAddress(Secp256k1::toUPub(privKey))), nonce(0) {}
  // Copy constructor
  WorkerAccount(const WorkerAccount& other) : privKey(other.privKey), address(other.address), nonce(other.nonce) {}
};

namespace Faucet {
  class Manager;
  // An class for the FaucetWorker.
  // Consumes Manager::dripQueue_, setting it to nullptr after copying it.
  // Locks dripMutex_ to get the next list to drip to.
  class FaucetWorker {
    private:
      Manager& manager_;
      HTTPSyncClient client_;
      bool run();
      /// Future for the run function so we know when it should stop.
      std::future<bool> runFuture_;
      std::atomic<bool> stop_ = false;

    public:
      FaucetWorker(Manager& manager, const std::pair<net::ip::address_v4, uint16_t>& httpEndpoint) : manager_(manager), client_(httpEndpoint.first.to_string(), std::to_string(httpEndpoint.second)) {
        this->client_.connect();
      }
      ~FaucetWorker() {
        this->client_.close();
        this->stop();
      }
      FaucetWorker(const FaucetWorker& other) = delete;
      FaucetWorker& operator=(const FaucetWorker& other) = delete;
      FaucetWorker(FaucetWorker&& other) = delete;
      FaucetWorker& operator=(FaucetWorker&& other) = delete;
      void start();
      void stop();

  };

  class Manager {
    private:
      FaucetWorker faucetWorker_;
      BS::thread_pool_light threadPool_;
      std::vector<WorkerAccount> faucetWorkers_;
      const uint64_t chainId_;
      HTTPServer server_;
      const std::pair<net::ip::address_v4, uint16_t> httpEndpoint_;  // HTTP endpoint to be used for the client
      const uint16_t port_; // Port to be used for the server
      std::mutex dripMutex_;
      std::unique_ptr<std::vector<Address>> dripQueue_;
      std::mutex lastIndexMutex_;
      uint64_t lastIndex_ = 0;
      std::shared_mutex accountsMutex_;
      std::unordered_set<Address, SafeHash> accounts_;
    public:

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

      Manager(const Manager& other) = delete;
      Manager& operator=(const Manager& other) = delete;
      Manager(Manager&& other) = delete;
      Manager& operator=(Manager&& other) = delete;

      static std::string makeDripToAddress(const Address& address);

      // Make a new "send" tx. return json string eth_sendRawTransaction
      static std::string createTransactions(WorkerAccount& account,
                                 const uint256_t& txNativeBalance,
                                 const uint64_t& chainId,
                                 const Address& to);


      void setup();
      void run();
      void processDripToAddress(const Address& address);

      void dripToAddress(const Address& address);
      friend class FaucetWorker;
  };
};

#endif  // FAUCETMANAGER_H
