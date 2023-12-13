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
  this->txHash_ = Hash(obj["txHash"].get<std::string>());
  this->txIndex_ = obj["txIndex"].get<uint64_t>();
  this->blockHash_ = Hash(obj["blockHash"].get<std::string>());
  this->blockIndex_ = obj["blockIndex"].get<uint64_t>();
  this->address_ = Address(obj["address"].get<std::string>(), false);
  this->data_ = obj["data"].get<Bytes>();
  this->topics_ = obj["topics"].get<std::vector<Bytes>>();
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
    uint32_t dataLen = Utils::bytesToUint32(lenView);
    BytesArrView dataView = Utils::create_view_span(encRes, 64, dataLen);
    ret = Utils::sha3(dataView).asBytes();
  } else {
    ret = encRes; // (u)int, address, bool: regular padded type, business as usual
  }
  return ret;
}

std::string Event::serialize() {
  json obj = {
    {"name", this->name_},
    {"logIndex", this->logIndex_},
    {"txHash", this->txHash_.get()},
    {"txIndex", this->txIndex_},
    {"blockHash", this->blockHash_.get()},
    {"blockIndex", this->blockIndex_},
    {"address", this->address_.get()},
    {"data_", this->data_},
    {"topics_", this->topics_},
    {"anonymous", this->anonymous_}
  };
  return obj.dump();
}

EventManager::EventManager(const std::unique_ptr<DB>& db) : db_(db) {
  std::vector<DBEntry> allEvents = this->db_->getBatch(DBPrefix::events);
  for (DBEntry& event : allEvents) {
    std::string data = Utils::bytesToString(event.value); // Convert the database bytes back to a JSON string
    Event e(Utils::bytesToString(event.value)); // Create a new Event object by deserializing the JSON string
    this->events_.push_back(std::move(e)); // Move the object into the list
  }
}

EventManager::~EventManager() {
  DBBatch batchedOperations;
  {
    std::unique_lock<std::shared_mutex> lock(this->lock_);
    while (!this->events_.empty()) {
      // Build the key (address + block height + tx index + log index)
      auto it = this->events_.begin();
      Bytes key = it->getAddress().asBytes();
      key.reserve(key.size() + 8 + 8 + 8);
      Utils::appendBytes(key, Utils::uint64ToBytes(it->getBlockIndex()));
      Utils::appendBytes(key, Utils::uint64ToBytes(it->getTxIndex()));
      Utils::appendBytes(key, Utils::uint64ToBytes(it->getLogIndex()));

      // Serialize the value to a JSON string, insert into the batch and delete from the list
      batchedOperations.push_back(key, Utils::stringToBytes(it->serialize()), DBPrefix::events);
      this->events_.erase(this->events_.begin());
    }
  }
  this->db_->putBatch(batchedOperations); // Batch save to database
}

