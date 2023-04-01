#include "storage.h"

Storage::Storage(const std::unique_ptr<DB>& db, const std::unique_ptr<Options>& options) : db(db), options(options) {
  Utils::logToDebug(Log::storage, __func__, "Loading blockchain from DB");

  // Initialize the blockchain if latest block doesn't exist.
  initializeBlockchain();

  // Get the latest block from the database
  Utils::logToDebug(Log::storage, __func__, "Loading latest block");
  Block latest(this->db->get("latest", DBPrefix::blocks), this->options->getChainID());
  uint64_t depth = latest.getNHeight();
  Utils::logToDebug(Log::storage, __func__,
    std::string("Got latest block: ") + latest.hash().hex().get()
    + std::string(" - height ") + std::to_string(depth)
  );

  std::unique_lock<std::shared_mutex> lock(this->chainLock);

  // Parse block mappings (hash -> height / height -> hash) from DB
  Utils::logToDebug(Log::storage, __func__, "Parsing block mappings");
  std::vector<DBEntry> maps = this->db->getBatch(DBPrefix::blockHeightMaps);
  for (DBEntry& map : maps) {
    // TODO: Check if a block is missing.
    // Might be interesting to change DB::getBatch to return a map instead of a vector
    Utils::logToDebug(Log::storage, __func__, std::string(": ")
      + std::to_string(Utils::bytesToUint64(map.key))
      + std::string(", hash ") + Hash(map.value).hex().get()
    );
    this->blockHashByHeight.insert({Utils::bytesToUint64(map.key),Hash(map.value)});
    this->blockHeightByHash.insert({Hash(map.value), Utils::bytesToUint64(map.key)});
  }

  // Append up to 500 most recent blocks from DB to chain
  Utils::logToDebug(Log::storage, __func__, "Appending recent blocks");
  for (uint64_t i = 0; i <= 500 && i <= depth; i++) {
    Utils::logToDebug(Log::storage, __func__,
      std::string("Height: ") + std::to_string(depth - i) + ", Hash: "
      + this->blockHashByHeight[depth - i].hex().get()
    );
    Block block(this->db->get(this->blockHashByHeight[depth - i].get(), DBPrefix::blocks), this->options->getChainID());
    this->pushFrontInternal(std::move(block));
  }

  Utils::logToDebug(Log::storage, __func__, "Blockchain successfully loaded");
}

Storage::~Storage() {
  DBBatch blockBatch, heightBatch, txToBlockBatch;
  std::shared_ptr<const Block> latest;

  {
    std::unique_lock<std::shared_mutex> lock(this->chainLock);
    latest = this->chain.back();
    while (!this->chain.empty()) {
      // Batch block to be saved to the database.
      // We can't call this->popBack() because of the mutex
      std::shared_ptr<const Block> block = this->chain.front();
      blockBatch.puts.emplace_back(DBEntry(
        block->hash().get(), block->serializeBlock()
      ));
      heightBatch.puts.emplace_back(DBEntry(
        Utils::uint64ToBytes(block->getNHeight()), block->hash().get()
      ));

      // Batch txs to be saved to the database and delete them from the mappings
      auto Txs = block->getTxs();
      for (uint32_t i = 0; i < Txs.size(); i++) {
        const auto TxHash = Txs[i].hash();
        std::string value = block->hash().get() + Utils::uint32ToBytes(i) + Utils::uint64ToBytes(block->getNHeight());
        txToBlockBatch.puts.emplace_back(DBEntry(TxHash.get(), value));
        this->txByHash.erase(TxHash);
      }

      // Delete block from internal mappings and the chain
      this->blockByHash.erase(block->hash());
      this->chain.pop_front();
    }
  }

  // Batch save to database
  this->db->putBatch(blockBatch, DBPrefix::blocks);
  this->db->putBatch(heightBatch, DBPrefix::blockHeightMaps);
  this->db->putBatch(txToBlockBatch, DBPrefix::txToBlocks);
  this->db->put("latest", latest->serializeBlock(), DBPrefix::blocks);
}

void Storage::initializeBlockchain() {
  if (!this->db->has("latest", DBPrefix::blocks)) {
    // Create a new genesis block if one doesn't exist (fresh new blockchain)
    Utils::logToDebug(Log::storage, __func__, "No history found, creating genesis block.");
    Block genesis(Hash(Utils::uint256ToBytes(0)), 1656356645000000, 0);

    // Genesis Keys:
    // Private: 0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867
    // Address: 0x00dead00665771855a34155f5e7405489df2c3c6
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")), 1656356646000000);
    this->db->put("latest", genesis.serializeBlock(), DBPrefix::blocks);
    this->db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    this->db->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);
    Utils::logToDebug(Log::storage, __func__,
      std::string("Created genesis block: ") + Hex::fromBytes(genesis.hash().get()).get()
    );
  }  
}

const TxBlock Storage::getTxFromBlockWithIndex(const std::string_view blockData, const uint64_t& txIndex) {
  uint64_t index = 217; // Start of block tx range
  /// Count txs until index.
  uint64_t currentTx = 0;
  while (currentTx < txIndex) {
    uint32_t txSize = Utils::bytesToUint32(blockData.substr(index, 4));
    index += txSize + 4;
    ++currentTx;
  }
  uint64_t txSize = Utils::bytesToUint32(blockData.substr(index, 4));
  index += 4;
  return TxBlock(blockData.substr(index, txSize), this->options->getChainID());
}

void Storage::pushBackInternal(Block&& block) {
  // Push the new block and get a pointer to it
  if (this->chain.size() != 0) {
    if (this->chain.back()->hash() != block.getPrevBlockHash()) {
      throw std::runtime_error("Block " + block.hash().hex().get()
        + " does not have the correct previous block hash.");
    }
    if (block.getNHeight() != this->chain.back()->getNHeight() + 1) {
      throw std::runtime_error("Block " + block.hash().hex().get()
        + " does not have the correct height.");
    }
  }
  this->chain.emplace_back(std::make_shared<Block>(std::move(block)));
  std::shared_ptr<const Block> newBlock = this->chain.back();

  // Add block and txs to mappings
  this->blockByHash.insert({newBlock->hash(), newBlock});
  this->blockHashByHeight.insert({newBlock->getNHeight(), newBlock->hash()});
  this->blockHeightByHash.insert({newBlock->hash(), newBlock->getNHeight()});
  const auto& Txs = newBlock->getTxs();
  for (uint32_t i = 0; i < Txs.size(); ++i) {
    this->txByHash.insert({ Txs[i].hash(), { newBlock->hash(), i, newBlock->getNHeight() }});
  }
}

void Storage::pushFrontInternal(Block&& block) {
  // Push the new block and get a pointer to it
  if (this->chain.size() != 0) {
    if (this->chain.front()->getPrevBlockHash() != block.hash()) {
      throw std::runtime_error("Block " + block.hash().hex().get()
        + " does not have the correct previous block hash.");
    }
    if (block.getNHeight() != this->chain.front()->getNHeight() - 1) {
      throw std::runtime_error("Block " + block.hash().hex().get()
        + " does not have the correct height.");
    }
  }
  this->chain.emplace_front(std::make_shared<Block>(std::move(block)));
  std::shared_ptr<const Block> newBlock = this->chain.front();

  // Add block and txs to mappings
  this->blockByHash.insert({newBlock->hash(), newBlock});
  this->blockHashByHeight.insert({newBlock->getNHeight(), newBlock->hash()});
  this->blockHeightByHash.insert({newBlock->hash(), newBlock->getNHeight()});
  const auto& Txs = newBlock->getTxs();
  for (uint32_t i = 0; i < Txs.size(); ++i) {
    this->txByHash.insert({Txs[i].hash(), { newBlock->hash(), i, newBlock->getNHeight()}});
  }
}

void Storage::pushBack(Block&& block) {
  std::unique_lock<std::shared_mutex> lock(this->chainLock);
  this->pushBackInternal(std::move(block));
}

void Storage::pushFront(Block&& block) {
  std::unique_lock<std::shared_mutex> lock(this->chainLock);
  this->pushFrontInternal(std::move(block));
}

void Storage::popBack() {
  // Delete block and its txs from the mappings, then pop it from the chain
  std::unique_lock<std::shared_mutex> lock(this->chainLock);
  std::shared_ptr<const Block> block = this->chain.back();
  for (const TxBlock& tx : block->getTxs()) {
    this->txByHash.erase(tx.hash());
  }
  this->blockByHash.erase(block->hash());
  this->chain.pop_back();
}

void Storage::popFront() {
  // Delete block and its txs from the mappings, then pop it from the chain
  std::unique_lock<std::shared_mutex> lock(this->chainLock);
  std::shared_ptr<const Block> block = this->chain.front();
  for (const TxBlock& tx : block->getTxs()) {
    this->txByHash.erase(tx.hash());
  }
  this->blockByHash.erase(block->hash());
  this->chain.pop_front();
}

StorageStatus Storage::blockExists(const Hash& hash) {
  // Check chain first, then cache, then database
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  if (this->blockByHash.contains(hash)) {
    return StorageStatus::OnChain;
  } else if (this->cachedBlocks.contains(hash)) {
    return StorageStatus::OnCache;
  } else if (this->db->has(hash.get(), DBPrefix::blocks)) {
    return StorageStatus::OnDB;
  } else {
    return StorageStatus::NotFound;
  }
}

StorageStatus Storage::blockExists(const uint64_t& height) {
  // Check chain first, then cache, then database
  std::shared_lock<std::shared_mutex> lock(this->chainLock);

  auto it = this->blockHashByHeight.find(height);
  if (it != this->blockHashByHeight.end()) {
    if (this->blockByHash.contains(it->second)) return StorageStatus::OnChain;
    std::shared_lock lock(this->cacheLock);
    if (this->cachedBlocks.contains(it->second)) return StorageStatus::OnCache;
    return StorageStatus::OnDB;
  } else {
    return StorageStatus::NotFound;
  }


  return StorageStatus::NotFound;
}

const std::shared_ptr<const Block> Storage::getBlock(const Hash& hash) {
  // Check chain first, then cache, then database
  StorageStatus blockStatus = this->blockExists(hash);
  Utils::logToDebug(Log::storage, __func__, "hash: " + hash.get());
  switch (blockStatus) {
    case StorageStatus::NotFound: {
      return nullptr;
    }
    case StorageStatus::OnChain: {
      std::shared_lock lock(this->chainLock);
      return this->blockByHash.find(hash)->second;
    }
    case StorageStatus::OnCache: {
      std::shared_lock lock(this->cacheLock);
      return this->cachedBlocks[hash];
    }
    case StorageStatus::OnDB: {
      std::unique_lock lock(this->cacheLock);
      this->cachedBlocks.insert({hash, std::make_shared<Block>(this->db->get(hash.get(), DBPrefix::blocks), this->options->getChainID())});
      return this->cachedBlocks[hash];
    }
  }
  return nullptr;
}

const std::shared_ptr<const Block> Storage::getBlock(const uint64_t& height) {
  // Check chain first, then cache, then database
  StorageStatus blockStatus = this->blockExists(height);
  if (blockStatus == StorageStatus::NotFound) return nullptr;
  Utils::logToDebug(Log::storage, __func__, "height: " + std::to_string(height));
  switch (blockStatus) {
    case StorageStatus::NotFound: {
      return nullptr;
    }
    case StorageStatus::OnChain: {
      std::shared_lock lock(this->chainLock);
      return this->blockByHash.find(this->blockHashByHeight.find(height)->second)->second;
    }
    case StorageStatus::OnCache: {
      std::shared_lock lock(this->cacheLock);
      Hash hash = this->blockHashByHeight.find(height)->second;
      return this->cachedBlocks.find(hash)->second;
    }
    case StorageStatus::OnDB: {
      std::unique_lock lock(this->cacheLock);
      Hash hash = this->blockHashByHeight.find(height)->second;
      auto blockData = this->db->get(hash.get(), DBPrefix::blocks);
      cachedBlocks.insert({hash, std::make_shared<Block>(blockData, this->options->getChainID())});
      return cachedBlocks[hash];
    }
  }
  return nullptr;
}

StorageStatus Storage::txExists(const Hash& tx) {
  // Check chain first, then cache, then database
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  if (this->txByHash.contains(tx)) {
    return StorageStatus::OnChain;
  } else if (this->cachedTxs.contains(tx)) {
    return StorageStatus::OnCache;
  } else if (this->db->has(tx.get(), DBPrefix::txToBlocks)) {
    return StorageStatus::OnDB;
  } else {
    return StorageStatus::NotFound;
  }
}

const std::tuple<const std::shared_ptr<const TxBlock>,
  const Hash,
  const uint64_t,
  const uint64_t> Storage::getTx(const Hash& tx) {
  // Check chain first, then cache, then database
  StorageStatus txStatus = this->txExists(tx);
  switch (txStatus) {
    case StorageStatus::NotFound: {
      return {nullptr, Hash(), 0, 0};
    }
    case StorageStatus::OnChain: {
      std::shared_lock<std::shared_mutex> lock(this->chainLock);
      const auto& [blockHash, blockIndex, blockHeight] = this->txByHash.find(tx)->second;
      const auto transaction = blockByHash[blockHash]->getTxs()[blockIndex];
      if (transaction.hash() != tx) {
        throw std::runtime_error("Tx hash mismatch");
      }
      return {std::make_shared<const TxBlock>(transaction), blockHash, blockIndex, blockHeight};
    }
    case StorageStatus::OnCache: {
      std::shared_lock(this->cacheLock);
      return this->cachedTxs[tx];
    }
    case StorageStatus::OnDB: {
      std::string txData(this->db->get(tx.get(), DBPrefix::txToBlocks));
      Hash blockHash = Hash(std::string_view(txData).substr(0, 32));
      uint64_t blockIndex = Utils::bytesToUint32(std::string_view(txData).substr(32, 4));
      uint64_t blockHeight = Utils::bytesToUint64(std::string_view(txData).substr(36,8));
      std::string_view blockData(this->db->get(blockHash.get(), DBPrefix::blocks));
      auto Tx = this->getTxFromBlockWithIndex(blockData, blockIndex);
      std::unique_lock(this->cacheLock);
      this->cachedTxs.insert({tx, {std::make_shared<const TxBlock>(Tx), blockHash, blockIndex, blockHeight}});
      return this->cachedTxs[tx];
    }
  }
  return {nullptr, Hash(), 0, 0};
}

const std::tuple<const std::shared_ptr<const TxBlock>,
  const Hash,
  const uint64_t,
  const uint64_t> Storage::getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex) {
  auto Status = this->blockExists(blockHash);
  switch (Status) {
    case StorageStatus::NotFound: {
      return { nullptr, Hash(), 0, 0 };
    }
    case StorageStatus::OnChain: {
      std::shared_lock lock(this->chainLock);
      auto txHash = this->blockByHash[blockHash]->getTxs()[blockIndex].hash();
      const auto& [txBlockHash, txBlockIndex, txBlockHeight] = this->txByHash[txHash];
      if (txBlockHash != blockHash || txBlockIndex != blockIndex) {
        throw std::runtime_error("Tx hash mismatch");
      }
      const auto transaction = blockByHash[blockHash]->getTxs()[blockIndex];
      return {std::make_shared<const TxBlock>(transaction), txBlockHash, txBlockIndex, txBlockHeight};
    }
    case StorageStatus::OnCache: {
      std::shared_lock lock(this->cacheLock);
      auto txHash = this->cachedBlocks[blockHash]->getTxs()[blockIndex].hash();
      return this->cachedTxs[txHash];
    }
    case StorageStatus::OnDB: {
      std::string blockData = this->db->get(blockHash.get(), DBPrefix::blocks);
      auto tx = this->getTxFromBlockWithIndex(blockData, blockIndex);
      std::unique_lock lock(this->cacheLock);
      auto blockHeight = this->blockHeightByHash[blockHash];
      this->cachedTxs.insert({tx.hash(), {std::make_shared<TxBlock>(tx), blockHash, blockIndex, blockHeight}});
      return this->cachedTxs[tx.hash()];
    }
  }
  return { nullptr, Hash(), 0, 0};
}

const std::tuple<const std::shared_ptr<const TxBlock>,
  const Hash,
  const uint64_t,
  const uint64_t> Storage::getTxByBlockNumberAndIndex(const uint64_t& blockHeight, const uint64_t blockIndex) {
  auto Status = this->blockExists(blockHeight);
  switch (Status) {
    case StorageStatus::NotFound: {
      return { nullptr, Hash(), 0, 0 };
    }
    case StorageStatus::OnChain: {
      std::shared_lock lock(this->chainLock);
      auto blockHash = this->blockHashByHeight.find(blockHeight)->second;
      auto txHash = this->blockByHash[blockHash]->getTxs()[blockIndex].hash();
      const auto& [txBlockHash, txBlockIndex, txBlockHeight] = this->txByHash[txHash];
      const auto transaction = blockByHash[blockHash]->getTxs()[blockIndex];
      return {std::make_shared<TxBlock>(transaction), txBlockHash, txBlockIndex, txBlockHeight};
    }
    case StorageStatus::OnCache: {
      std::shared_lock lock(this->cacheLock);
      auto blockHash = this->blockHashByHeight.find(blockHeight)->second;
      auto txHash = this->cachedBlocks[blockHash]->getTxs()[blockIndex].hash();
      return this->cachedTxs[txHash];
    }
    case StorageStatus::OnDB: {
      auto blockHash = this->blockHashByHeight.find(blockHeight)->second;
      std::string blockData = this->db->get(blockHash.get(), DBPrefix::blocks);
      auto tx = this->getTxFromBlockWithIndex(blockData, blockIndex);
      std::unique_lock lock(this->cacheLock);
      auto blockHeight = this->blockHeightByHash[blockHash];
      this->cachedTxs.insert({tx.hash(), { std::make_shared<TxBlock>(tx), blockHash, blockIndex, blockHeight}});
      return this->cachedTxs[tx.hash()];
    }
  }
  return { nullptr, Hash(), 0, 0};
}

const std::shared_ptr<const Block> Storage::latest() {
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  return this->chain.back();
}

uint64_t Storage::currentChainSize() {
  return this->latest()->getNHeight() + 1;
}

void Storage::periodicSaveToDB() {
  while (!this->stopPeriodicSave) {
    std::this_thread::sleep_for(std::chrono::seconds(this->periodicSaveCooldown));
    if (!this->stopPeriodicSave &&
      (this->cachedBlocks.size() > 1000 || this->cachedTxs.size() > 1000000)
    ) {
      // TODO: Properly implement periodic save to DB, saveToDB() function saves **everything** to DB.
      // Requirements:
      // 1. Save up to 50% of current block list size to DB (e.g. 500 blocks if there are 1000 blocks).
      // 2. Save all tx references existing on these blocks to DB.
      // 3. Check if block is **unique** to Storage class (use shared_ptr::use_count()), if it is, save it to DB.
      // 4. Take max 1 second, Storage::periodicSaveToDB() should never lock this->chainLock for too long, otherwise chain might stall.
      // use_count() of blocks inside Storage would be 2 if on chain (this->chain + this->blockByHash) and not being used anywhere else on the program.
      // or 1 if on cache (cachedBlocks).
      // if ct > 3 (or 1), we have to wait until whoever is using the block
      // to stop using it so we can save it.
    }
  }
}

