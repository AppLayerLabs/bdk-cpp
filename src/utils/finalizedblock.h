/*
Copyright (c) [2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef FINALIZEDBLOCK_H
#define FINALIZEDBLOCK_H

#include <future>
#include <thread>

#include "utils.h"
#include "tx.h"
#include "strings.h"
#include "merkle.h"
#include "ecdsa.h"

/**
 * Abstraction of a finalized block. Generated directly from a MutableBlock.
 * Members are const in purpose due to the immutable nature of the structure.
 */
class FinalizedBlock {
  private:
    const Signature validatorSig_;                ///< Validator signature for the block.
    const UPubKey validatorPubKey_;               ///< Public key of the Validator that signed the block.
    const Hash prevBlockHash_;                    ///< Hash of the previous block.
    const Hash blockRandomness_;                  ///< Current block randomness based on rdPoS.
    const Hash validatorMerkleRoot_;              ///< Merkle root for Validator transactions.
    const Hash txMerkleRoot_;                     ///< Merkle root for block transactions.
    const uint64_t timestamp_;                    ///< Epoch timestamp of the block, in microseconds.
    const uint64_t nHeight_;                      ///< Height of the block in chain.
    const std::vector<TxValidator> txValidators_; ///< List of Validator transactions.
    const std::vector<TxBlock> txs_;              ///< List of block transactions.
    const Hash hash_;                             ///< Cached hash of the block.

  public:
    /**
     * Constructor.
     * @param validatorSig Validator signature for the block.
     * @param validatorPubKey Public key of the Validator that signed the block.
     * @param prevBlockHash Hash of the previous block.
     * @param blockRandomness Current block randomness based on rdPoS.
     * @param validatorMerkleRoot Merkle root for the Validator transactions.
     * @param txMerkleRoot Merkle root for the block transactions.
     * @param timestamp Epoch timestamp of the block, in microseconds.
     * @param nHeight Height of the block in chain.
     * @param txValidators Lost of Validator transactions.
     * @param txs List of block transactions.
     * @param hash Cached hash of the block.
     */
    FinalizedBlock(
        Signature&& validatorSig,
        UPubKey&& validatorPubKey,
        Hash&& prevBlockHash,
        Hash&& blockRandomness,
        Hash&& validatorMerkleRoot,
        Hash&& txMerkleRoot,
        uint64_t timestamp, // Primitive types like uint64_t can be passed by value
        uint64_t nHeight, // Same for nHeight
        std::vector<TxValidator>&& txValidators,
        std::vector<TxBlock>&& txs,
        Hash&& hash
    ) : validatorSig_(std::move(validatorSig)), validatorPubKey_(std::move(validatorPubKey)),
        prevBlockHash_(std::move(prevBlockHash)), blockRandomness_(std::move(blockRandomness)),
        validatorMerkleRoot_(std::move(validatorMerkleRoot)), txMerkleRoot_(std::move(txMerkleRoot)),
        timestamp_(timestamp), nHeight_(nHeight),
        txValidators_(std::move(txValidators)), txs_(std::move(txs)), hash_(std::move(hash))
    {}

    ///@{
    /** Getter. */
    const Signature& getValidatorSig() const { return this->validatorSig_; }
    const UPubKey& getValidatorPubKey() const { return this->validatorPubKey_; }
    const Hash& getPrevBlockHash() const { return this->prevBlockHash_; }
    const Hash& getBlockRandomness() const { return this->blockRandomness_; }
    const Hash& getValidatorMerkleRoot() const { return this->validatorMerkleRoot_; }
    const Hash& getTxMerkleRoot() const { return this->txMerkleRoot_; }
    const uint64_t& getTimestamp() const { return this->timestamp_; }
    const uint64_t& getNHeight() const { return this->nHeight_; }
    const std::vector<TxValidator>& getTxValidators() const { return this->txValidators_; }
    const std::vector<TxBlock>& getTxs() const { return this->txs_; }
    const Hash& getHash() const { return this->hash_; }
    ///@}

    /// Equality operator. Checks the block hash AND signature of both blocks.
    bool operator==(const FinalizedBlock& b) const {
      return ((this->getHash() == b.getHash()) && (this->getValidatorSig() == b.getValidatorSig()));
    }
};

#endif  // FINALIZEDBLOCK_H
