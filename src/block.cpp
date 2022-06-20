#include "block.h"

Block::Block(const std::vector<uint8_t> &blockData) {
  // Split the block data into different vectors.

  std::vector<uint8_t> prevBlockHashBytes; // uint256_t
  std::vector<uint8_t> timestampBytes;     // uint64_t
  std::vector<uint8_t> nHeightBytes;       // uint64_t
  std::vector<uint8_t> txArraySizeBytes;   // uint32_t
  std::vector<uint8_t> rawTransactions;    // N

  std::copy(blockData.begin(), blockData.begin() + 32, std::back_inserter(prevBlockHashBytes));
  std::copy(blockData.begin() + 32, blockData.begin() + 32 + 8, std::back_inserter(timestampBytes));
  std::copy(blockData.begin() + 32 + 8, blockData.begin() + 32 + 8 + 8, std::back_inserter(nHeightBytes));
  std::copy(blockData.begin() + 32 + 8 + 8, blockData.begin() + 32 + 8 + 8 + 4, std::back_inserter(txArraySizeBytes));
  std::copy(blockData.begin() + 32 + 8 + 8 + 4, blockData.end(), std::back_inserter(rawTransactions));

  this->_prevBlockHash = Utils::bytesToUint256(prevBlockHashBytes);
  this->_timestamp = Utils::bytesToUint64(timestampBytes);
  this->_nHeight = Utils::bytesToUint64(nHeightBytes);
  this->_txCount = Utils::bytesToUint32(txArraySizeBytes);

  // Parse and push transactions into block.
  uint64_t nextTx = 0;
  for (uint32_t i = 0; i < this->_txCount; ++i) {
    std::vector<uint8_t> txBytes; 
    std::vector<uint8_t> txSizeBytes;
    // Copy transaction size.
    std::copy(rawTransactions.begin() + nextTx, rawTransactions.begin() + nextTx + 4, std::back_inserter(txSizeBytes));
    uint32_t txSize = Utils::bytesToUint32(txSizeBytes);
    // Copy transacion itself.
    std::copy(rawTransactions.begin() + nextTx + 4, rawTransactions.begin() + nextTx + 4 + txSize, std::back_inserter(txBytes));
    nextTx = nextTx + 4 + txSize;

    this->_transactions.push_back(dev::eth::TransactionBase(txBytes, dev::eth::CheckTransaction::None));
  }
}

std::vector<uint8_t> Block::serializeToBytes() {
  // Raw Block = prevBlockHash + timestamp + nHeight + txCount + [ txSize, tx, ...]
  std::vector<uint8_t> ret;
  std::vector<uint8_t> prevBlockHashBytes = Utils::uint256ToBytes(this->_prevBlockHash);
  std::vector<uint8_t> timestampBytes = Utils::uint64ToBytes(this->_timestamp);
  std::vector<uint8_t> nHeightBytes = Utils::uint64ToBytes(this->_nHeight);
  std::vector<uint8_t> txCountBytes = Utils::uint32ToBytes(this->_txCount);
  // Append Header.
  std::copy(prevBlockHashBytes.begin(), prevBlockHashBytes.end(), std::back_inserter(ret));
  std::copy(timestampBytes.begin(), timestampBytes.end(), std::back_inserter(ret));
  std::copy(nHeightBytes.begin(), nHeightBytes.end(), std::back_inserter(ret));
  std::copy(txCountBytes.begin(), txCountBytes.end(), std::back_inserter(ret));

  // Append Transactions
  // For each transaction, we need to parse both their size and their data.
  for (auto transaction : this->_transactions) {
    std::vector<uint8_t> txBytes = transaction.rlp(dev::eth::IncludeSignature::WithSignature);
    std::vector<uint8_t> txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    std::copy(txSizeBytes.begin(), txSizeBytes.end(), std::back_inserter(ret));
    std::copy(txBytes.begin(), txBytes.end(), std::back_inserter(ret));
  }

  return ret;
}

std::array<uint8_t,32> Block::getBlockHash() {
  return dev::sha3(this->serializeToBytes()).asArray();
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