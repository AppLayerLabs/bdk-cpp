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
 *
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
 *
 * TODO: Add chainId into the validator signature.
 */
class Block {
  private:
    /// Validator signature for the block.
    Signature validatorSig_;

    /// Previous block hash.
    Hash prevBlockHash_;

    /// Current block randomness based on rdPoS.
    Hash blockRandomness_;

    /// Merkle root for Validator transactions.
    Hash validatorMerkleRoot_;

    /// Merkle root for block transactions.
    Hash txMerkleRoot_;

    /// Epoch timestamp of the block, in microsseconds.
    uint64_t timestamp_ = 0;

    /// Height of the block in chain.
    uint64_t nHeight_ = 0;

    /// List of Validator transactions.
    std::vector<TxValidator> txValidators_;

    /// List of block transactions.
    std::vector<TxBlock> txs_;

    /// Validator public key for the block.
    UPubKey validatorPubKey_;

    /// Indicates whether the block is finalized or not. See finalize().
    bool finalized_ = false;

  public:
    /**
     * Constructor from network/RPC.
     * @param bytes The raw block data string to parse.
     * @param requiredChainId The chain ID that the block and its transactions belong to.
     * @throw std::runtime_error on any invalid block parameter (size, signature, etc.).
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
      finalized_(block.finalized_)
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
      finalized_(std::move(block.finalized_))
    { block.finalized_ = false; return; } // Block moved -> invalid block, as members of block were moved

    /// Getter for `validatorSig_`.
    const Signature& getValidatorSig() const { return this->validatorSig_; }

    /// Getter for `prevBlockHash_`.
    const Hash& getPrevBlockHash() const { return this->prevBlockHash_; }

    /// Getter for `blockRandomness_`.
    const Hash& getBlockRandomness() const { return this->blockRandomness_; }

    /// Getter for `validatorMerkleRoot_`.
    const Hash& getValidatorMerkleRoot() const { return this->validatorMerkleRoot_; }

    /// Getter for `txMerkleRoot_`.
    const Hash& getTxMerkleRoot() const { return this->txMerkleRoot_; }

    /// Getter for `timestamp_`.
    uint64_t getTimestamp() const { return this->timestamp_; }

    /// Getter for `nHeight_`.
    uint64_t getNHeight() const { return this->nHeight_; }

    /// Getter for `txValidators_`.
    const std::vector<TxValidator>& getTxValidators() const { return this->txValidators_; }

    /// Getter for `txs_`.
    const std::vector<TxBlock>& getTxs() const { return this->txs_; }

    /// Getter for `validatorPubKey_`.
    const UPubKey& getValidatorPubKey() const { return this->validatorPubKey_; }

    /// Getter for `finalized_`.
    bool isFinalized() const { return this->finalized_; }

    // ========================
    // Serialization Functions
    // ========================

    /**
     * Serialize the block header (prev block hash + randomness +
     * Validator Merkle Root + Transaction Merkle Root + timestamp_ + nHeight_).
     * @return The serialized header string.
     */
    const Bytes serializeHeader() const;

    /**
     * Serialize the entire block and its contents.
     * @return The serialized block string.
     */
    const Bytes serializeBlock() const;

    /**
     * SHA3-hash the block header (calls serializeHeader() internally).
     * @return The hash of the block header.
     */
    const Hash hash() const;

    // ==============================
    // Transaction related functions
    // ==============================

    /**
     * Append a block transaction to the block.
     * @param tx The transaction to append.
     * @return `true` on success, `false` if block is finalized_.
     */
    bool appendTx(const TxBlock& tx);

    /**
     * Append a Validator transaction to the block.
     * @param tx The transaction to append.
     * @return `true` on success, `false` if block is finalized_.
     */
    bool appendTxValidator(const TxValidator& tx);

    /**
     * Finalize the block.
     * This means the block will be "closed" to new transactions,
     * signed and validated by the Validator, and a new randomness seed
     * will be generated for the next block.
     * @return `true` on success, `false` if block is already finalized_.
     */
    bool finalize(const PrivKey& validatorPrivKey, const uint64_t& newTimestamp);

    /// Equality operator. Checks the block hash AND signature of both objects.
    const bool operator==(const Block& b) const { return ((this->hash() == b.hash()) && (this->getValidatorSig() == b.getValidatorSig())); }

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
      return *this;
    }
};

#endif // BLOCK_H
