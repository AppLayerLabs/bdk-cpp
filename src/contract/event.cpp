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
    this->events_.insert(std::move(e));
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
    const uint64_t& fromBlock, const uint64_t& toBlock, 
    const std::optional<Address>& address, const std::optional<std::vector<Hash>>& topics
) {
    std::vector<Event> ret;

    // Check if the block range is within limits
    uint64_t heightDiff = std::max(fromBlock, toBlock) - std::min(fromBlock, toBlock);
    if (heightDiff > this->blockCap_) {
        throw std::runtime_error("Block range too large for event grepping!");
    }

    // Step 1: Initial Filtering from MultiIndex Container
    std::vector<Event> filteredEvents = filterEventsInMemory(fromBlock, toBlock, address);

    // Step 2: Manual Topic Matching for In-Memory Events
    for (const Event& event : filteredEvents) {
        if (matchTopics(event, topics) && ret.size() < this->logCap_) {
            ret.push_back(event);
        }
    }

    // Step 3: Fetch and Filter Events from the Database
    if (ret.size() < this->logCap_) {
        fetchAndFilterEventsFromDB(fromBlock, toBlock, address, topics, ret);
    }

    return ret;
}

std::vector<Event> EventManager::filterEventsInMemory(const uint64_t& fromBlock, const uint64_t& toBlock, const std::optional<Address>& address) {
    std::vector<Event> filteredEvents;

    if (address.has_value()) {
        auto& addressIndex = events_.get<1>();
        for (auto it = addressIndex.lower_bound(address.value());
             it != addressIndex.end() && it->getBlockIndex() <= toBlock;
             ++it) {
            if (it->getBlockIndex() >= fromBlock) {
                filteredEvents.push_back(*it);
            }
        }
    } else {
        auto& blockIndex = events_.get<0>();
        // Capture 'fromBlock' and 'toBlock' explicitly in a lambda capture clause
        for (const Event& e : blockIndex) {
            if (e.getBlockIndex() >= fromBlock && e.getBlockIndex() <= toBlock) {
                filteredEvents.push_back(e);
            }
        }
    }

    return filteredEvents;
}

void EventManager::fetchAndFilterEventsFromDB(
    const uint64_t& fromBlock, const uint64_t& toBlock, 
    const std::optional<Address>& address, const std::optional<std::vector<Hash>>& topics, 
    std::vector<Event>& ret
) {
    // Database fetching logic
    std::vector<Bytes> dbKeys;
    for (Bytes key : this->db_->getKeys(DBPrefix::events)) {
        Address addr(Utils::create_view_span(key, 0, 20)); // Extract address from key
        uint64_t blockHeight = Utils::bytesToUint64(Utils::create_view_span(key, 20, 8)); // Extract block height from key

        if ((fromBlock <= blockHeight && blockHeight <= toBlock) && 
            (!address.has_value() || address.value() == addr)) {
            dbKeys.push_back(key);
        }
    }

    for (DBEntry item : this->db_->getBatch(DBPrefix::events, dbKeys)) {
        Event e(Utils::bytesToString(item.value));
        if (matchTopics(e, topics) && ret.size() < this->logCap_) {
            ret.push_back(e);
        }
    }
}

bool EventManager::matchTopics(const Event& event, const std::optional<std::vector<Hash>>& topics) {
    if (!topics.has_value()) return true; // No topic filter applied
    const std::vector<Hash>& eventTopics = event.getTopics();
    if (eventTopics.size() < topics->size()) return false;

    for (size_t i = 0; i < topics->size(); ++i) {
        if ((*topics)[i] != eventTopics[i]) {
            return false;
        }
    }
    return true;
}

