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
void Storage::setGetTxCacheSize(const uint64_t cacheSize) {
  txCacheSize_ = cacheSize;
  if (txCacheSize_ == 0) {
    std::scoped_lock lock(txCacheMutex_);
    txCache_[0].clear();
    txCache_[1].clear();
    txCacheBucket_ = 0;
  }
}
*/
/*

  no longer needed
  changed to cometbft-bdk w/ sha3 txhash

void Storage::putTxMap(Hash txHashSha3, Hash txHashSha256) {
  txMapDb_.put(txHashSha3.asBytes(), txHashSha256.asBytes(), DBPrefix::txSha3ToSha256);
}

bool Storage::getTxMap(Hash txHashSha3, Hash& txHashSha256) const {
  const Bytes hashData = txMapDb_.get(txHashSha3, DBPrefix::txSha3ToSha256);
  if (hashData.empty()) return false;
  txHashSha256 = Hash(hashData);
  return true;
}
*/

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

TxBlock Storage::getTxFromBlockWithIndex(bytes::View blockData, uint64_t txIndex) const {
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

std::shared_ptr<const FinalizedBlock> Storage::latest() const { return latest_.load(); }

uint64_t Storage::currentChainSize() const {
  auto latest = latest_.load();
  return latest ? (latest->getNHeight() + 1) : 0u;
}

bool Storage::txExists(const Hash& tx) const { return blocksDb_.has(tx, DBPrefix::txToBlock); }
*/

/*
void Storage::putTx(const Hash& tx, const StorageGetTxResultType& val) {
  std::scoped_lock lock(txCacheMutex_);

  auto& activeBucket = txCache_[txCacheBucket_];
  activeBucket[tx] = val;

  if (activeBucket.size() >= txCacheSize_) {
    txCacheBucket_ = 1 - txCacheBucket_;
    txCache_[txCacheBucket_].clear();
  }
}
*/

//StorageGetTxResultType Storage::getTx(const Hash& tx) const {
  /*
  const Bytes txData = blocksDb_.get(tx, DBPrefix::txToBlock);
  if (txData.empty()) return std::make_tuple(nullptr, Hash(), 0u, 0u);

  const bytes::View txDataView = txData;
  const Hash blockHash(txDataView.subspan(0, 32));
  const uint64_t blockIndex = UintConv::bytesToUint32(txDataView.subspan(32, 4));
  const uint64_t blockHeight = UintConv::bytesToUint64(txDataView.subspan(36, 8));
  const Bytes blockData = blocksDb_.get(blockHash, DBPrefix::blocks);

  return std::make_tuple(
    std::make_shared<const TxBlock>(getTxFromBlockWithIndex(blockData, blockIndex)),
    blockHash, blockIndex, blockHeight
  );
  */
/*
  // First thing we would do is check a TxBlock object cached in RAM (or the
  //  TxBlock plus all the other elements in the tuple that we are fetching).
  //
  // FIXME: we NEED this RAM cache because the transaction indexer at the cometbft
  //  end lags a bit -- the transaction simply isn't there for a while AFTER the block
  //  is delivered, so we need to cache this on our end in RAM anyway.
  //  (use the txCache that was in the Comet driver, move it to the Storage class
  //   and use it to cache TxBlock objects instead of raw tx bytes).
  //  the disk retrieval works fine for older data that has been flushed from the
  //  RAM cache already.

  std::unique_lock<std::mutex> lock(txCacheMutex_);
  for (int i = 0; i < 2; ++i) {
    auto& bucket = txCache_[(txCacheBucket_ + i) % 2];
    auto it = bucket.find(tx);
    if (it != bucket.end()) {
      LOGTRACE("Storage::getTx(" + tx.hex().get() + "): cache hit");
      return it->second;
    }
  }
  lock.unlock();
  LOGTRACE("Storage::getTx(" + tx.hex().get() + "): cache miss");

  // Right now we don't have this cache so we just hit cometbft:
  // HACK: sleep a bit so cometbft has time to index the transaction
  // FIXME: take this out when the first test is passing
  //        then add the txCache to Storage
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // Translate `tx` (BDK sha3 hash) to a CometBFT sha256 hash
  Hash txSha256;
  if (getTxMap(tx, txSha256)) {

    // Get the data via a sync (blocking) RPC request to cometbft
    Bytes hx = Hex::toBytes(txSha256.hex());
    std::string encodedHexBytes = base64::encode_into<std::string>(hx.begin(), hx.end());
    json params = { {"hash", encodedHexBytes} };
    json ret;
    if (blockchain_.comet().rpcSyncCall("tx", params, ret)) {
      if (ret.is_object() && ret.contains("result") && ret["result"].is_object()) {
        // Validate returned JSON
        const auto& result = ret["result"];
        if (
            result.contains("tx") && result["tx"].is_string() &&
            result.contains("height") && result["height"].is_string() &&
            result.contains("index") && result["index"].is_number_integer()
          )
        {
          // Base64-decode the tx string data into a Bytes
          Bytes txBytes = base64::decode_into<Bytes>(result["tx"].get<std::string>());

          // Decode Bytes into a TxBlock
          uint64_t chainId = blockchain_.opt().getChainID();
          std::shared_ptr<TxBlock> txBlock = std::make_shared<TxBlock>(txBytes, chainId);

          // Block height and index of tx within block
          uint64_t blockHeight = std::stoull(result["height"].get<std::string>());
          uint64_t blockIndex = result["index"].get<int>();

          // REVIEW: For some reason we need to fill in the hash of the
          //   block as well, but that would be another lookup for the hash
          //   of the block at a given block height.
          //   (This value seems unused and we should remove it anyways).
          Hash blockHash;

          LOGTRACE(
            "getTx(" + tx.hex().get() + "): blockIndex=" +
            std::to_string(blockIndex) + " blockHeight=" +
            std::to_string(blockHeight)
          );

          // Assemble return value
          return std::make_tuple(txBlock, blockHash, blockIndex, blockHeight);
        } else {
         LOGTRACE("getTx(): bad tx call result: " + result.dump());
        }
      } else {
        LOGTRACE("getTx(): bad rpcSyncCall result: " + ret.dump());
      }
    } else {
      LOGTRACE("getTx(): rpcSyncCall('tx') failed");
    }
  } else {
    LOGTRACE("getTx(): cannot find tx sha256");
  }
  LOGTRACE("getTx(" + tx.hex().get() + ") FAIL!");
  return std::make_tuple(nullptr, Hash(), 0u, 0u);
}
*/

/*
std::tuple<
  const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
> Storage::getTxByBlockHashAndIndex(const Hash& blockHash, const uint64_t blockIndex) const {
  const Bytes blockData = blocksDb_.get(blockHash, DBPrefix::blocks);
  if (blockData.empty()) std::make_tuple(nullptr, Hash(), 0u, 0u);

  const uint64_t blockHeight = UintConv::bytesToUint64(bytes::View(blockData).subspan(201, 8));
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

