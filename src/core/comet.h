/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef COMET_H
#define COMET_H

// TODO: refactor this comet.h / comet.cpp component (which is in a working state now)

#include <thread>
//#include <grpcpp/server.h> // abci=grpc
#include <boost/asio.hpp>  // abci=socket

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

// Needed in this header for cometbft::abci::v1::ABCIService::Service below; Generated from .proto files.
#include <cometbft/abci/v1/service.grpc.pb.h>

class AbciServer;


/// Sets up and maintains a running cometbft instance.
class Comet : public Log::LogicalLocationProvider {
  private:
    const std::string instanceIdStr_; ///< Identifier for logging
    const Options options_; ///< Copy of the supplied Options.

    std::string cometUNIXSocketPath_ = "/tmp/abci.sock"; ///< Best-known path so far for the abci.sock UNIX sockets file

    std::future<void> loopFuture_; ///< Future object holding the consensus engine thread.
    std::atomic<bool> stop_ = false; ///< Flag for stopping the consensus engine thread.

    // TODO: all requests to the consensus engine will be asynchronous and probably queued
    //       at this object for execution when cometbft is fully up and running.

    /*
      The Comet class does not have any upstream knowledge: it does not know what system is using it.
      This allows it to be transparently tested, to be reused, and to be decoupled from upstream code
        that uses it.

      Comet does not throw exceptions, even if it experiences a fatal condition. Throwing errors or
        halting the program is the responsibility of user code, by checking Comet::getStatus().
    */

    std::atomic<bool> status_ = true;   ///< Global status (true = OK, false = failed/terminated).
    std::string errorStr_;              ///< Error message (if any).

    std::atomic<CometState> state_ = CometState::STOPPED; ///< Current step the Comet instance is in.
    std::atomic<CometState> pauseState_ = CometState::NONE; ///< Step to pause/hold the comet instance at, if any.

    void setState(const CometState& state); ///< Apply a state transition, possibly pausing at the new state.
    void setError(const std::string& errorStr); ///< Signal a fatal error condition.
    void resetError(); ///< Reset internal error condition.
    void workerLoop(); ///< Worker loop responsible for establishing and managing a connection to cometbft.
    void workerLoopInner(); ///< Called by workerLoop().

    // The stuff below was added to run the GRPC server
    // TODO: reorganize these fields/methods later

    std::unique_ptr<cometbft::abci::v1::ABCIService::Service> abciService_; ///< Pointer to the ABCI interface impl

    std::atomic<bool> grpcServerStarted_ = false; ///< Controls whether the gRPC server has been started (needed because of grpcServerThread_)
    std::atomic<bool> grpcServerRunning_ = false; ///< Controls whether we know the gRPC server is actually up and running (needed because of grpcServerThread_)

    // gRPC stuff no longer needed, using only protobuf w/ ABCI=Socket connection
    //std::unique_ptr<grpc::Server> grpcServer_; ///< Pointer to the gRPC server implementation
    std::shared_ptr<AbciServer> abciServer_;
    std::optional<std::thread> grpcServerThread_; ///< Dedicated thread that blocks running the gRPC server -- recycled for ABCI io context thread pool join

    void grpcServerRun(); ///< Thread that blocks running the gRPC server

    // new ABCI Socket stuff
    std::unique_ptr<boost::asio::io_context> ioContext_; // the I/O context that runs all socket communications for ABCI via sockets
    std::unique_ptr<boost::asio::thread_pool> threadPool_; // the threadpool that runs ioContext.run()

    // The stuff below is for the running cometbft process in 'cometbft start'
    // TODO: reorganize

    std::optional<boost::process::child> process_; ///< boost::process that points to the running "cometbft start" process

  public:
    /**
     * Constructor.
     * @param instanceIdStr Instance ID string to use for logging.
     * @param options Reference to the Options singleton.
     */
    explicit Comet(std::string instanceIdStr, const Options& options);

    virtual ~Comet() { this->stop(); } ///< Destructor, make sure the thread is stopped

    std::string getLogicalLocation() const override { return instanceIdStr_; } ///< Log instance

    /**
     * Get the global status of the Comet worker.
     * @return `true` if it is in a working state, `false` if it is in a failed/terminated state.
     */
    bool getStatus() { return status_; }

    /**
     * Get the last error message from the Comet worker (not synchronized; call only after getStatus() returns false).
     * @return Error message or empty string if no error.
     */
    const std::string& getErrorStr() { return errorStr_; }

    /**
     * Get the current state of the Comet worker (or last known state before failure/termination; must also call getStatus()).
     * @return The current state.
     */
    CometState getState() { return state_; }

    /**
     * Set the pause state (this is not set by start(), but is set to NONE by stop()).
     * @param pauseState State to pause at, or CometState::NONE to disable.
     */
    void setPauseState(const CometState pauseState = CometState::NONE) { pauseState_ = pauseState; }

    /**
     * Get the pause state.
     * @return The current pause state.
     */
    CometState getPauseState() { return pauseState_; }

    /**
     * Busy wait for the pause state, a given timeout in milliseconds, or an error status.
     * @param timeoutMillis Timeout in milliseconds, or zero to wait forever.
     * @return If error status set, the error string, or "TIMEOUT" in case of timeout, or an empty string if OK.
     */
    std::string waitPauseState(uint64_t timeoutMillis);

    void start(); ///< Start (or restart) the consensus engine loop.
    void stop();  ///< Stop the consensus engine loop.

    // HACKS: just to create a basic test for now
    //        this has to be replaced with something better
    bool gotInitChain();
    int lastBlockHeight();
};

#endif  // COMET_H
