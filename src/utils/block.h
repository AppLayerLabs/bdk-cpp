#ifndef BLOCK_H
#define BLOCK_H

#include "../core/rdpos.h"
#include "utils.h"
#include "tx.h"
#include "strings.h"
#include "merkle.h"
#include "ecdsa.h"

/**
 * Abstraction of a block.
 * Does NOT check transaction logic or signatures, it's only the block's
 * structure/data and some functions to manage it.
 * Block structure is as follows:
 *
 * OUTSIDE OF BLOCK HEADER:
 *  65 BYTES - VALIDATOR SIGNATURE
 * BLOCK HEADER:
 *   32 BYTES - PREV BLOCK HASH
 *   32 BYTES - BLOCK RANDOMNESS
 *   32 BYTES - VALIDATOR MERKLE ROOT
 *   32 BYTES - TRANSACTION MERKLE ROOT
 *   8 BYTES  - TIMESTAMP
 *   8 BYTES  - NHEIGHT
 * CONTENT:
 *   8 BYTES  - TX VALIDATOR ARRAY START
 *   [
 *     4 BYTES - TX SIZE
 *     X BYTES - TX
 *     ,
 *     4 BYTES - TX SIZE
 *     X BYTES - TX
 *     ,
 *     ...
 *   ]
 *   [
 *     4 BYTES - TX SIZE
 *     X BYTES - TX
 *     ,
 *     4 BYTES - TX SIZE
 *     X BYTES - TX
 *     ,
 *     ...
 *   ]
 */

class Block {
  private:

    // Block validator signature
    Signature validatorSig_;

    // Previous block hash
    Hash prevBlockHash_;

    // Current block randomness based on rdPoS
    Hash blockRandomness_;

    // Merkle root of the validator transactions
    Hash validatorMerkleRoot_;

    // Merkle root of the transactions

    Hash txMerkleRoot_;

    // Block timestamp
    uint64_t timestamp_ = 0;

    // Block nHeight in chain
    uint64_t nHeight_ = 0;

    // Block validator transactions
    std::vector<TxValidator> txValidators_;
    
    // Block transactions.
    std::vector<TxBlock> txs_;

    // Block Validator pubkey.
    UPubKey validatorPubKey_;

    bool finalized = false;
  
  public:

    /**
     * Constructor from network/RPC.
     * @param rawData The raw block data to parse.
     */

    Block(std::string_view bytes);

    /**
     * Constructor from creation.
     * @param prevBlockHash The previous block hash.
     * @param timestamp The epoch timestamp of the block.
     * @param nHeight The height of the block.
     */
    Block(const Hash& prevBlockHash, const uint64_t& timestamp, const uint64_t& nHeight)
      : prevBlockHash_(prevBlockHash), timestamp_(timestamp), nHeight_(nHeight) {}


    // Serialization Functions

    // Serialize only the block header (Prev block hash, Block Randomness, Validator Merkle Root, Transaction Merkle Root, Timestamp and nHeight)
    const std::string serializeHeader() const;

    // Serialize the entire block and content
    const std::string serializeBlock() const;

    // Give the block hash (sha3(serializeHeader()).
    const Hash hash() const;

    // Transaction related functions

    bool appendTx(const TxBlock& tx);
    bool appendTxValidator(const TxValidator& tx);

    // Block Finalizator.
    bool finalize(const PrivKey& validatorPrivKey);

    // Equality operator. Checks the block hash of both objects.
    const bool operator==(const Block& rBlock) const {
      return this->hash() == rBlock.hash();
    }

    // Inequality operator. Checks the block hash of both objects.
    const bool operator!=(const Block& rBlock) const {
      return this->hash() != rBlock.hash();
    }

    const Signature& validatorSig() const { return validatorSig_; }
    const Hash& prevBlockHash() const { return prevBlockHash_; }
    const Hash& blockRandomness() const { return blockRandomness_; }
    const Hash& validatorMerkleRoot() const { return validatorMerkleRoot_; }
    const std::vector<TxValidator>& txValidators() const { return txValidators_; }
    const std::vector<TxBlock>& txs() const { return txs_; }
    uint64_t timestamp() const { return timestamp_; }
    uint64_t nHeight() const { return nHeight_; }

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
      this->finalized = other.finalized;
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
      this->finalized = std::move(other.finalized);
      return *this;
    }
};


#endif // BLOCK_H