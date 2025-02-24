#include "blockobservers.h"
#include "contracthost.h"
#include "bytes/random.h"

BlockObservers::BlockObservers(
  evmc_vm *vm,
  DumpManager& manager,
  Storage& storage,
  Contracts& contracts,
  Accounts& accounts,
  VmStorage& vmStorage,
  const Options& options)
    : vm_(vm),
      manager_(manager),
      storage_(storage),
      contracts_(contracts),
      accounts_(accounts),
      vmStorage_(vmStorage),
      options_(options) {}

void BlockObservers::add(BlockNumberObserver observer) {
  blockNumberQueue_.push(std::move(observer));
}

void BlockObservers::add(BlockTimestampObserver observer) {
  blockTimestampQueue_.push(std::move(observer));
}

void BlockObservers::notify(const FinalizedBlock& block) {
  notifyNumberQueue(block);
  notifyTimestampQueue(block);
}

void BlockObservers::notifyNumberQueue(const FinalizedBlock& block) {
  if (blockNumberQueue_.empty()) {
    return;
  }

  while (blockNumberQueue_.top().blockNumber <= block.getNHeight()) {
    BlockNumberObserver observer = blockNumberQueue_.top();
    blockNumberQueue_.pop();

    try {
      Hash seed = bytes::random();

      ExecutionContext context = ExecutionContext::Builder{}
      .storage(vmStorage_)
      .accounts(accounts_)
      .contracts(contracts_)
      .blockHash(block.getHash())
      .txHash(Hash())
      .txOrigin(Address())
      .blockCoinbase(ContractGlobals::getCoinbase())
      .txIndex(0)
      .blockNumber(ContractGlobals::getBlockHeight())
      .blockTimestamp(ContractGlobals::getBlockTimestamp())
      .blockGasLimit(10'000'000)
      .txGasPrice(0)
      .chainId(this->options_.getChainID())
      .build();

      ContractHost host(
        vm_,
        manager_,
        storage_,
        seed,
        context,
        this
      );

      std::invoke(observer.callback, host);
    } catch (...) {}

    observer.blockNumber = block.getNHeight() + observer.step;
  
    blockNumberQueue_.push(observer);
  }
}

void BlockObservers::notifyTimestampQueue(const FinalizedBlock& block) {
  if (blockTimestampQueue_.empty()) {
    return;
  }

  while (blockTimestampQueue_.top().timestamp <= block.getTimestamp()) {
    BlockTimestampObserver observer = blockTimestampQueue_.top();
    
    blockTimestampQueue_.pop();

    try {
      Hash seed = bytes::random();

      ExecutionContext context = ExecutionContext::Builder{}
      .storage(vmStorage_)
      .accounts(accounts_)
      .contracts(contracts_)
      .blockHash(block.getHash())
      .txHash(Hash())
      .txOrigin(Address())
      .blockCoinbase(ContractGlobals::getCoinbase())
      .txIndex(0)
      .blockNumber(ContractGlobals::getBlockHeight())
      .blockTimestamp(ContractGlobals::getBlockTimestamp())sc
      .blockGasLimit(10'000'000)
      .txGasPrice(0)
      .chainId(this->options_.getChainID())
      .build();

      ContractHost host(
        vm_,
        manager_,
        storage_,
        block.getBlockRandomness(),
        context,
        this
      );

      std::invoke(observer.callback, host);
    } catch (...) {}

    observer.timestamp = observer.step + block.getTimestamp();

    blockTimestampQueue_.push(observer);
  }
}
