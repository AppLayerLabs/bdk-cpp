/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "block.h"
#include "../core/rdpos.h"

Block::Block(const BytesArrView bytes, const uint64_t& requiredChainId) {
  try {
    // Split the bytes string
    if (bytes.size() < 217) throw std::runtime_error("Invalid block size - too short");
    this->validatorSig_ = Signature(bytes.subspan(0, 65));
    this->prevBlockHash_ = Hash(bytes.subspan(65, 32));
    this->blockRandomness_= Hash(bytes.subspan(97, 32));
    this->validatorMerkleRoot_ = Hash(bytes.subspan(129, 32));
    this->txMerkleRoot_ = Hash(bytes.subspan(161, 32));
    this->timestamp_ = Utils::bytesToUint64(bytes.subspan(193, 8));
    this->nHeight_ = Utils::bytesToUint64(bytes.subspan(201, 8));
    uint64_t txValidatorStart = Utils::bytesToUint64(bytes.subspan(209, 8));

    // Count how many block txs are in the block
    uint64_t txCount = 0;
    uint64_t index = 217; // Start of block tx range
    while (index < txValidatorStart) {
      uint64_t txSize = Utils::bytesToUint32(bytes.subspan(index, 4));
      index += txSize + 4;
      txCount++;
    }

    // Count how many Validator txs are in the block
    uint64_t valTxCount = 0;
    index = txValidatorStart;
    while (index < bytes.size()) {
      uint64_t txSize = Utils::bytesToUint32(bytes.subspan(index, 4));
      index += txSize + 4;
      valTxCount++;
    }
    index = 217;  // Rewind to start of block tx range

    // If we have up to X block txs or only one physical thread
    // for some reason, deserialize normally.
    // Otherwise, parallelize into threads/asyncs.
    unsigned int thrNum = std::thread::hardware_concurrency();
    if (thrNum <= 1 || txCount <= 2000) {
      for (uint64_t i = 0; i < txCount; ++i) {
        uint64_t txSize = Utils::bytesToUint32(bytes.subspan(index, 4));
        index += 4;
        this->txs_.emplace_back(bytes.subspan(index, txSize), requiredChainId);
        index += txSize;
      }
    } else {
      // Logically divide txs equally into one-time hardware threads/asyncs.
      // Division reminder always goes to the LAST thread (e.g. 11/4 = 2+2+2+5)
      std::vector<uint64_t> txsPerThr(thrNum, txCount / thrNum);
      txsPerThr.back() += txCount % thrNum;

      // Deserialize the txs with parallelized asyncs
      std::vector<std::future<std::vector<TxBlock>>> f;
      f.reserve(thrNum);
      uint64_t thrOff = index;
      for (uint64_t i = 0; i < txsPerThr.size(); i++) {
        // Find out how many txs this thread will work with,
        // then update offset for next thread
        uint64_t startIdx = thrOff;
        uint64_t nTxs = txsPerThr[i];

        // Work that sucker to death, c'mon now
        std::future<std::vector<TxBlock>> txF = std::async(
          [&, startIdx, nTxs](){
            std::vector<TxBlock> txVec;
            uint64_t idx = startIdx;
            for (uint64_t ii = 0; ii < nTxs; ii++) {
              uint64_t len = Utils::bytesToUint32(bytes.subspan(idx, 4));
              idx += 4;
              txVec.emplace_back(bytes.subspan(idx, len), requiredChainId);
              idx += len;
            }
            return txVec;
          }
        );
        f.emplace_back(std::move(txF));

        // Update offset, skip if this is the last thread
        if (i < txsPerThr.size() - 1) {
          for (uint64_t ii = 0; ii < nTxs; ii++) {
            uint64_t len = Utils::bytesToUint32(bytes.subspan(thrOff, 4));
            thrOff += len + 4;
          }
        }
      }

      // Wait for asyncs and fill the block tx vector
      for (int i = 0; i < f.size(); i++) {
        f[i].wait();
        for (TxBlock tx : f[i].get()) this->txs_.emplace_back(tx);
      }
    }

    // Deserialize the Validator transactions normally, no need to thread
    index = txValidatorStart;
    for (uint64_t i = 0; i < valTxCount; ++i) {
      uint64_t txSize = Utils::bytesToUint32(bytes.subspan(index, 4));
      index += 4;
      this->txValidators_.emplace_back(bytes.subspan(index, txSize), requiredChainId);
      if (this->txValidators_.back().getNHeight() != this->nHeight_) {
        throw std::runtime_error("Invalid validator tx height");
      }
      index += txSize;
    }
    // Sanity check the Merkle roots, block randomness and signature
    auto expectedTxMerkleRoot = Merkle(this->txs_).getRoot();
    auto expectedValidatorMerkleRoot = Merkle(this->txValidators_).getRoot();
    auto expectedRandomness = rdPoS::parseTxSeedList(this->txValidators_);
    if (expectedTxMerkleRoot != this->txMerkleRoot_) {
      throw std::runtime_error("Invalid tx merkle root");
    }
    if (expectedValidatorMerkleRoot != this->validatorMerkleRoot_) {
      throw std::runtime_error("Invalid validator merkle root");
    }
    if (expectedRandomness != this->blockRandomness_) {
      throw std::runtime_error("Invalid block randomness");
    }
    Hash msgHash = this->hash();
    if (!Secp256k1::verifySig(
      this->validatorSig_.r(), this->validatorSig_.s(), this->validatorSig_.v()
    )) {
      throw std::runtime_error("Invalid validator signature");
    }
    // Get the signature and finalize the block
    this->validatorPubKey_ = Secp256k1::recover(this->validatorSig_, msgHash);
    this->finalized_ = true;
  } catch (std::exception &e) {
    Logger::logToDebug(LogType::ERROR, Log::block, __func__,
      "Error when deserializing a block: " + std::string(e.what())
    );
    // Throw again because invalid blocks should not be created at all.
    throw std::runtime_error(std::string(__func__) + ": " + e.what());
  }
}

const Bytes Block::serializeHeader() const {
  // Block header = 144 bytes = {
  //  bytes(prevBlockHash) + bytes(blockRandomness) +
  //  bytes(validatorMerkleRoot) + bytes(txMerkleRoot) +
  //  bytes(timestamp) + bytes(nHeight)
  // }
  Bytes ret;
  ret.reserve(144);
  ret.insert(ret.end(), this->prevBlockHash_.cbegin(), this->prevBlockHash_.cend());
  ret.insert(ret.end(), this->blockRandomness_.cbegin(), this->blockRandomness_.cend());
  ret.insert(ret.end(), this->validatorMerkleRoot_.cbegin(), this->validatorMerkleRoot_.cend());
  ret.insert(ret.end(), this->txMerkleRoot_.cbegin(), this->txMerkleRoot_.cend());
  Utils::appendBytes(ret, Utils::uint64ToBytes(this->timestamp_));
  Utils::appendBytes(ret, Utils::uint64ToBytes(this->nHeight_));
  return ret;
}

const Bytes Block::serializeBlock() const {
  Bytes ret;
  // Block = bytes(validatorSig) + bytes(BlockHeader) +
  // TxValidatorStart + [TXs] + [TxValidators]
  ret.insert(ret.end(), this->validatorSig_.cbegin(), this->validatorSig_.cend());
  Utils::appendBytes(ret, this->serializeHeader());

  // Fill in the txValidatorStart with 0s for now, keep track of the index
  uint64_t txValidatorStartLoc = ret.size();
  ret.insert(ret.end(), 8, 0x00);

  // Serialize the transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txs_) {
    Bytes txBytes = tx.rlpSerialize();
    Utils::appendBytes(ret, Utils::uint32ToBytes(txBytes.size()));
    ret.insert(ret.end(), txBytes.begin(), txBytes.end());
  }

  // Insert the txValidatorStart
  BytesArr<8> txValidatorStart = Utils::uint64ToBytes(ret.size());
  std::memcpy(&ret[txValidatorStartLoc], txValidatorStart.data(), 8);

  // Serialize the Validator Transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txValidators_) {
    Bytes txBytes = tx.rlpSerialize();
    Utils::appendBytes(ret, Utils::uint32ToBytes(txBytes.size()));
    ret.insert(ret.end(), txBytes.begin(), txBytes.end());
  }

  return ret;
}

const Hash Block::hash() const { return Utils::sha3(this->serializeHeader()); }

bool Block::appendTx(const TxBlock &tx) {
  if (this->finalized_) {
    Logger::logToDebug(LogType::ERROR, Log::block, __func__,
      "Cannot append tx to finalized block"
    );
    return false;
  }
  this->txs_.push_back(tx);
  return true;
}

bool Block::appendTxValidator(const TxValidator &tx) {
  if (this->finalized_) {
    Logger::logToDebug(LogType::ERROR, Log::block, __func__,
      "Cannot append tx to finalized block"
    );
    return false;
  }
  this->txValidators_.push_back(tx);
  return true;
}

bool Block::finalize(const PrivKey& validatorPrivKey, const uint64_t& newTimestamp) {
  if (this->finalized_) {
    Logger::logToDebug(LogType::ERROR, Log::block, __func__, "Block is already finalized");
    return false;
  }
  // Allow rdPoS to improve block time only if new timestamp is better than old timestamp
  if (this->timestamp_ > newTimestamp) {
    Logger::logToDebug(LogType::ERROR, Log::block, __func__,
      "Block timestamp not satisfiable, expected higher than " +
      std::to_string(this->timestamp_) + " got " + std::to_string(newTimestamp)
    );
    return false;
  }
  this->timestamp_ = newTimestamp;
  this->txMerkleRoot_ = Merkle(this->txs_).getRoot();
  this->validatorMerkleRoot_ = Merkle(this->txValidators_).getRoot();
  this->blockRandomness_= rdPoS::parseTxSeedList(this->txValidators_);
  this->validatorSig_ = Secp256k1::sign(this->hash(), validatorPrivKey);
  this->validatorPubKey_ = Secp256k1::recover(this->validatorSig_, this->hash());
  this->finalized_ = true;
  return true;
}

