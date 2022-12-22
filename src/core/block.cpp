#include "block.h"
#include "blockmanager.h"

Block::Block(const std::string_view &blockData, bool fromDB) {
  // Split the block data into different byte arrays.
  try {
    uint64_t txValidatorStart;
    uint64_t txStart;
    this->finalized = true;
    this->indexed = true;
    this->_validatorSignature = Signature(blockData.substr(0, 65));
    this->_prevBlockHash = Hash(blockData.substr(65, 32));
    this->_randomness = Hash(blockData.substr(65 + 32, 32));
    this->_validatorMerkleRoot = Hash(blockData.substr(65 + 32 + 32, 32));
    this->_transactionMerkleRoot = Hash(blockData.substr(65 + 32 + 32 + 32, 32));
    this->_timestamp = Utils::bytesToUint64(blockData.substr(65 + 32 + 32 + 32 + 32, 8));
    this->_nHeight = Utils::bytesToUint64(blockData.substr(65 + 32 + 32 + 32 + 32 + 8, 8));
    this->_txValidatorsCount = Utils::bytesToUint64(blockData.substr(65 + 32 + 32 + 32 + 32 + 8 + 8, 8));
    this->_txCount = Utils::bytesToUint64(blockData.substr(65 + 32 + 32 + 32 + 32 + 8 + 8 + 8, 8));
    txValidatorStart = Utils::bytesToUint64(blockData.substr(65 + 32 + 32 + 32 + 32 + 8 + 8 + 8 + 8, 8));
    txStart = Utils::bytesToUint64(blockData.substr(65 + 32 + 32 + 32 + 32 + 8 + 8 + 8 + 8 + 8, 8));

    // String view to create a reference to a substring of the block data, no copy!
    std::string_view rawValidatorTransactions(&blockData[txValidatorStart], txStart - txValidatorStart);
    std::string_view rawTransactions(&blockData[txStart], blockData.size() - txStart);

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
        this->_validatorTransactions[i] = Tx::Validator(txBytes);
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
        if (this->_transactions[i].to() == ContractAddresses::BlockManager) {
          Utils::LogPrint(Log::block, __func__, "Error: transaction inside tx list does call blockManager.");
          throw std::runtime_error("transaction inside tx list does call blockManager.");
        }
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
            if (transaction.to() == ContractAddresses::BlockManager) {
              Utils::LogPrint(Log::block, __func__, "Error: transaction inside tx list does call blockManager.");
              throw std::runtime_error("transaction inside tx list does call blockManager.");
            }
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

    // Check merkle roots and randomness.
    auto validatorMerkleRoot = Merkle(this->_validatorTransactions).root();
    Utils::LogPrint(Log::block, __func__, std::string("validatorMerkleRoot: ") + validatorMerkleRoot.hex());
    if (this->_validatorMerkleRoot != Merkle(this->_validatorTransactions).root()) {
      Utils::LogPrint(Log::block, __func__, "Error: Validator merkle root does not match. expected: " 
                      + this->_validatorMerkleRoot.hex() + " got: " + Merkle(this->_validatorTransactions).root().hex() + " tx size: " + std::to_string(this->_validatorTransactions.size()));
      throw std::runtime_error("Validator merkle root does not match.");
    }

    if (this->_transactionMerkleRoot != Merkle(this->_transactions).root()) {
      Utils::LogPrint(Log::block, __func__, "Error: Transaction merkle root does not match.");
      throw std::runtime_error("Transaction merkle root does not match.");
    }

    if (this->_randomness != BlockManager::parseTxListSeed(this->_validatorTransactions)) {
      Utils::LogPrint(Log::block, __func__, "Error: Randomness does not match.");
      throw std::runtime_error("Randomness does not match.");
    }

    // Check if signature is valid.
    auto messageHash = this->getBlockHash();
    auto pubkey = Secp256k1::recover(this->_validatorSignature, messageHash);

    // TODO: Re-enable this after finishing blockManager
    //if (!Secp256k1::verify(pubkey, this->_validatorSignature, messageHash)) {
    //  Utils::LogPrint(Log::block, __func__, "Error: Signature is not valid.");
    //  throw std::runtime_error("Signature is not valid.");
    //}
  } catch (std::exception &e) {
    Utils::LogPrint(Log::block, __func__, "Error: " + std::string(e.what()) + " " + dev::toHex(blockData));
    throw std::runtime_error(std::string(__func__) + ": " + e.what());
  }
}

std::string Block::serializeHeader() const {
  // Header = prevBlockHash + blockRandomness + validatorMerkleRoot
  // + transactionMerkleRoot + timestamp + nHeight
  std::string ret;
  ret += this->_prevBlockHash.get();
  ret += this->_randomness.get();
  ret += this->_validatorMerkleRoot.get();
  ret += this->_transactionMerkleRoot.get();
  ret += Utils::uint64ToBytes(this->_timestamp);
  ret += Utils::uint64ToBytes(this->_nHeight);
  return ret;
}

std::string Block::serializeToBytes(bool db) const {
  // Raw Block = prevBlockHash + timestamp + nHeight + txCount + [ txSize, tx, ... ]
  std::string ret;

  ret += this->_validatorSignature.get(); // Append Signature
  ret += this->serializeHeader(); // Append Header
  ret += Utils::uint64ToBytes(this->_txValidatorsCount);
  ret += Utils::uint64ToBytes(this->_txCount);
  uint64_t txValidatorStart = ret.size() + 16;
  ret += Utils::uint64ToBytes(txValidatorStart);
  uint64_t txStartLocation = ret.size();
  ret += Utils::uint64ToBytes(0); // 8 bytes, will be appended later after txs are serialized

  // Append validator transactions - parse both size and data for each transaction
  for (uint64_t i = 0; i < this->_txValidatorsCount; ++i) {
    std::string txBytes = this->_validatorTransactions.find(i)->second.rlpSerialize(true);
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += std::move(txSizeBytes);
    ret += std::move(txBytes);
  }
  uint64_t txStart = ret.size();
  std::memcpy(&ret[txStartLocation], &txStart, 8);  // Append txStart location
  // Append transactions - parse both size and data for each transaction
  for (uint64_t i = 0; i < this->_txCount; ++i) {
    std::string txBytes = (db) ? this->_transactions.find(i)->second.serialize() : this->_transactions.find(i)->second.rlpSerialize(true);
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += std::move(txSizeBytes);
    ret += std::move(txBytes);
  }
  return ret;
}

Hash Block::getBlockHash() const {
  return Utils::sha3(this->serializeHeader());  // HASH = SHA3(HEADER)
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
    Utils::LogPrint(Log::block, __func__, "Block is finalized.");
    return false;
  }
  this->_transactions[_txCount] = (tx);
  _txCount++;
  return true;
}

bool Block::appendValidatorTx(const Tx::Validator &tx) {
  if (this->finalized) {
    Utils::LogPrint(Log::block, __func__, "Block is finalized.");
    return false;
  }
  this->_validatorTransactions[_txValidatorsCount] = tx;
  _txValidatorsCount++;
  return true;
}

void Block::indexTxs() {
  Utils::LogPrint(Log::block, __func__, "Indexing transactions...");
  if (this->finalized) {
    if (this->indexed) {
      Utils::LogPrint(Log::block, __func__, "Block is in chain and txs are already indexed.");
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
    Utils::LogPrint(Log::block, __func__, "Block is not finalized. Cannot index transactions, ignoring call.");
  }
}

bool Block::finalizeBlock(const PrivKey &validatorPrivKey) {
  if (this->finalized) return false;

  this->_validatorMerkleRoot = Merkle(this->_validatorTransactions).root();
  this->_transactionMerkleRoot = Merkle(this->_transactions).root();
  this->_randomness = BlockManager::parseTxListSeed(_validatorTransactions);
  this->_validatorSignature = Secp256k1::sign(validatorPrivKey, this->getBlockHash());
  this->finalized = true;
  this->indexTxs();
  return true;
}