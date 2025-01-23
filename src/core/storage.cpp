/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "storage.h"

#include "../utils/strconv.h"
#include "../utils/uintconv.h"

#include "blockchain.h"

bool Storage::topicsMatch(const Event& event, const std::vector<Hash>& topics) {
  if (topics.empty()) return true; // No topic filter applied
  const std::vector<Hash>& eventTopics = event.getTopics();
  if (eventTopics.size() < topics.size()) return false;
  for (size_t i = 0; i < topics.size(); i++) if (topics.at(i) != eventTopics[i]) return false;
  return true;
}

Storage::Storage(Blockchain& blockchain)
  : blockchain_(blockchain),
    blocksDb_(blockchain_.opt().getRootPath() + "/blocksDb/"), // Uncompressed
    eventsDb_(blockchain_.opt().getRootPath() + "/eventsDb/", true) // Compressed
{
}

std::string Storage::getLogicalLocation() const {
  return blockchain_.getLogicalLocation();
}

IndexingMode Storage::getIndexingMode() const {
  return blockchain_.opt().getIndexingMode();
}

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

