#include "storage.h"

void Storage::pushBackInternal(Block&& block) {
  // Push the new block and get a pointer to it
  if (this->chain.size() != 0) {
    if (this->chain.back()->hash() != block.getPrevBlockHash()) {
      throw std::runtime_error("Block " + block.hash().hex().get() + " does not have the correct previous block hash.");
    }

    if (block.getNHeight() != this->chain.back()->getNHeight() + 1) {
      throw std::runtime_error("Block " + block.hash().hex().get() + " does not have the correct height.");
    }
  }

  this->chain.emplace_back(std::make_shared<Block>(std::move(block)));
  std::shared_ptr<const Block> newBlock = this->chain.back();

  // Add block to mappings
  this->blockByHash[newBlock->hash()] = newBlock;
  this->blockHashByHeight[newBlock->getNHeight()] = newBlock->hash();
  this->blockHeightByHash[newBlock->hash()] = newBlock->getNHeight();

  // Add block txs to mappings
  for (const TxBlock& tx : newBlock->getTxs()) {
    this->txByHash[tx.hash()] = std::make_shared<TxBlock>(tx);
    this->blockByTxHash[tx.hash()] = newBlock;
  }
}

void Storage::pushFrontInternal(Block&& block) {
  // Push the new block and get a pointer to it
  if (this->chain.size() != 0) {
    if (this->chain.front()->getPrevBlockHash() != block.hash()) {
      throw std::runtime_error("Block " + block.hash().hex().get() + " does not have the correct previous block hash.");
    } 
  
    if (block.getNHeight() != this->chain.front()->getNHeight() - 1) {
      throw std::runtime_error("Block " + block.hash().hex().get() + " does not have the correct height.");
    }
  }
  
  this->chain.emplace_front(std::make_shared<Block>(std::move(block)));
  std::shared_ptr<const Block> newBlock = this->chain.front();

  // Add block to mappings
  this->blockByHash[newBlock->hash()] = newBlock;
  this->blockHashByHeight[newBlock->getNHeight()] = newBlock->hash();
  this->blockHeightByHash[newBlock->hash()] = newBlock->getNHeight();

  // Add block txs to mappings
  for (const TxBlock& tx : newBlock->getTxs()) {
    this->txByHash[tx.hash()] = std::make_shared<TxBlock>(tx);
    this->blockByTxHash[tx.hash()] = newBlock;
  }
}

// TODO: Maybe move SaveToDB during destructor and not within a function?
// That requires us to make sure that the destructor is called after everyone is done using the Storage object.
void Storage::saveToDB() {
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

      // Batch txs to be saved to the database and delete them from mappings
      for (const TxBlock& tx : block->getTxs()) {
        txToBlockBatch.puts.emplace_back(DBEntry(
          tx.hash().get(), block->hash().get()
        ));
        this->txByHash.erase(tx.hash());
        this->blockByTxHash.erase(tx.hash());
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

void Storage::loadFromDB() {
  // Create a new genesis block if one doesn't exist (fresh new blockchain)
  Utils::logToDebug(Log::storage, __func__, "Loading blockchain from DB");
  if (!this->db->has("latest", DBPrefix::blocks)) {
    Utils::logToDebug(Log::storage, __func__, "No history found, creating genesis block.");
    Block genesis(Hash(Utils::uint256ToBytes(0)), 1656356645000000, 0);
    // Genesis Keys:
    // Private: 0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867 Address: 0x00dead00665771855a34155f5e7405489df2c3c6
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")));
    this->db->put("latest", genesis.serializeBlock(), DBPrefix::blocks);
    this->db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    this->db->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);
    // TODO: CHANGE THIS ON PUBLIC!!! THOSE PRIVATE KEYS SHOULD ONLY BE USED FOR LOCAL TESTING
    this->db->put(Utils::uint64ToBytes(0), Address(Hex::toBytes("0x7588b0f553d1910266089c58822e1120db47e572"), true).get(), DBPrefix::validators); // 0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759
    this->db->put(Utils::uint64ToBytes(1), Address(Hex::toBytes("0xcabf34a268847a610287709d841e5cd590cc5c00"), true).get(), DBPrefix::validators); // 0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4
    this->db->put(Utils::uint64ToBytes(2), Address(Hex::toBytes("0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3"), true).get(), DBPrefix::validators); // 0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751
    this->db->put(Utils::uint64ToBytes(3), Address(Hex::toBytes("0x795083c42583842774febc21abb6df09e784fce5"), true).get(), DBPrefix::validators); // 0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7
    this->db->put(Utils::uint64ToBytes(4), Address(Hex::toBytes("0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"), true).get(), DBPrefix::validators); // 0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6
    Utils::logToDebug(Log::storage, __func__,
      std::string("Created genesis block: ") + Hex::fromBytes(genesis.hash().get()).get()
    );
  }

  Utils::logToDebug(Log::storage, __func__, "Loading latest block");
  // Get the latest block from the database
  Block latest(this->db->get("latest", DBPrefix::blocks));
  uint64_t depth = latest.getNHeight();
  Utils::logToDebug(Log::storage, __func__,
    std::string("Got latest block: ") + latest.hash().hex().get()
    + std::string(" - height ") + std::to_string(depth)
  );

  // Parse block mappings (hash -> height / height -> hash) from DB
  Utils::logToDebug(Log::storage, __func__, "Parsing block mappings");
  std::unique_lock<std::shared_mutex> lock(this->chainLock);
  std::vector<DBEntry> maps = this->db->getBatch(DBPrefix::blockHeightMaps);
  for (DBEntry& map : maps) {
    Utils::logToDebug(Log::storage, __func__,
      std::string(" ")
      + std::to_string(Utils::bytesToUint64(map.key))
      + std::string(", hash ") + Hash(map.value).hex().get()
    );
    this->blockHashByHeight[Utils::bytesToUint64(map.key)] = Hash(map.value);
    this->blockHeightByHash[Hash(map.value)] = Utils::bytesToUint64(map.key);
  }

  // Append up to 500 most recent blocks from DB to chain
  Utils::logToDebug(Log::storage, __func__, "Appending recent blocks");
  for (uint64_t i = 0; i <= 500 && i <= depth; i++) {
    Utils::logToDebug(Log::storage, __func__,
      std::string("Height: ") + std::to_string(depth - i) + " hash: "  + this->blockHashByHeight[depth - i].hex().get()
    );
    Block block(this->db->get(this->blockHashByHeight[depth - i].get(), DBPrefix::blocks));
    this->pushFrontInternal(std::move(block));
  }

  Utils::logToDebug(Log::storage, __func__, "Blockchain successfully loaded");
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
  std::unique_lock<std::shared_mutex> lock(this->chainLock);
  std::shared_ptr<const Block> block = this->chain.back();

  // Delete block and its txs from mappings, then pop it from the chain
  for (const TxBlock& tx : block->getTxs()) {
    this->txByHash.erase(tx.hash());
    this->blockByTxHash.erase(tx.hash());
  }
  this->blockByHash.erase(block->hash());
  this->chain.pop_back();
}

void Storage::popFront() {
  std::unique_lock<std::shared_mutex> lock(this->chainLock);
  std::shared_ptr<const Block> block = this->chain.front();

  // Delete block and its txs from mappings, then pop it from the chain
  for (const TxBlock& tx : block->getTxs()) {
    this->txByHash.erase(tx.hash());
    this->blockByTxHash.erase(tx.hash());
  }
  this->blockByHash.erase(block->hash());
  this->chain.pop_front();
}

bool Storage::hasBlock(const Hash& hash) {
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  return this->blockByHash.contains(hash);
}

bool Storage::hasBlock(const uint64_t& height) {
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  auto it = this->blockHashByHeight.find(height);
  if (it != this->blockHashByHeight.end()) {
    if (this->blockByHash.contains(it->second)) {
      return true;
    }
  }
  return false;
}

bool Storage::exists(const Hash& hash) {
  if (this->hasBlock(hash)) return true;
  return this->db->has(hash.get(), DBPrefix::blocks);
}

bool Storage::exists(const uint64_t& height) {
  if (this->hasBlock(height)) return true;
  return this->db->has(Utils::uint64ToBytes(height), DBPrefix::blockHeightMaps);
}

const std::shared_ptr<const Block> Storage::getBlock(const Hash& hash) {
  if (!this->exists(hash)) return nullptr;
  Utils::logToDebug(Log::storage, __func__, "hash: " + hash.get());
  std::shared_ptr<const Block> ret;

  // Check chain first, then cache, then database
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  if (this->hasBlock(hash)) {
    ret = this->blockByHash.find(hash)->second;
  } else if (this->cachedBlocks.contains(hash)) {
    ret = this->cachedBlocks[hash];
  } else {
    ret = std::make_shared<Block>(this->db->get(hash.get(), DBPrefix::blocks));
  }
  return ret;
}

const std::shared_ptr<const Block> Storage::getBlock(const uint64_t& height) {
  if (!this->exists(height)) return nullptr;
  Utils::logToDebug(Log::storage, __func__, "height: " + std::to_string(height));
  std::shared_ptr<const Block> ret;

  // Check chain first, then cache, then database
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  if (this->hasBlock(height)) {
    ret = this->blockByHash.find(this->blockHashByHeight.find(height)->second)->second;
  } else {
    Hash hash(this->db->get(Utils::uint64ToBytes(height), DBPrefix::blockHeightMaps));
    ret = (this->cachedBlocks.contains(hash))
      ? this->cachedBlocks[hash]
      : std::make_shared<Block>(this->db->get(hash.get(), DBPrefix::blocks));
  }
  return ret;
}

bool Storage::hasTx(const Hash& tx) {
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  return this->txByHash.contains(tx);
}

const std::shared_ptr<const TxBlock> Storage::getTx(const Hash& tx) {
  std::shared_ptr<const TxBlock> ret = nullptr;

  // Check chain first, then cache, then database.
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  if (this->hasTx(tx)) {
    ret = this->txByHash.find(tx)->second;
  } else if (this->cachedTxs.contains(tx)) {
    ret = this->cachedTxs[tx];
  } else if (this->db->has(tx.get(), DBPrefix::txToBlocks)) {
    Hash hash(this->db->get(tx.get(), DBPrefix::txToBlocks));
    std::string txBytes = this->db->get(hash.get(), DBPrefix::blocks);
    ret = std::make_shared<TxBlock>(txBytes);
  }
  return ret;
}

const std::shared_ptr<const Block> Storage::getBlockFromTx(const Hash& tx) {
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  const std::shared_ptr<const Block> ret = (this->hasTx(tx)) ? this->blockByTxHash.find(tx)->second : nullptr;
  return ret;
}

const std::shared_ptr<const Block> Storage::latest() {
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  const std::shared_ptr<const Block> ret = this->chain.back();
  return ret;
}

uint64_t Storage::blockSize() {
  std::shared_lock<std::shared_mutex> lock(this->chainLock);
  uint64_t ret = this->chain.size();
  return ret;
}

void Storage::periodicSaveToDB() {
  while (!this->stopPeriodicSave) {
    std::this_thread::sleep_for(std::chrono::seconds(this->periodicSaveCooldown));
    if (
      !this->stopPeriodicSave &&
      (this->cachedBlocks.size() > 1000 || this->cachedTxs.size() > 1000000)
    ) {
      // TODO: Properly implement pariodic save to DB, saveToDB() function saves **everything** to DB.
      // Requirements: 
      // 1. Save up to 50% of current block list size to DB.(e.g. 500 blocks if there are 1000 blocks)
      // 2. Save all transactions references existing on these blocks to DB.
      // 3. Check if the block is **unique** to the Storage class (use shared_ptr::use_count()). If it is, save it to DB.
    }
  }
}

