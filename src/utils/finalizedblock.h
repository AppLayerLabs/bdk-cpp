/*
Copyright (c) [2024] [AppLayer Developers]

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

#include "../bytes/join.h"

/// Abstraction of a finalized block. Members are purposefully const due to the immutable nature of the structure.
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
    const size_t size_;                           ///< Total size of the block, in bytes.

    /**
     * Serialize the block header (144 bytes = previous block hash + block randomness
     * + validator merkle root + tx merkle root + timestamp + block height).
     * @return The serialized header string.
     */
    Bytes serializeHeader() const;

  public:
    /**
     * Move Constructor.
     * Only the move constructor is declared, simply because there is no reason
     * within BDKD to copy the arguments when creating a new block.
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
     * @param size Total size of the block, in bytes.
     */
    FinalizedBlock(
      Signature&& validatorSig,
      UPubKey&& validatorPubKey,
      Hash&& prevBlockHash,
      Hash&& blockRandomness,
      Hash&& validatorMerkleRoot,
      Hash&& txMerkleRoot,
      uint64_t timestamp, // Primitive types like uint64_t can (and should) be passed by value, no &&
      uint64_t nHeight, // Same for nHeight
      std::vector<TxValidator>&& txValidators,
      std::vector<TxBlock>&& txs,
      Hash&& hash,
      size_t size
    ) : validatorSig_(std::move(validatorSig)), validatorPubKey_(std::move(validatorPubKey)),
      prevBlockHash_(std::move(prevBlockHash)), blockRandomness_(std::move(blockRandomness)),
      validatorMerkleRoot_(std::move(validatorMerkleRoot)), txMerkleRoot_(std::move(txMerkleRoot)),
      timestamp_(timestamp), nHeight_(nHeight),
      txValidators_(std::move(txValidators)), txs_(std::move(txs)), hash_(std::move(hash)), size_(size)
    { LOGXTRACE("Finalized block moved"); }

    /**
     * Move constructor.
     * @param block The FinalizedBlock to move.
     */
    FinalizedBlock(FinalizedBlock&& block) :
      validatorSig_(std::move(block.validatorSig_)),
      validatorPubKey_(std::move(block.validatorPubKey_)), // not "noexcept" because this can throw (FixedBytes)
      prevBlockHash_(std::move(block.prevBlockHash_)),
      blockRandomness_(std::move(block.blockRandomness_)),
      validatorMerkleRoot_(std::move(block.validatorMerkleRoot_)),
      txMerkleRoot_(std::move(block.txMerkleRoot_)),
      timestamp_(block.timestamp_),
      nHeight_(block.nHeight_),
      txValidators_(std::move(block.txValidators_)),
      txs_(std::move(block.txs_)),
      hash_(std::move(block.hash_)),
      size_(block.size_)
    {
      LOGXTRACE("Finalized block moved");
    }

    /**
     * Copy constructor.
     * @param block The FinalizedBlock to copy.
     */
    FinalizedBlock(const FinalizedBlock& block) :
      validatorSig_(block.validatorSig_),
      validatorPubKey_(block.validatorPubKey_),
      prevBlockHash_(block.prevBlockHash_),
      blockRandomness_(block.blockRandomness_),
      validatorMerkleRoot_(block.validatorMerkleRoot_),
      txMerkleRoot_(block.txMerkleRoot_),
      timestamp_(block.timestamp_),
      nHeight_(block.nHeight_),
      txValidators_(block.txValidators_),
      txs_(block.txs_),
      hash_(block.hash_),
      size_(block.size_)
    {
      LOGXTRACE("Finalized block copied");
    }

    /**
     * Deserialize a given raw bytes string and turn it into a FinalizedBlock.
     * @param bytes The raw bytes string to de-serialize.
     * @param requiredChainId The chain ID to which the block belongs.
     * @return A FinalizedBlock instance.
     * @throw std::domain_error if deserialization fails for some reason.
     */
    static FinalizedBlock fromBytes(const View<Bytes> bytes, const uint64_t& requiredChainId);

    /**
     * Serialize the entire block (including the header) to a raw bytes string.
     * @return The serialized block.
     */
    Bytes serializeBlock() const;

    /*
     * Create a new valid block given the arguments.
     * `std::move()` MUST be used when passing the txs and txValidators vectors.
     * The function will automatically derive the block randomness, validator merkle root,
     * tx merkle root and block hash, besides also signing the block with the provided private key.
     * @param txs List of block transactions.
     * @param txValidators List of Validator transactions.
     * @param prevBlockHash Hash of the previous block.
     * @param timestamp Epoch timestamp of the block, in microseconds.
     * @param nHeight Height of the block in chain.
     * @param validatorPrivKey Private key of the Validator to sign the block.
     * @return The new valid block.
     */
    static FinalizedBlock createNewValidBlock(
      std::vector<TxBlock>&& txs,
      std::vector<TxValidator>&& txValidators,
      Hash prevBlockHash,
      const uint64_t& timestamp,
      const uint64_t& nHeight,
      const PrivKey& validatorPrivKey
    );

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
    const size_t& getSize() const { return this->size_; }
    ///@}

    /// Equality operator. Checks the block hash AND signature of both blocks.
    bool operator==(const FinalizedBlock& b) const {
      return ((this->getHash() == b.getHash()) && (this->getValidatorSig() == b.getValidatorSig()));
    }
};

#endif  // FINALIZEDBLOCK_H
