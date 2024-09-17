/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef COMET_H
#define COMET_H

#include <thread>

#include "../net/p2p/managernormal.h"
#include "../utils/options.h"
#include "../utils/logger.h"

/// Sets up and maintains a running cometbft instance.
class Comet : public Log::LogicalLocationProvider {
  private:
    State& state_; ///< Reference to the State object.
    P2P::ManagerNormal& p2p_; ///< Reference to the P2P connection manager.
    const Storage& storage_; ///< Reference to the blockchain storage.
    const Options& options_; ///< Reference to the Options singleton.

    std::future<void> loopFuture_; ///< Future object holding the consensus engine thread.
    std::atomic<bool> stop_ = false; ///< Flag for stopping the consensus engine thread.

    // TODO: we will need to add a variable here (or more) to register the current state
    //       of the cometbft manager, if anything for diagnostics.
    //       all requests to the consensus engine will be asynchronous and probably queued
    //       at this object for execution when cometbft is fully up and running.


  public:
    /**
     * Constructor.
     * @param state Reference to the State object.
     * @param p2p Reference to the P2P connection manager.
     * @param storage Reference to the blockchain storage.
     * @param options Reference to the Options singleton.
     */
    explicit Comet(State& state, P2P::ManagerNormal& p2p, const Storage& storage, const Options& options) :
      state_(state), p2p_(p2p), storage_(storage), options_(options) {}

    std::string getLogicalLocation() const override { return p2p_.getLogicalLocation(); } ///< Log instance from P2P

    /**
     * Worker loop responsible for establishing and managing a connection to cometbft.
     */
    void workerLoop();

    void start(); ///< Start the consensus engine loop.
    void stop();  ///< Stop the consensus engine loop.
};

#endif  // COMET_H
