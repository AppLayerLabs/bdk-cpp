#include "block.h"

Block::Block(std::string_view& rawData, bool fromDB) {
  // Split block data into different byte arrays
  try {
    this->finalized = true;
    this->validatorSig = Signature(rawData.substr(0, 65));
    this->prevBlockHash = Hash(rawData.substr(65, 32));
    this->randomness = Hash(rawData.substr(65 + 32, 32));
    this->validatorTxMerkleRoot = Hash(rawData.substr(65 + 32 + 32, 32));
    this->txMerkleRoot = Hash(rawData.substr(65 + 32 + 32 + 32, 32));
    this->timestamp = Utils::bytesToUint64(rawData.substr(65 + 32 + 32 + 32 + 32, 8));
    this->nHeight = Utils::bytesToUint64(rawData.substr(65 + 32 + 32 + 32 + 32 + 8, 8));
    this->validatorTxCount = Utils::bytesToUint64(rawData.substr(65 + 32 + 32 + 32 + 32 + 8 + 8, 8));
    this->txCount = Utils::bytesToUint64(rawData.substr(65 + 32 + 32 + 32 + 32 + 8 + 8 + 8, 8));
    uint64_t validatorTxStart = Utils::bytesToUint64(rawData.substr(65 + 32 + 32 + 32 + 32 + 8 + 8 + 8 + 8, 8));
    uint64_t txStart = Utils::bytesToUint64(rawData.substr(65 + 32 + 32 + 32 + 32 + 8 + 8 + 8 + 8 + 8, 8));

    // String view to create a reference to a substring of the block data, no copy!
    std::string_view rawValidatorTxs(&rawData[validatorTxStart], txStart - validatorTxStart);
    std::string_view rawTxs(&rawData[txStart], rawData.size() - txStart);

    // Parse and push txs into block.
    // Parsing Validator txs is not multithreaded because there isn't that many
    {
      uint64_t nextTx = 0;
      for (uint32_t i = 0; i < this->validatorTxCount; i++) {
        // Copy the tx size, then the tx itself
        std::string txSizeBytes = rawValidatorTxs.substr(nextTx, 4);
        uint32_t txSize = Utils::bytesToUint32(txSizeBytes);
        std::string_view txBytes(&rawValidatorTxs.data()[nextTx + 4], txSize);
        this->validatorTxs[i] = TxValidator(txBytes);
        nextTx += txSize + 4;
      }
    }

    // Txs parsing through blocks is multithreaded because the upper limit
    // is 400k tx per block. If tx count is less than 1000, we can use just
    // one thread as spawning many threads can slow things down
    if (this->txCount < 1000) {
      uint64_t nextTx = 0;
      for (uint32_t i = 0; i < this->txCount; ++i) {
        // Copy the tx size, then the tx itself
        std::string txSizeBytes = rawTxs.substr(nextTx, 4);
        uint32_t txSize = Utils::bytesToUint32(txSizeBytes);
        std::string_view txBytes(&rawTxs.data()[nextTx + 4], txSize);
        this->txs[i] = TxBlock(txBytes, false);
        if (this->txs[i].getTo() == ContractAddresses::BlockManager) {
          Utils::logToDebug(Log::block, __func__, "Error: tx inside list calls rdPoS/BlockManager");
          throw std::runtime_error("tx inside list calls rdPoS/BlockManager");
        }
        nextTx += txSize + 4;
      }
    } else {
      const auto procCt = std::thread::hardware_concurrency();  // Processor count
      std::vector<std::thread> txThreads;
      std::mutex txLock;
      std::vector<uint64_t> workPerThread(procCt, this->txCount / procCt);
      workPerThread[procCt - 1] += this->txCount % procCt;  // Push remainder to last thread
      uint64_t nextTx = 0;
      uint64_t index = 0;
      this->txs.rehash(this->txCount);
      this->txs.reserve(this->txCount);

      // TODO: What happens if a thread fails? Figure out a way to stop other threads if one fails
      for (uint64_t i = 0; i < procCt; i++) {
        txThreads.emplace_back([&, i, nextTx, index] () mutable {
          for (uint64_t j = 0; j < workPerThread[i]; ++j) {
            uint32_t txSize = Utils::bytesToUint32(rawTxs.substr(nextTx, 4)); // Get tx size
            std::string_view txBytes(&rawTxs.data()[nextTx + 4], txSize); // String view so we don't copy
            TxBlock tx = TxBlock(txBytes, fromDB);  // Push tx to block
            if (tx.getTo() == ContractAddresses::BlockManager) {
              Utils::logToDebug(Log::block, __func__, "Error: tx inside list calls rdPoS/BlockManager");
              throw std::runtime_error("tx inside list calls rdPoS/BlockManager");
            }
            txLock.lock();
            this->txs[index] = std::move(tx);
            txLock.unlock();
            nextTx += txSize + 4;
            index++;
          }
        });
        // Calculate next tx offset for next thread
        for (uint64_t j = 0; j < workPerThread[i]; ++j) {
          uint32_t txSize = Utils::bytesToUint32(rawTxs.substr(nextTx, 4));
          nextTx += txSize + 4;
          index++;
        }
      }

      // Figure out thread start on the rawTxs string and join all threads
      for (auto& t : txThreads) t.join();
    }

    // Check merkle roots and randomness
    Hash valMerkleRootHash = Merkle(this->validatorTxs).getRoot();
    Hash txMerkleRootHash = Merkle(this->txs).getRoot();
    Utils::logToDebug(Log::block, __func__,
      std::string("Validator Tx Merkle Root: ") + valMerkleRootHash.hex()
    );
    Utils::logToDebug(Log::block, __func__,
      std::string("Tx Merkle Root: ") + txMerkleRootHash.hex()
    );
    if (this->validatorTxMerkleRoot != valMerkleRootHash) {
      Utils::logToDebug(Log::block, __func__,
        "Error: Validator Tx Merkle Root does not match - expected: "
        + this->validatorTxMerkleRoot.hex() + ", got: "
        + valMerkleRootHash.hex() + ", tx size: "
        + std::to_string(this->validatorTxs.size())
      );
      throw std::runtime_error("Validator Tx Merkle Root does not match");
    }
    if (this->txMerkleRoot != txMerkleRootHash) {
      Utils::logToDebug(Log::block, __func__, "Error: Tx Merkle Root does not match");
      throw std::runtime_error("Tx Merkle Root does not match");
    }
    if (this->randomness != rdPoS::parseTxListSeed(this->validatorTxs)) {
      Utils::logToDebug(Log::block, __func__, "Error: Randomness does not match");
      throw std::runtime_error("Randomness does not match");
    }

    // Check if signature is valid
    Hash msgHash = this->getBlockHash();
    UPubkey key = Secp256k1::recover(this->validatorSig, msgHash);
    if (!Secp256k1::verify(key, this->validatorSig, msgHash)) {
      Utils::logToDebug(Log::block, __func__, "Error: Signature is not valid.");
      throw std::runtime_error("Signature is not valid.");
    }
  } catch (std::exception &e) {
    Utils::logToDebug(Log::block, __func__, "Error: "
      + std::string(e.what()) + " " + dev::toHex(rawData)
    );
    throw std::runtime_error(std::string(__func__) + ": " + e.what());
  }
}

const uint64_t Block::blockSize() {
  // ret = prevBlockHash + timestamp + nHeight + txCount
  // ret += [ txSize, tx, ... ]
  uint64_t ret = 32 + 8 + 8 + 4;
  for (const auto& tx : this->txs) ret += 4 + tx.second.rlpSerialize().size();
  return ret;
}

const std::string Block::serializeToBytes(bool fromDB) {
  /**
   * Raw Block = prevBlockHash + timestamp + nHeight
   * + txCount + [ txSize, tx, ... ]
   */
  std::string ret;

  // Append Validator Signature, header, number of txs and their offsets
  ret += this->validatorSig.get();
  ret += this->serializeHeader();
  ret += Utils::uint64ToBytes(this->validatorTxCount);
  ret += Utils::uint64ToBytes(this->txCount);
  uint64_t validatorTxStart = ret.size() + 16;
  ret += Utils::uint64ToBytes(validatorTxStart);
  uint64_t txStartLoc = ret.size();
  ret += Utils::uint64ToBytes(0); // 8 bytes, appended later after txs are serialized

  // Append Validator txs - parse both size and data for each tx
  for (uint64_t i = 0; i < this->validatorTxCount; i++) {
    std::string txBytes = this->validatorTxs.find(i)->second.rlpSerialize(true);
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += std::move(txSizeBytes);
    ret += std::move(txBytes);
  }

  // Append offset for Block txs
  uint64_t txStart = ret.size();
  std::memcpy(&ret[txStartLoc], &txStart, 8);

  // Append Block txs - parse both size and data for each transaction
  for (uint64_t i = 0; i < this->txCount; i++) {
    std::string txBytes = this->txs.find(i)->second.rlpSerialize(true, fromDB);
    std::string txSizeBytes = Utils::uint32ToBytes(txBytes.size());
    ret += std::move(txSizeBytes);
    ret += std::move(txBytes);
  }
  return ret;
}

const std::string Block::serializeHeader() {
  /**
   * Header = prevBlockHash + blockRandomness + validatorMerkleRoot
   * + transactionMerkleRoot + timestamp + nHeight
   */
  std::string ret;
  ret += this->prevBlockHash.get();
  ret += this->randomness.get();
  ret += this->validatorTxMerkleRoot.get();
  ret += this->txMerkleRoot.get();
  ret += Utils::uint64ToBytes(this->timestamp);
  ret += Utils::uint64ToBytes(this->nHeight);
  return ret;
}

bool Block::appendTx(const TxBlock& tx) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Block is finalized");
    return false;
  }
  this->txs[this->txCount] = tx;
  this->txCount++;
  return true;
}

bool Block::appendValidatorTx(const TxValidator& tx) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Block is finalized");
    return false;
  }
  this->validatorTxs[this->validatorTxCount] = tx;
  this->validatorTxCount++;
  return true;
}

bool Block::finalize(const PrivKey& validatorKey) {
  if (this->finalized) {
    Utils::logToDebug(Log::block, __func__, "Block is already finalized");
    return false;
  }
  this->validatorTxMerkleRoot = Merkle(this->validatorTxs).root();
  this->txMerkleRoot = Merkle(this->txs).root();
  this->randomness = rdPoS::parseTxListSeed(this->validatorTxs);
  this->validatorSig = Secp256k1::sign(validatorKey, this->getBlockHash());
  this->finalized = true;
  this->indexTxs();
  return true;
}

