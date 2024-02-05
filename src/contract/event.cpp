/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

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

std::string Event::serializeForRPC() const {
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

EventManager::EventManager(
  const std::unique_ptr<DB>& db, const std::unique_ptr<Options>& options
) : db_(db), options_(options) {
  std::vector<DBEntry> allEvents = this->db_->getBatch(DBPrefix::events);
  for (const DBEntry& event : allEvents) {
    Event e(Utils::bytesToString(event.value)); // Create a new Event object by deserializing
    this->events_.insert(std::move(e)); // Use insert for MultiIndex container
  }
}

EventManager::~EventManager() {
  DBBatch batchedOperations;
  {
    std::unique_lock<std::shared_mutex> lock(this->lock_);
    for (const auto& e : this->events_) {
      // Build the key (block height + tx index + log index + address)
      Bytes key;
      key.reserve(8 + 8 + 8 + key.size());
      Utils::appendBytes(key, Utils::uint64ToBytes(e.getBlockIndex()));
      Utils::appendBytes(key, Utils::uint64ToBytes(e.getTxIndex()));
      Utils::appendBytes(key, Utils::uint64ToBytes(e.getLogIndex()));
      Utils::appendBytes(key, e.getAddress().asBytes());
      // Serialize the value to a JSON string and insert into the batch
      batchedOperations.push_back(key, Utils::stringToBytes(e.serialize()), DBPrefix::events);
    }
  }
  // Batch save to database and clear the list
  this->db_->putBatch(batchedOperations);
  this->events_.clear();
}

const std::vector<Event> EventManager::getEvents(
  const uint64_t& fromBlock, const uint64_t& toBlock,
  const Address& address, const std::vector<Hash>& topics
) const {
  std::vector<Event> ret;
  // Check if block range is within limits
  uint64_t heightDiff = std::max(fromBlock, toBlock) - std::min(fromBlock, toBlock);
  if (heightDiff > this->options_->getEventBlockCap()) throw std::out_of_range(
    "Block range too large for event querying! Max allowed is " +
    std::to_string(this->options_->getEventBlockCap())
  );
  // Fetch from memory, then match topics from memory
  for (const Event& e : this->filterFromMemory(fromBlock, toBlock, address)) {
    if (this->matchTopics(e, topics) && ret.size() < this->options_->getEventLogCap()) {
      ret.push_back(e);
    }
  }
  if (ret.size() >= this->options_->getEventLogCap()) return ret;
  // Fetch from database if we have space left
  for (const Event& e : this->filterFromDB(fromBlock, toBlock, address, topics)) {
    if (ret.size() >= this->options_->getEventLogCap()) break;
    ret.push_back(std::move(e));
  }
  return ret;
}

const std::vector<Event> EventManager::getEvents(
  const Hash& txHash, const uint64_t& blockIndex, const uint64_t& txIndex
) const {
  std::vector<Event> ret;
  // Fetch from memory
  const auto& txHashIndex = this->events_.get<2>(); // txHash is the third index
  auto [start, end] = txHashIndex.equal_range(txHash);
  for (auto it = start; it != end; it++) {
    if (ret.size() >= this->options_->getEventLogCap()) break;
    const Event& e = *it;
    if (e.getBlockIndex() == blockIndex && e.getTxIndex() == txIndex) ret.push_back(e);
  }
  // Fetch from DB
  Bytes fetchBytes = DBPrefix::events;
  Utils::appendBytes(fetchBytes, Utils::uint64ToBytes(blockIndex));
  Utils::appendBytes(fetchBytes, Utils::uint64ToBytes(txIndex));
  for (DBEntry entry : this->db_->getBatch(fetchBytes)) {
    if (ret.size() >= this->options_->getEventLogCap()) break;
    Event e(Utils::bytesToString(entry.value));
    ret.push_back(e);
  }
  return ret;
}

const std::vector<Event> EventManager::filterFromMemory(
  const uint64_t& fromBlock, const uint64_t& toBlock, const Address& address
) const {
  std::vector<Event> ret;
  if (address != Address()) {
    auto& addressIndex = this->events_.get<1>();
    for (
      auto it = addressIndex.lower_bound(address);
      it != addressIndex.end() && it->getBlockIndex() <= toBlock;
      it++
    ) { if (it->getBlockIndex() >= fromBlock) ret.push_back(*it); }
  } else {
    const auto& blockIndex = this->events_.get<0>();
    for (const Event& e : blockIndex) {
      uint64_t idx = e.getBlockIndex();
      if (idx >= fromBlock && idx <= toBlock) ret.push_back(e);
    }
  }
  return ret;
}

const std::vector<Event> EventManager::filterFromDB(
  const uint64_t& fromBlock, const uint64_t& toBlock,
  const Address& address, const std::vector<Hash>& topics
) const {
  // Filter by block range
  std::vector<Event> ret;
  std::vector<Bytes> dbKeys;
  Bytes startBytes;
  Bytes endBytes;
  Utils::appendBytes(startBytes, Utils::uint64ToBytes(fromBlock));
  Utils::appendBytes(endBytes, Utils::uint64ToBytes(toBlock));

  // Get the keys first, based on block height, then filter by address if there is one
  for (Bytes key : this->db_->getKeys(DBPrefix::events, startBytes, endBytes)) {
    uint64_t nHeight = Utils::bytesToUint64(Utils::create_view_span(key, 0, 8));
    Address addr(Utils::create_view_span(key, 24, 20));
    if (
      (fromBlock <= nHeight && nHeight <= toBlock) &&
      (address == Address() || address == addr)
    ) dbKeys.push_back(key);
  }

  // Get the key values
  for (DBEntry item : this->db_->getBatch(DBPrefix::events, dbKeys)) {
    if (ret.size() >= this->options_->getEventLogCap()) break;
    Event e(Utils::bytesToString(item.value));
    if (this->matchTopics(e, topics)) ret.push_back(e);
  }
  return ret;
}

bool EventManager::matchTopics(
  const Event& event, const std::vector<Hash>& topics
) const {
  if (topics.empty()) return true; // No topic filter applied
  const std::vector<Hash>& eventTopics = event.getTopics();
  if (eventTopics.size() < topics.size()) return false;
  for (size_t i = 0; i < topics.size(); i++) if (topics.at(i) != eventTopics[i]) return false;
  return true;
}

