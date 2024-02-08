/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "storage.h"

Storage::Storage(const std::unique_ptr<DB>& db, const std::unique_ptr<Options>& options) : db_(db), options_(options) {
  Logger::logToDebug(LogType::INFO, Log::storage, __func__, "Loading blockchain from DB");

  // Initialize the blockchain if latest block doesn't exist.
  initializeBlockchain();

  // Get the latest block from the database
  Logger::logToDebug(LogType::INFO, Log::storage, __func__, "Loading latest block");
  auto blockBytes = this->db_->get(Utils::stringToBytes("latest"), DBPrefix::blocks);
  Block latest(blockBytes, this->options_->getChainID());
  uint64_t depth = latest.getNHeight();
  Logger::logToDebug(LogType::INFO, Log::storage, __func__,
    std::string("Got latest block: ") + latest.hash().hex().get()
    + std::string(" - height ") + std::to_string(depth)
  );

  std::unique_lock<std::shared_mutex> lock(this->chainLock_);

  // Parse block mappings (hash -> height / height -> hash) from DB
  Logger::logToDebug(LogType::INFO, Log::storage, __func__, "Parsing block mappings");
  std::vector<DBEntry> maps = this->db_->getBatch(DBPrefix::blockHeightMaps);
  for (DBEntry& map : maps) {
    // TODO: Check if a block is missing.
    // Might be interesting to change DB::getBatch to return a map instead of a vector
    Logger::logToDebug(LogType::DEBUG, Log::storage, __func__, std::string(": ")
      + std::to_string(Utils::bytesToUint64(map.key))
      + std::string(", hash ") + Hash(map.value).hex().get()
    );
    this->blockHashByHeight_.insert({Utils::bytesToUint64(map.key),Hash(map.value)});
    this->blockHeightByHash_.insert({Hash(map.value), Utils::bytesToUint64(map.key)});
  }

  // Append up to 500 most recent blocks from DB to chain
  Logger::logToDebug(LogType::INFO, Log::storage, __func__, "Appending recent blocks");
  for (uint64_t i = 0; i <= 500 && i <= depth; i++) {
    Logger::logToDebug(LogType::DEBUG, Log::storage, __func__,
      std::string("Height: ") + std::to_string(depth - i) + ", Hash: "
      + this->blockHashByHeight_[depth - i].hex().get()
    );
    Block block(this->db_->get(this->blockHashByHeight_[depth - i].get(), DBPrefix::blocks), this->options_->getChainID());
    this->pushFrontInternal(std::move(block));
  }

  Logger::logToDebug(LogType::INFO, Log::storage, __func__, "Blockchain successfully loaded");
}

Storage::~Storage() {
  DBBatch batchedOperations;
  std::shared_ptr<const Block> latest;
  {
    std::unique_lock<std::shared_mutex> lock(this->chainLock_);
    latest = this->chain_.back();
    while (!this->chain_.empty()) {
      // Batch block to be saved to the database.
      // We can't call this->popBack() because of the mutex
      std::shared_ptr<const Block> block = this->chain_.front();
      batchedOperations.push_back(block->hash().get(), block->serializeBlock(), DBPrefix::blocks);
      batchedOperations.push_back(Utils::uint64ToBytes(block->getNHeight()), block->hash().get(), DBPrefix::blockHeightMaps);

      // Batch txs to be saved to the database and delete them from the mappings
      auto Txs = block->getTxs();
      for (uint32_t i = 0; i < Txs.size(); i++) {
        const auto TxHash = Txs[i].hash();
        Bytes value = block->hash().asBytes();
        value.reserve(value.size() + 4 + 8);
        Utils::appendBytes(value, Utils::uint32ToBytes(i));
        Utils::appendBytes(value, Utils::uint64ToBytes(block->getNHeight()));
        batchedOperations.push_back(TxHash.get(), value, DBPrefix::txToBlocks);
        this->txByHash_.erase(TxHash);
      }

      // Delete block from internal mappings and the chain
      this->blockByHash_.erase(block->hash());
      this->chain_.pop_front();
    }
  }

  // Batch save to database
  this->db_->putBatch(batchedOperations);
  this->db_->put(std::string("latest"), latest->serializeBlock(), DBPrefix::blocks);
}

void Storage::initializeBlockchain() {
  if (!this->db_->has(std::string("latest"), DBPrefix::blocks)) {
    /// Genesis block comes from Options, not hardcoded
    const auto genesis = this->options_->getGenesisBlock();
    if (genesis.getNHeight() != 0) {
      throw DynamicException("Genesis block height is not 0");
    }
    this->db_->put(std::string("latest"), genesis.serializeBlock(), DBPrefix::blocks);
    this->db_->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    this->db_->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);
    Logger::logToDebug(LogType::INFO, Log::storage, __func__,
      std::string("Created genesis block: ") + Hex::fromBytes(genesis.hash().get()).get()
    );
  }
  /// Sanity check for genesis block. (check if genesis in DB matches genesis in Options)
  const auto genesis = this->options_->getGenesisBlock();
  const auto genesisInDBHash = Hash(this->db_->get(Utils::uint64ToBytes(0), DBPrefix::blockHeightMaps));
  const auto genesisInDB = Block(this->db_->get(genesisInDBHash, DBPrefix::blocks), this->options_->getChainID());
  if (genesis != genesisInDB) {
    Logger::logToDebug(LogType::ERROR, Log::storage, __func__, "Sanity Check! Genesis block in DB does not match genesis block in Options");
    throw DynamicException("Sanity Check! Genesis block in DB does not match genesis block in Options");
  }
}

TxBlock Storage::getTxFromBlockWithIndex(const BytesArrView blockData, const uint64_t& txIndex) const {
  uint64_t index = 217; // Start of block tx range
  /// Count txs until index.
  uint64_t currentTx = 0;
  while (currentTx < txIndex) {
    uint32_t txSize = Utils::bytesToUint32(blockData.subspan(index, 4));
    index += txSize + 4;
    currentTx++;
  }
  uint64_t txSize = Utils::bytesToUint32(blockData.subspan(index, 4));
  index += 4;
  return TxBlock(blockData.subspan(index, txSize), this->options_->getChainID());
}

StorageStatus Storage::blockExistsInternal(const Hash& hash) const {
  // Check chain first, then cache, then database
  if (this->blockByHash_.contains(hash)) {
    return StorageStatus::OnChain;
  } else if (this->cachedBlocks_.contains(hash)) {
    return StorageStatus::OnCache;
  } else if (this->db_->has(hash.get(), DBPrefix::blocks)) {
    return StorageStatus::OnDB;
  } else {
    return StorageStatus::NotFound;
  }
  return StorageStatus::NotFound;
}

StorageStatus Storage::blockExistsInternal(const uint64_t& height) const {
  // Check chain first, then cache, then database
  auto it = this->blockHashByHeight_.find(height);
  if (it != this->blockHashByHeight_.end()) {
    if (this->blockByHash_.contains(it->second)) return StorageStatus::OnChain;
    if (this->cachedBlocks_.contains(it->second)) return StorageStatus::OnCache;
    return StorageStatus::OnDB;
  } else {
    return StorageStatus::NotFound;
  }
  return StorageStatus::NotFound;
}

StorageStatus Storage::txExistsInternal(const Hash& tx) const {
  // Check chain first, then cache, then database
  if (this->txByHash_.contains(tx)) {
    return StorageStatus::OnChain;
  } else if (this->cachedTxs_.contains(tx)) {
    return StorageStatus::OnCache;
  } else if (this->db_->has(tx.get(), DBPrefix::txToBlocks)) {
    return StorageStatus::OnDB;
  } else {
    return StorageStatus::NotFound;
  }
}

void Storage::pushBackInternal(Block&& block) {
  // Push the new block and get a pointer to it
  if (!this->chain_.empty()) {
    if (this->chain_.back()->hash() != block.getPrevBlockHash()) {
      throw DynamicException("Block " + block.hash().hex().get()
        + " does not have the correct previous block hash."
      );
    }
    if (block.getNHeight() != this->chain_.back()->getNHeight() + 1) {
      throw DynamicException("Block " + block.hash().hex().get()
        + " does not have the correct height."
      );
    }
  }
  this->chain_.emplace_back(std::make_shared<Block>(std::move(block)));
  std::shared_ptr<const Block> newBlock = this->chain_.back();

  // Add block and txs to mappings
  this->blockByHash_.insert({newBlock->hash(), newBlock});
  this->blockHashByHeight_.insert({newBlock->getNHeight(), newBlock->hash()});
  this->blockHeightByHash_.insert({newBlock->hash(), newBlock->getNHeight()});
  const auto& Txs = newBlock->getTxs();
  for (uint32_t i = 0; i < Txs.size(); i++) {
    this->txByHash_.insert({ Txs[i].hash(), { newBlock->hash(), i, newBlock->getNHeight() }});
  }
}

void Storage::pushFrontInternal(Block&& block) {
  // Push the new block and get a pointer to it
  if (!this->chain_.empty()) {
    if (this->chain_.front()->getPrevBlockHash() != block.hash()) {
      throw DynamicException("Block " + block.hash().hex().get()
        + " does not have the correct previous block hash."
      );
    }
    if (block.getNHeight() != this->chain_.front()->getNHeight() - 1) {
      throw DynamicException("Block " + block.hash().hex().get()
        + " does not have the correct height."
      );
    }
  }
  this->chain_.emplace_front(std::make_shared<Block>(std::move(block)));
  std::shared_ptr<const Block> newBlock = this->chain_.front();

  // Add block and txs to mappings
  this->blockByHash_.insert({newBlock->hash(), newBlock});
  this->blockHashByHeight_.insert({newBlock->getNHeight(), newBlock->hash()});
  this->blockHeightByHash_.insert({newBlock->hash(), newBlock->getNHeight()});
  const auto& Txs = newBlock->getTxs();
  for (uint32_t i = 0; i < Txs.size(); i++) {
    this->txByHash_.insert({Txs[i].hash(), { newBlock->hash(), i, newBlock->getNHeight()}});
  }
}

void Storage::pushBack(Block&& block) {
  std::unique_lock<std::shared_mutex> lock(this->chainLock_);
  this->pushBackInternal(std::move(block));
}

void Storage::pushFront(Block&& block) {
  std::unique_lock<std::shared_mutex> lock(this->chainLock_);
  this->pushFrontInternal(std::move(block));
}

void Storage::popBack() {
  // Delete block and its txs from the mappings, then pop it from the chain
  std::unique_lock<std::shared_mutex> lock(this->chainLock_);
  std::shared_ptr<const Block> block = this->chain_.back();
  for (const TxBlock& tx : block->getTxs()) this->txByHash_.erase(tx.hash());
  this->blockByHash_.erase(block->hash());
  this->chain_.pop_back();
}

void Storage::popFront() {
  // Delete block and its txs from the mappings, then pop it from the chain
  std::unique_lock<std::shared_mutex> lock(this->chainLock_);
  std::shared_ptr<const Block> block = this->chain_.front();
  for (const TxBlock& tx : block->getTxs()) this->txByHash_.erase(tx.hash());
  this->blockByHash_.erase(block->hash());
  this->chain_.pop_front();
}

bool Storage::blockExists(const Hash& hash) const {
  std::shared_lock<std::shared_mutex> lockChain(this->chainLock_);
  std::shared_lock<std::shared_mutex> lockCache(this->cacheLock_);
  return this->blockExistsInternal(hash) != StorageStatus::NotFound;
}

bool Storage::blockExists(const uint64_t& height) const {
  std::shared_lock<std::shared_mutex> lockChain(this->chainLock_);
  std::shared_lock<std::shared_mutex> lockCache(this->cacheLock_);
  return this->blockExistsInternal(height) != StorageStatus::NotFound;
}

bool Storage::txExists(const Hash& tx) const {
  std::shared_lock<std::shared_mutex> lockChain(this->chainLock_);
  std::shared_lock<std::shared_mutex> lockCache(this->cacheLock_);
  return this->txExistsInternal(tx) != StorageStatus::NotFound;
}

std::shared_ptr<const Block> Storage::getBlock(const Hash& hash) const {
  // Check chain first, then cache, then database
  std::shared_lock<std::shared_mutex> lockChain(this->chainLock_);
  std::shared_lock<std::shared_mutex> lockCache(this->cacheLock_);
  StorageStatus blockStatus = this->blockExistsInternal(hash);
  switch (blockStatus) {
    case StorageStatus::NotFound: {
      return nullptr;
    }
    case StorageStatus::OnChain: {
      return this->blockByHash_.at(hash);
    }
    case StorageStatus::OnCache: {
      return this->cachedBlocks_.at(hash);
    }
    case StorageStatus::OnDB: {
      lockCache.unlock(); // Unlock shared lock so we can lock uniquely and insert into cache
      std::unique_lock<std::shared_mutex> lock(this->cacheLock_);
      this->cachedBlocks_.insert({hash, std::make_shared<Block>(
        this->db_->get(hash.get(), DBPrefix::blocks), this->options_->getChainID()
      )});
      return this->cachedBlocks_.at(hash);
    }
  }
  return nullptr;
}

std::shared_ptr<const Block> Storage::getBlock(const uint64_t& height) const {
  // Check chain first, then cache, then database
  std::shared_lock<std::shared_mutex> lockChain(this->chainLock_);
  std::shared_lock<std::shared_mutex> lockCache(this->cacheLock_);
  StorageStatus blockStatus = this->blockExistsInternal(height);
  if (blockStatus == StorageStatus::NotFound) return nullptr;
  Logger::logToDebug(LogType::INFO, Log::storage, __func__, "height: " + std::to_string(height));
  switch (blockStatus) {
    case StorageStatus::NotFound: {
      return nullptr;
    }
    case StorageStatus::OnChain: {
      return this->blockByHash_.at(this->blockHashByHeight_.at(height));
    }
    case StorageStatus::OnCache: {
      Hash hash = this->blockHashByHeight_.find(height)->second;
      return this->cachedBlocks_.find(hash)->second;
    }
    case StorageStatus::OnDB: {
      lockCache.unlock(); /// Unlock shared lock so we can lock uniquely and insert into cache
      std::unique_lock<std::shared_mutex> lock(this->cacheLock_);
      Hash hash = this->blockHashByHeight_.find(height)->second;
      auto blockData = this->db_->get(hash.get(), DBPrefix::blocks);
      this->cachedBlocks_.insert({hash, std::make_shared<Block>(blockData, this->options_->getChainID())});
      return this->cachedBlocks_.at(hash);
    }
  }
  return nullptr;
}


std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
> Storage::getTx(const Hash& tx) const {
  // Check chain first, then cache, then database
  std::shared_lock<std::shared_mutex> lockChain(this->chainLock_);
  std::shared_lock<std::shared_mutex> lockCache(this->cacheLock_);
  StorageStatus txStatus = this->txExistsInternal(tx);
  switch (txStatus) {
    case StorageStatus::NotFound: {
      return {nullptr, Hash(), 0, 0};
    }
    case StorageStatus::OnChain: {
      const auto& [blockHash, blockIndex, blockHeight] = this->txByHash_.find(tx)->second;
      const auto& transactionList = blockByHash_.at(blockHash)->getTxs(); /// We can use at() because we know it exists
      /// Check if transactionList can be accessed at the index
      if (transactionList.size() <= blockIndex) throw DynamicException("Tx index out of bounds");
      const auto& transaction = transactionList[blockIndex];
      if (transaction.hash() != tx) throw DynamicException("Tx hash mismatch");
      return {std::make_shared<const TxBlock>(transaction), blockHash, blockIndex, blockHeight};
    }
    case StorageStatus::OnCache: {
      return this->cachedTxs_.at(tx);
    }
    case StorageStatus::OnDB: {
      lockCache.unlock();
      Bytes txData(this->db_->get(tx.get(), DBPrefix::txToBlocks));
      BytesArrView txDataView(txData);
      auto blockHash = Hash(txDataView.subspan(0, 32));
      uint64_t blockIndex = Utils::bytesToUint32(txDataView.subspan(32, 4));
      uint64_t blockHeight = Utils::bytesToUint64(txDataView.subspan(36,8));
      Bytes blockData(this->db_->get(blockHash.get(), DBPrefix::blocks));
      auto Tx = this->getTxFromBlockWithIndex(blockData, blockIndex);
      std::unique_lock<std::shared_mutex> lock(this->cacheLock_);
      this->cachedTxs_.insert({tx, {std::make_shared<const TxBlock>(Tx), blockHash, blockIndex, blockHeight}});
      return this->cachedTxs_.at(tx);
    }
  }
  return { nullptr, Hash(), 0, 0 };
}

std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
> Storage::getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex) const {
  std::shared_lock<std::shared_mutex> lockChain(this->chainLock_);
  std::shared_lock<std::shared_mutex> lockCache(this->cacheLock_);
  auto Status = this->blockExistsInternal(blockHash);
  switch (Status) {
    case StorageStatus::NotFound: {
      return { nullptr, Hash(), 0, 0 };
    }
    case StorageStatus::OnChain: {
      const auto& transactionList = this->blockByHash_.at(blockHash)->getTxs(); /// We can use at() because we know it exists
      if (transactionList.size() <= blockIndex) throw DynamicException("Tx index out of bounds");
      const auto& transaction = transactionList[blockIndex];
      const auto& txHash = transaction.hash();  /// We can use at() because we know it exists
      const auto& [txBlockHash, txBlockIndex, txBlockHeight] = this->txByHash_.at(txHash);
      if (txBlockHash != blockHash || txBlockIndex != blockIndex) {
        throw DynamicException("Tx hash mismatch");
      }
      return {std::make_shared<const TxBlock>(transaction), txBlockHash, txBlockIndex, txBlockHeight};
    }
    case StorageStatus::OnCache: {
      auto txHash = this->cachedBlocks_.at(blockHash)->getTxs()[blockIndex].hash();
      return this->cachedTxs_.at(txHash);
    }
    case StorageStatus::OnDB: {
      lockCache.unlock();
      Bytes blockData = this->db_->get(blockHash.get(), DBPrefix::blocks);
      auto tx = this->getTxFromBlockWithIndex(blockData, blockIndex);
      std::unique_lock<std::shared_mutex> lock(this->cacheLock_);
      auto blockHeight = this->blockHeightByHash_.at(blockHash);
      this->cachedTxs_.insert({tx.hash(), {std::make_shared<TxBlock>(tx), blockHash, blockIndex, blockHeight}});
      return this->cachedTxs_.at(tx.hash());
    }
  }
  return { nullptr, Hash(), 0, 0 };
}

std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
> Storage::getTxByBlockNumberAndIndex(const uint64_t& blockHeight, const uint64_t blockIndex) const {
  std::shared_lock<std::shared_mutex> lockChain(this->chainLock_);
  std::shared_lock<std::shared_mutex> lockCache(this->cacheLock_);
  auto Status = this->blockExistsInternal(blockHeight);
  switch (Status) {
    case StorageStatus::NotFound: {
      return { nullptr, Hash(), 0, 0 };
    }
    case StorageStatus::OnChain: {
      auto blockHash = this->blockHashByHeight_.find(blockHeight)->second;
      const auto& transactionList = this->blockByHash_.at(blockHash)->getTxs();
      if (transactionList.size() <= blockIndex) throw DynamicException("Tx index out of bounds");
      const auto& transaction = transactionList[blockIndex];
      const auto& txHash = transaction.hash();
      const auto& [txBlockHash, txBlockIndex, txBlockHeight] = this->txByHash_.at(txHash);
      return {std::make_shared<TxBlock>(transaction), txBlockHash, txBlockIndex, txBlockHeight};
    }
    case StorageStatus::OnCache: {
      auto blockHash = this->blockHashByHeight_.find(blockHeight)->second;
      auto txHash = this->cachedBlocks_.at(blockHash)->getTxs()[blockIndex].hash();
      return this->cachedTxs_.at(txHash);
    }
    case StorageStatus::OnDB: {
      lockCache.unlock();
      auto blockHash = this->blockHashByHeight_.find(blockHeight)->second;
      Bytes blockData = this->db_->get(blockHash.get(), DBPrefix::blocks);
      auto tx = this->getTxFromBlockWithIndex(blockData, blockIndex);
      std::unique_lock<std::shared_mutex> lock(this->cacheLock_);
      auto blockHeight2 = this->blockHeightByHash_.at(blockHash);
      this->cachedTxs_.insert({tx.hash(), { std::make_shared<TxBlock>(tx), blockHash, blockIndex, blockHeight2}});
      return this->cachedTxs_.at(tx.hash());
    }
  }
  return { nullptr, Hash(), 0, 0 };
}

std::shared_ptr<const Block> Storage::latest() {
  std::shared_lock<std::shared_mutex> lock(this->chainLock_);
  return this->chain_.back();
}

uint64_t Storage::currentChainSize() { return this->latest()->getNHeight() + 1; }

void Storage::periodicSaveToDB() const {
  while (!this->stopPeriodicSave_) {
    std::this_thread::sleep_for(std::chrono::seconds(this->periodicSaveCooldown_));
    if (!this->stopPeriodicSave_ &&
      (this->cachedBlocks_.size() > 1000 || this->cachedTxs_.size() > 1000000)
    ) {
      // TODO: Properly implement periodic save to DB, saveToDB() function saves **everything** to DB.
      // Requirements:
      // 1. Save up to 50% of current block list size to DB (e.g. 500 blocks if there are 1000 blocks).
      // 2. Save all tx references existing on these blocks to DB.
      // 3. Check if block is **unique** to Storage class (use shared_ptr::use_count()), if it is, save it to DB.
      // 4. Take max 1 second, Storage::periodicSaveToDB() should never lock this->chainLock_ for too long, otherwise chain might stall.
      // use_count() of blocks inside Storage would be 2 if on chain (this->chain + this->blockByHash_) and not being used anywhere else on the program.
      // or 1 if on cache (cachedBlocks).
      // if ct > 3 (or 1), we have to wait until whoever is using the block
      // to stop using it so we can save it.
    }
  }
}

