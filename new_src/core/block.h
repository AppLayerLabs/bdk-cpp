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

class Block {
  private:
    Signature validatorSig = Signature();
    Hash prevBlockHash;
    Hash randomness;
    Hash validatorTxMerkleRoot;
    Hash txMerkleRoot;
    uint64_t timestamp = 0;
    uint64_t nHeight = 0;
    uint64_t validatorTxCount = 0;
    uint64_t txCount = 0;
    std::unordered_map<uint64_t, Tx, SafeHash> validatorTxs;
    std::unordered_map<uint64_t, Tx, SafeHash> txs;
    bool finalized = false;
    bool indexed = false;
  public:
    Block(std::string_view& rawData, bool fromDB);
    Block(const Hash& prevBlockHash, const uint64_t& timestamp, const uint64_t& nHeight);
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
      this->indexed = other.indexed;
    }
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
      finalized(std::move(other.finalized)),
      indexed(std::move(other.indexed))
    {}

    const Signature& getValidatorSig() { return this->validatorSig; }
    const Hash& getPrevBlockHash() { return this->prevBlockHash; }
    const Hash& getRandomness() { return this->randomness; }
    const Hash& getValidatorTxMerkleRoot() { return this->validatorTxMerkleRoot; }
    const Hash& getTxMerkleRoot() { return this->txMerkleRoot; }
    const uint64_t& getTimestamp() { return this->timestamp; }
    const uint64_t& getNHeight() { return this->nHeight; }
    const uint64_t& getValidatorTxCount() { return this->validatorTxCount; }
    const uint64_t& getTxCount() { return this->txCount; }
    const std::unordered_map<uint64_t, Tx, SafeHash>& getValidatorTxs() { return this->validatorTxs; }
    const std::unordered_map<uint64_t, Tx, SafeHash>& getTxs() { return this->txs; }

    const uint64_t timestampInSeconds() { return this->_timestamp / 1000000000; }
    const uint64_t blockSize();
    const std::string serializeToBytes(bool fromDB);
    const std::string serializeHeader();
    const Hash hashHeader();
    void indexTxs();
    bool appendTx(const Tx& tx);
    bool appendValidatorTx(const Tx& tx);
    bool finalize(const PrivKey& validatorKey);

    const bool operator==(const Block& rBlock) {
      return this->getBlockHash() == rBlock.getBlockHash();
    }
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
      this->indexed = other.indexed;
      return *this;
    }
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
      this->indexed = std::move(other.indexed);
      return *this;
    }
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
      this->indexed = other->indexed;
      return *this;
    }
};

#endif  // BLOCK_H
