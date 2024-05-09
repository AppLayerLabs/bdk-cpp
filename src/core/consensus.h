/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BROADCASTER_H
#define BROADCASTER_H

#include <thread>

#include "rdpos.h"

#include "../utils/ecdsa.h"
#include "../utils/logger.h"
#include "../utils/strings.h"
#include "../utils/tx.h"

class Blockchain; // Forward declaration.

// TODO: tests for Consensus (if necessary)

/// Class responsible for processing blocks and transactions.
class Consensus {
  private:
    State& state_; ///< Reference to the State object.
    P2P::ManagerNormal& p2p_; ///< Reference to the P2P connection manager.
    const Storage& storage_; ///< Reference to the blockchain storage.
    const Options& options_; ///< Reference to the Options singleton.

    std::future<void> loopFuture_;  ///< Future object holding the thread for the consensus loop.
    std::atomic<bool> stop_ = false; ///< Flag for stopping the consensus processing.

    /**
     * Create and broadcast a Validator block (called by validatorLoop()).
     * If the node is a Validator and it has to create a new block,
     * this function will be called, the new block will be created based on the
     * current State and rdPoS objects, and then it will be broadcast.
     * @throw DynamicException if block is invalid.
     */
    void doValidatorBlock();

    /**
     * Wait for a new block (called by validatorLoop()).
     * If the node is a Validator, this function will be called to make the
     * node wait until it receives a new block.
     */
    void doValidatorTx(const uint64_t& nHeight, const Validator& me);

  public:
    /**
     * Constructor.
     * @param state Reference to the State object.
     * @param p2p Reference to the P2P connection manager.
     * @param storage Reference to the blockchain storage.
     * @param options Reference to the Options singleton.
     */
    explicit Consensus(State& state, P2P::ManagerNormal& p2p, const Storage& storage, const Options& options) :
      state_(state), p2p_(p2p), storage_(storage), options_(options) {}

    /**
     * Entry function for the worker thread (runs the workerLoop() function).
     * @return `true` when done running.
     */
    bool workerLoop();
    void validatorLoop(); ///< Routine loop for when the node is a Validator.

    void start(); ///< Start the consensus loop. Should only be called after node is synced.
    void stop();  ///< Stop the consensus loop.
};

#endif  // BROADCASTER_H
