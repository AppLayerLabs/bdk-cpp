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

std::string Event::serialize() {
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
    Event e(Utils::bytesToString(event.value)); // Create a new Event object by deserializing the JSON string
    this->events_.push_back(std::move(e)); // Move the object into the list
  }
}

EventManager::~EventManager() {
  DBBatch batchedOperations;
  {
    std::unique_lock<std::shared_mutex> lock(this->lock_);
    for (auto it = this->events_.begin(); it != this->events_.end(); it++) {
      // Build the key (address + block height + tx index + log index)
      Bytes key = it->getAddress().asBytes();
      key.reserve(key.size() + 8 + 8 + 8);
      Utils::appendBytes(key, Utils::uint64ToBytes(it->getBlockIndex()));
      Utils::appendBytes(key, Utils::uint64ToBytes(it->getTxIndex()));
      Utils::appendBytes(key, Utils::uint64ToBytes(it->getLogIndex()));
      // Serialize the value to a JSON string and insert into the batch
      batchedOperations.push_back(key, Utils::stringToBytes(it->serialize()), DBPrefix::events);
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

  // Do not allow event grepping if difference is greater than this->blockCap_
  uint64_t heightDiff = std::max(fromBlock, toBlock) - std::min(fromBlock, toBlock);
  if (heightDiff > this->blockCap_) {
    throw std::runtime_error("Block range too large for event grepping!");
  }

  // Get events in memory first
  for (const Event& e : this->events_) {
    if ((fromBlock <= e.getBlockIndex() <= toBlock) && (address != Address() || address == e.getAddress())) {
      if (!topics.empty()) {
        bool hasTopic = true;
        const std::vector<Hash>& eventTopics = e.getTopics();
        /// event topics should be at least as many as the topics we are looking for
        if (eventTopics.size() < topics.size()) continue;
        /// Check if the items within event topics matches the topics we are looking for
        for (size_t i = 0; i < topics.size(); ++i) {
          if (topics.at(i) != eventTopics.at(i)) { hasTopic = false; break; }
        }
        if (!hasTopic) continue;
      }
      ret.push_back(e);
      if (ret.size() >= this->logCap_) return ret;
    }
  }

  // Check relevant keys in the database
  // TODO: Uhhh, this is getting **ALL KEYS** from the database, and then filtering them in memory
  // Getting all the keys from the database is a very expensive operation, specially on large blockchains
  // as you might have millions of events in the database
  // We might want to consider using subkeys for this, or at least a better filtering mechanism
  for (Bytes key : this->db_->getKeys(DBPrefix::events)) {
    Address add(Utils::create_view_span(key, 0, 20)); // (0, 20) = address, (20, 8) = block height
    uint64_t blockHeight = Utils::bytesToUint64(Utils::create_view_span(key, 20, 8));
    if ((fromBlock <= blockHeight <= toBlock) && (address != Address() || address == add)) {
      dbKeys.push_back(key);
    }
  }

  // Get events in the database
  for (DBEntry item : this->db_->getBatch(DBPrefix::events, dbKeys)) {
    Event e(Utils::bytesToString(item.value));
    if (!topics.empty()) {
      bool hasTopic = true;
      const std::vector<Hash>& eventTopics = e.getTopics();
      // event topics should be at least as many as the topics we are looking for
      if (eventTopics.size() < topics.size()) continue;
      // Check if the items within event topics matches the topics we are looking for
      for (size_t i = 0; i < topics.size(); ++i) {
        if (topics.at(i) != eventTopics.at(i)) { hasTopic = false; break; }
      }
      if (!hasTopic) continue;
    }
    ret.push_back(e);
    if (ret.size() >= this->logCap_) return ret;
  }

  return ret;
}

