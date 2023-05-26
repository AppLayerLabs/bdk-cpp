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
 * Does NOT check transaction logic or signatures, it's only the block's
 * structure/data and some functions to manage it.
 *
 * (Summed up) Structure is as follows:
 *
 * ```
 * OUTSIDE OF BLOCK HEADER:
 *   65 BYTES - VALIDATOR SIGNATURE
 *
 * BLOCK HEADER:
 *   32 BYTES - PREV BLOCK HASH
 *   32 BYTES - BLOCK RANDOMNESS
 *   32 BYTES - VALIDATOR MERKLE ROOT
 *   32 BYTES - TRANSACTION MERKLE ROOT
 *   8 BYTES  - TIMESTAMP (MICROSECONDS)
 *   8 BYTES  - NHEIGHT
 *
 * BLOCK CONTENT:
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
 * ```
 */
class Block {
  private:
    /// Validator signature for the block.
    Signature validatorSig;

    /// Previous block hash.
    Hash prevBlockHash;

    /// Current block randomness based on rdPoS.
    Hash blockRandomness;

    /// Merkle root for Validator transactions.
    Hash validatorMerkleRoot;

    /// Merkle root for block transactions.
    Hash txMerkleRoot;

    /// Epoch timestamp of the block, in microsseconds.
    uint64_t timestamp = 0;

    /// Height of the block in chain.
    uint64_t nHeight = 0;

    /// List of Validator transactions.
    std::vector<TxValidator> txValidators;

    /// List of block transactions.
    std::vector<TxBlock> txs;

    /// Validator public key for the block.
    UPubKey validatorPubKey;

    /// Indicates whether the block is finalized or not. See finalize().
    bool finalized = false;

  public:
    /**
     * Constructor from network/RPC.
     * @param bytes The raw block data string to parse.
     * @param requiredChainId The chain ID that the block and its transactions belong to.
     * @throw std::runtime_error on any invalid block parameter (size, signature, etc.).
     */
    Block(std::string_view bytes, const uint64_t& requiredChainId);

    /**
     * Constructor from creation.
     * @param prevBlockHash The previous block hash.
     * @param timestamp The epoch timestamp of the block.
     * @param nHeight The height of the block.
     */
    Block(const Hash& prevBlockHash, const uint64_t& timestamp, const uint64_t& nHeight)
      : prevBlockHash(prevBlockHash), timestamp(timestamp), nHeight(nHeight) {}

    /// Copy constructor.
    Block(const Block& block) :
      validatorSig(block.validatorSig),
      prevBlockHash(block.prevBlockHash),
      blockRandomness(block.blockRandomness),
      validatorMerkleRoot(block.validatorMerkleRoot),
      txMerkleRoot(block.txMerkleRoot),
      timestamp(block.timestamp),
      nHeight(block.nHeight),
      txValidators(block.txValidators),
      txs(block.txs),
      validatorPubKey(block.validatorPubKey),
      finalized(block.finalized)
    {}

    /// Move constructor.
    Block(Block&& block) :
      validatorSig(std::move(block.validatorSig)),
      prevBlockHash(std::move(block.prevBlockHash)),
      blockRandomness(std::move(block.blockRandomness)),
      validatorMerkleRoot(std::move(block.validatorMerkleRoot)),
      txMerkleRoot(std::move(block.txMerkleRoot)),
      timestamp(std::move(block.timestamp)),
      nHeight(std::move(block.nHeight)),
      txValidators(std::move(block.txValidators)),
      txs(std::move(block.txs)),
      validatorPubKey(std::move(block.validatorPubKey)),
      finalized(std::move(block.finalized))
    { block.finalized = false; return; } // Block moved -> invalid block, as members of block were moved

    /// Getter for `validatorSig`.
    const Signature& getValidatorSig() const { return validatorSig; }

    /// Getter for `prevBlockHash`.
    const Hash& getPrevBlockHash() const { return prevBlockHash; }

    /// Getter for `blockRandomness`.
    const Hash& getBlockRandomness() const { return blockRandomness; }

    /// Getter for `validatorMerkleRoot`.
    const Hash& getValidatorMerkleRoot() const { return validatorMerkleRoot; }

    /// Getter for `txMerkleRoot`.
    const Hash& getTxMerkleRoot() const { return txMerkleRoot; }

    /// Getter for `timestamp`.
    uint64_t getTimestamp() const { return timestamp; }

    /// Getter for `nHeight`.
    uint64_t getNHeight() const { return nHeight; }

    /// Getter for `txValidators`.
    const std::vector<TxValidator>& getTxValidators() const { return txValidators; }

    /// Getter for `txs`.
    const std::vector<TxBlock>& getTxs() const { return txs; }

    /// Getter for `validatorPubKey`.
    const UPubKey& getValidatorPubKey() const { return validatorPubKey; }

    /// Getter for `finalized`.
    bool isFinalized() const { return finalized; }

    // ========================
    // Serialization Functions
    // ========================

    /**
     * Serialize the block header (prev block hash + randomness +
     * Validator Merkle Root + Transaction Merkle Root + timestamp + nHeight).
     * @return The serialized header string.
     */
    const std::string serializeHeader() const;

    /**
     * Serialize the entire block and its contents.
     * @return The serialized block string.
     */
    const std::string serializeBlock() const;

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
     * @return `true` on success, `false` if block is finalized.
     */
    bool appendTx(const TxBlock& tx);

    /**
     * Append a Validator transaction to the block.
     * @param tx The transaction to append.
     * @return `true` on success, `false` if block is finalized.
     */
    bool appendTxValidator(const TxValidator& tx);

    /**
     * Finalize the block.
     * This means the block will be "closed" to new transactions,
     * signed and validated by the Validator, and a new randomness seed
     * will be generated for the next block.
     * @return `true` on success, `false` if block is already finalized.
     */
    bool finalize(const PrivKey& validatorPrivKey, const uint64_t& newTimestamp);

    /// Equality operator. Checks the block hash of both objects.
    const bool operator==(const Block& b) const { return this->hash() == b.hash(); }

    /// Inequality operator. Checks the block hash of both objects.
    const bool operator!=(const Block& b) const { return this->hash() != b.hash(); }

    /// Copy assignment operator.
    Block& operator=(const Block& other) {
      this->validatorSig = other.validatorSig;
      this->prevBlockHash = other.prevBlockHash;
      this->blockRandomness = other.blockRandomness;
      this->validatorMerkleRoot = other.validatorMerkleRoot;
      this->txMerkleRoot = other.txMerkleRoot;
      this->timestamp = other.timestamp;
      this->nHeight = other.nHeight;
      this->txValidators = other.txValidators;
      this->txs = other.txs;
      this->validatorPubKey = other.validatorPubKey;
      this->finalized = other.finalized;
      return *this;
    }

    /// Move assignment operator.
    Block& operator=(Block&& other) {
      this->validatorSig = std::move(other.validatorSig);
      this->prevBlockHash = std::move(other.prevBlockHash);
      this->blockRandomness = std::move(other.blockRandomness);
      this->validatorMerkleRoot = std::move(other.validatorMerkleRoot);
      this->txMerkleRoot = std::move(other.txMerkleRoot);
      this->timestamp = std::move(other.timestamp);
      this->nHeight = std::move(other.nHeight);
      this->txValidators = std::move(other.txValidators);
      this->txs = std::move(other.txs);
      this->validatorPubKey = std::move(other.validatorPubKey);
      this->finalized = std::move(other.finalized);
      return *this;
    }
};

#endif // BLOCK_H
