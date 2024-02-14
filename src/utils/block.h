/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BLOCK_H
#define BLOCK_H

#include <future>
#include <thread>

#include "utils.h"
#include "tx.h"
#include "strings.h"
#include "merkle.h"
#include "ecdsa.h"

/**
 * Abstraction of a block.
 * Does NOT check transaction logic but does check Tx Signature validty.
 * Logic is handled by State.
 * (Summed up) Structure is as follows:
 *
 * ```
 * OUTSIDE OF BLOCK HEADER:
 *   65 BYTES - VALIDATOR SIGNATURE
 *
 * BLOCK HEADER (144 BYTES):
 *   32 BYTES - PREV BLOCK HASH
 *   32 BYTES - BLOCK RANDOMNESS
 *   32 BYTES - VALIDATOR MERKLE ROOT
 *   32 BYTES - TRANSACTION MERKLE ROOT
 *   8 BYTES  - TIMESTAMP (MICROSECONDS)
 *   8 BYTES  - NHEIGHT
 *
 * BLOCK CONTENT:
 *   8 BYTES  - VALIDATOR TX ARRAY START
 *   [
 *     4 BYTES - BLOCK TX SIZE
 *     N BYTES - BLOCK TX
 *     ...
 *   ]
 *   [
 *     4 BYTES - VALIDATOR TX SIZE
 *     N BYTES - VALIDATOR TX
 *     ...
 *   ]
 * ```
 */
class Block {
  // TODO: Add chainId into the validator signature.
  private:
    Signature validatorSig_;                ///< Validator signature for the block.
    Hash prevBlockHash_;                    ///< Previous block hash.
    Hash blockRandomness_;                  ///< Current block randomness based on rdPoS.
    Hash validatorMerkleRoot_;              ///< Merkle root for Validator transactions.
    Hash txMerkleRoot_;                     ///< Merkle root for block transactions.
    uint64_t timestamp_ = 0;                ///< Epoch timestamp of the block, in microsseconds.
    uint64_t nHeight_ = 0;                  ///< Height of the block in chain.
    std::vector<TxValidator> txValidators_; ///< List of Validator transactions.
    std::vector<TxBlock> txs_;              ///< List of block transactions.
    UPubKey validatorPubKey_;               ///< Validator public key for the block.
    bool finalized_ = false;                ///< Indicates whether the block is finalized or not. See finalize().
    Hash hash_;                             ///< Cached hash of the block.

  public:
    /**
     * Constructor from network/RPC.
     * @param bytes The raw block data string to parse.
     * @param requiredChainId The chain ID that the block and its transactions belong to.
     * @throw DynamicException on any invalid block parameter (size, signature, etc.).
     */
    Block(const BytesArrView bytes, const uint64_t& requiredChainId);

    /**
     * Constructor from creation.
     * @param prevBlockHash_ The previous block hash.
     * @param timestamp_ The epoch timestamp_ of the block.
     * @param nHeight_ The height of the block.
     */
    Block(const Hash& prevBlockHash_, const uint64_t& timestamp_, const uint64_t& nHeight_)
      : prevBlockHash_(prevBlockHash_), timestamp_(timestamp_), nHeight_(nHeight_) {}

    /// Copy constructor.
    Block(const Block& block) :
      validatorSig_(block.validatorSig_),
      prevBlockHash_(block.prevBlockHash_),
      blockRandomness_(block.blockRandomness_),
      validatorMerkleRoot_(block.validatorMerkleRoot_),
      txMerkleRoot_(block.txMerkleRoot_),
      timestamp_(block.timestamp_),
      nHeight_(block.nHeight_),
      txValidators_(block.txValidators_),
      txs_(block.txs_),
      validatorPubKey_(block.validatorPubKey_),
      finalized_(block.finalized_),
      hash_(block.hash_)
    {}

    /// Move constructor.
    Block(Block&& block) :
      validatorSig_(std::move(block.validatorSig_)),
      prevBlockHash_(std::move(block.prevBlockHash_)),
      blockRandomness_(std::move(block.blockRandomness_)),
      validatorMerkleRoot_(std::move(block.validatorMerkleRoot_)),
      txMerkleRoot_(std::move(block.txMerkleRoot_)),
      timestamp_(std::move(block.timestamp_)),
      nHeight_(std::move(block.nHeight_)),
      txValidators_(std::move(block.txValidators_)),
      txs_(std::move(block.txs_)),
      validatorPubKey_(std::move(block.validatorPubKey_)),
      finalized_(std::move(block.finalized_)),
      hash_(std::move(block.hash_))
    { block.finalized_ = false; return; } // Block moved -> invalid block, as members of block were moved

    ///@{
    /** Getter. */
    const Signature& getValidatorSig() const { return this->validatorSig_; }
    const Hash& getPrevBlockHash() const { return this->prevBlockHash_; }
    const Hash& getBlockRandomness() const { return this->blockRandomness_; }
    const Hash& getValidatorMerkleRoot() const { return this->validatorMerkleRoot_; }
    const Hash& getTxMerkleRoot() const { return this->txMerkleRoot_; }
    uint64_t getTimestamp() const { return this->timestamp_; }
    uint64_t getNHeight() const { return this->nHeight_; }
    const std::vector<TxValidator>& getTxValidators() const { return this->txValidators_; }
    const std::vector<TxBlock>& getTxs() const { return this->txs_; }
    const UPubKey& getValidatorPubKey() const { return this->validatorPubKey_; }
    bool isFinalized() const { return this->finalized_; }
    ///@}

    /**
     * Serialize the block header (144 bytes = previous block hash + block randomness
     * + validator merkle root + tx merkle root + timestamp + block height).
     * @return The serialized header string.
     */
    Bytes serializeHeader() const;

    /**
     * Serialize the entire block and its contents (validator signature + block header
     * + validator tx offset + [block txs...] + [validator txs...]).
     * @return The serialized block string.
     */
    Bytes serializeBlock() const;

    /**
     * SHA3-hash the block header (calls serializeHeader() internally).
     * @return The hash of the block header.
     */
    const Hash& hash() const;

    /**
     * Append a transaction to the block.
     * @param tx The transaction to append.
     * @return `true` on success, `false` if block is finalized.
     */
    bool appendTx(const TxBlock& tx);

    /**
     * Append a Validator transaction to the block.
     * @param tx The Validator transaction to append.
     * @return `true` on success, `false` if block is finalized.
     */
    bool appendTxValidator(const TxValidator& tx);

    /**
     * Finalize the block (close it so new transactions won't be accepted,
     * sign and validate it through a Validator, and generate a new randomness
     * seed for the next block).
     * @return `true` on success, `false` if block is already finalized_.
     */
    bool finalize(const PrivKey& validatorPrivKey, const uint64_t& newTimestamp);

    /// Equality operator. Checks the block hash AND signature of both objects.
    bool operator==(const Block& b) const {
      return ((this->hash() == b.hash()) && (this->getValidatorSig() == b.getValidatorSig()));
    }

    /// Copy assignment operator.
    Block& operator=(const Block& other) {
      this->validatorSig_ = other.validatorSig_;
      this->prevBlockHash_ = other.prevBlockHash_;
      this->blockRandomness_ = other.blockRandomness_;
      this->validatorMerkleRoot_ = other.validatorMerkleRoot_;
      this->txMerkleRoot_ = other.txMerkleRoot_;
      this->timestamp_ = other.timestamp_;
      this->nHeight_ = other.nHeight_;
      this->txValidators_ = other.txValidators_;
      this->txs_ = other.txs_;
      this->validatorPubKey_ = other.validatorPubKey_;
      this->finalized_ = other.finalized_;
      this->hash_ = other.hash_;
      return *this;
    }

    /// Move assignment operator.
    Block& operator=(Block&& other) {
      this->validatorSig_ = std::move(other.validatorSig_);
      this->prevBlockHash_ = std::move(other.prevBlockHash_);
      this->blockRandomness_ = std::move(other.blockRandomness_);
      this->validatorMerkleRoot_ = std::move(other.validatorMerkleRoot_);
      this->txMerkleRoot_ = std::move(other.txMerkleRoot_);
      this->timestamp_ = std::move(other.timestamp_);
      this->nHeight_ = std::move(other.nHeight_);
      this->txValidators_ = std::move(other.txValidators_);
      this->txs_ = std::move(other.txs_);
      this->validatorPubKey_ = std::move(other.validatorPubKey_);
      this->finalized_ = std::move(other.finalized_);
      this->hash_ = std::move(other.hash_);
      return *this;
    }
};

#endif // BLOCK_H
