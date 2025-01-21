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

FinalizedBlock FinalizedBlock::fromCometBlock(const CometBlock& block) {

  // proposerAddr is optional in CometBlock
  Address proposerAddr;
  if (!block.proposerAddr.empty()) {
    proposerAddr = Address(block.proposerAddr);
  }

  // prevBlockHash is optional in CometBlock
  Hash prevBlockHash;
  if (!block.prevHash.empty()) {
    prevBlockHash = Hash(block.prevHash);
  }

  uint64_t requiredChainId = 0; // FIXME

  // compute txs here txblocks
  SLOGTRACE("Deserializing transactions...");
  std::vector<std::shared_ptr<TxBlock>> txs;
  uint64_t txCount = block.txs.size();
  for (uint64_t i = 0; i < txCount; i++) {
    // We can skip signature verification because fromCometBlock() is called from
    // the FinalizedBlock ABCI callback. If the block is finalized, then all transactions
    // have long been all checked for signature validity upstream.
    txs.push_back(std::make_shared<TxBlock>(block.txs[i], requiredChainId, false));
  }

  // Same merkle root value as before cometbft integration
  auto txMerkleRoot = Merkle(txs).getRoot();

  uint64_t timestamp = block.timeNanos / 1000; // FinalizedBlock uses microseconds
  uint64_t nHeight = block.height;

  Hash hash(block.hash);

  return {
    std::move(proposerAddr),
    std::move(prevBlockHash),
    std::move(txMerkleRoot),
    timestamp,
    nHeight,
    std::move(txs),
    std::move(hash)
  };
}

FinalizedBlock FinalizedBlock::fromRPC(const json& ret) {
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

  if (!header.contains("last_block_id") || !header["last_block_id"].is_object()) {
    throw DynamicException("Invalid block data");
  }
  const auto& last_block_id = header["last_block_id"];

  if (!block.contains("data") || !block["data"].is_object()) {
    throw DynamicException("Invalid block data");
  }
  const auto& data = block["data"];

  SLOGTRACE("Deserializing transactions...");
  uint64_t requiredChainId = 0; // FIXME
  std::vector<std::shared_ptr<TxBlock>> txs;
  if (data.contains("txs") && data["txs"].is_array()) {
    // There is data.txs in the response, so unpack the transactions
    for (const auto& tx : data["txs"]) {
      if (tx.is_string()) {
        // Compute tx Bytes from the base64-encoded tx data string
        std::string txBase64 = tx.get<std::string>();
        Bytes txBytes = base64::decode_into<Bytes>(txBase64);
        // Create TxBlock object from tx Bytes
        // We can skip signature verification because this is retrieving a finalized block
        // from CometBFT node storage; signatures for all txs have been validated long ago.
        txs.push_back(std::make_shared<TxBlock>(txBytes, requiredChainId, false));
      } else {
        throw DynamicException("Invalid block data");
      }
    }
  }

  if (!last_block_id.contains("hash") || !last_block_id["hash"].is_string()) {
    throw DynamicException("Invalid block data");
  }

  std::string lastBlockHashStr = last_block_id["hash"].get<std::string>();
  Hash prevBlockHash(Hex::toBytes(lastBlockHashStr)); // may be "" (for block at height == 1)

  if (!header.contains("proposer_address") || !header["proposer_address"].is_string()) {
    throw DynamicException("Invalid block data");
  }

  std::string proposerAddrStr = header["proposer_address"].get<std::string>();
  Address proposerAddr(Hex::toBytes(proposerAddrStr));

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

  return {
    std::move(proposerAddr),
    std::move(prevBlockHash),
    std::move(txMerkleRoot),
    timestamp,
    nHeight,
    std::move(txs),
    std::move(hash)
  };
}
