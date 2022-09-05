#include "block.h"

Block::Block(const std::string &blockData) {
  // Split the block data into different byte arrays.
  try {
    std::string prevBlockHashBytes; // uint256_t
    std::string timestampBytes;     // uint64_t
    std::string nHeightBytes;       // uint64_t
    std::string txArraySizeBytes;   // uint32_t
    this->finalized = true;
    this->indexed = true;

    prevBlockHashBytes = blockData.substr(0, 32);
    timestampBytes = blockData.substr(32, 8);
    nHeightBytes = blockData.substr(32 + 8, 8);
    txArraySizeBytes = blockData.substr(32 + 8 + 8, 4);
    // String view to create a reference to a substring of the block data, no copy!
    std::string_view rawTransactions(&blockData[32 + 8 + 8 + 4], blockData.size() - (32 + 8 + 8 + 4));
    this->_prevBlockHash = Utils::bytesToUint256(prevBlockHashBytes);
    this->_timestamp = Utils::bytesToUint64(timestampBytes);
    this->_nHeight = Utils::bytesToUint64(nHeightBytes);
    this->_txCount = Utils::bytesToUint32(txArraySizeBytes);

    // Parse and push transactions into block.
    // Transactions parsing through blocks is multithreaded.
    // If tx count is less than 1000, just use one thread as spawning thread can slow down things.
    uint64_t maxIndex = 0;
    if (this->_txCount < 1000) {
      uint64_t nextTx = 0;
      for (uint32_t i = 0; i < this->_txCount; ++i) {
        std::string txSizeBytes;
        // Copy the transaction size.
        txSizeBytes = rawTransactions.substr(nextTx, 4);
        uint32_t txSize = Utils::bytesToUint32(txSizeBytes);
        // Copy the transaction itself.
        std::string_view txBytes(&rawTransactions.data()[nextTx + 4], txSize);
        this->_transactions[i] = Tx::Base(txBytes, false);
        nextTx = nextTx + 4 + txSize;
      }
    } else {
      const auto processor_count = std::thread::hardware_concurrency();
      std::vector<std::thread> txThreads;
      std::mutex transactionLock;
      std::vector<uint64_t> workPerThread(processor_count, this->_txCount / processor_count);
      // Push remainder to the last thread.
      workPerThread[processor_count - 1] += this->_txCount % processor_count;

      uint64_t nextTx = 0;
      uint64_t index = 0;
      for (uint64_t i = 0; i < processor_count; ++i) {
        txThreads.emplace_back([&,i,nextTx,index] () mutable {
          for (uint64_t j = 0; j < workPerThread[i]; ++j) { 
            std::string txSizeBytes;
            // Get tx size.
            uint32_t txSize = Utils::bytesToUint32(rawTransactions.substr(nextTx, 4));
            // String view so we don't copy.
            std::string_view txBytes(&rawTransactions.data()[nextTx + 4], txSize);
            // push tx to block.
            auto transaction = Tx::Base(txBytes, false);
            transactionLock.lock();
            this->_transactions[index] = std::move(transaction);
            transactionLock.unlock();
            nextTx = nextTx + 4 + txSize;
            ++index;
          }
        });
        // Calculate next tx offset for next thread.
        for (uint64_t j = 0; j < workPerThread[i]; ++j) {
          uint32_t txSize = Utils::bytesToUint32(rawTransactions.substr(nextTx, 4));
          nextTx = nextTx + 4 + txSize;
          ++index;
        }
      }

      // Figure out thread start on the rawTransactions string.
      // Join all threads.
      for (auto &thread : txThreads) {
        thread.join();
      }
    }

  } catch (std::exception &e) {
    Utils::LogPrint(Log::block, __func__, "Error: " + std::string(e.what()) + " " + dev::toHex(blockData));
    throw std::runtime_error(std::string(__func__) + ": " + e.what());
  }
}

std::string Block::serializeToBytes() const {
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
   * TODO: there two uneeded copies here, we can optimize this.
   */
  for (uint64_t i = 0; i < this->_txCount; ++i) {
    std::string txBytes = _transactions.find(i)->second.rlpSerialize(true);
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += std::move(txSizeBytes);
    ret += std::move(txBytes);
  }
  return ret;
}

std::string Block::getBlockHash() const {
  std::string ret;
  Utils::sha3(this->serializeToBytes(), ret);
  return ret;
}

uint64_t Block::blockSize() const {
  uint64_t ret = 32 + 8 + 8 + 4; // prevBlockHash + timestamp + nHeight + txCount
  for (auto const &transaction : this->_transactions) { // + [ txSize, tx, ... ]
    ret += 4 + transaction.second.serialize().size();
  }
  return ret;
}

bool Block::appendTx(const Tx::Base &tx) {
  if (this->finalized) {
    Utils::LogPrint(Log::block, __func__, " Block is finalized.");
    return false;
  }
  this->_transactions[_txCount] = (std::move(tx));
  _txCount++;
  return true;
}

void Block::indexTxs() {
  Utils::LogPrint(Log::block, __func__, "Indexing transactions...");
  if (this->finalized) {
    if (this->indexed) {
      Utils::LogPrint(Log::block, __func__, " Block is in chain and txs are already indexed.");
      return;
    }
    uint64_t index = 0;
    for (auto &tx : this->_transactions) {
      tx.second.setBlockIndex(index);
      ++index;
    }
    this->indexed = true;
    Utils::LogPrint(Log::block, __func__, "Indexing transactions... done");
  } else {
    Utils::LogPrint(Log::block, __func__, " Block is not finalized. cannot index transactions, ignoring call.");
  }
}

bool Block::finalizeBlock() {
  if (this->finalized) {
    return false;
  }
  this->finalized = true;
  return true;
}

