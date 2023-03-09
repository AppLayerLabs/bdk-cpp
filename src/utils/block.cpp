#include "block.h"


Block::Block(std::string_view bytes) {
  try {
    if (bytes.size() < 209) {
      throw std::runtime_error("Invalid block size");
    }
    this->validatorSig_ = Signature(bytes.substr(0, 65));
    this->prevBlockHash_ = Hash(bytes.substr(65, 32));
    this->blockRandomness_ = Hash(bytes.substr(97, 32));
    this->validatorMerkleRoot_ = Hash(bytes.substr(129, 32));
    this->txMerkleRoot_ = Hash(bytes.substr(161, 32));
    this->timestamp_ = Utils::bytesToUint64(bytes.substr(193, 8));
    this->nHeight_ = Utils::bytesToUint64(bytes.substr(201, 8));

    uint64_t txValidatorStart = Utils::bytesToUint64(bytes.substr(209, 8));

    // Count how many txs are in the block
    uint64_t txCount = 0;
    uint64_t index = 210;
    while (index < txValidatorStart) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += txSize + 4;
      txCount++;
    }

    // Count how many validator txs are in the block
    uint64_t valTxCount = 0;
    index = txValidatorStart;
    while (index < bytes.size()) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += txSize + 4;
      valTxCount++;
    }

    // Deserialize the transactions
    index = 210;
    for (uint64_t i = 0; i < txCount; ++i) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += 4;
      this->txs_.emplace_back(bytes.substr(index, txSize));
      index += txSize;
    }

    // Deserialize the validator transactions
    index = txValidatorStart;
    for (uint64_t i = 0; i < valTxCount; ++i) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += 4;
      this->txValidators_.emplace_back(bytes.substr(index, txSize));
      index += txSize;
    }


    auto expectedTxMerkleRoot = Merkle(txs_).getRoot();
    auto expectedValidatorMerkleRoot = Merkle(txValidators_).getRoot();
    auto expectedRandomness = rdPoS::parseTxSeedList(txValidators_);

    if (expectedTxMerkleRoot != txMerkleRoot_) {
      throw std::runtime_error("Invalid tx merkle root");
    }

    if (expectedValidatorMerkleRoot != validatorMerkleRoot_) {
      throw std::runtime_error("Invalid validator merkle root");
    }

    if (expectedRandomness != blockRandomness_) {
      throw std::runtime_error("Invalid block randomness");
    }

    Hash msgHash = this->hash();
    if (Secp256k1::verifySig(this->validatorSig_.r(), this->validatorSig_.s(), this->validatorSig_.v())) {
      throw std::runtime_error("Invalid validator signature");
    }    

    this->validatorPubKey_ = Secp256k1::recover(this->validatorSig_, msgHash);

    this->finalized = true;
  } catch (std::exception &e) {
    Utils::logToDebug(Log::block, __func__, "Error when deserializing a block: "
      + std::string(e.what())
    );
  }
}


const std::string Block::serializeHeader() const {
  // Block header = 209 bytes
  // Block header == bytes(prevBlockHash) + bytes(blockRandomness) + bytes(validatorMerkleRoot) + bytes(txMerkleRoot) + bytes(timestamp) + bytes(nHeight) 
  std::string ret;
  ret += this->prevBlockHash_.get();
  ret += this->blockRandomness_.get();
  ret += this->validatorMerkleRoot_.get();
  ret += this->txMerkleRoot_.get();
  ret += Utils::uint64ToBytes(this->timestamp_);
  ret += Utils::uint64ToBytes(this->nHeight_);
  return ret;
}

const std::string Block::serializeBlock() const {
  std::string ret;
  // Block = bytes(validatorSig) + bytes(BlockHeader) + TxValidatorStart + [TXs] + [TxValidators]
  ret += this->validatorSig_.get();
  ret += this->serializeHeader();
  
  // Fill in the txValidatorStart with 0s for now, keep track of the index
  uint64_t txValidatorStartLoc = ret.size();
  ret += Utils::uint64ToBytes(0);

  // Serialize the transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txs_) {
    std::string txBytes = tx.rlpSerialize();
    ret += Utils::uint32ToBytes(txBytes.size());
    ret += txBytes;
  }

  // Insert the txValidatorStart
  std::string txValidatorStart = Utils::uint64ToBytes(ret.size());
  std::memcpy(&ret[txValidatorStartLoc], txValidatorStart.data(), 8);

  // Serialize the Validator Transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txValidators_) {
    std::string txBytes = tx.rlpSerialize();
    ret += Utils::uint32ToBytes(txBytes.size());
    ret += txBytes;
  }  

  return ret;
}

const Hash Block::hash() const {
  return Utils::sha3(this->serializeHeader());
}


bool Block::appendTx(const TxBlock &tx) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Cannot append tx to finalized block");
    return false;
  }

  this->txs_.push_back(tx);
  return true;
}

bool Block::appendTxValidator(const TxValidator &tx) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Cannot append tx to finalized block");
    return false;
  }

  this->txValidators_.push_back(tx);
  return true;
}

bool Block::finalize(const PrivKey& validatorPrivKey) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Block is already finalized");
    return false;
  }

  this->txMerkleRoot_ = Merkle(this->txs_).getRoot();
  this->validatorMerkleRoot_ = Merkle(this->txValidators_).getRoot();
  this->blockRandomness_ = rdPoS::parseTxSeedList(this->txValidators_);
  this->validatorSig_ = Secp256k1::sign(validatorPrivKey, this->hash());
  this->finalized = true;
  return true;
}