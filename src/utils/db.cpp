/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "db.h"

DB::DB(const std::string path) {
  this->opts_.create_if_missing = true;
  if (!std::filesystem::exists(path)) { // Ensure the database path can actually be found
    std::filesystem::create_directories(path);
  }
  auto status = rocksdb::DB::Open(this->opts_, path, &this->db_);
  if (!status.ok()) {
    Logger::logToDebug(LogType::ERROR, Log::db, __func__, "Failed to open DB: " + status.ToString());
    throw std::runtime_error("Failed to open DB: " + status.ToString());
  }
}

bool DB::putBatch(const DBBatch& batch) const {
  std::lock_guard lock(this->batchLock_);
  rocksdb::WriteBatch wb;
  for (const rocksdb::Slice &deletes : batch.getDelsSlices()) { wb.Delete(deletes); };
  for (const auto& entry : batch.getPutsSlices()) wb.Put(entry.first, entry.second);
  rocksdb::Status s = this->db_->Write(rocksdb::WriteOptions(), &wb);
  return s.ok();
}

std::vector<DBEntry> DB::getBatch(
  const Bytes& bytesPfx, const std::vector<Bytes>& keys
) const {
  std::lock_guard lock(this->batchLock_);
  std::vector<DBEntry> ret;
  rocksdb::Iterator *it = this->db_->NewIterator(rocksdb::ReadOptions());
  rocksdb::Slice pfx(reinterpret_cast<const char*>(bytesPfx.data()), bytesPfx.size());

  // Search for all entries
  if (keys.empty()) {
    for (it->Seek(pfx); it->Valid(); it->Next()) {
      if (it->key().starts_with(pfx)) {
        auto keySlice = it->key();
        keySlice.remove_prefix(pfx.size());
        ret.emplace_back(Bytes(keySlice.data(), keySlice.data() + keySlice.size()), Bytes(it->value().data(), it->value().data() + it->value().size()));
      }
    }
    delete it;
    return ret;
  }

  // Search for specific entries from keys
  for (it->Seek(pfx); it->Valid(); it->Next()) {
    if (it->key().starts_with(pfx)) {
      auto keySlice = it->key();
      keySlice.remove_prefix(pfx.size());
      for (const Bytes& key : keys) {
        if (keySlice == rocksdb::Slice(reinterpret_cast<const char*>(key.data()), key.size())) {
          ret.emplace_back(Bytes(keySlice.data(), keySlice.data() + keySlice.size()), Bytes(it->value().data(), it->value().data() + it->value().size()));
        }
      }
    }
  }
  delete it;
  return ret;
}

std::vector<Bytes> DB::getKeys(const Bytes& pfx, const Bytes& start, const Bytes& end) {
  std::vector<Bytes> ret;
  rocksdb::Iterator* it = this->db_->NewIterator(rocksdb::ReadOptions());
  Bytes startBytes = pfx;
  Bytes endBytes = pfx;
  if (!start.empty()) Utils::appendBytes(startBytes, start);
  if (!end.empty()) Utils::appendBytes(endBytes, end);
  rocksdb::Slice startSlice(reinterpret_cast<const char*>(startBytes.data()), startBytes.size());
  rocksdb::Slice endSlice(reinterpret_cast<const char*>(endBytes.data()), endBytes.size());
  for (it->Seek(startSlice); it->Valid() && this->opts_.comparator->Compare(it->key(), endSlice) <= 0; it->Next()) {
    auto keySlice = it->key();
    keySlice.remove_prefix(pfx.size());
    ret.emplace_back(Bytes(keySlice.data(), keySlice.data() + keySlice.size()));
  }
  delete it;
  return ret;
}

