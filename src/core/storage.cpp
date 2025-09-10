/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "storage.h"

#include "../utils/strconv.h"
#include "../utils/uintconv.h"

bool Storage::topicsMatch(const Event& event, const std::vector<Hash>& topics) {
  if (topics.empty()) return true; // No topic filter applied
  const std::vector<Hash>& eventTopics = event.getTopics();
  if (eventTopics.size() < topics.size()) return false;
  for (size_t i = 0; i < topics.size(); i++) if (topics.at(i) != eventTopics[i]) return false;
  return true;
}

void Storage::storeBlock(DB& db, const FinalizedBlock& block, bool indexingEnabled) {
  DBBatch batch;
  batch.push_back(block.getHash(), block.serializeBlock(), DBPrefix::blocks);
  batch.push_back(UintConv::uint64ToBytes(block.getNHeight()), block.getHash(), DBPrefix::heightToBlock);

  if (indexingEnabled) {
    const auto& Txs = block.getTxs();
    for (uint32_t i = 0; i < Txs.size(); i++) {
      const auto& TxHash = Txs[i].hash();
      const FixedBytes<44> value(
        bytes::join(block.getHash(), UintConv::uint32ToBytes(i), UintConv::uint64ToBytes(block.getNHeight()))
      );
      batch.push_back(TxHash, value, DBPrefix::txToBlock);
    }
  }
  db.putBatch(batch);
}

void Storage::reindexTransactions(const FinalizedBlock& block, DBBatch& batch) {
  const auto& Txs = block.getTxs();
  for (uint32_t i = 0; i < Txs.size(); i++) {
    const auto& TxHash = Txs[i].hash();
    const FixedBytes<44> value(
      bytes::join(block.getHash(), UintConv::uint32ToBytes(i), UintConv::uint64ToBytes(block.getNHeight()))
    );
    batch.push_back(TxHash, value, DBPrefix::txToBlock);
  }
}
void Storage::dumpToDisk(DBBatch &batch) {
  blocksDb_.putBatch(batch);
}

Storage::Storage(std::string instanceIdStr, const Options& options)
  : blocksDb_(options.getRootPath() + "/blocksDb/"),  // Uncompressed
    eventsDb_(options.getRootPath() + "/newEventsDb/"),
    options_(options), instanceIdStr_(std::move(instanceIdStr))
{
  // Initialize the blockchain if latest block doesn't exist.
  LOGINFO("Loading blockchain from DB");
  initializeBlockchain();

  // Get the latest block from the database
  LOGINFO("Loading latest block");
  const Bytes latestBlockHash = blocksDb_.getLastByPrefix(DBPrefix::heightToBlock);
  if (latestBlockHash.empty()) throw DynamicException("Latest block hash not found in DB");

  const Bytes latestBlockBytes = blocksDb_.get(latestBlockHash, DBPrefix::blocks);
  if (latestBlockBytes.empty()) throw DynamicException("Latest block bytes not found in DB");

  latest_ = std::make_shared<const FinalizedBlock>(FinalizedBlock::fromBytes(latestBlockBytes, this->options_.getChainID()));
  LOGINFO("Latest block successfully loaded");

  // If the legacy events folder exists, migrate them to the new folder
  std::filesystem::path legacyEventsPath(options.getRootPath() + "/eventsDb/");
  if (std::filesystem::exists(legacyEventsPath)) {
    Utils::safePrint("Detected legacy event DB, migrating to new DB");
    DB legacyEvents(legacyEventsPath);

    auto transaction = eventsDb_.transaction();

    for (uint64_t block = 0; block <= latest_.load()->getNHeight(); block++) {
      if (block % 10000 == 0) {
        Utils::safePrint("Migrated events for block " + std::to_string(block) + " total block: " + std::to_string(latest_.load()->getNHeight()) + " current progress: " + std::to_string(block * 100 / latest_.load()->getNHeight()) + "%");
      }
      if (block % 200000 == 0) {
        Utils::safePrint("Reloading SQLite DB transactions");
        transaction->commit();
        transaction = eventsDb_.transaction();
      }
      for (const Event& event : getEventsLegacy(legacyEvents, block, block, Address(), {})) {
        eventsDb_.putEvent(event);
      }
    }

    transaction->commit();
    assert(legacyEvents.close());

    std::filesystem::rename(legacyEventsPath, options.getRootPath() + "/legacyEventsDb/");
  }
}

void Storage::initializeBlockchain() {
  // Genesis block comes from Options, not hardcoded
  const auto& genesis = options_.getGenesisBlock();

  if (
    const Bytes latestBlockHash = blocksDb_.getLastByPrefix(DBPrefix::heightToBlock);
    latestBlockHash.empty()
  ) {
    if (genesis.getNHeight() != 0) throw DynamicException("Genesis block height is not 0");
    storeBlock(blocksDb_, genesis, options_.getIndexingMode() != IndexingMode::DISABLED);
    LOGINFO(std::string("Created genesis block: ") + Hex::fromBytes(genesis.getHash()).get());
  }

  // Sanity check for genesis block. (check if genesis in DB matches genesis in Options)
  const Hash genesisInDBHash(blocksDb_.get(UintConv::uint64ToBytes(0), DBPrefix::heightToBlock));

  FinalizedBlock genesisInDB = FinalizedBlock::fromBytes(
    blocksDb_.get(genesisInDBHash, DBPrefix::blocks), options_.getChainID()
  );

  if (genesis != genesisInDB) {
    LOGERROR("Sanity Check! Genesis block in DB does not match genesis block in Options");
    throw DynamicException("Sanity Check! Genesis block in DB does not match genesis block in Options");
  }
}

TxBlock Storage::getTxFromBlockWithIndex(View<Bytes> blockData, uint64_t txIndex) const {
  uint64_t index = 217; // Start of block tx range
  // Count txs until index.
  uint64_t currentTx = 0;
  while (currentTx < txIndex) {
    uint32_t txSize = UintConv::bytesToUint32(blockData.subspan(index, 4));
    index += txSize + 4;
    currentTx++;
  }
  uint64_t txSize = UintConv::bytesToUint32(blockData.subspan(index, 4));
  index += 4;
  return TxBlock(blockData.subspan(index, txSize), this->options_.getChainID());
}

void Storage::pushBlock(FinalizedBlock block) {
  if (auto previousBlock = latest_.load(); previousBlock->getHash() != block.getPrevBlockHash()) {
    throw DynamicException("\"previous hash\" of new block does not match the latest block hash");
  }
  auto newBlock = std::make_shared<FinalizedBlock>(std::move(block));
  latest_.store(newBlock);
  storeBlock(blocksDb_, *newBlock, options_.getIndexingMode() != IndexingMode::DISABLED);
}

bool Storage::blockExists(const Hash& hash) const { return blocksDb_.has(hash, DBPrefix::blocks); }

bool Storage::blockExists(uint64_t height) const { return blocksDb_.has(UintConv::uint64ToBytes(height), DBPrefix::heightToBlock); }

bool Storage::txExists(const Hash& tx) const { return blocksDb_.has(tx, DBPrefix::txToBlock); }

std::shared_ptr<const FinalizedBlock> Storage::getBlock(const Hash& hash) const {
  Bytes blockBytes = blocksDb_.get(hash, DBPrefix::blocks);
  if (blockBytes.empty()) return nullptr;
  return std::make_shared<FinalizedBlock>(FinalizedBlock::fromBytes(blockBytes, this->options_.getChainID()));
}

std::shared_ptr<const FinalizedBlock> Storage::getBlock(uint64_t height) const {
  Bytes blockHash = blocksDb_.get(UintConv::uint64ToBytes(height), DBPrefix::heightToBlock);
  if (blockHash.empty()) return nullptr;
  Bytes blockBytes = blocksDb_.get(blockHash, DBPrefix::blocks);
  return std::make_shared<FinalizedBlock>(FinalizedBlock::fromBytes(blockBytes, this->options_.getChainID()));
}

std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
> Storage::getTx(const Hash& tx) const {
  const Bytes txData = blocksDb_.get(tx, DBPrefix::txToBlock);
  if (txData.empty()) return std::make_tuple(nullptr, Hash(), 0u, 0u);

  const View<Bytes> txDataView = txData;
  const Hash blockHash(txDataView.subspan(0, 32));
  const uint64_t blockIndex = UintConv::bytesToUint32(txDataView.subspan(32, 4));
  const uint64_t blockHeight = UintConv::bytesToUint64(txDataView.subspan(36, 8));
  const Bytes blockData = blocksDb_.get(blockHash, DBPrefix::blocks);

  return std::make_tuple(
    std::make_shared<const TxBlock>(getTxFromBlockWithIndex(blockData, blockIndex)),
    blockHash, blockIndex, blockHeight
  );
}

std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
> Storage::getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex) const {
  const Bytes blockData = blocksDb_.get(blockHash, DBPrefix::blocks);
  if (blockData.empty()) std::make_tuple(nullptr, Hash(), 0u, 0u);

  const uint64_t blockHeight = UintConv::bytesToUint64(View<Bytes>(blockData).subspan(201, 8));
  return std::make_tuple(
    std::make_shared<TxBlock>(getTxFromBlockWithIndex(blockData, blockIndex)),
    blockHash, blockIndex, blockHeight
  );
}

std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
> Storage::getTxByBlockNumberAndIndex(uint64_t blockHeight, uint64_t blockIndex) const {
  const Bytes blockHash = blocksDb_.get(UintConv::uint64ToBytes(blockHeight), DBPrefix::heightToBlock);
  if (blockHash.empty()) return std::make_tuple(nullptr, Hash(), 0u, 0u);

  const Bytes blockData = blocksDb_.get(blockHash, DBPrefix::blocks);
  if (blockData.empty()) return std::make_tuple(nullptr, Hash(), 0u, 0u);

  return std::make_tuple(
    std::make_shared<TxBlock>(getTxFromBlockWithIndex(blockData, blockIndex)),
    Hash(blockHash), blockIndex, blockHeight
  );
}

std::shared_ptr<const FinalizedBlock> Storage::latest() const { return latest_.load(); }

uint64_t Storage::currentChainSize() const {
  auto latest = latest_.load();
  return latest ? (latest->getNHeight() + 1) : 0u;
}

void Storage::putCallTrace(const Hash& txHash, const trace::Call& callTrace) {
  Bytes serial;
  zpp::bits::out out(serial);
  out(callTrace).or_throw();
  blocksDb_.put(txHash, serial, DBPrefix::txToCallTrace);
}

std::optional<trace::Call> Storage::getCallTrace(const Hash& txHash) const {
  Bytes serial = blocksDb_.get(txHash, DBPrefix::txToCallTrace);
  if (serial.empty()) return std::nullopt;

  trace::Call callTrace;
  zpp::bits::in in(serial);
  in(callTrace).or_throw();

  return callTrace;
}

void Storage::putTxAdditionalData(const TxAdditionalData& txData) {
  Bytes serialized;
  zpp::bits::out out(serialized);
  out(txData).or_throw();
  std::cout << "putTxAdditionalData:" << Hex::fromBytes(serialized).get() << std::endl;
  blocksDb_.put(txData.hash, serialized, DBPrefix::txToAdditionalData);
}

std::optional<TxAdditionalData> Storage::getTxAdditionalData(const Hash& txHash) const {
  Bytes serialized = blocksDb_.get(txHash, DBPrefix::txToAdditionalData);
  if (serialized.empty()) return std::nullopt;

  TxAdditionalData txData;
  zpp::bits::in in(serialized);
  in(txData).or_throw();
  return txData;
}

std::vector<Event> Storage::getEventsLegacy(const DB& legacyEventsDB, uint64_t fromBlock, uint64_t toBlock, const Address& address, const std::vector<Hash>& topics) const {
  if (toBlock < fromBlock) std::swap(fromBlock, toBlock);

  if (uint64_t count = toBlock - fromBlock + 1; count > options_.getEventBlockCap()) {
    throw std::out_of_range(
      "Block range too large for event querying! Max allowed is " +
      std::to_string(this->options_.getEventBlockCap())
    );
  }

  std::vector<Event> events;
  std::vector<Bytes> keys;

  const Bytes startBytes = Utils::makeBytes(UintConv::uint64ToBytes(fromBlock));
  const Bytes endBytes = Utils::makeBytes(UintConv::uint64ToBytes(toBlock));

  auto dbKeys = legacyEventsDB.getKeys(DBPrefix::events, startBytes, endBytes);

  for (Bytes key : dbKeys) {
    uint64_t nHeight = UintConv::bytesToUint64(Utils::create_view_span(key, 0, 8));
    Address currentAddress(Utils::create_view_span(key, 24, 20));
    if (fromBlock > nHeight || toBlock < nHeight) {
      continue;
    }
    if (address == currentAddress || address == Address()) keys.push_back(std::move(key));
  }

  if (!keys.empty()) {
    for (DBEntry item : legacyEventsDB.getBatch(DBPrefix::events, keys)) {
      if (events.size() >= options_.getEventLogCap()) break;
      Event event(StrConv::bytesToString(item.value));
      if (topicsMatch(event, topics)) events.push_back(std::move(event));
    }
  }
  return events;
}
