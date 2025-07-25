/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "finalizedblock.h"

#include "../core/rdpos.h" // net/p2p/managernormal.h -> net/p2p/nodeconns.h (and broadcaster.h) -> thread

#include "../utils/uintconv.h"

FinalizedBlock FinalizedBlock::fromBytes(const View<Bytes> bytes, const uint64_t& requiredChainId) {
  try {
    // Verify minimum size for a valid block
    SLOGTRACE("Deserializing block...");
    if (bytes.size() < 217) throw std::length_error("Invalid block size - too short");

    // Parsing fixed-size fields
    auto validatorSig = Signature(bytes.subspan(0, 65));
    auto prevBlockHash = Hash(bytes.subspan(65, 32));
    auto blockRandomness = Hash(bytes.subspan(97, 32));
    auto validatorMerkleRoot = Hash(bytes.subspan(129, 32));
    auto txMerkleRoot = Hash(bytes.subspan(161, 32));
    uint64_t timestamp = UintConv::bytesToUint64(bytes.subspan(193, 8));
    uint64_t nHeight = UintConv::bytesToUint64(bytes.subspan(201, 8));
    uint64_t txValidatorStart = UintConv::bytesToUint64(bytes.subspan(209, 8));

    SLOGTRACE("Deserializing transactions...");

    std::vector<TxBlock> txs;
    std::vector<TxValidator> txValidators;

    // Count how many block txs are in the block
    uint64_t txCount = 0;
    uint64_t index = 217; // Start of block tx range
    while (index < txValidatorStart) {
      uint64_t txSize = UintConv::bytesToUint32(bytes.subspan(index, 4));
      index += txSize + 4;
      txCount++;
    }

    // Count how many Validator txs are in the block
    uint64_t valTxCount = 0;
    index = txValidatorStart;
    while (index < bytes.size()) {
      uint64_t txSize = UintConv::bytesToUint32(bytes.subspan(index, 4));
      index += txSize + 4;
      valTxCount++;
    }
    index = 217;  // Rewind to start of block tx range

    // If we have up to X block txs or only one physical thread
    // for some reason, deserialize normally.
    // Otherwise, parallelize into threads/asyncs.
    if (
      unsigned int thrNum = std::thread::hardware_concurrency();
      thrNum <= 1 || txCount <= 2000
    ) {
      for (uint64_t i = 0; i < txCount; i++) {
        uint64_t txSize = UintConv::bytesToUint32(bytes.subspan(index, 4));
        index += 4;
        txs.emplace_back(bytes.subspan(index, txSize), requiredChainId);
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
          [&bytes, requiredChainId, startIdx, nTxs](){
            std::vector<TxBlock> txVec;
            uint64_t idx = startIdx;
            for (uint64_t ii = 0; ii < nTxs; ii++) {
              uint64_t len = UintConv::bytesToUint32(bytes.subspan(idx, 4));
              idx += 4;
              txVec.emplace_back(bytes.subspan(idx, len), requiredChainId);
              idx += len;
            }
            return txVec;
          }
        );
        f.emplace_back(std::move(txF));

        // Update offset, skip if this is the last thread
        if (i >= txsPerThr.size() - 1) continue;
        for (uint64_t ii = 0; ii < nTxs; ii++) {
          uint64_t len = UintConv::bytesToUint32(bytes.subspan(thrOff, 4));
          thrOff += len + 4;
        }
      }

      // Wait for asyncs and fill the block tx vector
      for (int i = 0; i < f.size(); i++) {
        f[i].wait();
        for (const TxBlock& tx : f[i].get()) txs.emplace_back(tx);
      }
    }

    // Deserialize the Validator transactions normally, no need to thread
    index = txValidatorStart;
    for (uint64_t i = 0; i < valTxCount; ++i) {
      uint64_t txSize = UintConv::bytesToUint32(bytes.subspan(index, 4));
      index += 4;
      txValidators.emplace_back(bytes.subspan(index, txSize), requiredChainId);
      if (txValidators.back().getNHeight() != nHeight) {
        SLOGERROR("Invalid validator tx height");
        throw DynamicException("Invalid validator tx height");
      }
      index += txSize;
    }

    // Sanity check the Merkle roots, block randomness and signature
    auto expectedTxMerkleRoot = Merkle(txs).getRoot();
    auto expectedValidatorMerkleRoot = Merkle(txValidators).getRoot();
    auto expectedRandomness = rdPoS::parseTxSeedList(txValidators);
    if (expectedTxMerkleRoot != txMerkleRoot) throw std::invalid_argument("Invalid tx merkle root");
    if (expectedValidatorMerkleRoot != validatorMerkleRoot) throw std::invalid_argument("Invalid validator merkle root");
    if (expectedRandomness != blockRandomness) throw std::invalid_argument("Invalid block randomness");

    /// Block header to hash is the 144 after the signature
    View<Bytes> headerBytes = bytes.subspan(65, 144);
    Hash hash = Utils::sha3(headerBytes);
    UPubKey validatorPubKey = Secp256k1::recover(validatorSig, hash);
    return {
      std::move(validatorSig),
      std::move(validatorPubKey),
      std::move(prevBlockHash),
      std::move(blockRandomness),
      std::move(validatorMerkleRoot),
      std::move(txMerkleRoot),
      timestamp,
      nHeight,
      std::move(txValidators),
      std::move(txs),
      std::move(hash),
      bytes.size()
    };
  } catch (const std::exception &e) {
    SLOGERROR("Error when deserializing a FinalizedBlock: " + std::string(e.what()));
    throw std::domain_error(std::string("Error when deserializing a FinalizedBlock: ") + e.what());
  }
}

FinalizedBlock FinalizedBlock::createNewValidBlock(
  std::vector<TxBlock>&& txs,
  std::vector<TxValidator>&& txValidators,
  Hash prevBlockHash,
  const uint64_t& timestamp,
  const uint64_t& nHeight,
  const PrivKey& validatorPrivKey
) {
  // We need to sign the block header
  // The block header is composed of the following fields:
  // prevBlockHash + blockRandomness + validatorMerkleRoot + txMerkleRoot + timestamp + nHeight
  Hash blockRandomness = rdPoS::parseTxSeedList(txValidators);
  Hash validatorMerkleRoot = Merkle(txValidators).getRoot();
  Hash txMerkleRoot = Merkle(txs).getRoot();

  Bytes header = Utils::makeBytes(bytes::join(
    prevBlockHash, blockRandomness, validatorMerkleRoot, txMerkleRoot,
    UintConv::uint64ToBytes(timestamp), UintConv::uint64ToBytes(nHeight)
  ));

  Hash headerHash = Utils::sha3(header);
  Signature signature = Secp256k1::sign(headerHash, validatorPrivKey);
  UPubKey validatorPubKey = Secp256k1::recover(signature, headerHash);

  // The block size is AT LEAST the size of the header
  uint64_t blockSize = 217;
  for (const auto &tx : txs) blockSize += tx.rlpSize() + 4;
  for (const auto &tx : txValidators) blockSize += tx.rlpSize() + 4;

  // For each transaction, we need to sum on the blockSize the size of the transaction + 4 bytes for the serialized size
  // In order to get the size of a transaction without actually serializing it, we can use the rlpSize() method
  return {
    std::move(signature),
    std::move(validatorPubKey),
    std::move(prevBlockHash),
    std::move(blockRandomness),
    std::move(validatorMerkleRoot),
    std::move(txMerkleRoot),
    timestamp,
    nHeight,
    std::move(txValidators),
    std::move(txs),
    std::move(headerHash),
    blockSize
  };
}

Bytes FinalizedBlock::serializeHeader() const {
  return Utils::makeBytes(bytes::join(
    prevBlockHash_, blockRandomness_, validatorMerkleRoot_, txMerkleRoot_,
    UintConv::uint64ToBytes(timestamp_), UintConv::uint64ToBytes(nHeight_)
  ));
}

Bytes FinalizedBlock::serializeBlock() const {
  Bytes ret;
  ret.insert(ret.end(), this->validatorSig_.cbegin(), this->validatorSig_.cend());
  Utils::appendBytes(ret, this->serializeHeader());

  // Fill in the txValidatorStart with 0s for now, keep track of the index
  uint64_t txValidatorStartLoc = ret.size();
  ret.insert(ret.end(), 8, 0x00);

  // Serialize the transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txs_) {
    Bytes txBytes = tx.rlpSerialize();
    Utils::appendBytes(ret, UintConv::uint32ToBytes(txBytes.size()));
    ret.insert(ret.end(), txBytes.begin(), txBytes.end());
  }

  // Insert the txValidatorStart
  BytesArr<8> txValidatorStart = UintConv::uint64ToBytes(ret.size());
  std::memcpy(&ret[txValidatorStartLoc], txValidatorStart.data(), 8);

  // Serialize the Validator Transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txValidators_) {
    Bytes txBytes = tx.rlpSerialize();
    Utils::appendBytes(ret, UintConv::uint32ToBytes(txBytes.size()));
    ret.insert(ret.end(), txBytes.begin(), txBytes.end());
  }

  return ret;
}

