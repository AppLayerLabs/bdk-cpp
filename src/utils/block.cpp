#include "block.h"
#include "../core/rdpos.h"

Block::Block(std::string_view bytes, const uint64_t& requiredChainId) {
  try {
    // Split the bytes string
    if (bytes.size() < 217) throw std::runtime_error("Invalid block size - too short");
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
    uint64_t index = 217; // Start of block tx range
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
    index = 217;  // Rewind to start of block tx range

    // If we have up to X block txs or only one physical thread
    // for some reason, deserialize normally.
    // Otherwise, parallelize into threads/asyncs.
    unsigned int thrNum = std::thread::hardware_concurrency();
    if (thrNum <= 1 || txCount <= 2000) {
      for (uint64_t i = 0; i < txCount; ++i) {
        uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
        index += 4;
        this->txs.emplace_back(bytes.substr(index, txSize), requiredChainId);
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
            for (uint64_t i = 0; i < nTxs; i++) {
              uint64_t len = Utils::bytesToUint32(bytes.substr(idx, 4));
              idx += 4;
              txVec.emplace_back(bytes.substr(idx, len), requiredChainId);
              idx += len;
            }
            return txVec;
          }
        );
        f.emplace_back(std::move(txF));

        // Update offset, skip if this is the last thread
        if (i < txsPerThr.size() - 1) {
          for (uint64_t i = 0; i < nTxs; i++) {
            uint64_t len = Utils::bytesToUint32(bytes.substr(thrOff, 4));
            thrOff += len + 4;
          }
        }
      }

      // Wait for asyncs and fill the block tx vector
      for (int i = 0; i < f.size(); i++) {
        f[i].wait();
        for (TxBlock tx : f[i].get()) this->txs.emplace_back(tx);
      }
    }

    // Deserialize the Validator transactions normally, no need to thread
    index = txValidatorStart;
    for (uint64_t i = 0; i < valTxCount; ++i) {
      uint64_t txSize = Utils::bytesToUint32(bytes.substr(index, 4));
      index += 4;
      this->txValidators.emplace_back(bytes.substr(index, txSize), requiredChainId);
      if (txValidators.back().getNHeight() != this->nHeight) {
        throw std::runtime_error("Invalid validator tx height");
      }
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

bool Block::finalize(const PrivKey& validatorPrivKey, const uint64_t& newTimestamp) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Block is already finalized");
    return false;
  }
  // Allow rdPoS to improve block time only if new timestamp is better than old timestamp
  if (this->timestamp > newTimestamp) {
    Utils::logToDebug(Log::block, __func__,
      "Block timestamp not satisfiable, expected higher than " +
      std::to_string(this->timestamp) + " got " + std::to_string(newTimestamp)
    );
    return false;
  }
  this->timestamp = newTimestamp;
  this->txMerkleRoot = Merkle(this->txs).getRoot();
  this->validatorMerkleRoot = Merkle(this->txValidators).getRoot();
  this->blockRandomness = rdPoS::parseTxSeedList(this->txValidators);
  this->validatorSig = Secp256k1::sign(this->hash(), validatorPrivKey);
  this->validatorPubKey = Secp256k1::recover(this->validatorSig, this->hash());
  this->finalized = true;
  return true;
}

