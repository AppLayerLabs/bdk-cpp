#ifndef BLOCK_H
#define BLOCK_H

#include <array>
#include <cstdint>
#include <vector>
#include <thread>

#include "../utils/transaction.h"
#include "../utils/utils.h"
#include "../utils/merkle.h"
#include "../utils/secp256k1Wrapper.h"
#include "../contract/contract.h"


/**
 * The Block class only contains the structure of blocks and some utilities
 * to deal with them. It does **NOT** check transaction logic/signature.
 * It is meant to be a fast way to parse from/to the network and disk.
 *
 * BLOCK STRUCTURE:
 * 65 BYTES - VALIDATOR SIGNATURE
 * HEADER:
 *  32 BYTES - PREV BLOCK HASH
 *  32 BYTES - BLOCK RANDOMNESS
 *  32 BYTES - VALIDATOR MERKLE ROOT
 *  32 BYTES - TRANSACTION MERKLE ROOT
 *  8 BYTES  - TIMESTAMP
 *  8 BYTES  - NHEIGHT
 * CONTENT:
 *  8 BYTES  - TX VALIDATOR COUNT
 *  8 BYTES  - TX COUNT
 *  8 BYTES  - TX VALIDATOR ARRAY START
 *  8 BYTES  - TX ARRAY START
 * [
 *   4 BYTES - TX SIZE
 *   X BYTES - TX
 *   ,
 *   4 BYTES - TX SIZE
 *   X BYTES - TX
 *   ,
 *   ...
 * ]
 * [
 *   4 BYTES - TX SIZE
 *   X BYTES - TX
 *   ,
 *   4 BYTES - TX SIZE
 *   X BYTES - TX
 *   ,
 *   ...
 * ]
 *
 * rawBlock: 5c37d504e9415c3b75afaa3ad24484382274bba31f10dcd268e554785d5b807500000181810EB6507a8b54dfbfe9f21d00000001000000aff8ad81be850c92a69c0082e18c94d586e7f844cea2f87f50152665bcbc2c279d8d7080b844a9059cbb00000000000000000000000026548521f99d0709f615aa0f766a7df60f99250b00000000000000000000000000000000000000000000002086ac351052600000830150f7a07e16328b7f3823abeb13d0cab11cdffaf967c9b2eaf3757c42606d6f2ecd8ce6a040684c94b289cdda22822e5cb374ea374d7a3ba581a9014faf35b19e5345ab92
 * Signature is purposely left out of the block structure, the message hash is the block hash, blocks transmitted in the network should always include the signature.
 */

// Genesis Address: 0xbadbad86d54e2a2617d05010ade59b6a960d6f12
// Genesis Private Key: 0x90565a523eb7b46465f95334cd86b93ad9b4637c9c44c029ad8adf2d85486f9c
// TODO: include merkle root of transactions in block.
// TODO: separate the block header from the block transactions. this allows for the block hash to be the block header itself and not the entire block.
class Block {
  private:
    // TODO: Add creator Header (might require writing a secp256k1 wrapper)
    Signature _validatorSignature = Signature();
    Hash _prevBlockHash;
    Hash _randomness;
    Hash _validatorMerkleRoot;
    Hash _transactionMerkleRoot;
    uint64_t _timestamp = 0; // Timestamp in nanoseconds
    uint64_t _nHeight = 0;
    uint64_t _txCount = 0;
    uint64_t _txValidatorsCount = 0;
    // The reason to have it as unordered_map is to be able to parse transactions asynchronously and index them without having to sync all into a vector.
    // TODO: is it worth it? maybe just use a vector and parse them in order.
    std::unordered_map<uint64_t, Tx::Base, SafeHash> _validatorTransactions; // Tx Index > tx.
    std::unordered_map<uint64_t, Tx::Base, SafeHash> _transactions; // Tx Index > tx.
    bool finalized = false;
    bool indexed = false;

  public:
    // Constructor.
    // bool flag takes advantage of not running secp256k1 for 40x performance
    // tradeoff: having to store extra 25 bytes per tx
    explicit Block(const std::string_view &blockData, bool fromDB);

    // Constructor from creation.
    Block(
      const Hash &_prevBlockHashC, const uint64_t &_timestampC, const uint64_t &_nHeightC
    ) : _prevBlockHash(_prevBlockHashC), _timestamp(_timestampC), _nHeight(_nHeightC), _txCount(0), _txValidatorsCount(0) {}

    // Copy constructor.
    Block(const Block& other) {
      this->_validatorSignature = other._validatorSignature;
      this->_prevBlockHash = other._prevBlockHash;
      this->_txCount = other._txCount;
      this->_validatorSignature = other._validatorSignature;
      this->_txValidatorsCount = other._txValidatorsCount;
      this->_transactions = other._transactions;
      this->finalized = other.finalized;
      this->indexed = other.indexed;
    }

    // Move constructor.
    Block(Block&& other) noexcept :
      _validatorSignature(std::move(other._validatorSignature)),
      _prevBlockHash(std::move(other._prevBlockHash)),
      _randomness(std::move(other._randomness)),
      _validatorMerkleRoot(std::move(other._validatorMerkleRoot)),
      _transactionMerkleRoot(std::move(other._transactionMerkleRoot)),
      _timestamp(std::move(other._timestamp)),
      _nHeight(std::move(other._nHeight)),
      _txCount(std::move(other._txCount)),
      _txValidatorsCount(std::move(other._txValidatorsCount)),
      _transactions(std::move(other._transactions)),
      finalized(std::move(other.finalized)),
      indexed(std::move(other.indexed))
    {}

    // Getters.
    const Hash prevBlockHash() const { return this->_prevBlockHash; };
    const Hash randomness() const { return this->_randomness; };
    const Hash validatorMerkleRoot() const { return this->_validatorMerkleRoot; };
    const Hash transactionMerkleRoot() const { return this->_transactionMerkleRoot; };
    const uint64_t& timestamp() const { return this->_timestamp; };
    const uint64_t timestampInSeconds() const { return this->_timestamp / 1000000000; };
    const uint64_t& nHeight() const { return this->_nHeight; };
    const Signature signature() const { return this->_validatorSignature; };
    const uint64_t& txCount() const { return this->_txCount; };
    const uint64_t& txValidatorsCount() const { return this->_txValidatorsCount; };
    const std::unordered_map<uint64_t, Tx::Base, SafeHash>& transactions() const { return this->_transactions; };
    const std::unordered_map<uint64_t, Tx::Base, SafeHash>& validatorTransactions() const { return this->_validatorTransactions; };
    uint64_t blockSize() const;
    Hash getBlockHash() const; // Hash (in bytes)
    std::string serializeToBytes(bool db) const; // Tells tx's to be serialized using secp256k1 for extra calculation or not on deserialization.
    std::string serializeHeader() const;
    void indexTxs();  // When transactions are indexed, the block is considered to be on chain
    bool appendTx(const Tx::Base &tx); // Transaction logic validity is not checked during appending
    bool appendValidatorTx(const Tx::Base &tx); // Neither validators Tx's.
    // TODO: Only finalize after all validator txs are appended.
    bool finalizeBlock();
    void signBlock(const PrivKey &privateKey);

    // Equality operator.
    bool operator==(const Block& rBlock) const {
      return bool(
        this->getBlockHash() == rBlock.getBlockHash()
      );
    }

    // Copy assignment operator.
    Block& operator=(const Block& block) {
      this->_prevBlockHash = block._prevBlockHash;
      this->_timestamp = block._timestamp;
      this->_nHeight = block._nHeight;
      this->_txCount = block._txCount;
      this->_transactions = block._transactions;
      this->finalized = block.finalized;
      this->indexed = block.indexed;
      return *this;
    }

    // Move assignment operator.
    Block& operator=(Block&& other) {
      this->_prevBlockHash = std::move(other._prevBlockHash);
      this->_timestamp = std::move(other._timestamp);
      this->_nHeight = std::move(other._nHeight);
      this->_txCount = std::move(other._txCount);
      this->_transactions = std::move(other._transactions);
      this->finalized = std::move(other.finalized);
      this->indexed = std::move(other.indexed);
      return *this;
    }

    // It is safe to use reference here because the constructor does not create another thread using the data.
    // Copy assignment operator.
    Block& operator=(const std::shared_ptr<const Block>& block) {
      this->_prevBlockHash = block->_prevBlockHash;
      this->_timestamp = block->_timestamp;
      this->_nHeight = block->_nHeight;
      this->_txCount = block->_txCount;
      this->_transactions = block->_transactions;
      this->finalized = block->finalized;
      this->indexed = block->indexed;
      return *this;
    }

    // Move assignment operator.
    Block& operator=(std::shared_ptr<const Block>&& other) {
      this->_prevBlockHash = std::move(other->_prevBlockHash);
      this->_timestamp = std::move(other->_timestamp);
      this->_nHeight = std::move(other->_nHeight);
      this->_txCount = std::move(other->_txCount);
      this->_transactions = std::move(other->_transactions);
      this->finalized = std::move(other->finalized);
      this->indexed = std::move(other->indexed);
      return *this;
    }
};

#endif  // BLOCK_H
