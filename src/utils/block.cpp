#include "block.h"

Block::Block(std::string_view bytes) {
  try {
    // Split the bytes string
    if (bytes.size() < 217) throw std::runtime_error("Invalid block size");
    this->validatorSig = Signature(bytes.substr(0, 65));
    this->prevBlockHash = Hash(bytes.substr(65, 32));
    this->blockRandomness = Hash(bytes.substr(97, 32));
    this->validatorMerkleRoot = Hash(bytes.substr(129, 32));
    this->txMerkleRoot = Hash(bytes.substr(161, 32));
    this->timestamp = Utils::bytesToUint64(bytes.substr(193, 8));
    this->nHeight = Utils::bytesToUint64(bytes.substr(201, 8));
    uint64_t txValidatorStart = Utils::bytesToUint64(bytes.substr(209, 8));

    // Count how many block txs are in the block
    uint64_t txCount = 0;
    uint64_t index = 217;
    while (index < txValidatorStart) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += txSize + 4;
      txCount++;
    }

    // Count how many Validator txs are in the block
    uint64_t valTxCount = 0;
    index = txValidatorStart;
    while (index < bytes.size()) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += txSize + 4;
      valTxCount++;
    }

    // Deserialize the transactions
    index = 217;
    for (uint64_t i = 0; i < txCount; ++i) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += 4;
      this->txs.emplace_back(bytes.substr(index, txSize));
      index += txSize;
    }

    // Deserialize the Validator transactions
    index = txValidatorStart;
    for (uint64_t i = 0; i < valTxCount; ++i) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += 4;
      this->txValidators.emplace_back(bytes.substr(index, txSize));
      index += txSize;
    }

    // Sanity check the Merkle roots, block randomness and signature
    auto expectedTxMerkleRoot = Merkle(txs).getRoot();
    auto expectedValidatorMerkleRoot = Merkle(txValidators).getRoot();
    auto expectedRandomness = rdPoS::parseTxSeedList(txValidators);
    if (expectedTxMerkleRoot != txMerkleRoot) {
      throw std::runtime_error("Invalid tx merkle root");
    }
    if (expectedValidatorMerkleRoot != validatorMerkleRoot) {
      throw std::runtime_error("Invalid validator merkle root");
    }
    if (expectedRandomness != blockRandomness) {
      throw std::runtime_error("Invalid block randomness");
    }
    Hash msgHash = this->hash();
    if (!Secp256k1::verifySig(
      this->validatorSig.r(), this->validatorSig.s(), this->validatorSig.v()
    )) {
      throw std::runtime_error("Invalid validator signature");
    }

    // Get the signature and finalize the block
    this->validatorPubKey = Secp256k1::recover(this->validatorSig, msgHash);
    this->finalized = true;
  } catch (std::exception &e) {
    Utils::logToDebug(Log::block, __func__,
      "Error when deserializing a block: " + std::string(e.what())
    );
    // Throw again because invalid blocks should not be created at all.
    throw std::runtime_error(std::string(__func__) + ": " + e.what());
  }
}

const std::string Block::serializeHeader() const {
  // Block header = 218 bytes = {
  //  bytes(prevBlockHash) + bytes(blockRandomness) +
  //  bytes(validatorMerkleRoot) + bytes(txMerkleRoot) +
  //  bytes(timestamp) + bytes(nHeight)
  // }
  std::string ret;
  ret += this->prevBlockHash.get();
  ret += this->blockRandomness.get();
  ret += this->validatorMerkleRoot.get();
  ret += this->txMerkleRoot.get();
  ret += Utils::uint64ToBytes(this->timestamp);
  ret += Utils::uint64ToBytes(this->nHeight);
  return ret;
}

const std::string Block::serializeBlock() const {
  std::string ret;
  // Block = bytes(validatorSig) + bytes(BlockHeader) +
  // TxValidatorStart + [TXs] + [TxValidators]
  ret += this->validatorSig.get();
  ret += this->serializeHeader();

  // Fill in the txValidatorStart with 0s for now, keep track of the index
  uint64_t txValidatorStartLoc = ret.size();
  ret += Utils::uint64ToBytes(0);

  // Serialize the transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txs) {
    std::string txBytes = tx.rlpSerialize();
    ret += Utils::uint32ToBytes(txBytes.size());
    ret += txBytes;
  }

  // Insert the txValidatorStart
  std::string txValidatorStart = Utils::uint64ToBytes(ret.size());
  std::memcpy(&ret[txValidatorStartLoc], txValidatorStart.data(), 8);

  // Serialize the Validator Transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txValidators) {
    std::string txBytes = tx.rlpSerialize();
    ret += Utils::uint32ToBytes(txBytes.size());
    ret += txBytes;
  }

  return ret;
}

const Hash Block::hash() const { return Utils::sha3(this->serializeHeader()); }

bool Block::appendTx(const TxBlock &tx) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Cannot append tx to finalized block");
    return false;
  }
  this->txs.push_back(tx);
  return true;
}

bool Block::appendTxValidator(const TxValidator &tx) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Cannot append tx to finalized block");
    return false;
  }
  this->txValidators.push_back(tx);
  return true;
}

bool Block::finalize(const PrivKey& validatorPrivKey) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Block is already finalized");
    return false;
  }
  this->txMerkleRoot = Merkle(this->txs).getRoot();
  this->validatorMerkleRoot = Merkle(this->txValidators).getRoot();
  this->blockRandomness = rdPoS::parseTxSeedList(this->txValidators);
  this->validatorSig = Secp256k1::sign(validatorPrivKey, this->hash());
  this->validatorPubKey = Secp256k1::recover(this->validatorSig, this->hash());
  this->finalized = true;
  return true;
}

