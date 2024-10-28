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
     * @param instanceIdStr Instance ID string to use for logging.
     * @param options Reference to the Options singleton.
     */
    explicit Comet(std::string instanceIdStr, const Options& options);

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

#endif