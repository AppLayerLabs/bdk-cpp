/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef COMET_H
#define COMET_H

#include "../utils/options.h"
#include "../utils/logger.h"

/// Comet driver states
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
  TERMINATED       = 13, ///< Comet worker somehow ran out of work (this is always an error)
  FINISHED         = 14, ///< Comet worker loop quit (stopped for some explicit reason)
  NONE             = 15  ///< Dummy state to disable state stepping
};

/**
 * The Comet class notifies its user of events through the CometListener interface.
 * Users of the Comet class must implement a CometListener class and pass a pointer
 * to a CometListener object to Comet so that they can receive Comet events.
 *
 * NOTE: These callbacks may or may not be invoked in parallel. You should not assume
 * that these are invoked in any particular order or that they aren't concurrent.
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
};

class CometImpl;

/**
 * The Comet class is instantiated by the BDK node to serve as an interface to CometBFT.
 * Most of its implementation details are private and contained in CometImpl, which is
 * declared and defined in comet.cpp, in order to keep this header short and simple.
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
     * @param stepMode If true, no empty blocks are ever produced (for unit testing only).
     */
    explicit Comet(CometListener* listener, std::string instanceIdStr, const Options& options, bool stepMode = false);

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

    /**
     * Return an instance (object) identifier for all LOGxxx() messages emitted this class.
     */
    std::string getLogicalLocation() const override { return instanceIdStr_; }
};

/// Consensus parameter: PBTS Clock drift.
/// This must be at least 500ms due to leap seconds.
/// Give 1 whole second for clock drift, which is plenty.
#define COMETBFT_PBTS_SYNCHRONY_PARAM_PRECISION_SECONDS 1

/// Consensus parameter: PBTS Maximum network delay for proposal (header) propagation.
/// This cannot be too small or else performance is affected.
/// If it is too large, malicious validators can timestamp blocks up to this amount in the future,
/// which slows downs the network (need to wait for real time to pass until it catches up with the
/// timestamp of the previous block) but that's externally observable and punishable by governance.
/// Five seconds is plenty for the propagation of a timestamp worldwide.
#define COMETBFT_PBTS_SYNCHRONY_PARAM_MESSAGE_DELAY_SECONDS 5

#endif