/*
Copyright (c) [2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef MUTABLEBLOCK_H
#define MUTABLEBLOCK_H

#include <future>
#include <thread>

#include "finalizedblock.h"
#include "utils.h"
#include "tx.h"
#include "strings.h"
#include "merkle.h"
#include "ecdsa.h"

/**
 * Abstraction of a non-finalized block. Used for generating a FinalizedBlock.
 * Members are non-const in purpose due to the mutable nature of the structure.
 */
class MutableBlock {
  private:
    Hash prevBlockHash_;                    ///< Hash of the previous block.
    Hash blockRandomness_;                  ///< Current block randomness based on rdPoS.
    uint64_t timestamp_ = 0;                ///< Epoch timestamp of the block, in microsseconds.
    uint64_t nHeight_ = 0;                  ///< Height of the block in chain.
    std::vector<TxBlock> txs_;              ///< List of block transactions.
    std::vector<TxValidator> txValidators_; ///< List of Validator transactions.
    bool isDeserialized_ = false;          ///< Flag to prevent new transactions from being added after deserialization.
    Hash hash_;                             ///< Hash of the block.

    /**
     * Helper method for deserializing a raw byte string into block data.
     * @param bytes The raw byte string to deserialize.
     * @param requiredChainId The chain ID that the block belongs to.
     */
    void deserialize(const BytesArrView bytes, const uint64_t& requiredChainId);

  public:
    /**
     * Constructor.
     * @param bytes The raw byte string to deserialize.
     * @param requiredChainId The chain ID that the block belongs to.
     */
    MutableBlock(const BytesArrView bytes, const uint64_t& requiredChainId);

    /**
     * Constructor from creation.
     * @param prevBlockHash_ The previous block hash.
     * @param timestamp_ The epoch timestamp_ of the block.
     * @param nHeight_ The height of the block.
     */
    MutableBlock(const Hash& prevBlockHash_, const uint64_t& timestamp_, const uint64_t& nHeight_)
      : prevBlockHash_(prevBlockHash_), timestamp_(timestamp_), nHeight_(nHeight_) {}

    /// Copy constructor.
    MutableBlock(const MutableBlock& block) :
      prevBlockHash_(block.prevBlockHash_), blockRandomness_(block.blockRandomness_),
      timestamp_(block.timestamp_), nHeight_(block.nHeight_),
      txValidators_(block.txValidators_), txs_(block.txs_)
    {}

    /// Move constructor.
    MutableBlock(MutableBlock&& block) :
      prevBlockHash_(std::move(block.prevBlockHash_)), blockRandomness_(std::move(block.blockRandomness_)),
      timestamp_(std::move(block.timestamp_)), nHeight_(std::move(block.nHeight_)),
      txValidators_(std::move(block.txValidators_)), txs_(std::move(block.txs_))
    {}

    /**
     * Add a transaction to the block.
     * @param tx The transaction to add.
     * @return `true` if the transaction was added succesfully, `false` otherwise.
     */
    bool appendTx(const TxBlock& tx);

    /**
     * Add a Validator transaction to the block.
     * @param tx The transaction to add.
     * @return `true` if the transaction was added succesfully, `false` otherwise.
     */
    bool appendTxValidator(const TxValidator& tx);

    /**
    * Serialize the mutable header of the block.
    * @param validatorMerkleRoot The merkle root of the Validator transactions.
    * @param txMerkleRoot The merkle root of the block transactions.
    * @return The serialized mutable header.
    */
    Bytes serializeMutableHeader(Hash validatorMerkleRoot, Hash txMerkleRoot) const;

    /**
     * Finalize the block, preventing any further modifications.
     * @param validatorPrivKey The private key of the Validator that will sign the block.
     * @param newTimestamp The new timestamp for the block.
     * @param bytes The raw byte string to finalize.
     * @return A finalized and signed instance of the block.
     */
    FinalizedBlock finalize(const PrivKey& validatorPrivKey, const uint64_t& newTimestamp);

    ///@{
    /** Getter. */
    Hash& getPrevBlockHash() { return this->prevBlockHash_; }
    const Hash& getBlockRandomness() const { return this->blockRandomness_; }
    const uint64_t& getTimestamp() const { return this->timestamp_; }
    const uint64_t& getNHeight() const { return this->nHeight_; }
    std::vector<TxValidator>& getTxValidators() { return this->txValidators_; }
    std::vector<TxBlock>& getTxs() { return this->txs_; }
    ///@}

    /// Copy assignment operator.
    MutableBlock& operator=(const MutableBlock& other) {
      this->prevBlockHash_ = other.prevBlockHash_;
      this->blockRandomness_ = other.blockRandomness_;
      this->timestamp_ = other.timestamp_;
      this->nHeight_ = other.nHeight_;
      this->txValidators_ = other.txValidators_;
      this->txs_ = other.txs_;
      return *this;
    }

    /// Move assignment operator.
    MutableBlock& operator=(MutableBlock&& other) {
      this->prevBlockHash_ = std::move(other.prevBlockHash_);
      this->blockRandomness_ = std::move(other.blockRandomness_);
      this->timestamp_ = std::move(other.timestamp_);
      this->nHeight_ = std::move(other.nHeight_);
      this->txValidators_ = std::move(other.txValidators_);
      this->txs_ = std::move(other.txs_);
      return *this;
    }
};

#endif // MUTABLEBLOCK_H
