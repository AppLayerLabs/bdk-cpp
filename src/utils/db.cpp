/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "db.h"

DB::DB(const std::filesystem::path& path) {
  std::cout << "Opening DB at path: " << path << std::endl;
  this->opts_.create_if_missing = true;
  if (!std::filesystem::exists(path)) { // Ensure the database path can actually be found
    std::filesystem::create_directories(path);
  }
  auto status = rocksdb::DB::Open(this->opts_, path, &this->db_);
  if (!status.ok()) {
    Logger::logToDebug(LogType::ERROR, Log::db, __func__, "Failed to open DB: " + status.ToString());
    throw DynamicException("Failed to open DB: " + status.ToString());
  }
}

bool DB::putBatch(const DBBatch& batch) {
  std::lock_guard lock(this->batchLock_);
  rocksdb::WriteBatch wb;
  for (const rocksdb::Slice& dels : batch.getDelsSlices()) { wb.Delete(dels); }
  for (const auto& [key, value] : batch.getPutsSlices()) wb.Put(key, value);
  rocksdb::Status s = this->db_->Write(rocksdb::WriteOptions(), &wb);
  return s.ok();
}

std::vector<DBEntry> DB::getBatch(
  const Bytes& bytesPfx, const std::vector<Bytes>& keys
) const {
  std::lock_guard lock(this->batchLock_);
  std::vector<DBEntry> ret;
  std::unique_ptr<rocksdb::Iterator> it(this->db_->NewIterator(rocksdb::ReadOptions()));
  rocksdb::Slice pfx(reinterpret_cast<const char*>(bytesPfx.data()), bytesPfx.size());

  // Search for all entries
  if (keys.empty()) {
    for (it->Seek(pfx); it->Valid() && it->key().starts_with(pfx); it->Next()) {
      auto keySlice = it->key();
      keySlice.remove_prefix(pfx.size());
      ret.emplace_back(Bytes(keySlice.data(), keySlice.data() + keySlice.size()), Bytes(it->value().data(), it->value().data() + it->value().size()));
    }
    it.reset();
    return ret;
  }

  // Search for specific entries from keys
  for (it->Seek(pfx); it->Valid() && it->key().starts_with(pfx); it->Next()) {
    auto keySlice = it->key();
    keySlice.remove_prefix(pfx.size());
    for (const Bytes& key : keys) {
      if (keySlice == rocksdb::Slice(reinterpret_cast<const char*>(key.data()), key.size())) {
        ret.emplace_back(Bytes(keySlice.data(), keySlice.data() + keySlice.size()), Bytes(it->value().data(), it->value().data() + it->value().size()));
      }
    }
  }
  it.reset();
  return ret;
}

std::vector<Bytes> DB::getKeys(const Bytes& pfx, const Bytes& start, const Bytes& end) const {
  std::vector<Bytes> ret;
  std::unique_ptr<rocksdb::Iterator> it(this->db_->NewIterator(rocksdb::ReadOptions()));
  Bytes startBytes = pfx;
  Bytes endBytes = pfx;
  if (!start.empty()) Utils::appendBytes(startBytes, start);
  if (!end.empty()) Utils::appendBytes(endBytes, end);
  rocksdb::Slice startSlice(reinterpret_cast<const char*>(startBytes.data()), startBytes.size());
  rocksdb::Slice endSlice(reinterpret_cast<const char*>(endBytes.data()), endBytes.size());
  for (it->Seek(startSlice); it->Valid() && this->opts_.comparator->Compare(it->key(), endSlice) <= 0; it->Next()) {
    rocksdb::Slice keySlice = it->key();
    keySlice.remove_prefix(pfx.size());
    ret.emplace_back(keySlice.data(), keySlice.data() + keySlice.size());
  }
  it.reset();
  return ret;
}

