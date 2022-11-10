#include "block.h"

Block::Block(const std::string_view &blockData, bool fromDB) {
  // Split the block data into different byte arrays.
  try {
    /*
      Hash _prevBlockHash;
    uint64_t _timestamp; // Timestamp in nanoseconds
    uint64_t _nHeight;
    Signature validatorSignature;
    uint64_t _txCount;
    uint64_t _txValidatorsCount;*/
    std::string_view prevBlockHashBytes; // uint256_t
    std::string_view timestampBytes;     // uint64_t
    std::string_view nHeightBytes;       // uint64_t
    std::string_view validatorSignatureBytes; // 65 bytes
    std::string_view txCountBytes;       // uint64_t
    std::string_view txValidatorCountBytes;   // uint64_t
    uint64_t txValidatorStart;
    uint64_t txStart;
    this->finalized = true;
    this->indexed = true;

    prevBlockHashBytes = blockData.substr(0, 32);
    timestampBytes = blockData.substr(32, 8);
    nHeightBytes = blockData.substr(32 + 8, 8);
    validatorSignatureBytes = blockData.substr(32 + 8 + 8, 65);
    txCountBytes = blockData.substr(32 + 8 + 8 + 65, 8);
    txValidatorCountBytes = blockData.substr(32 + 8 + 8 + 65 + 8, 8);
    txValidatorStart = Utils::bytesToUint64(blockData.substr(32 + 8 + 8 + 65 + 8 + 8, 8));
    txStart = Utils::bytesToUint64(blockData.substr(32 + 8 + 8 + 65 + 8 + 8 + 8, 8));

    // String view to create a reference to a substring of the block data, no copy!
    std::string_view rawValidatorTransactions(&blockData[txValidatorStart], txStart - txValidatorStart);
    std::string_view rawTransactions(&blockData[txStart], blockData.size() - txStart);
    this->_prevBlockHash = Hash(prevBlockHashBytes);
    this->_timestamp = Utils::bytesToUint64(timestampBytes);
    this->_nHeight = Utils::bytesToUint64(nHeightBytes);
    this->_validatorSignature = Signature(validatorSignatureBytes);
    this->_txCount = Utils::bytesToUint64(txCountBytes);
    this->_txValidatorsCount = Utils::bytesToUint64(txValidatorCountBytes);

    // Parse and push transactions into block.
    // Parsing validator transactions are not multithreaded because there isn't many of them.

    {
      uint64_t nextTx = 0;
      for (uint32_t i = 0; i < this->_txValidatorsCount; ++i) {
        std::string txSizeBytes;
        // Copy the transaction size.
        txSizeBytes = rawValidatorTransactions.substr(nextTx, 4);
        uint32_t txSize = Utils::bytesToUint32(txSizeBytes);
        // Copy the transaction itself.
        std::string_view txBytes(&rawValidatorTransactions.data()[nextTx + 4], txSize);
        this->_validatorTransactions[i] = Tx::Base(txBytes, false);
        nextTx = nextTx + 4 + txSize;
      }
    }

    // Transactions parsing through blocks is multithreaded because the upper limit is 400k tx per block
    // If tx count is less than 1000, just use one thread as spawning thread can slow down things.
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

      this->_transactions.rehash(this->_txCount);
      this->_transactions.reserve(this->_txCount);

      // TODO: What happens if a thread fails? 
      // Figure out a way to stop the other threads if one fails. 
      for (uint64_t i = 0; i < processor_count; ++i) {
        txThreads.emplace_back([&,i,nextTx,index] () mutable {
          for (uint64_t j = 0; j < workPerThread[i]; ++j) {
            std::string txSizeBytes;
            // Get tx size.
            uint32_t txSize = Utils::bytesToUint32(rawTransactions.substr(nextTx, 4));
            // String view so we don't copy.
            std::string_view txBytes(&rawTransactions.data()[nextTx + 4], txSize);
            // push tx to block.
            auto transaction = Tx::Base(txBytes, fromDB);
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

std::string Block::serializeToBytes(bool db) const {
  // Raw Block = prevBlockHash + timestamp + nHeight + txCount + [ txSize, tx, ... ]
  std::string ret;

  // Append header.
  ret += this->_prevBlockHash.get();
  ret += Utils::uint64ToBytes(this->_timestamp);
  ret += Utils::uint64ToBytes(this->_nHeight);
  ret += this->_validatorSignature.get();
  ret += Utils::uint64ToBytes(this->_txValidatorsCount);
  ret += Utils::uint64ToBytes(this->_txCount);
  uint64_t txValidatorStart = ret.size() + 16;
  ret += Utils::uint64ToBytes(txValidatorStart);
  uint64_t txStartLocation = ret.size();
  ret += Utils::uint64ToBytes(0); // 8 bytes, will be appended later after txs are serialized.


  // Append validator transactions - parse both size and data for each transaction.
  for (uint64_t i = 0; i < this->_txValidatorsCount; ++i) {
    std::string txBytes = (db) ? this->_validatorTransactions.find(i)->second.serialize() : this->_validatorTransactions.find(i)->second.rlpSerialize(true);
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += std::move(txSizeBytes);
    ret += std::move(txBytes);
  }
  uint64_t txStart = ret.size();
  // Append txStart location.
  std::memcpy(&ret[txStartLocation], &txStart, 8);
  // Append transactions - parse both size and data for each transaction.
  for (uint64_t i = 0; i < this->_txCount; ++i) {
    std::string txBytes = (db) ? this->_transactions.find(i)->second.serialize() : this->_transactions.find(i)->second.rlpSerialize(true);
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += std::move(txSizeBytes);
    ret += std::move(txBytes);
  }
  return ret;
}

Hash Block::getBlockHash() const {
  return Utils::sha3(this->serializeToBytes(false));
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

bool Block::appendValidatorTx(const Tx::Base &tx) {
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

