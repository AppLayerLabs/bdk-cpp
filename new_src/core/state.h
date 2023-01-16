#ifndef STATE_H
#define STATE_H

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "block.h"
#include "blockChain.h"
#include "blockManager.h"
//#include "subnet.h" // TODO: fix circular dep
#include "../net/grpcclient.h"
#include "../utils/db.h"
#include "../utils/random.h"
#include "../utils/transaction.h"
#include "../utils/utils.h"

class State {
  private:
    std::unordered_map<Address, Account, SafeHash> nativeAccounts;
    std::unordered_map<Hash, Tx, SafeHash> mempool;
    const std::shared_ptr<DB> db;
    const std::shared_ptr<BlockChain> chain;
    const std::shared_ptr<BlockMempool> mempool;
    const std::shared_ptr<BlockManager> mgr;
    const std::shared_ptr<gRPCClient> grpcClient;
    std::mutex stateLock;
    RandomGen gen;
    bool saveToDB();
    bool loadFromDB();
    bool processNewTx(const Tx& tx);
  public:
    State(
      const std::shared_ptr<DB>& db,
      const std::shared_ptr<BlockChain>& chain,
      const std::shared_ptr<BlockMempool>& mempool,
      const std::shared_ptr<BlockManager>& mgr,
      const std::shared_ptr<gRPCClient>& grpcClient
    ) : db(db), chain(chain), mempool(mempool), mgr(mgr), grpcClient(grpcClient) {
      this->loadFromDB();
    }
    const std::unordered_map<Hash, Tx, SafeHash>& getMempool() { return this->mempool; }
    uint256_t getNativeBalance(const Address& add);
    uint256_t getNativeNonce(const Address& add);
    bool validateNewBlock(const std::shared_ptr<const Block>& block);
    void processNewBlock(const std::shared_ptr<const Block>&& block);
    const std::shared_ptr<const Block> createNewBlock();
    bool validateTxForBlock(const Tx& tx);
    const std::pair<int, string> validateTxForRPC(const Tx& tx);
    void addBalance(const Address& add);
};

#endif  // STATE_H
