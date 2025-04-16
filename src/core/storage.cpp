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
    eventsDb_(blockchain_.opt().getRootPath() + "/newEventsDb/")
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

void Storage::putValidatorUpdates(uint64_t height, Bytes validatorUpdates) {
  blocksDb_.put(UintConv::uint64ToBytes(height), validatorUpdates, DBPrefix::validatorUpdates);
}

