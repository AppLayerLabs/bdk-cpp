#include "block.h"

Block::Block(const std::string &blockData) {
  // Split the block data into different byte arrays.
  try {
    std::string prevBlockHashBytes; // uint256_t
    std::string timestampBytes;     // uint64_t
    std::string nHeightBytes;       // uint64_t
    std::string txArraySizeBytes;   // uint32_t
    std::string rawTransactions;    // N

    prevBlockHashBytes = blockData.substr(0, 32);
    timestampBytes = blockData.substr(32, 8);
    nHeightBytes = blockData.substr(32 + 8, 8);
    txArraySizeBytes = blockData.substr(32 + 8 + 8, 4);
    rawTransactions = blockData.substr(32 + 8 + 8 + 4);
    this->_prevBlockHash = Utils::bytesToUint256(prevBlockHashBytes);
    this->_timestamp = Utils::bytesToUint64(timestampBytes);
    this->_nHeight = Utils::bytesToUint64(nHeightBytes);
    this->_txCount = Utils::bytesToUint32(txArraySizeBytes);

    // Parse and push transactions into block.
    uint64_t nextTx = 0;
    for (uint32_t i = 0; i < this->_txCount; ++i) {
      std::string txBytes;
      std::string txSizeBytes;
      // Copy the transaction size.
      txSizeBytes = rawTransactions.substr(nextTx, 4);
      uint32_t txSize = Utils::bytesToUint32(txSizeBytes);
      // Copy the transaction itself.
      txBytes = rawTransactions.substr(nextTx + 4, txSize);

      nextTx = nextTx + 4 + txSize;
      // We consider loading transactions from the block the same as reading from DB
      this->_transactions.push_back(Tx::Base(txBytes, true));
    }
  } catch (std::exception &e) {
    Utils::LogPrint(Log::block, __func__, "Error: " + std::string(e.what()) + " " + dev::toHex(blockData));
    throw;
  }
}

std::string Block::serializeToBytes() {
  // Raw Block = prevBlockHash + timestamp + nHeight + txCount + [ txSize, tx, ... ]
  std::string ret;
  std::string prevBlockHashBytes = Utils::uint256ToBytes(this->_prevBlockHash);
  std::string timestampBytes = Utils::uint64ToBytes(this->_timestamp);
  std::string nHeightBytes = Utils::uint64ToBytes(this->_nHeight);
  std::string txCountBytes = Utils::uint32ToBytes(this->_txCount);

  // Append header.
  ret += prevBlockHashBytes;
  ret += timestampBytes;
  ret += nHeightBytes;
  ret += txCountBytes;

  /**
   * Append transactions.
   * For each transaction we need to parse both their size and their data.
   */
  for (auto transaction : this->_transactions) {
    std::string txBytes = transaction.serialize();
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += txSizeBytes;
    std::copy(txBytes.begin(), txBytes.end(), std::back_inserter(ret));
  }
  return ret;
}

std::string Block::getBlockHash() {
  std::string ret;
  Utils::sha3(this->serializeToBytes(), ret);
  return ret;
}

const uint64_t Block::blockSize() {
  uint64_t ret = 32 + 8 + 8 + 4; // prevBlockHash + timestamp + nHeight + txCount
  for (auto transaction : this->_transactions) { // + [ txSize, tx, ... ]
    ret += 4 + transaction.serialize().size();
  }
  return ret;
}

bool Block::appendTx(Tx::Base &tx) {
  if (this->finalized) {
    Utils::LogPrint(Log::block, __func__, " Block is finalized.");
    return false;
  }
  this->_transactions.emplace_back(tx);
  ++_txCount;
  return true;
}

void Block::indexTxs() {
  Utils::LogPrint(Log::block, __func__, "Indexing transactions...");
  if (this->finalized) {
    if (this->inChain) {
      Utils::LogPrint(Log::block, __func__, " Block is in chain and txs are already indexed.");
      return;
    }
    uint64_t index = 0;
    for (auto &tx : this->_transactions) {
      tx.setBlockIndex(index);
      ++index;
    }
    this->inChain = true;
    Utils::LogPrint(Log::block, __func__, "Indexing transactions... done");
  } else {
    Utils::LogPrint(Log::block, __func__, " Block is not finalized. cannot index transactions, ignoring call.");
  }
}

bool Block::finalizeBlock() {
  if (this->finalized) return false;
  this->finalized = true;
  return true;
}

