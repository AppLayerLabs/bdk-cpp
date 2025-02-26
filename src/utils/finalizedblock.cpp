/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "finalizedblock.h"

#include "../utils/uintconv.h"

#include "../core/comet.h"

#include "../libs/base64.hpp"

FinalizedBlock FinalizedBlock::fromCometBlock(
  const CometBlock& block, std::unordered_map<Hash, std::shared_ptr<TxBlock>, SafeHash>* mempoolPtr
) {

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

  // the last tx is the block randomness (random hash)
  if (block.txs.empty()) {
    throw DynamicException("Invalid CometBlock: empty tx list (no block randomness)");
  }
  size_t lastTxIndex = block.txs.size()-1;
  if (block.txs[lastTxIndex].size() != 32) {
    throw DynamicException("Invalid CometBlock: txs[lastTxIndex].size() != 32 bytes (bad block randomness)");
  }
  Hash randomness = Hash(block.txs[lastTxIndex]);

  // compute txs here txblocks
  std::vector<std::shared_ptr<TxBlock>> txs;
  uint64_t txCount = lastTxIndex; // NOTE: last tx is skipped (block.txs.size()-1) since that is the randomHash
  if (mempoolPtr == nullptr) {
    // Deserializing transactions
    for (uint64_t i = 0; i < txCount; i++) {
      // We can skip signature verification because fromCometBlock() is called from
      // the FinalizedBlock ABCI callback. If the block is finalized, then all transactions
      // have long been all checked for signature validity upstream.
      txs.push_back(std::make_shared<TxBlock>(block.txs[i], requiredChainId, false));
    }
  } else {
    // Moving transactions from the mirror mempool
    for (uint64_t i = 0; i < txCount; i++) {
      Hash txHash = Utils::sha3(block.txs[i]);
      auto it = mempoolPtr->find(txHash);
      if (it != mempoolPtr->end()) {
        // Found the unconfirmed TxBlock object already built and in the mirror mempool, so reuse it
        txs.push_back(it->second);
        // And now remove it from the unconfirmed txs list since it has been included in a block, which is final
        mempoolPtr->erase(it);
      } else {
        SLOGTRACE("WARNING! Transaction not found in mirror mempool: " + txHash.hex().get());
        // This recomputes txHash, but since we don't really expect this to ever execute (since txs included
        // in a new block should always have been in the mirror mempool first as unconfirmed txs), that's fine.
        txs.push_back(std::make_shared<TxBlock>(block.txs[i], requiredChainId, false));
      }
    }
  }

  auto txMerkleRoot = Merkle(txs).getRoot();

  uint64_t timestamp = block.timeNanos / 1000; // FinalizedBlock uses microseconds, so convert nanos to micros
  uint64_t nHeight = block.height;

  Hash hash(block.hash);

  return {
    std::move(proposerAddr),
    std::move(prevBlockHash),
    std::move(txMerkleRoot),
    timestamp,
    nHeight,
    std::move(txs),
    std::move(hash),
    std::move(randomness)
  };
}

FinalizedBlock FinalizedBlock::fromRPC(const json& ret) {
  if (!ret.is_object() || !ret.contains("result") || !ret["result"].is_object()) {
    throw DynamicException("Invalid block data (result)");
  }
  const auto& result = ret["result"];

  if (!result.contains("block_id") || !result["block_id"].is_object()) {
    throw DynamicException("Invalid block data (block_id)");
  }
  const auto& block_id = result["block_id"];

  if (!result.contains("block") || !result["block"].is_object()) {
    throw DynamicException("Invalid block data (block)");
  }
  const auto& block = result["block"];

  if (!block.contains("header") || !block["header"].is_object()) {
    throw DynamicException("Invalid block data (header)");
  }
  const auto& header = block["header"];

  if (!header.contains("last_block_id") || !header["last_block_id"].is_object()) {
    throw DynamicException("Invalid block data (last_block_id)");
  }
  const auto& last_block_id = header["last_block_id"];

  if (!block.contains("data") || !block["data"].is_object()) {
    throw DynamicException("Invalid block data (data)");
  }
  const auto& data = block["data"];

  // Deserializing transactions
  Hash randomness;
  uint64_t requiredChainId = 0; // FIXME
  std::vector<std::shared_ptr<TxBlock>> txs;
  if (data.contains("txs") && data["txs"].is_array()) {
    // There is data.txs in the response, so unpack the transactions
    const auto& dataTxs = data["txs"];
    size_t dataTxsSize = dataTxs.size();

    // last Tx is the randomhash
    if (dataTxsSize == 0) {
      throw DynamicException("Invalid RPC block: empty tx list (no block randomness)");
    }
    const auto& randomHashTx = dataTxs[dataTxsSize-1]; // last tx is the randomhash tx
    std::string randomHashBase64 = randomHashTx.get<std::string>();
    Bytes randomHashBytes = base64::decode_into<Bytes>(randomHashBase64);
    if (randomHashBytes.size() != 32) {
      throw DynamicException("Invalid RPC block: last tx is not 32 bytes long (bad block randomness)");
    }
    randomness = Hash(randomHashBytes);

    // NOTE: skip last tx (-1) since that is not a real tx but the block randomness hash
    for (uint64_t i = 1; i < dataTxsSize-1; ++i) {
      const auto& tx = dataTxs[i];
      if (tx.is_string()) {
        // Compute tx Bytes from the base64-encoded tx data string
        std::string txBase64 = tx.get<std::string>();
        Bytes txBytes = base64::decode_into<Bytes>(txBase64);
        // Create TxBlock object from tx Bytes
        // We can skip signature verification because this is retrieving a finalized block
        // from CometBFT node storage; signatures for all txs have been validated long ago.
        txs.push_back(std::make_shared<TxBlock>(txBytes, requiredChainId, false));
      } else {
        throw DynamicException("Invalid block data (tx)");
      }
    }
  } else {
    throw DynamicException("Invalid RPC block: no tx list (no block randomness)");
  }

  if (!last_block_id.contains("hash") || !last_block_id["hash"].is_string()) {
    throw DynamicException("Invalid block data (last_block_id hash)");
  }

  std::string lastBlockHashStr = last_block_id["hash"].get<std::string>();
  Hash prevBlockHash;
  if (!lastBlockHashStr.empty()) { // may be "" (for block at height == 1)
    prevBlockHash = Hash(Hex::toBytes(lastBlockHashStr));
  }

  if (!header.contains("proposer_address") || !header["proposer_address"].is_string()) {
    throw DynamicException("Invalid block data (proposer_address)");
  }

  std::string proposerAddrStr = header["proposer_address"].get<std::string>();
  Address proposerAddr(Hex::toBytes(proposerAddrStr));

  if (!header.contains("height") || !header["height"].is_string()) {
    throw DynamicException("Invalid block data (height)");
  }

  uint64_t nHeight = std::stoull(header["height"].get<std::string>());

  if (!header.contains("time") || !header["time"].is_string()) {
    throw DynamicException("Invalid block data (time)");
  }

  uint64_t timestamp = Utils::stringToNanos(header["time"].get<std::string>());
  timestamp /= 1000; // FinalizedBlock uses microseconds, so convert nanos to micros

  if (!block_id.contains("hash") || !block_id["hash"].is_string()) {
    throw DynamicException("Invalid block data (block_id hash)");
  }
  std::string blockIdHashStr = block_id["hash"].get<std::string>();

  Hash hash(Hex::toBytes(blockIdHashStr));

  auto txMerkleRoot = Merkle(txs).getRoot();

  return {
    std::move(proposerAddr),
    std::move(prevBlockHash),
    std::move(txMerkleRoot),
    timestamp,
    nHeight,
    std::move(txs),
    std::move(hash),
    std::move(randomness)
  };
}
