#include "../src/core/state.h"
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

    /**
     * Force a contract call, regardless of the current state.
     */
    inline void call(const evmc_message& callInfo,
                const evmc_tx_context& txContext,
                const ContractType& type,
                const Hash& randomness,
                const Hash& txHash,
                const Hash& blockHash,
                int64_t& leftOverGas) {
      ContractHost host(
        this->vm_,
        this->dumpManager_,
        this->eventManager_,
        this->storage_,
        randomness,
        txContext,
        this->contracts_,
        this->accounts_,
        this->vmStorage_,
        this->txToAddr_,
        txHash,
        0,
        blockHash,
        leftOverGas
      );
      host.execute(callInfo, type);
    }
};