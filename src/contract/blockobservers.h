#ifndef BDK_CONTRACT_BLOCKOBSERVERS_H
#define BDK_CONTRACT_BLOCKOBSERVERS_H

#include "utils/tx.h"
#include "utils/finalizedblock.h"
#include "contract/contract.h"
#include <queue>
#include <functional>

class ContractHost;

struct BlockNumberObserver {
  std::function<void(ContractHost&)> callback;
  uint64_t blockNumber;
  uint64_t step;
};

struct BlockTimestampObserver {
  std::function<void(ContractHost&)> callback;
  uint64_t timestamp;
  uint64_t step;
};

constexpr bool operator<(const BlockNumberObserver& lhs, const BlockNumberObserver& rhs) {
  return lhs.blockNumber < rhs.blockNumber;
}

constexpr bool operator>(const BlockNumberObserver& lhs, const BlockNumberObserver& rhs) {
  return lhs.blockNumber > rhs.blockNumber;
}

constexpr bool operator<(const BlockTimestampObserver& lhs, const BlockTimestampObserver& rhs) {
  return lhs.timestamp < rhs.timestamp;
}

constexpr bool operator>(const BlockTimestampObserver& lhs, const BlockTimestampObserver& rhs) {
  return lhs.timestamp > rhs.timestamp;
}

class BlockObservers {
public:
  using Contracts = boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash>;
  using Accounts = boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash>;
  using VmStorage = boost::unordered_flat_map<StorageKey, Hash, SafeHash>;

  BlockObservers(evmc_vm *vm,
    DumpManager& manager,
    Storage& storage,
    Contracts& contracts,
    Accounts& accounts,
    VmStorage& vmStorage,
    const Options& options);

  void add(BlockNumberObserver observer);

  void add(BlockTimestampObserver observer);

  void notify(const FinalizedBlock& block);

private:
  void notifyNumberQueue(const FinalizedBlock& block);

  void notifyTimestampQueue(const FinalizedBlock& block);

  std::priority_queue<BlockNumberObserver, std::vector<BlockNumberObserver>, std::greater<BlockNumberObserver>> blockNumberQueue_;
  std::priority_queue<BlockTimestampObserver, std::vector<BlockTimestampObserver>, std::greater<BlockTimestampObserver>> blockTimestampQueue_;
  evmc_vm* vm_;
  DumpManager& manager_;
  Storage& storage_;
  Contracts& contracts_;
  Accounts& accounts_;
  VmStorage& vmStorage_;
  const Options& options_;
};

#endif // BDK_CONTRACT_BLOCKOBSERVERS_H
