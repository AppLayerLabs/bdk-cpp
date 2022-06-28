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
      // Copy transaction size.
      txSizeBytes = rawTransactions.substr(nextTx, 4);
      uint32_t txSize = Utils::bytesToUint32(txSizeBytes);
      // Copy transacion itself.
      txBytes = txBytes.substr(nextTx + 4, txSize);
      nextTx = nextTx + 4 + txSize;
      this->_transactions.push_back(dev::eth::TransactionBase(txBytes, dev::eth::CheckTransaction::None));
    }
  } catch (std::exception &e) {
    Utils::LogPrint(Log::block, __func__, "Error: " + std::string(e.what()) + " " + dev::toHex(blockData));
    throw "";
  }
}

std::string Block::serializeToBytes() {
  // Raw Block = prevBlockHash + timestamp + nHeight + txCount + [ txSize, tx, ...]
  std::string ret;
  std::string prevBlockHashBytes = Utils::uint256ToBytes(this->_prevBlockHash);
  std::string timestampBytes = Utils::uint64ToBytes(this->_timestamp);
  std::string nHeightBytes = Utils::uint64ToBytes(this->_nHeight);
  std::string txCountBytes = Utils::uint32ToBytes(this->_txCount);
  // Append Header.
  ret += prevBlockHashBytes;
  ret += timestampBytes;
  ret += nHeightBytes;
  ret += txCountBytes;

  // Append Transactions
  // For each transaction, we need to parse both their size and their data.
  for (auto transaction : this->_transactions) {
    bytes txBytes = transaction.rlp(dev::eth::IncludeSignature::WithSignature);
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += txSizeBytes;
    std::copy(txBytes.begin(), txBytes.end(), std::back_inserter(ret));
  }

  return ret;
}

std::string Block::getBlockHash() {
  auto blockHash = dev::sha3(this->serializeToBytes());
  std::string ret;
  std::copy(blockHash.begin(), blockHash.end(), std::back_inserter(ret));
  return ret;
}

const uint64_t Block::blockSize() {
  uint64_t ret = 0;
  // Prev Block Hash + Timestamp + nHeight + txCount + [ txSize, tx, ...]
  // 32 + 8 + 8 + 4 + Nx[4 + Ny]
  ret = ret + 32 + 8 + 8 + 4;
  // Append tx's size.
  for (auto transaction : this->_transactions) {
    // This is not optimized, it will be fixed later.
    ret = ret + 4 + transaction.rlp(dev::eth::IncludeSignature::WithSignature).size();
  }
  return ret;
}

bool Block::appendTx(dev::eth::TransactionBase &tx) {
  if (this->finalized) {
    Utils::LogPrint(Log::block, __func__, " Block is finalized.");
    return false;
  }
  this->_transactions.emplace_back(tx);
  return true;
}

bool Block::finalizeBlock() {
  if (this->finalized) {
    return false;
  }
  this->finalized = true;
  return true;
}