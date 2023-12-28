#include "event.h"

Event::Event(
  const std::string& name, Address address,
  std::vector<std::pair<BaseTypes, bool>> params, bool anonymous
) : name_(name), address_(address), anonymous_(anonymous)
{
  // Iterate through parameters, if any
  std::string funcStr = this->name_ + "(";
  ABI::Encoder::EncVar dataParams;
  if (!params.empty()) {
    for (std::pair<BaseTypes, bool> p : params) { // type, indexed
      ABI::Types funcType = ABI::BaseTypesToEnum(p.first);
      funcStr += ABI::getStringFromABIEnum(funcType) + ",";
      if (p.second && ((anonymous && this->topics_.size() < 4) || (this->topics_.size() < 3))) {
        // Indexed param goes to topics
        this->topics_.push_back(this->encodeTopicParam(p.first, funcType));
      } else {
        // Non-indexed param (or indexed param that doesn't fit in topics) goes to data
        dataParams.push_back(p.first);
      }
    }
    funcStr.pop_back(); // Remove last ","
    funcStr += ")";
  }

  // Fill up data
  if (!dataParams.empty()) {
    ABI::Encoder dataEnc(dataParams);
    this->data_ = dataEnc.getData();
  }

  // Insert event signature as topics[0] if non-anonymous
  if (!anonymous) {
    Bytes funcBytes = Utils::sha3(Utils::create_view_span(funcStr)).asBytes();
    this->topics_.insert(this->topics_.begin(), funcBytes);
  }
}

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

// TODO: check later if ABI encoding is correct for topics
Bytes Event::encodeTopicParam(BaseTypes& param, ABI::Types type) {
  Bytes ret;
  ABI::Encoder enc({param});
  Bytes encRes = enc.getData();
  std::string typeStr = ABI::getStringFromABIEnum(type);
  if (false) {
    ; // TODO: structs: keccak of concated values padded to a multiple of 32 bytes
  } else if (typeStr.ends_with("[]")) {
    // arrays: keccak of concated values padded to a multiple of 32 bytes and without any length prefix
    // we just discard both offset and length (substr(64,end)) and that's pretty much it
    BytesArrView dataView = Utils::create_view_span(encRes, 64, encRes.size());
    ret = Utils::sha3(dataView).asBytes();
  } else if (type == ABI::Types::bytes || type == ABI::Types::string) {
    // bytes, string: keccak of value without padding or length prefixes
    // we get the length of the real data, discard both offset and length,
    // and then get the data string without the padding
    // substr(0,32) = offset, substr(32,32) = length, substr(64,end) = data with padding
    BytesArrView lenView = Utils::create_view_span(encRes, 32, 32);
    uint256_t dataLenOri = Utils::bytesToUint256(lenView);
    uint64_t dataLen = uint64_t(dataLenOri);  // Max real string size IRL is uint64_t anyway
    BytesArrView dataView = Utils::create_view_span(encRes, 64, dataLen);
    ret = Utils::sha3(dataView).asBytes();
  } else {
    ret = encRes; // (u)int, address, bool: regular padded type, business as usual
  }
  return ret;
}

std::string Event::serialize() {
  json topicArr = json::array();
  for (Bytes b : this->topics_) topicArr.push_back(Hex::fromBytes(b, true).get());
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
  for (Bytes b : this->topics_) topicStr.push_back(Hex::fromBytes(b, true).get());
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
  uint64_t fromBlock, uint64_t toBlock, Address address, std::vector<Bytes> topics
) {
  std::vector<Event> ret;
  std::vector<Bytes> dbKeys;

  // Apply block cap (using starting block and later ranges as anchors)
  // e.g. 5000 - 1000 = 4000, range would be from 3000 to 5000, not 1000 to 3000
  if (fromBlock > toBlock) {
    uint64_t blockDiff = (fromBlock - toBlock) - this->blockCap_;
    toBlock += blockDiff;
  } else if (toBlock > fromBlock) {
    uint64_t blockDiff = (toBlock - fromBlock) - this->blockCap_;
    fromBlock += blockDiff;
  }

  // Get events in memory first
  for (Event e : this->events_) {
    if ((fromBlock <= e.getBlockIndex() <= toBlock) && (address != Address() || address == e.getAddress())) {
      if (!topics.empty()) {
        bool hasTopic = false;
        std::vector<Bytes> eventTopics = e.getTopics();
        for (size_t i = 0, j = 0;;) {
          if (j >= eventTopics.size()) { i++; j = 0; if (i >= topics.size()) break; }
          if (topics.at(i) == eventTopics.at(j)) { hasTopic = true; break; } else j++;
        }
        if (!hasTopic) continue;
      }
      ret.push_back(e);
      if (ret.size() >= this->logCap_) return ret;
    }
  }

  // Check relevant keys in the database
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
      bool hasTopic = false;
      std::vector<Bytes> eventTopics = e.getTopics();
      for (size_t i = 0, j = 0;;) {
        if (j >= eventTopics.size()) { i++; j = 0; if (i >= topics.size()) break; }
        if (topics.at(i) == eventTopics.at(j)) { hasTopic = true; break; } else j++;
      }
      if (!hasTopic) continue;
    }
    ret.push_back(e);
    if (ret.size() >= this->logCap_) return ret;
  }

  return ret;
}

