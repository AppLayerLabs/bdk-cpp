#include "event.h"

Event::Event(const std::string& jsonstr) {
  json obj = json::parse(jsonstr);
  this->name_ = obj["name"].get<std::string>();
  this->logIndex_ = obj["logIndex"].get<uint64_t>();
  this->txHash_ = Hash(Hex::toBytes(obj["txHash"].get<std::string>().substr(2)));
  this->txIndex_ = obj["txIndex"].get<uint64_t>();
  this->blockHash_ = Hash(Hex::toBytes(obj["blockHash"].get<std::string>().substr(2)));
  this->blockIndex_ = obj["blockIndex"].get<uint64_t>();
  this->address_ = Address(obj["address"].get<std::string>(), false);
  this->data_ = obj["data"].get<Bytes>();
  for (std::string topic : obj["topics"]) this->topics_.push_back(Hex::toBytes(topic));
  this->anonymous_ = obj["anonymous"].get<bool>();
}

std::string Event::serialize() const {
  json topicArr = json::array();
  for (const Hash& b : this->topics_) topicArr.push_back(b.hex(true).get());
  json obj = {
    {"name", this->name_},
    {"logIndex", this->logIndex_},
    {"txHash", this->txHash_.hex(true).get()},
    {"txIndex", this->txIndex_},
    {"blockHash", this->blockHash_.hex(true).get()},
    {"blockIndex", this->blockIndex_},
    {"address", this->address_.hex(true).get()},
    {"data", this->data_},
    {"topics", topicArr},
    {"anonymous", this->anonymous_}
  };
  return obj.dump();
}

std::string Event::serializeForRPC() {
  std::vector<std::string> topicStr;
  for (const Hash& b : this->topics_) topicStr.push_back(b.hex(true).get());
  json obj = {
    {"address", this->address_.hex(true).get()},
    {"blockHash", this->blockHash_.hex(true).get()},
    {"blockNumber", Hex::fromBytes(Utils::uint64ToBytes(this->blockIndex_), true).get()},
    {"data", Hex::fromBytes(this->data_, true).get()},
    {"logIndex", Hex::fromBytes(Utils::uint64ToBytes(this->logIndex_), true).get()},
    {"removed", false}, // We don't fake/alter events like Ethereum does
    {"topics", topicStr},
    {"transactionHash", this->txHash_.hex(true).get()},
    {"transactionIndex", Hex::fromBytes(Utils::uint64ToBytes(this->txIndex_), true).get()}
  };
  return obj.dump();
}

EventManager::EventManager(const std::unique_ptr<DB>& db) : db_(db) {
  std::vector<DBEntry> allEvents = this->db_->getBatch(DBPrefix::events);
  for (DBEntry& event : allEvents) {
    Event e(Utils::bytesToString(event.value)); // Create a new Event object by deserializing
    this->events_.insert(std::move(e)); // Use insert for MultiIndex container
  }
}

EventManager::~EventManager() {
  DBBatch batchedOperations;
  {
    std::unique_lock<std::shared_mutex> lock(this->lock_);
    for (const auto& event : this->events_) {
      // Build the key (address + block height + tx index + log index)
      Bytes key = event.getAddress().asBytes();
      key.reserve(key.size() + 8 + 8 + 8);
      Utils::appendBytes(key, Utils::uint64ToBytes(event.getBlockIndex()));
      Utils::appendBytes(key, Utils::uint64ToBytes(event.getTxIndex()));
      Utils::appendBytes(key, Utils::uint64ToBytes(event.getLogIndex()));
      // Serialize the value to a JSON string and insert into the batch
      batchedOperations.push_back(key, Utils::stringToBytes(event.serialize()), DBPrefix::events);
    }
  }
  // Batch save to database and clear the list
  this->db_->putBatch(batchedOperations);
  this->events_.clear();
}

std::vector<Event> EventManager::getEvents(
  const uint64_t& fromBlock, const uint64_t& toBlock, const Address& address, const std::vector<Hash>& topics
) {
  std::vector<Event> ret;
  std::vector<Bytes> dbKeys;

  // Throw if block height diff is greater than the block cap
  uint64_t heightDiff = std::max(fromBlock, toBlock) - std::min(fromBlock, toBlock);
  if (heightDiff > this->blockCap_) throw std::runtime_error(
    "Block range too large for event querying! Max allowed is " + std::to_string(this->blockCap_)
  );

  // Query the MultiIndex container
  auto& blockIndexIndex = this->events_.get<0>(); // Assuming blockIndex_ is the first index
  for (auto it = blockIndexIndex.lower_bound(fromBlock); it != blockIndexIndex.upper_bound(toBlock); ++it) {
    const Event& e = *it;
    if ((address != Address() && e.getAddress() != address)) continue;

    if (!topics.empty()) {
      bool hasTopic = true;
      const std::vector<Hash>& eventTopics = e.getTopics();
      if (eventTopics.size() < topics.size()) continue;
      for (size_t i = 0; i < topics.size(); i++) {
        if (topics.at(i) != eventTopics.at(i)) { hasTopic = false; break; }
      }
      if (!hasTopic) continue;
    }
    ret.push_back(e);
    if (ret.size() >= this->logCap_) return ret;
  }

  // Check relevant keys in the database, limiting the query to the specified block range
  Bytes fromBytes;
  Bytes toBytes;
  Utils::appendBytes(fromBytes, Utils::uint64ToBytes(fromBlock));
  Utils::appendBytes(toBytes, Utils::uint64ToBytes(toBlock));
  if (address != Address()) {
    Utils::appendBytes(fromBytes, address.asBytes());
    Utils::appendBytes(toBytes, address.asBytes());
  }

  for (Bytes key : this->db_->getKeys(DBPrefix::events, fromBytes, toBytes)) {
    uint64_t blockHeight = Utils::bytesToUint64(Utils::create_view_span(key, 0, 8));
    Address add(Utils::create_view_span(key, 8, 20));
    if ((fromBlock <= blockHeight && blockHeight <= toBlock) && (address != Address() || address == add)) {
      dbKeys.push_back(key);
      if (ret.size() + dbKeys.size() >= this->logCap_) break;
    }
  }

  // Get events from the database
  for (DBEntry item : this->db_->getBatch(DBPrefix::events, dbKeys)) {
    Event e(Utils::bytesToString(item.value));
    if (!topics.empty()) {
      bool hasTopic = true;
      const std::vector<Hash>& eventTopics = e.getTopics();
      if (eventTopics.size() < topics.size()) continue;
      for (size_t i = 0; i < topics.size(); i++) {
        if (topics.at(i) != eventTopics.at(i)) { hasTopic = false; break; }
      }
      if (!hasTopic) continue;
    }
    ret.push_back(e);
    if (ret.size() >= this->logCap_) return ret;
  }

  return ret;
}

