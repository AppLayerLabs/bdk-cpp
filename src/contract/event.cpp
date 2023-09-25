#include "event.h"

Event::Event(
  const std::string& name, uint64_t logIndex, Hash txHash, uint64_t txIndex,
  Hash blockHash, uint256_t blockIndex, Address address,
  std::vector<std::pair<BaseTypes, bool>> params, bool anonymous
) : name_(name), logIndex_(logIndex), txHash_(txHash), txIndex_(txIndex),
  blockHash_(blockHash), blockIndex_(blockIndex), address_(address), anonymous_(anonymous)
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

// TODO: check later if ABI encoding is correct for topics
Bytes Event::encodeTopicParam(BaseTypes& param, ABI::Types type) {
  Bytes ret;
  ABI::Encoder enc({param});
  Bytes encRes = enc.getData();
  std::string typeStr = ABI::getStringFromABIEnum(type);
  if (false) {
    // TODO: structs: keccak of concated values padded to a multiple of 32 bytes
    ;
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
    // (u)int, address, bool: regular padded type, business as usual
    ret = encRes;
  }
  return ret;
}

