/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef COMET_H
#define COMET_H

#include "../utils/options.h"
#include "../utils/logger.h"

/// Comet states
enum class CometState {
  STOPPED          =  0, ///< Comet is in stopped state (no worker thread started)
  STARTED          =  1, ///< Comet worker thread just started
  CONFIGURING      =  2, ///< Starting to set up comet config
  CONFIGURED       =  3, ///< Finished setting up comet config
  INSPECTING_COMET =  4, ///< Starting cometbft inspect
  INSPECTED_COMET  =  5, ///< Stopped cometbft inspect; all tests passed
  STARTING_ABCI    =  6, ///< Starting ABCI server on our end
  STARTED_ABCI     =  7, ///< Started ABCI server
  STARTING_COMET   =  8, ///< Running cometbft start
  STARTED_COMET    =  9, ///< cometbft start successful
  TESTING_COMET    = 10, ///< Starting to test cometbft connection
  TESTED_COMET     = 11, ///< Finished cometbft connection test; all tests passed
  RUNNING          = 12, ///< Comet is running
  TERMINATED       = 13, ///< Comet worker somehow ran out of work (probably an error)
  FINISHED         = 14, ///< Comet worker loop quit (stop_ set to true?)
  NONE             = 15  ///< Dummy state to disable state stepping
};

/**
 * The Comet class notifies its user of events through the CometListener interface.
 * Users of the Comet class must implement a CometListener class and pass a pointer
 * to a CometListener object to Comet so that they can receive Comet events.
 */
class CometListener {
  public:

    // NOTE: the return value of all callbacks is set to void because they are
    //       reserved for e.g. some status or error handling use.
    //       all user return values are outparams.

    /**
     * Called upon starting a fresh blockchain (i.e. empty block storage) from a given genesis config.
     * TODO: Check all parameters involved and use/handle them as needed.
     * @param appHash Outparam to be set to the application's initial app hash.
     */
    virtual void initChain(Bytes& appHash) {
      appHash.clear();
    }

    /**
     * Check if a transaction is valid.
     * @param tx The transaction to check.
     * @param accept Outparam to be set to `true` if the transaction is valid, `false` if it is invalid.
     */
    virtual void checkTx(const Bytes& tx, bool& accept) {
      accept = true;
    }

    /**
     * Notification of a new finalized block added to the chain.
     * @param height The block height of the new finalized block at the new head of the chain.
     * @param syncingToHeight If the blockchain is doing a replay, syncingToHeight > height, otherwise syncingToHeight == height.
     * @param txs All transactions included in the block, which need to be processed into the application state.
     * @param appHash Outparam to be set with the hash of the application state after all `txs` are processed into it.
     */
    virtual void incomingBlock(const uint64_t height, const uint64_t syncingToHeight, const std::vector<Bytes>& txs, Bytes& appHash) {
      appHash.clear();
    }

    /**
     * Validator node receives a block proposal from the block proposer, and must check if the proposal is a valid one.
     * @param height The block height of the new block being proposed.
     * @param txs All transactions included in the block, which need to be verified.
     * @param accept Outparam to be set to `true` if the proposed block is valid, `false` otherwise.
     */
    virtual void validateBlockProposal(const uint64_t height, const std::vector<Bytes>& txs, bool& accept) {
      accept = true;
    }

    /**
     * Callback from cometbft to check what is the current state of the application.
     * @param height Outparam to be set with the height of the last block processed to generate the current application state.
     * @param appHash Outparam to be set with the hash of the current application state (i.e. the state at `height`).
     */
    virtual void getCurrentState(uint64_t& height, Bytes& appHash) {
      height = 0;
      appHash.clear();
    }

    /**
     * Callback from cometbft to check if it can prune some old blocks from its block store.
     * @param height Outparam to be set with the height of the earliest block that must be kept (all earlier blocks are deleted).
     */
    virtual void getBlockRetainHeight(uint64_t& height) {
      height = 0;
    }

    /**
     * Notification of what the cometbft block store height is. If the application is ahead, it can bring itself to a height
     * that is equal or lower than this, or it can prepare to report a correct app_hash and validator set changes after
     * each incomingBlock() callback that it will get with a height that is lower or equal than its current heigh without
     * computing state (i.e. feeding it to Comet via stored historical data that it has computed in the past or that it has
     * obtained off-band). The application is free to block this callback for any amount of time.
     * @param height The current head height in the cometbft block store (the height that the cometbft node/db is at).
     */
    virtual void currentCometBFTHeight(const uint64_t height) {
    }

    /**
     * Notification that Comet failed to deliver a transaction to cometbft via the CometBFT RPC port.
     * @param tx Transaction that was previously sent via `Comet::sendTransaction()` but that failed to be sent.
     * @param error Error message (JSON-RPC response).
     */
    virtual void sendTransactionFailed(const Bytes& tx, const std::string& error) {
    }

    // InitChain callback
    //
    // configuration-heavy, this will probably forward lots of parameters and
    // get lots of parameters in return, not sure which ones will be hidden.
    //
    // void initChain( ..in params.. , ..out params.. )

    // CheckTx callback
    //
    // needed so the mempool is not filled with garbage.
    // MAY be enough to allow us to provide a default implementation of PrepareProposal
    // that just forwards all txs to cometbft instead of rechecking them all.
    // default for "accept" is false (it is the outparam)
    //
    // void checkTx(const Bytes& tx, bool& accept);

    // FinalizeBlock callback (?)
    // user should be notified of a new sequence of opaque transactions to execute
    // note that there should not be a Block type because we don't want to imply that
    // the user of the Comet class should ever be having or handling some kind of block
    // store; the block store is in the cometbft home dir.
    //
    // this call delivers transactions for execution in the execution environment.
    // the height parameter is a verification parameter; the execution environment
    // should already be at that height, and if it isn't then something went wrong
    // somewhere (i.e. it is a bug to receive this call while the execution environment
    // of the Comet user is not already at that height)
    //
    // appHash is an outparam and must be a deterministic result of executing these
    // transactions for that new block height at the user (execution environment).
    //
    // If the node is syncing/replaying blocks then syncing_to_height == target height. 
    // If not, syncing_to_height == height.
    //
    //void incomingBlock(int height, int, syncingToHeight,
    //                   const std::vector<Bytes>& txs, Hash& appHash)

    // ProcessProposal callback
    // user should validate a proposal (block)
    // this is fundamental to protect against a malicious proposer that is proposing
    // transaction content that renders the block invalid under block validation 
    // rules for the user's execution environment.
    // for example, transactions that can't offer payment for their own execution
    // because they don't have a proper authorization from a funded account that is
    // budgeting gas that could then be e.g. absorbed to pay for the rest of the
    // transaction being erroneous or faulty in some way -- these transactions are
    // attacks in the protocol and should probably render the entire proposed block
    // invalid.
    //
    // this is called inside the ABCI ProcessProposal callback. the execution environment
    // *could* execute these transactions to create a speculative version of state advancement
    // as well, and then in the subsequent incomingBlock() it would not have anything to
    // actually execute, it would just have to bless the speculative state instead of
    // rolling it back or discarding it.
    // HOWEVER, for now, we will implement it as BDK's execution environment needs it,
    // which is first just run basic validation on the transactions to see they don't
    // invalidate the whole block according to the execution environment's share of 
    // block validaiton rules, then actually execute them only at incomingBlock().
    //
    // void validateBlockProposal(int height, const std::vector<Bytes>& txs)

    // Info callback
    // cometbft is essentially asking for the block we are in.
    // here we return the block height and the block app hash (which we compute at the
    // end of executing each block, needs to be deterministic) we are in.
    // We will tell cometbft that state in memory is persisted and tell it to retain
    // the blocks between the memory state and the state we have actually snapshotted,
    // because on a subsequent recovery/sync we will tell it a different state (as if
    // the state was "lost" from permanent storage -- there's no such thing as
    // actually permanent storage in an execution environment) and it will replay the
    // blocks for us.
    //
    // void getCurrentState(int& height, Hash& appHash); // out params

    // Commit callback
    // when cometbft asks us to persist state, we don't actually do that since that
    // is too expensive. what we are really doing here is telling cometbft the 
    // block pruning window.
    //
    // void getBlockRetainHeight(int& height); // out param


    // absent callbacks (for now):
    // - prepareproposal: will just forward the same transactions to cometbft
    //   assuming that implementing CheckTx corretly is enough. if it isn't,
    //   we would have to recheck every transaction instead of just forwarding
    //   all of the transactions in the default impl of PrepareProposal.
    //
    // - echo: absorbed internally by Comet/ABCIHandler
    // - flush: just require sync callback impls
    // - query: absorbed internally, may be used to implement other callbacks
    // - everything about configs
    // - everything about validator set changes
    // - everything about gas
    // - everything about logging
    // - transaction arbitrary return results (byte arrays)
    // - etc.

};

class CometImpl;

/**
 * The Comet class is instantiated by the BDK node to serve as an interface to CometBFT.
 * Most of its implementation details are private and contained in CometImpl, which is
 * declared and defined in comet.cpp, in order to keep this header short and simple.
 *
 * TODO:
 * - comet / cometimpl cleanups
 */
class Comet : public Log::LogicalLocationProvider {
  private:
    const std::string instanceIdStr_; ///< Identifier for logging

    std::unique_ptr<CometImpl> impl_; ///< Private implementation details of a Comet instance.

  public:
    /**
     * Constructor.
     * @param listener Pointer to an object of a class that implements the CometListener interface
     *                 and that will receive event callbacks from this Comet instance.
     * @param instanceIdStr Instance ID string to use for logging.
     * @param options Reference to the Options singleton.
//     * @param extraArgs Extra command-line arguments to pass to 'cometbft start' (for testing).
     * @param stepMode If true, no empty blocks are ever produced (for unit testing only).
     */
    explicit Comet(CometListener* listener, std::string instanceIdStr, const Options& options, bool stepMode = false);
    //, const std::vector<std::string>& extraArgs = {}

    /**
     * Destructor; ensures all subordinate jobs are stopped.
     */
    virtual ~Comet();

    /**
     * Get the global status of the Comet worker.
     * @return `true` if it is in a working state, `false` if it is in a failed/terminated state.
     */
    bool getStatus();

    /**
     * Get the last error message from the Comet worker (not synchronized; call only after
     * getStatus() returns false).
     * @return Error message or empty string if no error.
     */
    const std::string& getErrorStr();

    /**
     * Get the current state of the Comet worker (or last known state before failure/termination;
     * must also call getStatus()).
     * @return The current state.
     */
    CometState getState();

    /**
     * Set the pause state (this is not set by start(), but is set to NONE by stop()).
     * The pause state is a state that the comet worker thread stops at when it is reached, which
     * is useful for writing unit tests for the Comet class.
     * @param pauseState State to pause at, or CometState::NONE to disable.
     */
    void setPauseState(const CometState pauseState = CometState::NONE);

    /**
     * Get the pause state.
     * @return The current pause state.
     */
    CometState getPauseState();

    /**
     * Busy wait for the pause state, a given timeout in milliseconds, or an error status.
     * @param timeoutMillis Timeout in milliseconds, or zero to wait forever.
     * @return If error status set, the error string, or "TIMEOUT" in case of timeout, or an empty string if OK.
     */
    std::string waitPauseState(uint64_t timeoutMillis);

    /**
     * Get the cometbft node ID; this is only guaranteed to be set from the INSPECTED_COMET state and ownards.
     * @return The cometbft node id string, or an empty string if unknown (not yet computed) at this point.
     */
    std::string getNodeID();

    /**
     * Enqueues a transaction to be sent to the cometbft node; will retry until the localhost cometbft
     * instance is running and it acknowledges the receipt of the transaction bytes, meaning it should
     * be in its mempool from now on (if it passes CheckTx, etc).
     * @param tx The raw bytes of the transaction object.
     */
    void sendTransaction(const Bytes& tx);

    /**
     * Start (or restart) the consensus engine loop.
     */
    void start();

    /**
     * Stop the consensus engine loop; sets the pause state to CometState::NONE.
     */
    void stop();

    // TODO: Review CometListener callback synchronization/multithreading requirements.
    //
    // This CometListener interface may or may not be called back from the ABCIHandler threads
    // (the ABCIServer threads) -- whatever satisfies the Comet class' interaction with cometbft;
    // the caller should assume the callbacks are not thread-safe, if they accidentally are
    // for some revision of the Comet class.

    /**
     * Return an instance (object) identifier for all LOGxxx() messages emitted this class.
     */
    std::string getLogicalLocation() const override { return instanceIdStr_; }
};

#endif