#include "storage.h"

void Storage::pushBackInternal(Block&& block) {
  // Push the new block and get a pointer to it
  this->chain.emplace_back(std::move(block));
  std::shared_ptr<const Block> latest = this->chain.back();

  // Add block to mappings
  this->blockByHash[latest->getBlockHash()] = latest;
  this->blockHashByHeight[latest->getNHeight()] = latest->getBlockHash();
  this->blockHeightByHash[latest->getBlockHash()] = latest->getNHeight();

  // Add block txs to mappings
  for (const auto& tx : latest->getTxs()) {
    this->txByHash[tx.second.hash()] = std::make_shared<TxBlock>(tx.second);
    this->blockByTxHash[tx.second.hash()] = latest;
  }
}

void Storage::pushFrontInternal(Block&& block) {
  // Push the new block and get a pointer to it
  this->chain.emplace_front(std::move(block));
  std::shared_ptr<const Block> latest = this->chain.front();

  // Add block to mappings
  this->blockByHash[latest->getBlockHash()] = latest;
  this->blockHashByHeight[latest->getNHeight()] = latest->getBlockHash();
  this->blockHeightByHash[latest->getBlockHash()] = latest->getNHeight();

  // Add block txs to mappings
  for (const auto& tx : latest->getTxs()) {
    this->txByHash[tx.second.hash()] = std::make_shared<TxBlock>(tx.second);
    this->blockByTxHash[tx.second.hash()] = latest;
  }
}

void Storage::saveToDB() {
  this->chainLock.lock();
  DBBatch blockBatch, heightBatch, txToBlockBatch;
  std::shared_ptr<const Block> latest = this->chain.back();

  while (!this->chain.empty()) {
    // Batch block to be saved to the database.
    // We can't call this->popBack() because of the mutex
    std::shared_ptr<const Block> block = this->chain.front();
    if (block == latest) break; // Skip latest block
    blockBatch.puts.emplace_back(DBEntry(
      block->getBlockHash().get(), block->serializeToBytes(true)
    ));
    heightBatch.puts.emplace_back(DBEntry(
      Utils::uint64ToBytes(block->getNHeight()), block->getBlockHash().get()
    ));

    // Batch txs to be saved to the database and delete them from mappings
    for (const auto& tx : block->getTxs()) {
      txToBlockBatch.puts.emplace_back(DBEntry(
        tx.second.hash().get(), block->getBlockHash().get()
      ));
      this->txByHash.erase(tx.second.hash());
      this->blockByTxHash.erase(tx.second.hash());
    }

    // Delete block from internal mappings and the chain
    this->blockByHash.erase(block->getBlockHash());
    this->chain.pop_front();
  }

  // Batch save to database
  this->db->putBatch(blockBatch, DBPrefix::blocks);
  this->db->putBatch(heightBatch, DBPrefix::blockHeightMaps);
  this->db->putBatch(txToBlockBatch, DBPrefix::txToBlocks);
  this->db->put("latest", latest->serializeToBytes(true), DBPrefix::blocks);

  this->chainLock.unlock();
}

void Storage::loadFromDB() {
  // Create a new genesis block if one doesn't exist (fresh new blockchain)
  Utils::logToDebug(Log::storage, __func__, "Loading blockchain from DB");
  if (!this->db->has("latest", DBPrefix::blocks)) {
    Block genesis(Hash(Utils::uint256ToBytes(0)), 1656356645000000, 0);
    genesis.finalize(PrivKey());
    this->db->put("latest", genesis.serializeToBytes(true), DBPrefix::blocks);
    this->db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.getBlockHash().get(), DBPrefix::blockHeightMaps);
    this->db->put(genesis.getBlockHash().get(), genesis.serializeToBytes(true), DBPrefix::blocks);
    // TODO: CHANGE THIS ON PUBLIC!!! THOSE PRIVATE KEYS SHOULD ONLY BE USED FOR LOCAL TESTING
    this->db->put(Utils::uint64ToBytes(0), Address(std::string("0x7588b0f553d1910266089c58822e1120db47e572"), true).get(), DBPrefix::validators); // 0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759
    this->db->put(Utils::uint64ToBytes(1), Address(std::string("0xcabf34a268847a610287709d841e5cd590cc5c00"), true).get(), DBPrefix::validators); // 0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4
    this->db->put(Utils::uint64ToBytes(2), Address(std::string("0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3"), true).get(), DBPrefix::validators); // 0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751
    this->db->put(Utils::uint64ToBytes(3), Address(std::string("0x795083c42583842774febc21abb6df09e784fce5"), true).get(), DBPrefix::validators); // 0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7
    this->db->put(Utils::uint64ToBytes(4), Address(std::string("0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"), true).get(), DBPrefix::validators); // 0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6
    Utils::logToDebug(Log::storage, __func__,
      std::string("Created genesis block: ") + Hex::fromBytes(genesis.getBlockHash().get()).get()
    );
  }

  // Get the latest block from the database
  Block latest = Block(this->db->get("latest", DBPrefix::blocks), true);
  uint64_t depth = latest.getNHeight();
  Utils::logToDebug(Log::storage, __func__,
    std::string("Got latest block: ") + dev::toHex(latest.getBlockHash())
    + std::string(" - height ") + std::to_string(depth)
  );

  // Parse block mappings (hash -> height / height -> hash) from DB
  Utils::logToDebug(Log::storage, __func__, "Parsing block mappings");
  this->chainLock.lock();
  std::vector<DBEntry> maps = this->db->getBatch(DBPrefix::blockHeightMaps);
  for (DBEntry& map : maps) {
    Utils::logToDebug(Log::storage, __func__,
      std::string("Indexing height ")
      + std::to_string(Utils::bytesToUint64(map.key))
      + std::string(", hash ") + Hash(map.value).hex().get()
    );
    this->blockHashByHeight[Utils::bytesToUint64(map.key)] = Hash(map.value);
    this->blockHeightByHash[Hash(map.value)] = Utils::bytesToUint64(map.key);
  }

  // Append up to 1000 most recent blocks from DB to chain
  Utils::logToDebug(Log::storage, __func__, "Appending recent blocks");
  for (uint64_t i = 0; i <= 1000 && i <= depth; i++) {
    Utils::logToDebug(Log::storage, __func__,
      std::to_string(i) + std::string(" - height: ") + Hex::fromBytes(
        this->db->get(this->blockHashByHeight[depth - i].get())
      ).get()
    );
    std::shared_ptr<const Block> block = std::make_shared<Block>(
      this->db->get(this->blockHashByHeight[depth - i].get(), DBPrefix::blocks), true
    );
    this->pushFrontInternal(block);
  }

  Utils::logToDebug(Log::storage, __func__, "Blockchain successfully loaded");
  this->chainLock.unlock();
}

void Storage::pushBack(Block&& block) {
  this->chainLock.lock();
  this->pushBackInternal(std::move(block));
  this->chainLock.unlock();
}

void Storage::pushFront(Block&& block) {
  this->chainLock.lock();
  this->pushFrontInternal(std::move(block));
  this->chainLock.unlock();
}

void Storage::popBack() {
  this->chainLock.lock();
  std::shared_ptr<const Block> block = this->chain.back();

  // Delete block and its txs from mappings, then pop it from the chain
  for (const auto& tx : block->getTxs()) {
    this->txByHash.erase(tx.second.hash());
    this->blockByTxHash.erase(tx.second.hash());
  }
  this->blockByHash.erase(block->getBlockHash());
  this->chain.pop_back();

  this->chainLock.unlock();
}

void Storage::popFront() {
  this->chainLock.lock();
  std::shared_ptr<const Block> block = this->chain.front();

  // Delete block and its txs from mappings, then pop it from the chain
  for (const auto& tx : block->getTxs()) {
    this->txByHash.erase(tx.second.hash());
    this->blockByTxHash.erase(tx.second.hash());
  }
  this->blockByHash.erase(block->getBlockHash());
  this->chain.pop_front();

  this->chainLock.unlock();
}

bool Storage::hasBlock(const Hash& hash) {
  this->chainLock.lock();
  bool result = this->blockByHash.count(hash) > 0;
  this->chainLock.unlock();
  return result;
}

bool Storage::hasBlock(const uint64_t& height) {
  this->chainLock.lock();
  bool result = this->blockHashByHeight.count(height) > 0;
  this->chainLock.unlock();
  return result;
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
  this->chainLock.lock();
  if (this->hasBlock(hash)) {
    ret = this->blockByHash.find(hash)->second;
  } else if (this->cachedBlocks.count(hash) > 0) {
    ret = this->cachedBlocks[hash];
  } else {
    ret = std::make_shared<Block>(this->db->get(hash.get(), DBPrefix::blocks), true);
  }
  this->chainLock.unlock();
  return ret;
}

const std::shared_ptr<const Block> Storage::getBlock(const uint64_t& height) {
  if (!this->exists(height)) return nullptr;
  Utils::logToDebug(Log::storage, __func__, "height: " + height);
  std::shared_ptr<const Block> ret;

  // Check chain first, then cache, then database
  this->chainLock.lock();
  if (this->hasBlock(height)) {
    ret = this->blockByHash.find(this->blockHashByHeight.find(height)->second)->second;
  } else {
    Hash hash(this->db->get(Utils::uint64ToBytes(height), DBPrefix::blockHeightMaps));
    ret = (this->cachedBlocks.count(hash) > 0)
      ? this->cachedBlocks[hash]
      : std::make_shared<Block>(this->db->get(hash.get(), DBPrefix::blocks), true);
  }
  this->chainLock.unlock();
  return ret;
}

bool Storage::hasTx(const Hash& tx) {
  this->chainLock.lock();
  bool ret = (this->txByHash.count(tx) > 0);
  this->chainLock.unlock();
  return ret;
}

const std::shared_ptr<const TxBlock> Storage::getTx(const Hash& tx) {
  std::shared_ptr<const TxBlock> ret = nullptr;

  // Check chain first, then cache, then database.
  this->chainLock.lock();
  if (this->hasTx(tx)) {
    ret = this->txByHash.find(tx)->second;
  } else if (this->cachedTxs.count(tx) > 0) {
    ret = this->cachedTxs[tx];
  } else if (this->db->has(tx.get(), DBPrefix::TxToBlocks)) {
    Hash hash(this->db->get(tx.get(), DBPrefix::TxToBlocks));
    std::string txBytes = this->db->get(hash.get(), DBPrefix::blocks);
    ret = std::make_shared<TxBlock>(txBytes, true);
  }
  this->chainLock.unlock();
  return ret;
}

const std::shared_ptr<const Block> Storage::getBlockFromTx(const Hash& tx) {
  this->chainLock.lock();
  const std::shared_ptr<const Block> ret = (this->hasTx(tx))
    ? this->blockByTxHash.find(tx)->second : nullptr;
  this->chainLock.unlock();
  return ret;
}

const std::shared_ptr<const Block> Storage::latest() {
  this->chainLock.lock();
  const std::shared_ptr<const Block> ret = this->chain.back();
  this->chainLock.unlock();
  return ret;
}

uint64_t Storage::blockSize() {
  this->chainLock.lock();
  uint64_t ret = this->chain.size();
  this->chainLock.unlock();
  return ret;
}

void Storage::periodicSaveToDB() {
  while (!this->stopPeriodicSave) {
    std::this_thread::sleep_for(std::chrono::seconds(this->periodicSaveCooldown));
    if (
      !this->stopPeriodicSave &&
      (this->cachedBlocks.size() > 1000 || this->cachedTxs.size() > 1000000)
    ) {
      this->saveToDB();
    }
  }
}

