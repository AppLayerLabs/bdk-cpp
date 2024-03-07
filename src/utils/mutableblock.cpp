/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "mutableblock.h"
#include "../core/rdpos.h"

MutableBlock::MutableBlock(const BytesArrView bytes, const uint64_t& requiredChainId) {
    try {
        // Verify minimum size for a valid block
        if (bytes.size() < 217) throw std::runtime_error("Invalid block size - too short");
        // Parsing fixed-size fields
        this->prevBlockHash_ = Hash(bytes.subspan(65, 32));
        this->timestamp_ = Utils::bytesToUint64(bytes.subspan(193, 8));
        this->nHeight_ = Utils::bytesToUint64(bytes.subspan(201, 8));

        // Initialization for transaction counts is not required here
        // since they will be calculated during the deserialization process

        Logger::logToDebug(LogType::INFO, Log::mutableblock, __func__, "Deserializing block...");
        this->deserialize(bytes, requiredChainId);
    } catch (const std::exception &e) {
        Logger::logToDebug(LogType::ERROR, Log::mutableblock, __func__, "Error when deserializing a MutableBlock: " + std::string(e.what()));
        throw std::runtime_error(std::string("Error when deserializing a MutableBlock: ") + e.what());
    }
}

void MutableBlock::deserialize(const BytesArrView bytes, const uint64_t& requiredChainId) {

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
        Logger::logToDebug(LogType::ERROR, Log::mutableblock, __func__, "Invalid validator tx height");
        throw DynamicException("Invalid validator tx height");
      }
      index += txSize;
    }

    Logger::logToDebug(LogType::INFO, Log::mutableblock, __func__, "Block deserialized successfully");
    this->isDeserialized_ = true;

}

bool MutableBlock::appendTx(const TxBlock& tx) {
  if (this->isDeserialized_) {
    Logger::logToDebug(LogType::ERROR, Log::mutableblock, __func__, "Block is already deserialized");
    return false;
  }
  this->txs_.emplace_back(tx);
  return true;
}

bool MutableBlock::appendTxValidator(const TxValidator& tx) {
  if (this->isDeserialized_) {
    Logger::logToDebug(LogType::ERROR, Log::mutableblock, __func__, "Block is already deserialized");
    return false;
  }
  this->txValidators_.emplace_back(tx);
  return true;
}

Bytes MutableBlock::serializeMutableHeader(Hash validatorMerkleRoot, Hash txMerkleRoot) const {
  Bytes ret;
  ret.reserve(144);
  ret.insert(ret.end(), this->prevBlockHash_.cbegin(), this->prevBlockHash_.cend());
  ret.insert(ret.end(), this->blockRandomness_.cbegin(), blockRandomness_.cend());
  ret.insert(ret.end(), validatorMerkleRoot.cbegin(), validatorMerkleRoot.cend());
  ret.insert(ret.end(), txMerkleRoot.cbegin(), txMerkleRoot.cend());
  Utils::appendBytes(ret, Utils::uint64ToBytes(this->timestamp_));
  Utils::appendBytes(ret, Utils::uint64ToBytes(this->nHeight_));
  return ret;
}

FinalizedBlock MutableBlock::finalize(const PrivKey& validatorPrivKey, const uint64_t& newTimestamp) {
    if (this->timestamp_ > newTimestamp) {
    Logger::logToDebug(LogType::ERROR, Log::mutableblock, __func__,
      "Block timestamp not satisfiable, expected higher than " +
      std::to_string(this->timestamp_) + " got " + std::to_string(newTimestamp)
    );
    throw DynamicException("Block timestamp not satisfiable");
  }

  this->timestamp_ = newTimestamp;

  Logger::logToDebug(LogType::INFO, Log::mutableblock, __func__, "Finalizing block...");
  Hash validatorMerkleRoot = Merkle(this->txValidators_).getRoot();
  Hash txMerkleRoot = Merkle(this->txs_).getRoot();
  this->blockRandomness_ = rdPoS::parseTxSeedList(this->txValidators_);
  Hash hash = Utils::sha3(this->serializeMutableHeader(validatorMerkleRoot, txMerkleRoot));
  Signature validatorSig = Secp256k1::sign(hash, validatorPrivKey);
  UPubKey validatorPubKey = Secp256k1::recover(validatorSig, hash);
  return FinalizedBlock(
      std::move(validatorSig),
      std::move(validatorPubKey),
      std::move(this->prevBlockHash_),
      std::move(this->blockRandomness_),
      std::move(validatorMerkleRoot),
      std::move(txMerkleRoot),
      this->timestamp_,
      this->nHeight_,
      std::move(this->txValidators_),
      std::move(this->txs_),
      std::move(hash)
  );
}