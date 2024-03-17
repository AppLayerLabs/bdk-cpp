/*
Copyright (c) [2023-2024] [Sparq Network]

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

// TODO: tests for Broadcaster (if necessary)

/// Class responsible for processing and broadcasting blocks and transactions to the network.
class Broadcaster {
  private:
    Blockchain& blockchain_; ///< Reference to the blockchain.

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
    void doValidatorTx() const;

  public:
    /**
     * Constructor.
     * @param blockchain Reference to the blockchain.
     */
    explicit Broadcaster(Blockchain& blockchain) : blockchain_(blockchain) {}

    void validatorLoop(); ///< Routine loop for when the node is a Validator.
    void nonValidatorLoop() const;  ///< Routine loop for when the node is NOT a Validator.
};

#endif  // BROADCASTER_H
