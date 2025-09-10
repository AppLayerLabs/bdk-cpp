/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STATETEST_HPP
#define STATETEST_HPP

#include "../src/core/state.h"
#include "../src/bytes/random.h"

#include "../src/contract/contracthost.h"


// TestState class
// The only purpose of this class is to be able to allow
// Direct call to internal methods of State class
// That is not necessary in production code.
// This is done to better isolate the code and test it.

class StateTest : public State {
  public:
    // StateTest has the same constructor as State
    StateTest(const DB& db, Storage& storage, P2P::ManagerNormal& p2pManager, const uint64_t& snapshotHeight, const Options& options) :
      State(db, storage, p2pManager, snapshotHeight, options) {};

    void call(const TxBlock& tx) {
      std::unique_lock lock(this->stateMutex_);
      ExecutionContext context = ExecutionContext::Builder{}
      .storage(this->vmStorage_)
      .accounts(this->accounts_)
      .contracts(this->contracts_)
      .evmContracts(this->evmContracts_)
      .blockHash(Hash())
      .txHash(tx.hash())
      .txOrigin(tx.getFrom())
      .blockCoinbase(ContractGlobals::getCoinbase())
      .txIndex(0)
      .blockNumber(ContractGlobals::getBlockHeight())
      .blockTimestamp(ContractGlobals::getBlockTimestamp())
      .blockGasLimit(100'000'000)
      .txGasPrice(tx.getMaxFeePerGas())
      .chainId(this->options_.getChainID())
      .build();

      ContractHost host(
        this->vm_,
        this->dumpManager_,
        this->storage_,
        Hash(),
        context);

      Gas gas(uint64_t(tx.getGasLimit()));

      std::visit([&] (auto&& msg) {
        host.execute(std::forward<decltype(msg)>(msg));
      }, tx.toMessage(gas));
    };
};

#endif // STATETEST_HPP
