#ifndef BLOCK_H
#define BLOCK_H

#include <array>
#include <cstdint>
#include <vector>
#include <thread>

#include "../contract/contract.h"
#include "../utils/hash.h"
#include "../utils/merkle.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"

/**
 * Abstraction of a block.
 * Does NOT check transaction logic or signatures, it's only the block's
 * structure/data and some functions to manage it.
 * Block structure is as follows:
 *
 * 65 BYTES - VALIDATOR SIGNATURE
 * HEADER:
 *   32 BYTES - PREV BLOCK HASH
 *   32 BYTES - BLOCK RANDOMNESS
 *   32 BYTES - VALIDATOR MERKLE ROOT
 *   32 BYTES - TRANSACTION MERKLE ROOT
 *   8 BYTES  - TIMESTAMP
 *   8 BYTES  - NHEIGHT
 * CONTENT:
 *   8 BYTES  - TX VALIDATOR COUNT
 *   8 BYTES  - TX COUNT
 *   8 BYTES  - TX VALIDATOR ARRAY START
 *   8 BYTES  - TX ARRAY START
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
    /// Signature of the Validator that created the block.
    Signature validatorSig = Signature();

    /// Previous block hash.
    Hash prevBlockHash;

    /// Randomness seed used for seeding and creating the next block.
    Hash randomness;

    /// Merkle root for Validator transactions.
    Hash validatorTxMerkleRoot;

    /// Merkle root for block transactions.
    Hash txMerkleRoot;

    /// Epoch timestamp of the block (time since 1970-01-01), in nanoseconds.
    uint64_t timestamp = 0;

    /// Height of the block.
    uint64_t nHeight = 0;

    /// Number of Validator transactions.
    uint64_t validatorTxCount = 0;

    /// Number of block transactions.
    uint64_t txCount = 0;

    /// List of Validator transactions.
    std::unordered_map<uint64_t, const Tx, SafeHash> validatorTxs;

    /// List of block transactions.
    std::unordered_map<uint64_t, const Tx, SafeHash> txs;

    /// Indicates whether the block is finalized or not.
    bool finalized = false;

  public:
    /**
     * Constructor from network/RPC.
     * @param rawData The raw block data to parse.
     * @param fromDB If `true`, skips Secp256k1 signature checking.
     *               This brings 40x more performance at the cost of storing
     *               25 extra bytes per transaction.
     *               See %Tx's constructor for more details.
     */
    Block(std::string_view& rawData, bool fromDB);

    /**
     * Constructor from creation.
     * @param prevBlockHash The previous block hash.
     * @param timestamp The epoch timestamp of the block.
     * @param nHeight The height of the block.
     */
    Block(const Hash& prevBlockHash, const uint64_t& timestamp, const uint64_t& nHeight);

    /// Copy constructor.
    Block(const Block& other) {
      this->validatorSig = other.validatorSig;
      this->prevBlockHash = other.prevBlockHash;
      this->randomness = other.randomness;
      this->validatorTxMerkleRoot = other.validatorTxMerkleRoot;
      this->txMerkleRoot = other.txMerkleRoot;
      this->timestamp = other.timestamp;
      this->nHeight = other.nHeight;
      this->validatorTxCount = other.validatorTxCount;
      this->txCount = other.txCount;
      this->validatorTxs = other.validatorTxs;
      this->txs = other.txs;
      this->finalized = other.finalized;
    }

    /// Move constructor.
    Block(Block&& other) noexcept :
      validatorSig(std::move(other.validatorSig)),
      prevBlockHash(std::move(other.prevBlockHash)),
      randomness(std::move(other.randomness)),
      validatorTxMerkleRoot(std::move(other.validatorTxMerkleRoot)),
      txMerkleRoot(std::move(other.txMerkleRoot)),
      timestamp(std::move(other.timestamp)),
      nHeight(std::move(other.nHeight)),
      validatorTxCount(std::move(other.validatorTxCount)),
      txCount(std::move(other.txCount)),
      validatorTxs(std::move(other.validatorTxs)),
      txs(std::move(other.txs)),
      finalized(std::move(other.finalized))
    {}

    /// Getter for `validatorSig`.
    const Signature& getValidatorSig() { return this->validatorSig; }

    /// Getter for `prevBlockHash`.
    const Hash& getPrevBlockHash() { return this->prevBlockHash; }

    /// Getter for `randomness`.
    const Hash& getRandomness() { return this->randomness; }

    /// Getter for `validatorTxMerkleRoot`.
    const Hash& getValidatorTxMerkleRoot() { return this->validatorTxMerkleRoot; }

    /// Getter for `txMerkleRoot`.
    const Hash& getTxMerkleRoot() { return this->txMerkleRoot; }

    /// Getter for `timestamp`.
    const uint64_t& getTimestamp() { return this->timestamp; }

    /// Getter for `nHeight`.
    const uint64_t& getNHeight() { return this->nHeight; }

    /// Getter for `validatorTxCount`.
    const uint64_t& getValidatorTxCount() { return this->validatorTxCount; }

    /// Getter for `txCount`.
    const uint64_t& getTxCount() { return this->txCount; }

    /// Getter for `validatorTxs`.
    const std::unordered_map<uint64_t, const Tx, SafeHash>& getValidatorTxs() { return this->validatorTxs; }

    /// Getter for `txs`.
    const std::unordered_map<uint64_t, const Tx, SafeHash>& getTxs() { return this->txs; }

    /// Same as `getTimestamp()`, but calculates the timestamp in seconds.
    const uint64_t timestampInSeconds() { return this->_timestamp / 1000000000; }

    /// Calculate and return the raw block size, in hex bytes.
    const uint64_t blockSize();

    /**
     * Serialize the raw block data to a hex string.
     * That would be `prevBlockHash + timestamp + nHeight + txCount + [ txSize, tx, ... ]`.
     * @param fromDB Same as constructor's.
     * @return The serialized raw block data as a hex string.
     */
    const std::string serializeToBytes(bool fromDB);

    /**
     * Serialize the block header to a hex string.
     * That would be `prevBlockHash + blockRandomness + validatorMerkleRoot + transactionMerkleRoot + timestamp + nHeight`.
     * @return The serialized block header as a hex string.
     */
    const std::string serializeHeader();

    /**
     * Calculate the SHA3 hash of the entire block.
     * @return The block hash.
     */
    inline const Hash getBlockHash() { return Utils::sha3(this->serializeHeader()); }

    /**
     * Add a transaction to the block.
     * Transaction logic validity is not checked.
     * @param tx The transaction to append.
     * @return `true` if transaction was included in the block, or
     *         `false` if the block is finalized.
     */
    bool appendTx(const Tx& tx);

    /**
     * Add a Validator transaction to the block.
     * Transaction logic validity is not checked.
     * @param tx The transaction to append.
     * @return `true` if transaction was included in the block, or
     *         `false` if the block is finalized.
     */
    bool appendValidatorTx(const Tx& tx);

    /**
     * Finalize the block.
     * This means the block is "closed" so no other transactions
     * can be included in it anymore.
     * @param validatorKey The private key of the Validator, used to sign the block.
     * @return `true` if the block is successfully finalized, or
     *         `false` if the block is finalized.
     */
    bool finalize(const PrivKey& validatorKey);

    /// Equality operator. Checks the block hash of both objects.
    const bool operator==(const Block& rBlock) {
      return this->getBlockHash() == rBlock.getBlockHash();
    }

    /// Inequality operator. Checks the block hash of both objects.
    const bool operator!=(const Block& rBlock) {
      return this->getBlockHash() != rBlock.getBlockHash();
    }

    /// Copy assignment operator.
    Block& operator=(const Block& other) {
      this->validatorSig = other.validatorSig;
      this->prevBlockHash = other.prevBlockHash;
      this->randomness = other.randomness;
      this->validatorTxMerkleRoot = other.validatorTxMerkleRoot;
      this->txMerkleRoot = other.txMerkleRoot;
      this->timestamp = other.timestamp;
      this->nHeight = other.nHeight;
      this->validatorTxCount = other.validatorTxCount;
      this->txCount = other.txCount;
      this->validatorTxs = other.validatorTxs;
      this->txs = other.txs;
      this->finalized = other.finalized;
      return *this;
    }

    /// Move assignment operator.
    Block& operator=(Block&& other) {
      this->validatorSig = std::move(other.validatorSig);
      this->prevBlockHash = std::move(other.prevBlockHash);
      this->randomness = std::move(other.randomness);
      this->validatorTxMerkleRoot = std::move(other.validatorTxMerkleRoot);
      this->txMerkleRoot = std::move(other.txMerkleRoot);
      this->timestamp = std::move(other.timestamp);
      this->nHeight = std::move(other.nHeight);
      this->validatorTxCount = std::move(other.validatorTxCount);
      this->txCount = std::move(other.txCount);
      this->validatorTxs = std::move(other.validatorTxs);
      this->txs = std::move(other.txs);
      this->finalized = std::move(other.finalized);
      return *this;
    }

    /// Copy assignment operator.
    Block& operator=(const std::shared_ptr<const Block>& other) {
      this->validatorSig = other->validatorSig;
      this->prevBlockHash = other->prevBlockHash;
      this->randomness = other->randomness;
      this->validatorTxMerkleRoot = other->validatorTxMerkleRoot;
      this->txMerkleRoot = other->txMerkleRoot;
      this->timestamp = other->timestamp;
      this->nHeight = other->nHeight;
      this->validatorTxCount = other->validatorTxCount;
      this->txCount = other->txCount;
      this->validatorTxs = other->validatorTxs;
      this->txs = other->txs;
      this->finalized = other->finalized;
      return *this;
    }
};

#endif  // BLOCK_H
