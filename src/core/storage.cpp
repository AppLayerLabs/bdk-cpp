/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "storage.h"

#include "../utils/strconv.h"
#include "../utils/uintconv.h"

#include "blockchain.h"

static bool topicsMatch(const Event& event, const std::vector<Hash>& topics) {
  if (topics.empty()) return true; // No topic filter applied
  const std::vector<Hash>& eventTopics = event.getTopics();
  if (eventTopics.size() < topics.size()) return false;
  for (size_t i = 0; i < topics.size(); i++) if (topics.at(i) != eventTopics[i]) return false;
  return true;
}

/*
static void storeBlock(DB& db, const FinalizedBlock& block, bool indexingEnabled) {
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
*/

Storage::Storage(Blockchain& blockchain)
  : blockchain_(blockchain),
    blocksDb_(blockchain_.opt().getRootPath() + "/blocksDb/"), // Uncompressed
    eventsDb_(blockchain_.opt().getRootPath() + "/eventsDb/", true), // Compressed
    txMapDb_(blockchain_.opt().getRootPath() + "/txMapDb/") // Uncompressed
{
}

std::string Storage::getLogicalLocation() const {
  return blockchain_.getLogicalLocation();
}

IndexingMode Storage::getIndexingMode() const {
  return blockchain_.opt().getIndexingMode();
}

/*
Storage::Storage(std::string instanceIdStr, const Options& options)
  : blocksDb_(options.getRootPath() + "/blocksDb/"),  // Uncompressed
    eventsDb_(options.getRootPath() + "/eventsDb/", true),  // Compressed
    options_(options), instanceIdStr_(std::move(instanceIdStr))
{
  // Initialize the blockchain if latest block doesn't exist.
  //LOGINFO("Loading blockchain from DB");
  //initializeBlockchain();

  // Get the latest block from the database
  //LOGINFO("Loading latest block");
  //const Bytes latestBlockHash = blocksDb_.getLastByPrefix(DBPrefix::heightToBlock);
  //if (latestBlockHash.empty()) throw DynamicException("Latest block hash not found in DB");

  //const Bytes latestBlockBytes = blocksDb_.get(latestBlockHash, DBPrefix::blocks);
  //if (latestBlockBytes.empty()) throw DynamicException("Latest block bytes not found in DB");

  //latest_ = std::make_shared<const FinalizedBlock>(FinalizedBlock::fromBytes(latestBlockBytes, this->options_.getChainID()));
  //LOGINFO("Latest block successfully loaded");
}
*/

/*
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

bool Storage::blockExists(const Hash& hash) const { return blocksDb_.has(hash, DBPrefix::blocks); }

bool Storage::blockExists(uint64_t height) const { return blocksDb_.has(UintConv::uint64ToBytes(height), DBPrefix::heightToBlock); }

uint64_t Storage::currentChainSize() const {
  auto latest = latest_.load();
  return latest ? (latest->getNHeight() + 1) : 0u;
}

bool Storage::txExists(const Hash& tx) const { return blocksDb_.has(tx, DBPrefix::txToBlock); }
*/

void Storage::putTxAdditionalData(const TxAdditionalData& txData) {
  Bytes serialized;
  zpp::bits::out out(serialized);
  out(txData).or_throw();
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

void Storage::putEvent(const Event& event) {
  //LOGDEBUG("Storage::putEvent("
  //  + std::to_string(event.getBlockIndex()) + ","
  //  + std::to_string(event.getTxIndex()) + ","
  //  + std::to_string(event.getLogIndex()) + ","
  //  + Hex::fromBytes(event.getAddress()).get()
  //);
  const Bytes key = Utils::makeBytes(bytes::join(
    UintConv::uint64ToBytes(event.getBlockIndex()),
    UintConv::uint64ToBytes(event.getTxIndex()),
    UintConv::uint64ToBytes(event.getLogIndex()),
    event.getAddress()
  ));
  eventsDb_.put(key, StrConv::stringToBytes(event.serializeToJson()), DBPrefix::events);
}

std::vector<Event> Storage::getEvents(uint64_t fromBlock, uint64_t toBlock, const Address& address, const std::vector<Hash>& topics) const {
  //std::string topicsStr;
  //for (const Hash& topic : topics) {
  //  topicsStr += topic.hex().get() + ", ";
  //}
  //LOGDEBUG("Storage::getEvents("
  //  + std::to_string(fromBlock) + ","
  //  + std::to_string(toBlock) + ","
  //  + Hex::fromBytes(address).get() + ","
  //  + topicsStr
  //);

  if (toBlock < fromBlock) std::swap(fromBlock, toBlock);

  if (uint64_t count = toBlock - fromBlock + 1; count > blockchain_.opt().getEventBlockCap()) {
    throw std::out_of_range(
      "Block range too large for event querying! Max allowed is " +
      std::to_string(this->blockchain_.opt().getEventBlockCap())
    );
  }

  std::vector<Event> events;
  std::vector<Bytes> keys;

  const Bytes startBytes = Utils::makeBytes(UintConv::uint64ToBytes(fromBlock));
  const Bytes endBytes = Utils::makeBytes(UintConv::uint64ToBytes(toBlock));

  for (Bytes key : eventsDb_.getKeys(DBPrefix::events, startBytes, endBytes)) {
    uint64_t nHeight = UintConv::bytesToUint64(Utils::create_view_span(key, 0, 8));
    Address currentAddress(Utils::create_view_span(key, 24, 20));
    if (fromBlock > nHeight || toBlock < nHeight) continue;
    if (address == currentAddress || address == Address()) keys.push_back(std::move(key));
  }

  for (DBEntry item : eventsDb_.getBatch(DBPrefix::events, keys)) {
    if (events.size() >= blockchain_.opt().getEventLogCap()) break;
    Event event(StrConv::bytesToString(item.value));
    if (topicsMatch(event, topics)) events.push_back(std::move(event));
  }
  return events;
}

std::vector<Event> Storage::getEvents(uint64_t blockIndex, uint64_t txIndex) const {
  //LOGDEBUG("Storage::getEvents("
  //  + std::to_string(blockIndex) + ","
  //  + std::to_string(txIndex)
  //);

  std::vector<Event> events;
  for (
    Bytes fetchBytes = Utils::makeBytes(bytes::join(
      DBPrefix::events, UintConv::uint64ToBytes(blockIndex), UintConv::uint64ToBytes(txIndex)
    ));
    DBEntry entry : eventsDb_.getBatch(fetchBytes)
  ) {
    if (events.size() >= blockchain_.opt().getEventLogCap()) break;
    events.emplace_back(StrConv::bytesToString(entry.value));
  }
  return events;
}

