/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "finalizedblock.h"

#include "../core/rdpos.h" // net/p2p/managernormal.h -> net/p2p/nodeconns.h (and broadcaster.h) -> thread

#include "../utils/uintconv.h"

#include "../core/comet.h"

#include "../libs/base64.hpp"



// FIXME/TODO
// This constructor will end up entirely removed, since the source of a FinalizedBlock instance
// is now a CometBlock instance (plus some info you can get via RPC to complement it if you need it).
// Ideally, FinalizedBlock depends only on CometBlock, and whatever downstream code depends on
// removed fields can itself be removed.
//FinalizedBlock FinalizedBlock::fromBytes(const bytes::View bytes, const uint64_t& requiredChainId) {
 // throw DynamicException("FOREVER UNSUPPORTED, convert callsite to fromCometBlock() or delete it");
  /*try {
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
    bytes::View headerBytes = bytes.subspan(65, 144);
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
  */
//}

FinalizedBlock FinalizedBlock::fromCometBlock(const CometBlock& block) {
  auto validatorSig = Signature();
  auto prevBlockHash = Hash();
  auto blockRandomness = Hash();
  auto validatorMerkleRoot = Hash();

  uint64_t requiredChainId = 0; // FIXME

  // compute txs here txblocks
  SLOGTRACE("Deserializing transactions...");
  std::vector<TxBlock> txs;
  uint64_t txCount = block.txs.size();
  for (uint64_t i = 0; i < txCount; i++) {
    // NOTE: requiredChainId seems to be a discarded argument of the TxBlock ctor
    txs.emplace_back(block.txs[i], requiredChainId);
  }

  // Same merkle root value as before cometbft integration
  auto txMerkleRoot = Merkle(txs).getRoot();

  uint64_t timestamp = block.timeNanos / 1000; // FinalizedBlock uses microseconds
  uint64_t nHeight = block.height;

  UPubKey validatorPubKey;

  uint64_t blockSize = 1; // FIXME

  Hash hash(block.hash);
  std::vector<TxValidator> txValidators;

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
    blockSize
  };
}

FinalizedBlock FinalizedBlock::fromRPC(const json& ret) {
  auto validatorSig = Signature();
  auto prevBlockHash = Hash();
  auto blockRandomness = Hash();
  auto validatorMerkleRoot = Hash();

  uint64_t requiredChainId = 0; // FIXME

  std::vector<TxBlock> txs;

  //----

  if (ret.is_object() && ret.contains("result") && ret["result"].is_object()) {
    throw DynamicException("Invalid block data");
  }
  const auto& result = ret["result"];
  if (!result.contains("block_id") || !result["block_id"].is_object()) {
    throw DynamicException("Invalid block data");
  }
  const auto& block_id = result["block_id"];
  if (!result.contains("block") || !result["block"].is_object()) {
    throw DynamicException("Invalid block data");
  }
  const auto& block = result["block"];
  if (!block.contains("header") || !block["header"].is_object()) {
    throw DynamicException("Invalid block data");
  }
  const auto& header = block["header"];
  if (!block.contains("data") || !block["data"].is_object()) {
    throw DynamicException("Invalid block data");
  }
  const auto& data = block["data"];
  SLOGTRACE("Deserializing transactions...");
  if (data.contains("txs") && data["txs"].is_array()) {
    // There is data.txs in the response, so unpack the transactions
    for (const auto& tx : data["txs"]) {
      if (tx.is_string()) {
        // Compute tx Bytes from the base64-encoded tx data string
        std::string txBase64 = tx.get<std::string>();
        Bytes txBytes = base64::decode_into<Bytes>(txBase64);
        // Create TxBlock object from tx Bytes
        txs.emplace_back(txBytes, requiredChainId);
      } else {
        throw DynamicException("Invalid block data");
      }
    }
  }

  if (!header.contains("height") || !header["height"].is_string()) {
    throw DynamicException("Invalid block data");
  }

  uint64_t nHeight = std::stoull(header["height"].get<std::string>());

  if (!header.contains("timestamp") || !header["timestamp"].is_string()) {
    throw DynamicException("Invalid block data");
  }

  uint64_t timestamp = Utils::stringToNanos(header["timestamp"].get<std::string>());
  timestamp /= 1000; // FinalizedBlock uses microseconds, so convert nanos to micros

  if (!block_id.contains("hash") || !block_id["hash"].is_string()) {
    throw DynamicException("Invalid block data");
  }
  std::string blockIdHashStr = block_id["hash"].get<std::string>();

  Hash hash(Hex::toBytes(blockIdHashStr));

  //----

  // Same merkle root value as before cometbft integration
  auto txMerkleRoot = Merkle(txs).getRoot();

  UPubKey validatorPubKey;

  uint64_t blockSize = 1; // FIXME

  std::vector<TxValidator> txValidators;

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
    blockSize
  };
}


//FinalizedBlock FinalizedBlock::createNewValidBlock(
//  std::vector<TxBlock>&& txs,
//  std::vector<TxValidator>&& txValidators,
//  Hash prevBlockHash,
//  const uint64_t& timestamp,
//  const uint64_t& nHeight,
//  const PrivKey& validatorPrivKey
//) {
  // "creating blocks" for testing no longer makes any sense
  // blocks are only created by cometbft
  // FinalizedBlock can only be constructed from an already existing and valid CometBFT block (CometBlock)
//  throw DynamicException("FOREVER UNSUPPORTED, delete all callsites then delete this method");

  /*
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
  */
//}
/*
Bytes FinalizedBlock::serializeHeader() const {

  // FIXME/TODO
  // If this is ever needed, what it actually means is you'd get
  // the bytes of the header from cometbft via RPC
  // OR,
  // maybe, what we mean here is the deterministic, standard ETH1 header
  // that we would emulate on top of the CometBFT block header.

  return Utils::makeBytes(bytes::join(
    prevBlockHash_, blockRandomness_, validatorMerkleRoot_, txMerkleRoot_,
    UintConv::uint64ToBytes(timestamp_), UintConv::uint64ToBytes(nHeight_)
  ));
}

Bytes FinalizedBlock::serializeBlock() const {

  // FIXME/TODO
  // If this is ever needed, what it actually means is you'd get
  // the bytes of the whole block from cometbft via RPC

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
*/
