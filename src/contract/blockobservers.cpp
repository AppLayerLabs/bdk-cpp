#include "blockobservers.h"
#include "contracthost.h"

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
      int64_t gas = 5'000'000;
      auto seed = Hash::random();
      evmc_tx_context txContext;
      Hash txHash;

      txContext.tx_gas_price = {};
      txContext.tx_origin = {};
      txContext.block_coinbase = ContractGlobals::getCoinbase().toEvmcAddress();
      txContext.block_number = block.getNHeight();
      txContext.block_timestamp = block.getTimestamp();
      txContext.block_gas_limit = 10000000;
      txContext.block_prev_randao = {};
      txContext.chain_id = EVMCConv::uint256ToEvmcUint256(options_.getChainID());
      txContext.block_base_fee = {};
      txContext.blob_base_fee = {};
      txContext.blob_hashes = nullptr;
      txContext.blob_hashes_count = 0;

      ContractHost host(
        vm_,
        manager_,
        storage_,
        seed,
        txContext,
        contracts_,
        accounts_,
        vmStorage_,
        txHash,
        0,
        block.getHash(),
        gas,
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
      int64_t gas = 5'000'000;
      auto seed = Hash::random();
      evmc_tx_context txContext;
      Hash txHash;

      txContext.tx_gas_price = {};
      txContext.tx_origin = {};
      txContext.block_coinbase = ContractGlobals::getCoinbase().toEvmcAddress();
      txContext.block_number = block.getNHeight();
      txContext.block_timestamp = block.getTimestamp();
      txContext.block_gas_limit = 10000000;
      txContext.block_prev_randao = {};
      txContext.chain_id = EVMCConv::uint256ToEvmcUint256(options_.getChainID());
      txContext.block_base_fee = {};
      txContext.blob_base_fee = {};
      txContext.blob_hashes = nullptr;
      txContext.blob_hashes_count = 0;

      ContractHost host(
        vm_,
        manager_,
        storage_,
        seed,
        txContext,
        contracts_,
        accounts_,
        vmStorage_,
        txHash,
        0,
        block.getHash(),
        gas,
        this
      );

      std::invoke(observer.callback, host);

    } catch (...) {}

    observer.timestamp = observer.step + block.getTimestamp();

    blockTimestampQueue_.push(observer);
  }
}
